#ifndef PAGEPREVIEW_H
#define PAGEPREVIEW_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QRegularExpression>

#include <QGraphicsPathItem>
#include <QDomDocument>
#include <QFile>

#include <QWheelEvent>
#include <QResizeEvent>
#include <QShowEvent>

#include <QGraphicsItem>
#include <QSvgRenderer>
#include <QXmlStreamReader>
#include <QBuffer>
//#include <QGraphicsSvgItem>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

//#include "pageoptions.h"

namespace Ui {
class PagePreview;
}



class ShapeItem : public QGraphicsItem
{
    QPainterPath path;

    QSvgRenderer m_renderer;
    QPixmap m_pixmap;
    QDomDocument doc;

    QByteArray doc_bytearray;
    QXmlStreamReader* xmlReader = nullptr;

    bool is_svg = false;
    
public:

    ShapeItem(QGraphicsItem* parent = nullptr)
        : QGraphicsItem(parent)
    {}

    ~ShapeItem()
    {
        if (xmlReader)
            delete xmlReader;
    }
    QRectF boundingRect() const override
    {
        if (is_svg)
            return QRectF(QPointF(0, 0), m_renderer.defaultSize());
        else
            return QRectF(QPointF(0, 0), m_pixmap.size());
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override
    {
        if (is_svg)
        {
            m_renderer.render(painter, boundingRect());
        }
        else
            painter->drawPixmap(boundingRect(), m_pixmap, boundingRect());
    }

    QByteArray data()
    {
        return doc.toByteArray();
    }

    void setStrokeWidth(double width)
    {
        changeAttributes("stroke-width", QString::number(width));
        m_renderer.load(doc.toByteArray());
    }

    void setStrokeStyle(QColor c)
    {
        changeAttributes("stroke", c.name().toUpper());
        m_renderer.load(doc.toByteArray());
    }

    void setFillStyle(QColor c)
    {
        changeAttributes("fill", c.name().toUpper());
        m_renderer.load(doc.toByteArray());
    }

    void changeAttributes(QString attName, QString attValue)
    {
        QDomElement rootElem = doc.documentElement();

        QDomNode n = rootElem.firstChild();
        while (!n.isNull())
        {
            if (n.isElement())
            {
                QDomElement e = n.toElement();
                if (e.hasAttribute(attName))
                    e.setAttribute(attName, attValue);

                if (n.hasChildNodes())
                    recursiveChangeAttributes(n.firstChild(), attName, attValue);
            }
            n = n.nextSibling();
        }
    }

    void recursiveChangeAttributes(QDomNode node, QString attName, QString attValue)
    {
        QDomNode n = node;
        while (!n.isNull())
        {
            if (n.isElement())
            {
                QDomElement e = n.toElement();
                if (e.hasAttribute(attName))
                    e.setAttribute(attName, attValue);

                if (n.hasChildNodes())
                    recursiveChangeAttributes(n.firstChild(), attName, attValue);
            }
            n = n.nextSibling();
        }
    }

    void normalizeShape()
    {
        if (!is_svg)
            return;

        QDomElement root = doc.documentElement();
        // Get all <polygon> elements in the SVG.
        QDomNodeList polygonNodes = root.elementsByTagName("polygon");
        if (polygonNodes.isEmpty())
            return;

        // First pass: determine the union bounding box over all polygon points.
        bool firstPoint = true;
        double unionMinX, unionMinY, unionMaxX, unionMaxY;

        // We'll also store the polygon elements for later update.
        QVector<QDomElement> polygons;
        for (int i = 0; i < polygonNodes.size(); i++) {
            QDomElement polyElem = polygonNodes.at(i).toElement();
            if (polyElem.isNull())
                continue;
            polygons.append(polyElem);

            QString pointsStr = polyElem.attribute("points");
            // Split on spaces and commas using QRegularExpression.
            QStringList tokens = pointsStr.split(QRegularExpression("[\\s,]+"), Qt::SkipEmptyParts);
            for (int j = 0; j < tokens.size(); j += 2) {
                if (j + 1 >= tokens.size())
                    break;
                bool ok1, ok2;
                double x = tokens[j].toDouble(&ok1);
                double y = tokens[j + 1].toDouble(&ok2);
                if (!ok1 || !ok2)
                    continue;
                if (firstPoint) {
                    unionMinX = unionMaxX = x;
                    unionMinY = unionMaxY = y;
                    firstPoint = false;
                }
                else {
                    unionMinX = qMin(unionMinX, x);
                    unionMaxX = qMax(unionMaxX, x);
                    unionMinY = qMin(unionMinY, y);
                    unionMaxY = qMax(unionMaxY, y);
                }
            }
        }
        if (firstPoint) // no valid points found
            return;

        double unionWidth = unionMaxX - unionMinX;
        double unionHeight = unionMaxY - unionMinY;
        if (unionWidth == 0 || unionHeight == 0)
            return;

        // Determine the target dimensions. We try to use the viewBox of the root element.
        double targetWidth = unionWidth;
        double targetHeight = unionHeight;
        QString viewBoxStr = root.attribute("viewBox");
        if (!viewBoxStr.isEmpty()) {
            QStringList vbTokens = viewBoxStr.split(QRegularExpression("[\\s,]+"), Qt::SkipEmptyParts);
            if (vbTokens.size() == 4) {
                bool ok1, ok2, ok3, ok4;
                double vbX = vbTokens[0].toDouble(&ok1);
                double vbY = vbTokens[1].toDouble(&ok2);
                double vbW = vbTokens[2].toDouble(&ok3);
                double vbH = vbTokens[3].toDouble(&ok4);
                if (ok1 && ok2 && ok3 && ok4) {
                    targetWidth = vbW;
                    targetHeight = vbH;
                }
            }
        }

        // Compute scale factors to map the union bounding box to the target dimensions.
        double scaleX = targetWidth / unionWidth;
        double scaleY = targetHeight / unionHeight;

        // Second pass: update every polygon's points using the computed transform.
        for (int i = 0; i < polygons.size(); i++) {
            QDomElement polyElem = polygons[i];
            QString pointsStr = polyElem.attribute("points");
            QStringList tokens = pointsStr.split(QRegularExpression("[\\s,]+"), Qt::SkipEmptyParts);
            QStringList newPointsList;
            for (int j = 0; j < tokens.size(); j += 2) {
                if (j + 1 >= tokens.size())
                    break;
                bool ok1, ok2;
                double x = tokens[j].toDouble(&ok1);
                double y = tokens[j + 1].toDouble(&ok2);
                if (!ok1 || !ok2)
                    continue;
                // Normalize: subtract the union min and scale.
                double newX = (x - unionMinX) * scaleX;
                double newY = (y - unionMinY) * scaleY;
                // Append in "x,y" format.
                newPointsList.append(QString("%1,%2").arg(newX).arg(newY));
            }
            // Set the updated points attribute.
            polyElem.setAttribute("points", newPointsList.join(" "));
        }

        // Optionally, update the root viewBox so that it starts at 0,0.
        root.setAttribute("viewBox", QString("0 0 %1 %2").arg(targetWidth).arg(targetHeight));

        // Finally, reload the renderer with the updated SVG document.
        m_renderer.load(doc.toByteArray());
    }


    void load_svg_from_memory(QString str)
    {
        is_svg = true;

        doc.setContent(str);
        doc_bytearray = doc.toByteArray();

        QBuffer buf;
        buf.open(QIODevice::ReadWrite);
        QTextStream* bufStream = new QTextStream(&buf);
        doc.save(*bufStream, 0);

        xmlReader = new QXmlStreamReader(doc_bytearray);
        buf.close();

        m_renderer.load(xmlReader);
    }

    void load(QString filepath)
    {
        is_svg = filepath.contains(".svg");

        if (is_svg)
        {
            QFile file(filepath);
            file.open(QIODevice::ReadOnly);

            QTextStream stream(&file);
            QString str = stream.readAll();

            doc.setContent(str);
            file.close();


            doc_bytearray = doc.toByteArray();

            QBuffer buf;
            buf.open(QIODevice::ReadWrite);
            QTextStream* bufStream = new QTextStream(&buf);
            doc.save(*bufStream, 0);

            xmlReader = new QXmlStreamReader(doc_bytearray);
            QSvgRenderer* renderer = new QSvgRenderer();
            buf.close();

            m_renderer.load(xmlReader);
        }
        else
            m_pixmap.load(filepath);
    }
};

struct ShapeInfo
{
    QString svg_data;
    QByteArray svg_icon;

    static ShapeInfo fromPath(QString path)
    {
        ShapeInfo ret;

        QFile file(path);
        file.open(QIODevice::ReadOnly);

        QTextStream stream(&file);
        ret.svg_data = stream.readAll();
        file.close();

        ret.makeIcon();
        return ret;
    }

    static ShapeInfo fromData(const QByteArray& data)
    {
        ShapeInfo ret;
        ret.svg_data = data;
        ret.makeIcon();
        return ret;
    }

    void makeIcon()
    {
        ShapeItem shape;
        shape.load_svg_from_memory(svg_data);
        shape.normalizeShape();

        shape.setStrokeWidth(2);
        shape.setStrokeStyle(Qt::black);
        shape.setFillStyle(Qt::black);

        svg_icon = shape.data();
    }

    void serialize(QJsonObject& info) const
    {
        info["svg"] = svg_data;
    }

    void deserialize(const QJsonObject& info)
    {
        svg_data = info["svg"].toString();
    }
};

struct ComposerInfo
{
    QColor tag_background_color;

    ShapeInfo shape;
    QColor shape_color;
};

class PageGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit PageGraphicsView(QWidget* parent = nullptr)
        : QGraphicsView(parent)
    {}

protected:

    void wheelEvent(QWheelEvent* e) override;
};

class PagePreview : public QWidget
{
    Q_OBJECT

public:
    explicit PagePreview(QWidget *parent = nullptr);
    ~PagePreview();

    QImage composeTag(const ComposerInfo& info, QSize size);
    QImage composeScene(const ComposerInfo& info);

private:
    Ui::PagePreview *ui;
    QGraphicsScene scene;

    //QPixmap shape_piximap;
    QPixmap logo_piximap;

    bool shown_event_once = false;
    bool initialed_layout = false;
    qreal zoom = 1;

protected:

    friend class PageGraphicsView;
    //PageOptions* options;

    bool fit_view_lock = true;

    const int info_row_count = 3;

    // Viewport
    const qreal view_margin_ratio = 0.03;

    // Page
    const qreal page_width = 2480;
    const qreal page_height = 3508;
    const qreal page_margin = page_height * 0.035;
    const qreal page_cx = page_width / 2.0;
    const qreal page_cy = page_width / 2.0;

    // Logo
    const qreal logo_height = page_height * 0.075;
    const qreal logo_padding = logo_height * 0.1;

    // tag
    const qreal tag_padding = page_height * 0.02;

    // Shape
    const qreal shape_padding = page_height * 0.08;

    // Info Boxes
    const qreal info_row_height = page_height * 0.15;

    ///
    /// Auto-determined
    ///
  
    const qreal view_margin = ((page_width + page_width) / 2.0) * view_margin_ratio;
    const qreal body_width = page_width - page_margin * 2;
    const qreal body_height = page_height - page_margin * 2;

    // Sections
    const qreal page_body_left = page_margin;
    const qreal page_body_top = page_margin;
    const qreal page_body_right = page_width - page_margin;
    const qreal page_body_bottom = page_height - page_margin;

    const qreal logo_section_top = page_margin + logo_padding;
    const qreal logo_section_bottom = logo_section_top + logo_height + logo_padding;
    const qreal info_section_height = info_row_height * info_row_count;

    const qreal tag_section_height = page_height - page_margin * 2 - info_section_height;
    const qreal tag_section_bottom = logo_section_bottom + tag_section_height;
    QRect tag_section_rect = QRect(QPoint(page_body_left, logo_section_bottom), QPoint(page_body_right, tag_section_bottom));
    //const qreal tag_size = tag_section_height - tag_padding * 2;

    // Tag rect
    QRect tag_body_rect = tag_section_rect.adjusted(tag_padding, tag_padding, -tag_padding, -tag_padding);
    QPoint tag_center = tag_body_rect.center();
    qreal tag_box_len = std::min(tag_body_rect.width(), tag_body_rect.height());
    QRect tag_rect = QRect(
        tag_center.x() - tag_box_len / 2,
        tag_center.y() - tag_box_len / 2,
        tag_box_len,
        tag_box_len
    );

    //QRect shape_body_rect = tag_rect.adjusted(
    //    shape_padding,
    //    shape_padding,
    //    -shape_padding,
    //    -shape_padding
    //);
    


    QRect page_rect = QRect(0, 0, page_width, page_height);
    QRect view_area_rect = page_rect.adjusted(-view_margin, -view_margin, view_margin, view_margin);
    QRect page_body_rect = page_rect.adjusted(page_margin, page_margin, -page_margin, -page_margin);
    

    

    void refitPageView();
    void resizeEvent(QResizeEvent* e) override;
    void showEvent(QShowEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

};

#endif // PAGEPREVIEW_H
