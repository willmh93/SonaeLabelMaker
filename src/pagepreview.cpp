#include "pagepreview.h"
#include "ui_pagepreview.h"

#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>

#include <QApplication>
#include <QGraphicsScene>
#include <QTimer>

#include <QGraphicsPixmapItem>
#include <QImage>
#include <QPixmap>
#include <QColor>

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
    info.tag_background_color = "#00ff00";
    QImage composed = composeScene(info);
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

        //shape_item->load(":/shapes/GREASE_GUN.svg");

        /*shape_item->setStrokeWidth(20);
        shape_item->setStrokeStyle(QColor(Qt::red));*/
        shape_item->setStrokeStyle(QColor(Qt::black));
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

QImage PagePreview::composeScene(const ComposerInfo& info)
{
    scene.clear();

    //QImage tag("C:/Git/C++/Projects/SonaeLabelMaker/tags/Grease Gun.png");
    //pixmap.fromImage(tag);

    //tag_piximap.load("C:/Git/C++/Projects/SonaeLabelMaker/tags/Grease Gun.png");
    logo_piximap.load(":/res/logo.png");

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
    //QGraphicsRectItem*   tag_item = scene.addRect(tag_rect, QPen(), QBrush(QColor(255,0,0)));
   
    QImage tag_img = composeTag(info, tag_rect.size());
    QPixmap tag_piximap = QPixmap::fromImage(tag_img);
    QGraphicsPixmapItem *tag_item = new QGraphicsPixmapItem(tag_piximap);
    scene.addItem(tag_item);

    

    body_item->setParentItem(page_item);
    logo_edge->setParentItem(body_item);
    tag_edge->setParentItem(body_item);
    logo_item->setParentItem(body_item);
    tag_item->setParentItem(body_item);

    //qreal 
    setItemHeight(logo_item, logo_height);
    //fitItemSize(tag_item, tag_rect.size());
    

    logo_item->setPos(
        page_cx - itemWidth(logo_item)/2,
        logo_section_top
    );

    tag_item->setPos(
        page_cx - itemWidth(tag_item) / 2,
        logo_section_bottom + tag_section_height / 2 - itemHeight(tag_item) / 2
    );

    // Render
    QImage image(page_rect.size(), QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    scene.render(&painter, page_rect, page_rect);

    return image;
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
