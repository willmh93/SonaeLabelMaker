#ifndef PAGEPREVIEW_H
#define PAGEPREVIEW_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>

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

//#include "pageoptions.h"

namespace Ui {
class PagePreview;
}

struct ShapeInfo
{
    QString svg_data;

    static ShapeInfo fromPath(QString path)
    {
        ShapeInfo ret;

        QFile file(path);
        file.open(QIODevice::ReadOnly);

        QTextStream stream(&file);
        ret.svg_data = stream.readAll();
        file.close();

        return ret;
    }
};

struct ComposerInfo
{
    QColor tag_background_color;

    ShapeInfo shape;
    QColor shape_color;
};

class ShapeItem : public QGraphicsItem
{
    QPainterPath path;

    QSvgRenderer m_renderer;
    QPixmap m_pixmap;
    QDomDocument doc;

    QByteArray doc_bytearray;
    QXmlStreamReader* xmlReader = nullptr;
    //QGraphicsSvgItem* item;

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
            QPen pen(Qt::red, 10, Qt::DashLine);
            painter->setPen(pen);
            m_renderer.render(painter, boundingRect());
        }
        else
            painter->drawPixmap(boundingRect(), m_pixmap, boundingRect());
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
        QSvgRenderer* renderer = new QSvgRenderer();

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

            //item->setSharedRenderer(renderer);
            //scene->addItem(item);
            buf.close();

            m_renderer.load(xmlReader);
            //m_renderer.load(filepath);
        }
        else
            m_pixmap.load(filepath);
    }
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
