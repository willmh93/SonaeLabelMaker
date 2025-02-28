#include "pagepreview.h"
#include "ui_pagepreview.h"

#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>

#include <QApplication>
#include <QGraphicsScene>
#include <QTimer>

#include <QGraphicsPixmapItem>
#include <qtextdocument.h>
#include <QImage>
#include <QPixmap>
#include <QColor>
#include <QFontDatabase>

#include "mainwindow.h"


QImage changeColor(QImage image, const QColor &newColor) {

    if (image.isNull()) {
        qWarning("Failed to load image!");
        return QImage();
    }

    // Convert to format that supports transparency
    image = image.convertToFormat(QImage::Format_ARGB32);

    // Iterate over each pixel
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QColor pixelColor = image.pixelColor(x, y);

            if (pixelColor.alpha() > 0) { // Only modify non-transparent pixels
                image.setPixelColor(x, y, QColor(newColor.red(), newColor.green(), newColor.blue(), pixelColor.alpha()));
            }
        }
    }

    return image;
}

qreal itemWidth(QGraphicsItem* item)
{
    return item->boundingRect().width() * item->scale();
}

qreal itemHeight(QGraphicsItem* item)
{
    return item->boundingRect().height() * item->scale();
}


void setItemWidth(QGraphicsItem* item, qreal width)
{
    qreal bmp_width = item->boundingRect().width();
    item->setScale(width / bmp_width);
}

void setItemHeight(QGraphicsItem* item, qreal height)
{
    qreal bmp_height = item->boundingRect().height();
    item->setScale(height / bmp_height);
}

void fitItemSize(QGraphicsItem* item, QSizeF size)
{
    qreal bmp_width = item->boundingRect().width();
    qreal bmp_height = item->boundingRect().height();
    qreal scale_x = size.width() / bmp_width;
    qreal scale_y = size.height() / bmp_height;
    if (scale_x < scale_y)
        item->setScale(scale_x);
    else
        item->setScale(scale_y);
}

void centerItemX(QGraphicsItem* item, const QRectF &r)
{
    item->setX(r.center().x() - (item->boundingRect().width() * item->scale()) / 2);
}

void centerItemY(QGraphicsItem* item, const QRectF& r)
{
    item->setY(r.center().y() - (item->boundingRect().height() * item->scale()) / 2);
}

void centerItem(QGraphicsItem* item, const QRectF& r)
{
    QPointF cen = r.center();
    item->setX(cen.x() - (item->boundingRect().width() * item->scale()) / 2);
    item->setY(cen.y() - (item->boundingRect().height() * item->scale()) / 2);
}

void adjustTextToFit(
    QGraphicsTextItem* item, 
    const QRectF& rect, 
    const QTextOption &textOption, 
    int minFontSize = 1,
    int maxFontSize=500)
{
    item->document()->setDefaultTextOption(textOption);
    item->setTextWidth(rect.width());

    QString text = item->toPlainText();
    QFont font = item->font();

    int origMinFontSize = minFontSize;
    int origMaxFontSize = maxFontSize;

    // Try to fit text on single line. Allow overflow if minimum font size reached
    int optimalFontSize = minFontSize;

    while (minFontSize <= maxFontSize) {
        int midFontSize = (minFontSize + maxFontSize) / 2;
        font.setPointSize(midFontSize);
        QFontMetricsF fm(font);
        QSizeF textSize = fm.size(Qt::TextSingleLine, text);

        // Check if the text fits in the rect.
        if (textSize.width() <= rect.width() && textSize.height() <= rect.height()) 
        {
            // It fits, so try a larger size.
            optimalFontSize = midFontSize;
            minFontSize = midFontSize + 1;
        }
        else {
            // Too big, try a smaller size.
            maxFontSize = midFontSize - 1;
        }
    }

    // If overflow on smallest font, try to wrap text
    font.setPointSize(optimalFontSize);
    QFontMetricsF fm(font);

    QSizeF textSize = fm.size(Qt::TextSingleLine, text);
    if (textSize.width() > rect.width() || textSize.height() > rect.height())
    {
        // Overflow, Look for ()
        int split_i = text.lastIndexOf('(');
        QString a = text.sliced(0, split_i);
        QString b = text.sliced(split_i);
        text = a.trimmed() + '\n' + b.trimmed();

        minFontSize = 1;
        maxFontSize = origMaxFontSize;

        while (minFontSize <= maxFontSize) {
            int midFontSize = (minFontSize + maxFontSize) / 2;
            font.setPointSize(midFontSize);
            QFontMetricsF fm(font);
            QSizeF textSize = fm.size(Qt::TextDontClip, text);
            //QSizeF textSize = fm.size(Qt::TextSingleLine, text);

            // Check if the text fits in the rect.
            if (textSize.width() <= rect.width() && textSize.height() <= rect.height())
            {
                // It fits, so try a larger size.
                optimalFontSize = midFontSize;
                minFontSize = midFontSize + 1;
            }
            else {
                // Too big, try a smaller size.
                maxFontSize = midFontSize - 1;
            }
        }
    }

    // Set the text item to use the optimal font size.
    font.setPointSize(optimalFontSize);
    item->setFont(font);
    item->setPlainText(text);

    // Center the text within the rectangle.
    centerItem(item, rect);
}

PagePreview::PagePreview(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PagePreview)
{
    ui->setupUi(this);


    //view->setSceneRect(page_rect);
    //view->fitInView(page_rect, Qt::KeepAspectRatio);
    //view->scale(0.25, 0.25);

    //MainWindow* main_window = qobject_cast<MainWindow*>(topLevelWidget());
    //options = main_window->pageOptions;

    maintext_fontId = QFontDatabase::addApplicationFont(":/res/arial_bold.ttf");
    barcode_fontId = QFontDatabase::addApplicationFont(":/res/code128.ttf");

    auto* view = ui->pageView;

    //view->setSceneRect(page_rect);
    view->setTransformationAnchor(QGraphicsView::ViewportAnchor::AnchorUnderMouse);
    view->setResizeAnchor(QGraphicsView::ViewportAnchor::AnchorViewCenter);
    view->setRenderHint(QPainter::Antialiasing);
    view->setAlignment(Qt::AlignmentFlag::AlignCenter);// Qt::AlignmentFlag::AlignLeft | Qt::AlignmentFlag::AlignTop);
    view->setInteractive(true);

    constexpr qreal margin = 0.05;


    //view->scale(zoom, zoom);
    //view->setSceneRect(page_rect);
    //view->update();

    view->show();
    //view->fitInView(view_area_rect, Qt::KeepAspectRatio);

    view->setSceneRect(page_rect);// 0, 0, 100, 100);
    view->setScene(&scene);

    ComposerInfo info;
    info.shape_color = "#ffffff";
    info.tag_background_color = "#ffffff";
    composeScene(info);
    //composed.save("label.png");

    view->setSceneRect(0, 0, 100, 100);

    // Prevent scene from resizing view
    

    /*view->update();
    view->updateGeometry();
    this->updateGeometry();*/

    /*view->setSceneRect(page_rect);*/

}

PagePreview::~PagePreview()
{
    delete ui;
}

QImage PagePreview::composeTag(const ComposerInfo& info, QSize size)
{
    QRect r(0, 0, size.width(), size.height());

    QGraphicsScene sub_scene;
    sub_scene.setSceneRect(r);

    // Draw tag
    //{
        // Create Coloured Tag Box Area
        QGraphicsRectItem *box = sub_scene.addRect(r, QPen(Qt::NoPen), QBrush(info.tag_background_color));
        
        // Load Shape Item
        //shape_piximap.load("C:/Git/C++/Projects/SonaeLabelMaker/shapes/test_svg.svg");
        //shape_piximap.load("C:/Git/C++/Projects/SonaeLabelMaker/shapes/Grease Gun.png");
        //QGraphicsPixmapItem* shape_item = sub_scene.addPixmap(shape_piximap);
        ShapeItem* shape_item = new ShapeItem(box);

        //shape_item->load("C:/Git/C++/Projects/SonaeLabelMaker/shapes/test_svg.svg");
        //shape_item->load("C:/Git/C++/Projects/SonaeLabelMaker/shapes/Grease Gun.png");

        shape_item->load_svg_from_memory(info.shape.svg_data);
        shape_item->normalizeShape();

        //shape_item->load(":/shapes/GREASE_GUN.svg");

        /*shape_item->setStrokeWidth(20);
        shape_item->setStrokeStyle(QColor(Qt::red));*/
        shape_item->setStrokeWidth(info.stroke_width);
        shape_item->setStrokeStyle(info.stroke_color);
        shape_item->setFillStyle(info.shape_color);
        //shape_item->load(":/shapes/Grease Gun.png");

        shape_item->setScale(1);

        // Fit Shape in Tag and Center
        QRectF shape_body_rect = r.adjusted(shape_padding, shape_padding, -shape_padding, -shape_padding);
        fitItemSize(shape_item, shape_body_rect.size());
        centerItem(shape_item, QRectF(0, 0, size.width(), size.height()));
    //}

    // Render
    QImage image(size, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    sub_scene.render(&painter, page_rect, page_rect);

    delete shape_item;

    return image;
}

QGraphicsScene *PagePreview::composeScene(const ComposerInfo& info)
{
    scene.clear();

    //QImage tag("C:/Git/C++/Projects/SonaeLabelMaker/tags/Grease Gun.png");
    //pixmap.fromImage(tag);

    //tag_piximap.load("C:/Git/C++/Projects/SonaeLabelMaker/tags/Grease Gun.png");
    logo_piximap.load(":/res/logo.png");
    logo2_piximap.load(":/res/logo2.jpg");

    // Are body border
    QPen line_pen(Qt::black);
    line_pen.setWidth(4);

    QGraphicsRectItem* page_item = scene.addRect(page_rect, QPen(Qt::NoPen), QBrush(Qt::white));
    QGraphicsRectItem* body_item = scene.addRect(page_body_rect, line_pen);
    QGraphicsLineItem* logo_edge = scene.addLine(QLineF(
        page_margin, logo_section_bottom,
        page_body_right, logo_section_bottom
    ), line_pen);
    QGraphicsLineItem* tag_edge = scene.addLine(QLineF(
        page_margin, tag_section_bottom,
        page_body_right, tag_section_bottom
    ), line_pen);

    QGraphicsPixmapItem* logo_item = scene.addPixmap(logo_piximap);
    QGraphicsPixmapItem* logo2_item = scene.addPixmap(logo2_piximap);
    //QGraphicsRectItem*   tag_item = scene.addRect(tag_rect, QPen(), QBrush(QColor(255,0,0)));
   
    QImage tag_img = composeTag(info, tag_rect.size());
    QPixmap tag_piximap = QPixmap::fromImage(tag_img);
    QGraphicsPixmapItem *tag_item = new QGraphicsPixmapItem(tag_piximap);
    scene.addItem(tag_item);

    double info_row1_bottom = tag_section_bottom + info_row_height;
    double info_row2_bottom = tag_section_bottom + info_row_height * 2;
    double info_row3_splitX = page_margin + page_body_rect.width() / 2.0;

    QGraphicsLineItem* info_row1_edge = scene.addLine(QLineF(
        page_margin, info_row1_bottom,
        page_body_right, info_row1_bottom
    ), line_pen);

    QGraphicsLineItem* info_row2_edge = scene.addLine(QLineF(
        page_margin, info_row2_bottom,
        page_body_right, info_row2_bottom
    ), line_pen);

    QGraphicsLineItem* info_row3_split_edge = scene.addLine(QLineF(
        info_row3_splitX, info_row2_bottom,
        info_row3_splitX, page_body_bottom
    ), line_pen);

    qreal text_margin_x = info_row_height * 0.3;
    qreal text_margin_y = info_row_height * 0.1;
    qreal barcode_margin = info_row_height * 0.05;

    QString maintext_fontFamily = QFontDatabase::applicationFontFamilies(maintext_fontId).at(0);
    QFont font(maintext_fontFamily);
    font.setBold(true);
    font.setPixelSize(static_cast<int>(info_row_height * 0.3));

    QString barcode_fontFamily = QFontDatabase::applicationFontFamilies(barcode_fontId).at(0);
    QFont barcodeFont(barcode_fontFamily, 40);
    barcodeFont.setPixelSize(static_cast<int>(info_row_height - barcode_margin * 2));
    
    QGraphicsTextItem* generic_code_item = scene.addText(info.generic_code, font);
    QGraphicsTextItem* product_name_item = scene.addText(info.product_name, font);
    QGraphicsTextItem* material_code_item = scene.addText(info.material_code, font);
    QGraphicsTextItem* material_barcode_item = scene.addText(info.material_code, barcodeFont);

    generic_code_item->setDefaultTextColor(Qt::black);
    product_name_item->setDefaultTextColor(Qt::black);
    material_code_item->setDefaultTextColor(Qt::black);
    material_barcode_item->setDefaultTextColor(Qt::black);

    // Center align
    textOption.setAlignment(Qt::AlignCenter);


    QRectF generic_code_rect = QRect(page_margin, tag_section_bottom, page_body_rect.width(), info_row_height);
    QRectF product_code_rect = QRect(page_margin, info_row1_bottom, page_body_rect.width(), info_row_height);
    QRectF material_code_rect = QRect(page_margin, info_row2_bottom, page_body_rect.width()/2, info_row_height);
    QRectF material_barcode_rect = QRect(info_row3_splitX, info_row2_bottom, page_body_rect.width()/2, info_row_height);

    generic_code_rect.adjust(text_margin_x, text_margin_y, -text_margin_x, -text_margin_y);
    product_code_rect.adjust(text_margin_x, text_margin_y, -text_margin_x, -text_margin_y);
    //material_code_rect.adjust(text_margin_y, text_margin_y, -text_margin_y, -text_margin_y);
    material_barcode_rect.adjust(barcode_margin, barcode_margin, -barcode_margin, -barcode_margin);
    //QGraphicsRectItem* barcode_body_rect = scene.addRect(material_barcode_rect, line_pen);
    
    
    adjustTextToFit(generic_code_item, generic_code_rect, textOption, 60, 75);
    adjustTextToFit(product_name_item, product_code_rect, textOption, 60, 75);
    //adjustTextToFit(material_barcode_item, material_barcode_rect, textOption, 50, 200);

    // Adjust final fonts
    {
        int smallestFontSize = std::min(
            generic_code_item->font().pointSize(),
            product_name_item->font().pointSize()
        );

        QFont common_font = generic_code_item->font();
        common_font.setPointSize(smallestFontSize);

        generic_code_item->setFont(common_font);
        product_name_item->setFont(common_font);
        material_code_item->setFont(common_font);

        centerItem(generic_code_item, generic_code_rect);
        centerItem(product_name_item, product_code_rect);
        centerItem(material_code_item, material_code_rect);
        centerItem(material_barcode_item, material_barcode_rect);
        material_barcode_item->setPos(material_barcode_item->x(), material_barcode_rect.y() - material_barcode_rect .height()*0.042);
    }

    body_item->setParentItem(page_item);
    logo_edge->setParentItem(body_item);
    tag_edge->setParentItem(body_item);
    logo_item->setParentItem(body_item);
    logo2_item->setParentItem(body_item);
    tag_item->setParentItem(body_item);

    //generic_code_item->setParentItem(body_item);

    //qreal 
    setItemHeight(logo_item, logo_height);
    setItemHeight(logo2_item, logo_height);
    //fitItemSize(tag_item, tag_rect.size());
    

    logo_item->setPos(
        page_cx - itemWidth(logo_item) * 1.5,///2,
        logo_section_top
    );

    logo2_item->setPos(
        page_cx + itemWidth(logo_item) * 0.2,
        logo_section_top + logo_height * 0.06
    );

    tag_item->setPos(
        page_cx - itemWidth(tag_item) / 2,
        logo_section_bottom + tag_section_height / 2 - itemHeight(tag_item) / 2
    );

    // Render
    //QImage image(page_rect.size(), QImage::Format_ARGB32);
    //image.fill(Qt::white);
    //QPainter painter(&image);
    //scene.render(&painter, page_rect, page_rect);
    //return image;

    return &scene;
}


void PagePreview::refitPageView()
{
    auto* view = ui->pageView;
    constexpr qreal margin = 0.05;

    view->setSceneRect(page_rect);

    ui->pageView->fitInView(page_rect.adjusted(
        -page_width * margin, -page_height * margin,
        page_width * margin, page_height * margin
    ), Qt::KeepAspectRatio);

    ui->pageView->scale(zoom, zoom);

    qDebug() << "repainted";
}


void PagePreview::resizeEvent(QResizeEvent* e)
{
    //if (!shown_event_once)
    //    return;

    initialed_layout = true;

    if (fit_view_lock)
        refitPageView();
}


void PagePreview::showEvent(QShowEvent* e)
{
    qDebug() << "showEvent";
    shown_event_once = true;

    auto* view = ui->pageView;
    //
    qDebug() << "PagePreview Size: " << this->width() << " " << this->height();
    qDebug() << "View Scene Rect: " << view->sceneRect();
    qDebug() << "Scene Rect: " << scene.sceneRect();

    //view->scale(0.2, 0.2);

    //view->setSceneRect(page_rect);
    //view->setSceneRect(view_area_rect);
    //view->fitInView(scene.itemsBoundingRect(), Qt::KeepAspectRatio);

    if (fit_view_lock)
        refitPageView();
}

void PagePreview::wheelEvent(QWheelEvent* e)
{
    auto* view = ui->pageView;
    

    //zoom += (qreal)e->angleDelta().y() / 1000.0;
    

    //refitPageView();
    //e->accept();

    //ui->pageView->viewport()->update();
    //ui->pageView->update();
    //scene.update();
}

void PageGraphicsView::wheelEvent(QWheelEvent* e)
{
    qreal sf = 1.0 + (qreal)e->angleDelta().y() / 1000.0;
    scale(sf, sf);
    e->accept();

    PagePreview* page_preview = qobject_cast<PagePreview*>(parent());
    page_preview->fit_view_lock = false;
}
