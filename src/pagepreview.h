#ifndef PAGEPREVIEW_H
#define PAGEPREVIEW_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QRegularExpression>

//#include <QGraphicsPathItem>
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

#include <QTableView>

#include "nanosvg.h"

#include <climits>
#include <algorithm>
//#include <execution>
#include <utility>

#ifdef PARALLEL
#include <execution>
namespace execution = std::execution;
#else
enum class execution { seq, unseq, par_unseq, par };
#endif

//#include "qtnanosvg.hpp"

//#include "pageoptions.h"

namespace Ui {
class PagePreview;
}



class ShapeItem : public QGraphicsItem
{
    //QPainterPath path;

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

    /*QPainterPath convertNSVGPath(NSVGpath* nsvgPath) {
        QPainterPath qpath;
        if (nsvgPath->npts < 1)
            return qpath;

        // NSVGpath->pts is a flat array: [x0, y0, x1, y1, ..., xn, yn]
        qpath.moveTo(nsvgPath->pts[0], nsvgPath->pts[1]);
        for (int i = 1; i < nsvgPath->npts; i++) {
            qpath.lineTo(nsvgPath->pts[i * 2], nsvgPath->pts[i * 2 + 1]);
        }
        if (nsvgPath->closed)
            qpath.closeSubpath();
        return qpath;
    }

    QList<QPainterPath> processNSVGImage(NSVGimage* image) 
    {
        QList<QPainterPath> paths;
        for (NSVGshape* shape = image->shapes; shape != nullptr; shape = shape->next) 
        {
            // For each shape, iterate over its paths.
            for (NSVGpath* nsvgPath = shape->paths; nsvgPath != nullptr; nsvgPath = nsvgPath->next) 
            {
                QPainterPath path = convertNSVGPath(nsvgPath);

                // Check if the shape is stroke-only.
                // NSVG_PAINT_NONE is used when no paint is specified.
                bool isStrokeOnly = (shape->fill.type == NSVG_PAINT_NONE) &&
                    (shape->stroke.type != NSVG_PAINT_NONE) &&
                    (shape->strokeWidth > 0);

                if (isStrokeOnly) {
                    QPainterPathStroker stroker;
                    stroker.setWidth(shape->strokeWidth);
                    // You may also adjust join and cap styles if needed.
                    path = stroker.createStroke(path);
                }

                paths.append(path);
            }
        }
        return paths;
    }

    QPainterPath mergePaths(const QList<QPainterPath>& paths) {
        QPainterPath merged;
        for (const QPainterPath& p : paths) {
            merged = merged.united(p);
        }
        return merged;
    }




    NSVGimage* img = nullptr;
    void beginSVGTransform()
    {
        img = nsvgParse((char*)doc.toByteArray().toStdString().c_str(), "px", 96);
    }*/

    QPainterPath convertNSVGPathToPainterPath(NSVGpath* nsvgPath) {
        QPainterPath painterPath;
        if (nsvgPath->npts < 1)
            return painterPath;

        // Start at the first point.
        painterPath.moveTo(nsvgPath->pts[0], nsvgPath->pts[1]);

        // Use line segments for subsequent points.
        for (int i = 1; i < nsvgPath->npts; ++i) {
            double x = nsvgPath->pts[i * 2];
            double y = nsvgPath->pts[i * 2 + 1];
            painterPath.lineTo(x, y);
        }

        // If the path is closed, close the subpath.
        if (nsvgPath->closed)
            painterPath.closeSubpath();

        return painterPath;
    }

    // Convert a complete NSVGshape into a QPainterPath by processing all its NSVGpath elements.
    // Only shapes with a valid fill (i.e. not "none") are considered.
    QPainterPath convertNSVGShapeToPainterPath(NSVGshape* shape) {
        QPainterPath shapePath;

        // Only process shapes that have a fill.
        if (shape->fill.type == NSVG_PAINT_NONE)
            return shapePath;

        for (NSVGpath* path = shape->paths; path != nullptr; path = path->next) {
            shapePath.addPath(convertNSVGPathToPainterPath(path));
        }
        return shapePath;
    }

    QPainterPath flattenPath(const QPainterPath& path, const QTransform& transform = QTransform()) {
        QPainterPath flatPath;
        // Extract polygons for each subpath
        QList<QPolygonF> polygons = path.toSubpathPolygons(transform);
        for (const QPolygonF& poly : polygons) {
            if (poly.isEmpty())
                continue;
            flatPath.moveTo(poly.first());
            for (int i = 1; i < poly.size(); ++i) {
                flatPath.lineTo(poly[i]);
            }
            flatPath.closeSubpath();
        }
        return flatPath;
    }

    inline auto inverse(float const* const f0) noexcept
    {
        std::array<float, 6> f1{
          f0[3], -f0[1],
          -f0[2], f0[0],
          f0[2] * f0[5] - f0[3] * f0[4],
          f0[1] * f0[4] - f0[0] * f0[5]
        };

        std::transform(
            //execution::unseq,//par_unseq,
            f1.cbegin(),
            f1.cend(),
            f1.begin(),
            [invdet(1.f / (f0[0] * f0[3] - f0[2] * f0[1]))](auto const f) noexcept
        {
            return f * invdet;
        }
        );

        return f1;
    }

    inline auto toQColor(quint32 const c, float const o) noexcept
    {
        #if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        return QColor(quint8(c), quint8(c >> 8), quint8(c >> 16),
            qRound(quint8(c >> 24) * o));
        #else
        return QColor(quint8(c >> 24), quint8(c >> 16), quint8(c >> 8),
            qRound(quint8(c) * o));
        #endif // Q_BYTE_ORDER
    }

    void plotPath(QPainterPath &qpath, struct NSVGshape const* const shape)
    {
        for (auto path(shape->paths); path; path = path->next)
        {
            {
                auto p(path->pts);

                qpath.moveTo(p[0], p[1]);

                auto const end(p + 2 * path->npts);

                for (p += 2; end != p; p += 6)
                {
                    qpath.cubicTo(p[0], p[1], p[2], p[3], p[4], p[5]);
                }
            }

            if (path->closed) qpath.closeSubpath();
        }

        // fill
        /*switch (auto const type(shape->fill.type); type)
        {
        case NSVG_PAINT_NONE:
            break;

        case NSVG_PAINT_COLOR:
        case NSVG_PAINT_LINEAR_GRADIENT:
        case NSVG_PAINT_RADIAL_GRADIENT:
        {
            switch (shape->fillRule)
            {
            case NSVG_FILLRULE_NONZERO:
                qpath.setFillRule(Qt::WindingFill);

                break;

            case NSVG_FILLRULE_EVENODD:
                qpath.setFillRule(Qt::OddEvenFill);

                break;

            default:
                Q_ASSERT(0);
            }

            auto const fillWithGradient([&](QGradient& gr)
            {
                auto const& g(*shape->fill.gradient);

                switch (g.spread)
                {
                case NSVG_SPREAD_PAD:
                    gr.setSpread(QGradient::PadSpread);

                    break;

                case NSVG_SPREAD_REFLECT:
                    gr.setSpread(QGradient::ReflectSpread);

                    break;

                case NSVG_SPREAD_REPEAT:
                    gr.setSpread(QGradient::RepeatSpread);

                    break;

                default:
                    Q_ASSERT(0);
                }

                {
                    auto const ns(g.nstops);

                    for (decltype(g.nstops) i{}; ns != i; ++i)
                    {
                        auto const& stp(g.stops[i]);

                        gr.setColorAt(stp.offset, toQColor(stp.color, shape->opacity));
                    }
                }

                p->fillPath(qpath, gr);
            }
            );

            switch (type)
            {
            case NSVG_PAINT_COLOR:
                p->fillPath(qpath, toQColor(shape->fill.color, shape->opacity));

                break;

            case NSVG_PAINT_LINEAR_GRADIENT:
            {
                QLinearGradient lgr;

                auto const t(inverse(shape->fill.gradient->xform));

                lgr.setStart(t[4], t[5]);
                lgr.setFinalStop(t[2] + t[4], t[3] + t[5]);

                fillWithGradient(lgr);

                break;
            }

            case NSVG_PAINT_RADIAL_GRADIENT:
            {
                QRadialGradient rgr;

                auto const& g(*shape->fill.gradient);

                auto const t(inverse(g.xform));
                auto const r(-t[0]);

                rgr.setCenter(g.fx * r, g.fy * r);
                rgr.setCenterRadius(0);

                rgr.setFocalPoint(t[4], t[5]);
                rgr.setFocalRadius(t[0]);

                fillWithGradient(rgr);

                break;
            }

            default:
                Q_ASSERT(0);
            }

            break;
        }

        default:
            Q_ASSERT(0);
        }

        // stroke
        switch (shape->stroke.type)
        {
        case NSVG_PAINT_NONE:
            break;

        case NSVG_PAINT_COLOR:
        {
            QPen pen(toQColor(shape->stroke.color, shape->opacity));

            pen.setWidthF(shape->strokeWidth);

            if (auto const count(shape->strokeDashCount); count)
            {
                pen.setDashOffset(shape->strokeDashOffset);
                pen.setDashPattern(
                    {
                      shape->strokeDashArray,
                      shape->strokeDashArray + count
                    }
                );
            }

            switch (shape->strokeLineCap)
            {
            case NSVG_CAP_BUTT:
                pen.setCapStyle(Qt::FlatCap);

                break;

            case NSVG_CAP_ROUND:
                pen.setCapStyle(Qt::RoundCap);

                break;

            case NSVG_CAP_SQUARE:
                pen.setCapStyle(Qt::SquareCap);

                break;

            default:
                Q_ASSERT(0);
            }

            switch (shape->strokeLineJoin)
            {
            case NSVG_JOIN_BEVEL:
                pen.setJoinStyle(Qt::BevelJoin);

                break;

            case NSVG_JOIN_MITER:
                pen.setJoinStyle(Qt::SvgMiterJoin);
                pen.setMiterLimit(shape->miterLimit);

                break;

            case NSVG_JOIN_ROUND:
                pen.setJoinStyle(Qt::RoundJoin);

                break;

            default:
                Q_ASSERT(0);
            }

            p->strokePath(qpath, pen);

            break;
        }

        default:
            Q_ASSERT(0);
        }*/
    }

    QPainterPath mergeFlattenedFilledShapes(NSVGimage* image) {
        QPainterPath mergedPath;
        for (NSVGshape* shape = image->shapes; shape != nullptr; shape = shape->next) {
            // Only consider shapes with a fill.
            if (shape->fill.type == NSVG_PAINT_NONE)
                continue;

            QPainterPath shapePath;
            for (NSVGpath* nsvgPath = shape->paths; nsvgPath != nullptr; nsvgPath = nsvgPath->next) {
                shapePath.addPath(convertNSVGPathToPainterPath(nsvgPath));
            }
            // Flatten the shape path by converting it into subpath polygons
            QPainterPath flatShape = flattenPath(shapePath);
            // Merge it with the previously merged shapes.
            mergedPath = mergedPath.united(flatShape);
        }
        return mergedPath;
    }

    QString convertQPainterPathToSvgPathData(const QPainterPath& path)
    {
        QString d;
        const int count = path.elementCount();
        for (int i = 0; i < count; ++i) {
            QPainterPath::Element element = path.elementAt(i);
            if (element.isMoveTo()) {
                d += QString("M %1 %2 ").arg(element.x).arg(element.y);
            }
            else if (element.isLineTo()) {
                d += QString("L %1 %2 ").arg(element.x).arg(element.y);
            }
            else if (element.type == QPainterPath::CurveToElement) {
                if (i + 2 < count) {
                    QPainterPath::Element ctrl1 = element;
                    QPainterPath::Element ctrl2 = path.elementAt(i + 1);
                    QPainterPath::Element end = path.elementAt(i + 2);
                    d += QString("C %1 %2 %3 %4 %5 %6 ")
                             .arg(ctrl1.x)
                             .arg(ctrl1.y)
                             .arg(ctrl2.x)
                             .arg(ctrl2.y)
                             .arg(end.x)
                             .arg(end.y);
                    i += 2; // Skip the two elements that have been processed.
                }
            }
        }
        return d.trimmed();
    }

    void flatten()
    {
        if (auto const nsi(nsvgParse((char*)doc.toByteArray().toStdString().c_str(), "px", 96)); nsi)
        {
            //QList<QPainterPath> painterPaths = processNSVGImage(nsi);

            // Merge all paths into a single QPainterPath.
            // draw shapes
            QPainterPath mergedPath;
            for (auto shape(nsi->shapes); shape; shape = shape->next)
            {
                //drawSVGShape(p, shape);
                QPainterPath shape_path;
                plotPath(shape_path, shape);

                mergedPath = mergedPath.united(shape_path);
            }

            //QPainterPath mergedPath = mergeFlattenedFilledShapes(nsi);

            // Optionally, create an outline around the merged shape.
            //qreal extraStrokeWidth = 50.0; // For example.
            //QPainterPathStroker outlineStroker;
            //outlineStroker.setWidth(extraStrokeWidth);
            //QPainterPath outline = outlineStroker.createStroke(mergedPath);

            // At this point, you have:
            // - mergedPath: the union of all vector shapes.
            // - outline: an extra outline around the merged geometry.
            
            QString fill_style = "#000000";
            QString stroke_style = "#000000";
            double stroke_width = 10.0;

            // ... (use mergedPath or outline as needed)
            QString svgPathData = convertQPainterPathToSvgPathData(mergedPath);
            QString svgOutput = QString("<svg xmlns='http://www.w3.org/2000/svg'>"
                "<path d='%1' fill='%2' stroke='%3' stroke-width='%4'/>"
                "</svg>").arg(svgPathData).arg(fill_style).arg(stroke_style).arg(stroke_width);

            doc.setContent(svgOutput);
            m_renderer.load(doc.toByteArray());

            // Clean up.
            nsvgDelete(nsi);
        }
    }

    //void flattenS

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

        flatten();
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
    bool valid = false;

    static ShapeInfo fromPath(QString path)
    {
        ShapeInfo ret;

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
        {
            ret.valid = false;
            return ret;
        }

        QTextStream stream(&file);
        ret.valid = true;
        ret.svg_data = stream.readAll();
        file.close();

        ret.makeIcon();
        return ret;
    }

    static ShapeInfo fromData(const QByteArray& data)
    {
        ShapeInfo ret;
        if (data.size() == 0)
        {
            ret.valid = false;
            return ret;
        }

        ret.valid = true;
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
        valid = svg_data.size() > 0;
    }
};



struct ComposerInfo
{
    ShapeInfo shape;
    QColor shape_color;
    QColor tag_background_color;
    QColor tag_inner_background_color;

    QString generic_code;
    QString material_code;
    QString product_name;

    double stroke_width;
    QColor stroke_color;

    /*bool colorsSimilar(const QColor& color1, const QColor& color2, float tolerance=0.1)
    {
        float hue1 = color1.hueF();
        float hue2 = color2.hueF();
        float sat1 = color1.saturationF();
        float sat2 = color2.saturationF();
        float val1 = color1.valueF();
        float val2 = color2.valueF();

        // Handle circular nature of hue
        float hueDiff = std::abs(hue1 - hue2);
        if (hueDiff > 0.5f)
            hueDiff = 1.0f - hueDiff;

        return (hueDiff < tolerance &&
            std::abs(sat1 - sat2) < tolerance &&
            std::abs(val1 - val2) < tolerance);
    }

    double relativeLuminance(const QColor& color)
    {
        // Convert sRGB channels to [0,1]
        auto toLinear = [](double channel) -> double {
            channel /= 255.0;
            return (channel <= 0.03928) ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
        };

        double r = toLinear(color.red());
        double g = toLinear(color.green());
        double b = toLinear(color.blue());

        // Calculate luminance using the Rec. 709 coefficients
        return 0.2126 * r + 0.7152 * g + 0.0722 * b;
    }*/

    QColor highContrastStrokeColor(const QColor& background)
    {
        double L_bg = relativeLuminance(background);

        // Calculate contrast ratios:
        // For black stroke, relative luminance of black is 0.
        double contrastBlack = (L_bg + 0.05) / (0.05);
        // For white stroke, relative luminance of white is 1.
        double contrastWhite = (1.05) / (L_bg + 0.05);

        // Return the stroke color that offers a higher contrast ratio.
        return (contrastBlack > contrastWhite) ? Qt::black : Qt::white;
    }

    // Convert an sRGB channel value (0-255) to linear space.
    double toLinear(double channel) {
        double normalized = channel / 255.0;
        return (normalized <= 0.03928) ? normalized / 12.92
            : std::pow((normalized + 0.055) / 1.055, 2.4);
    }

    // Compute the relative luminance of a color.
    double relativeLuminance(const QColor& color) {
        double r = toLinear(color.red());
        double g = toLinear(color.green());
        double b = toLinear(color.blue());
        return 0.2126 * r + 0.7152 * g + 0.0722 * b;
    }

    // Calculate the contrast ratio between two colors.
    double contrastRatio(const QColor& color1, const QColor& color2) {
        double L1 = relativeLuminance(color1);
        double L2 = relativeLuminance(color2);
        if (L1 < L2)
            std::swap(L1, L2); // Ensure L1 is the lighter color.
        return (L1 + 0.05) / (L2 + 0.05);
    }

    // Check if a stroke is needed based on a threshold contrast ratio.
    bool needsStroke(const QColor& fill, const QColor& background, double threshold = 1.5) {
        double ratio = contrastRatio(fill, background);
        return (ratio < threshold);
    }

    void autoDetermineStroke()
    {
        //if (colorsSimilar(shape_color, tag_background_color, 0.1f))
        QColor effective_background_color = tag_background_color;
        if (tag_inner_background_color.isValid())
            effective_background_color = tag_inner_background_color;

        if (needsStroke(shape_color, effective_background_color, 1.1))
        {
            stroke_width = 20;
            stroke_color = highContrastStrokeColor(effective_background_color);
        }
        else
        {
            stroke_width = 0;
            stroke_color = Qt::transparent;
        }
    }
};

enum PageTemplateType
{
    A4_300_DPI,
    A4_150_DPI,
    A4_50_DPI
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
    QGraphicsScene *composeScene(const ComposerInfo& info);

private:
    Ui::PagePreview *ui;
    QGraphicsScene scene;

    //QPixmap shape_piximap;
    QPixmap logo_piximap;
    QPixmap logo2_piximap;

    bool shown_event_once = false;
    bool initialed_layout = false;
    qreal zoom = 1;

    int maintext_fontId;
    int barcode_fontId;

    PageTemplateType page_type = PageTemplateType::A4_300_DPI;


public:

    friend class PageGraphicsView;
    //PageOptions* options;

    bool fit_view_lock = true;

    
    
    QTableView* getTable();

    void setTargetPageType(PageTemplateType type)
    {
        page_type = type;
    }
    int targetPageDPI()
    {
        switch (page_type)
        {
        case PageTemplateType::A4_300_DPI: return 300;
        case PageTemplateType::A4_150_DPI: return 150;
        case PageTemplateType::A4_50_DPI: return 50;
        }
        return 300;
    }
    QSizeF targetPageSize()
    {
        switch (page_type)
        {
        case PageTemplateType::A4_300_DPI: return QSizeF(2480, 3508);
        case PageTemplateType::A4_150_DPI: return QSizeF(2480/2, 3508/2);
        case PageTemplateType::A4_50_DPI: return QSizeF(2480/6, 3508/6);
        }
        return QSizeF(0, 0);
    }
    QRectF targetPageRect()
    {
        QSizeF size = targetPageSize();
        return QRectF(0, 0, size.width(), size.height());
    }
    int targetPageLineWidth()
    {
        return static_cast<int>(std::max(1.0, targetPageSize().width() * 0.002));
    }
    qreal targetPageMargin()
    {
        return (targetPageRect().height() * 0.035) - targetPageLineWidth();
    }

private:

    QTextOption textOption;
    
    

    void refitPageView();
    void resizeEvent(QResizeEvent* e) override;
    void showEvent(QShowEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

};

#endif // PAGEPREVIEW_H
