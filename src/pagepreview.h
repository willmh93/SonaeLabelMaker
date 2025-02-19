#ifndef PAGEPREVIEW_H
#define PAGEPREVIEW_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>

#include <QWheelEvent>
#include <QResizeEvent>
#include <QShowEvent>

#include <QGraphicsItem>
#include <QSvgRenderer>

namespace Ui {
class PagePreview;
}

struct ComposerInfo
{
    QColor tag_background_color;
};

class ShapeItem : public QGraphicsItem
{
    QSvgRenderer m_renderer;
    QPixmap m_pixmap;

    bool is_svg = false;
    
public:

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
            m_renderer.render(painter, boundingRect());
        else
            painter->drawPixmap(boundingRect(), m_pixmap, boundingRect());
    }

    ShapeItem(QGraphicsItem* parent = nullptr)
        : QGraphicsItem(parent)
    {}

    ~ShapeItem()
    {
    }

    void load(QString filepath)
    {
        is_svg = filepath.contains(".svg");

        if (is_svg)
            m_renderer.load(filepath);
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


    void wheelEvent(QWheelEvent* e) override
    {
        qreal sf = 1.0 + (qreal)e->angleDelta().y() / 1000.0;
        scale(sf, sf);
        e->accept();
    }
};

class PagePreview : public QWidget
{
    Q_OBJECT

public:
    explicit PagePreview(QWidget *parent = nullptr);
    ~PagePreview();

private:
    Ui::PagePreview *ui;
    QGraphicsScene scene;

    //QPixmap shape_piximap;
    QPixmap logo_piximap;

    bool shown_event_once = false;
    bool initialed_layout = false;
    qreal zoom = 1;

protected:

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
    const qreal tag_padding = page_height * 0.025;

    // Shape
    const qreal shape_padding = page_height * 0.04;

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
    

    QImage composeTag(const ComposerInfo& info, QSize size);
    QImage composeScene(const ComposerInfo &info);

    void refitPageView();
    void resizeEvent(QResizeEvent* e) override;
    void showEvent(QShowEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

};

#endif // PAGEPREVIEW_H
