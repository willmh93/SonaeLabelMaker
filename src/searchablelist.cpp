#include "searchablelist.h"
#include "ui_searchablelist.h"

#include <QEvent>
#include <QMouseEvent>
#include <QSvgRenderer>
#include <QTimer>

//int SearchableListItem::SearchableListItem_UID_Counter = 0;

/*void SearchableListView::mouseMoveEvent(QMouseEvent* event)
{
    mouse_pos = event->pos();

    QModelIndex hoveredIndex = indexAt(event->pos());
    SearchableList* searchable_list = qobject_cast<SearchableList*>(this->parent());
    
    SearchableListItem item = hoveredIndex.data(Qt::UserRole).value<SearchableListItem>();
    if (searchable_list->item_mousemove_callback)
    {
        searchable_list->item_mousemove_callback(item, mouse_pos);
    }

    // Trigger repaint
    viewport()->update();

    QListView::mouseMoveEvent(event);
}*/


SearchableList::SearchableList(QWidget *parent) : QWidget(parent), ui(new Ui::SearchableList)
{
    ui->setupUi(this);
    
    if (other_lists == nullptr)
        ui->combo->hide();

    item_delegate = new StyledItemDelegate(ui->list);
    proxyModel.setDynamicSortFilter(false);
    proxyModel.setSourceModel(&model);
    ui->list->setModel(&proxyModel);
    ui->list->setItemDelegate(item_delegate);
    ui->list->installEventFilter(item_delegate);

    connect(ui->filter, &QLineEdit::textChanged, this, [this](const QString& filter)
    {
        proxyModel.setFilterString(filter);
    });

    connect(ui->filter, &FocusableLineEdit::focusGained, this, [this]()
    {
        ui->combo->setChecked(true);
    });

    updateSelectionModel();

    connect(ui->combo, &QRadioButton::toggled, this, [this](bool b)
    {
        if (other_lists)
        {
            for (SearchableList* list : *other_lists)
            {
                if (list != this)
                    list->_setRadioChecked(false, true);
            }
            _setRadioChecked(true);
        }
        if (on_radio_toggle)
            on_radio_toggle(b);
    });

    /*connect(ui->list, &QListView::doubleClicked, this, [this](const QModelIndex& index)
    {
        if (item_doubleclick_callback)
        {
            SearchableListItem item = index.data(Qt::UserRole).value<SearchableListItem>();
            item_doubleclick_callback(item);
        }
    });*/
    
    QObject::connect(&model, &QStandardItemModel::dataChanged,
        [this](const QModelIndex& topLeft, 
               const QModelIndex& bottomRight,
               const QVector<int>& roles) 
    {
        if (roles.contains(Qt::EditRole) || roles.isEmpty()) 
        {
            //SearchableListItem& item = topLeft.data().value<SearchableListItem>();
            SearchableListItem& item = model.itemAt(topLeft.row());
            
            QTimer::singleShot(0, [this, &item]()
            {
                emit itemModified(item);
            });
            //qDebug() << "@@@@@@ Item edited:" << topLeft.data().toString();
        }
    });

    //filteredPopulate("");
}

SearchableList::~SearchableList()
{
    delete ui;
}

SearchableList *SearchableList::init(const std::string _field_id, const QString name)
{
    field_id = _field_id;
    setName(name);
    return this;
}

void SearchableList::setName(const QString &_name)
{
    name = _name;
    refreshLayoutUI();
}


//template<typename T>

void SearchableList::setCustomWidget(QWidget* w)
{
    QWidget* old_widget = custom_widget ? custom_widget : ui->custom_toolbar_items;
    ui->toolbar_layout->replaceWidget(old_widget, w);
    custom_widget = w;

    /*if (custom_widget)
    {
        ui->toolbar_layout->removeWidget(w);
        delete custom_widget;
    }

    w->setParent(this);
    w->setLayout(ui->toolbar_layout);*/

    //ui->tools_frame->la

    //if (custom_widget)
    //    delete custom_widget;

    //custom_widget = new T();
    //return custom_widget;
}

void SearchableList::refreshLayoutUI()
{
    if (other_lists != nullptr)
    {
        // Radio
        ui->combo->setText(name);
        ui->combo->show();
        ui->nameLabel->hide();
    }
    else
    {
        // Label
        ui->nameLabel->setText(name);
        ui->nameLabel->show();
        ui->combo->hide();
    }

    if (filterable)
        ui->filter->show();
    else
        ui->filter->hide();

 
}

void SearchableList::setFilterable(bool b)
{
    filterable = b;
    refreshLayoutUI();
}

void SearchableList::_setRadioChecked(bool b, bool blockSignals)
{
    QSignalBlocker blocker(ui->combo);
    ui->combo->setChecked(b);
}
void SearchableList::setRadioGroup(std::shared_ptr<std::vector<SearchableList*>> _other_lists)
{
    other_lists = _other_lists;
    refreshLayoutUI();
}

void SearchableList::setRadioChecked(bool b)
{
    for (SearchableList* list : *other_lists)
    {
        if (list != this)
            list->_setRadioChecked(list == this);
    }
}

bool SearchableList::getRadioChecked()
{
    return ui->combo->isChecked();
}

void SearchableList::refresh()
{
    // Get the model
    QAbstractItemModel* model = ui->list->model();
    if (model) 
    {
        QModelIndex topLeft = model->index(0, 0); // First item
        QModelIndex bottomRight = model->index(model->rowCount() - 1, 0); // Last item

        // Notify the view that the data has changed (forces repaint)
        emit model->dataChanged(topLeft, bottomRight);
    }
}

void SearchableList::updateSelectionModel()
{
    if (ui->list->selectionMode() != QAbstractItemView::SelectionMode::NoSelection)
    {
        connect(ui->list->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex& current, const QModelIndex& previous)
        {
            if (current.isValid())
            {
                QVariant data = current.data(Qt::UserRole); // Get custom data
                SearchableListItem item = data.value<SearchableListItem>();
                emit onChangedSelected(item);
            }
        });
    }
    else
    {
        disconnect(
            ui->list->selectionModel(), 
            &QItemSelectionModel::currentChanged,
            this, 0);
    }
}

void SearchableList::clearFilter()
{
    ui->filter->clear();
}

void SearchableList::setSelectable(bool b)
{
    if (b)
    {
        ui->list->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    }
    else
    {
        ui->list->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
    }
    updateSelectionModel();
}

void SearchableList::setCurrentItem(int i)
{
    QModelIndex modelIndex = proxyModel.index(i, 0);
    ui->list->setCurrentIndex(modelIndex);
    ui->list->selectionModel()->clearSelection();
    ui->list->selectionModel()->select(modelIndex, QItemSelectionModel::Select);

}

void SearchableList::setCurrentItem(const QString& txt)
{
    int i = proxyModel.find(txt);
    QModelIndex modelIndex = proxyModel.index(i, 0);
    ui->list->setCurrentIndex(modelIndex);
    //if (ui->list->selectionMode() != QAbstractItemView::SelectionMode::NoSelection)
    {
        ui->list->selectionModel()->clearSelection();
        ui->list->selectionModel()->select(modelIndex, QItemSelectionModel::Select);
    }
}

void SearchableList::setCurrentItem(SearchableListItem* item)
{
    int i = model.find(item);
    QModelIndex modelIndex = proxyModel.index(i, 0);
    ui->list->setCurrentIndex(modelIndex);
    //if (ui->list->selectionMode() != QAbstractItemView::SelectionMode::NoSelection)
    {
        ui->list->selectionModel()->clearSelection();
        ui->list->selectionModel()->select(modelIndex, QItemSelectionModel::Select);
    }
}

bool SearchableList::getCurrentItem(SearchableListItem* dest)
{
    QModelIndex selected_index = ui->list->selectionModel()->currentIndex();
    *dest = selected_index.data(Qt::UserRole).value<SearchableListItem>();

    return true;
}

void SearchableList::beginEdit(SearchableListItem* item)
{
    int i = model.find(item);
    QModelIndex proxyIndex = proxyModel.index(i, 0);
    if (proxyIndex.isValid())
    {
        //ui->list->setCurrentIndex(proxyIndex);
        ui->list->scrollTo(proxyIndex, QListView::PositionAtCenter);
        ui->list->edit(proxyIndex);
    }
    //ui->list->selectionModel()->edit
}

void SearchableList::setRowHeight(int height)
{
    item_delegate->setRowHeight(height);
    //ui->list->setGridSize(QSize(0, height)); // Increase row height to 50 pixels
    //list_view->setIconSize(QSize(height, height));
}

/*void SearchableList::addListboxItemFiltered(const QString& txt)
{
    QString filter = ui->filter->text();
    if (txt.contains(filter))
    {
        //QStandardItem* model_item = new QStandardItem(txt);
        model.appendRow(SearchableListItem(txt));
    }
}*/

QSize StyledItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return QSize(option.rect.width(), row_size); // Set row height to 50px
}

void StyledItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();

    // Draw the default background and focus rectangle
    //

    // Define the size and position of the "X"
    //int xOffset = 5;  // Offset from left
    /*int yOffset = option.rect.height() / 4; // Center it vertically
    int size = option.rect.height() / 2;  // Scale with row height

    // Get the text and calculate its bounding box
    QString text = index.data(Qt::DisplayRole).toString();
    QFontMetrics fm(option.font);
    int textWidth = fm.horizontalAdvance(text);  // Get text width

    int xOffset = textWidth + 10;
    QRect icon_rect(xOffset, option.rect.y() + yOffset, size, size);*/

    // Adjust text position by shifting right
    ///QStyleOptionViewItem textOption(option);
    ///textOption.rect.setX(option.rect.x() + icon_rect.width() + 10);  // Move text right
    ///QStyledItemDelegate::paint(painter, textOption, index);

    //const SearchableListView* list_view = qobject_cast<const SearchableListView*>(option.widget);
    SearchableListView* list_view = qobject_cast<SearchableListView*>(parent());
    SearchableList* searchableList = qobject_cast<SearchableList*>(list_view->parent());
    if (list_view && searchableList->item_paint_callback)
    {
        SearchableListItem item = index.data(Qt::UserRole).value<SearchableListItem>();
        QPoint mousePos = list_view->getMousePos();

        ListItemCallbackData cb_data = {
            list_view,
            this,
            option,
            nullptr,
            index,
            list_view->mouse_pos,
            nullptr
        };

        if (!searchableList->item_paint_callback(item, painter, cb_data))
        {
            QStyledItemDelegate::paint(painter, option, index);
        }
    }
    else
    {
        QStyledItemDelegate::paint(painter, option, index);
    }

    painter->restore();

    
}

bool StyledItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    SearchableListItem item = index.data(Qt::UserRole).value<SearchableListItem>();
    SearchableListView* list_view = qobject_cast<SearchableListView*>(parent());
    SearchableList* searchable_list = qobject_cast<SearchableList*>(list_view->parent());

    switch (event->type())
    {
        case QEvent::MouseMove:
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            list_view->mouse_pos = mouseEvent->pos();

            if (searchable_list->item_mousemove_callback)
            {
                ListItemCallbackData cb_data = {
                    list_view,
                    this,
                    option,
                    model,
                    index,
                    list_view->mouse_pos,
                    mouseEvent
                };

                searchable_list->item_mousemove_callback(item, cb_data);
            }

            // Trigger repaint
            list_view->viewport()->update();
        }
        break;

        case QEvent::MouseButtonPress:
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QRectF iconRect = getIconRect(option, index, 0);

            if (searchable_list->item_click_callback)
            {
                ListItemCallbackData cb_data = {
                    list_view,
                    this,
                    option,
                    model,
                    index,
                    list_view->mouse_pos,
                    mouseEvent
                };

                searchable_list->item_click_callback(item, cb_data);
            }

            //if (iconRect.contains(mouseEvent->pos()))
            //{
            //    if (searchable_list->icon_click_callback)
            //        searchable_list->icon_click_callback(item);
            //
            //    return true;
            //}
        }
        break;

        case QEvent::MouseButtonDblClick:
        {
            if (searchable_list->item_doubleclick_callback)
            {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                ListItemCallbackData cb_data = {
                    list_view,
                    this,
                    option,
                    model,
                    index,
                    list_view->mouse_pos,
                    mouseEvent
                };

                searchable_list->item_doubleclick_callback(item, cb_data);
            }
        }
        break;
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

QRectF StyledItemDelegate::getIconRect(const QStyleOptionViewItem& option, const QModelIndex& index, int iconIndex, bool from_end, qreal padding) const
{
    int yOffset = option.rect.height() / 4; // Center it vertically
    int size = option.rect.height() / 2;  // Scale with row height

    // Get the text and calculate its bounding box
    //QString text = index.data(Qt::DisplayRole).toString();
    //QFontMetrics fm(option.font);
    //int textWidth = fm.horizontalAdvance(text);  // Get text width

    if (from_end)
    {
        int xOffset = option.rect.x() + option.rect.width() - 10 - ((iconIndex + 1) * size + 4);
        QRectF xRect(xOffset, option.rect.y() + yOffset, size, size);
        return xRect.adjusted(-padding, -padding, padding, padding);
    }
    else
    {
        int xOffset = option.rect.x() + /*textWidth +*/ 4 + (iconIndex * size + 4);
        QRectF xRect(xOffset, option.rect.y() + yOffset, size, size);
        return xRect.adjusted(-padding, -padding, padding, padding);
    }
}

QWidget* StyledItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const
{
    CommittingLineEdit* editor = new CommittingLineEdit(parent);

    // Store old value in item
    ///SearchableListView* list_view = qobject_cast<SearchableListView*>(parent);
    ///SearchableList* searchable_list = qobject_cast<SearchableList*>(list_view->parent());
    ///CustomListModel* model = searchable_list->getModel();
    auto* proxy = qobject_cast<CustomFilterProxyModel*>(
        const_cast<QAbstractItemModel*>(index.model())
    );

    if (!proxy)
        return nullptr;

    auto sourceIndex = proxy->mapToSource(index);

    auto* model = qobject_cast<CustomListModel*>(proxy->sourceModel());
    if (!model)
        return nullptr;

    SearchableListItem& item = model->itemAt(sourceIndex.row());
    item.old_txt = index.data(Qt::DisplayRole).toString();

    current_editor = editor;
    return editor;
}

void StyledItemDelegate::closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint)
{
    QStyledItemDelegate::closeEditor(editor, hint);
}

void StyledItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor))
    {
        QString text = index.data(Qt::DisplayRole).toString();
        lineEdit->setText(text);
    }
}

void StyledItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    //SearchableListItem item = index.data(Qt::UserRole).value<SearchableListItem>();

    SearchableListView* list_view = qobject_cast<SearchableListView*>(parent());
    SearchableList* searchable_list = qobject_cast<SearchableList*>(list_view->parent());

    int editor_margin_left = searchable_list->editorMarginLeft();
    int editor_margin_right = searchable_list->editorMarginRight();

    // Example: shrink the editor to 70% width, centered
    QRect fullRect = option.rect;
    //int margin = fullRect.width() * 0.15;

    QRect adjusted = fullRect.adjusted(editor_margin_left, 0, -editor_margin_right, 0);
    editor->setGeometry(adjusted);
}

/*bool StyledItemDelegate::eventFilter(QObject* obj, QEvent* event)
{
    SearchableListView* list_view = qobject_cast<SearchableListView*>(obj);
    if (!list_view)
        return false;

    SearchableList* searchable_list = qobject_cast<SearchableList*>(list_view->parent());

    if (event->type() == QEvent::Leave)
    {
        if (searchable_list->item_mousemove_callback)
        {
            QStyleOptionViewItem option;
            option.initFrom(list_view);

            for (int row = 0; row < list_view->model()->rowCount(); ++row)
            {
                QModelIndex index = list_view->model()->index(row, 0);
                SearchableListItem item = index.data(Qt::UserRole).value<SearchableListItem>();

                ListItemCallbackData cb_data = {
                    list_view,
                    this,
                    option,
                    nullptr,
                    index,
                    list_view->mouse_pos,
                };
                searchable_list->item_mousemove_callback(item, cb_data);
            }
        }
        list_view->viewport()->update();
    }

    return false;  // Continue default event processing
}*/

void ListItemCallbackData::drawSelectionHighlightRect(QPainter* painter)
{
    auto selected_indexes = list_view->selectionModel()->selectedIndexes();
    int selected_row = selected_indexes.empty() ? -1 : selected_indexes.at(0).row();
    bool selected = (selected_row == model_index.row());
    if (selected)
        painter->fillRect(style_option_view.rect, QColor(55, 138, 221));
}

bool ListItemCallbackData::overIcon(QRectF r)
{
    return r.adjusted(-3, -3, 3, 3).contains(mouse_pos);
}

bool ListItemCallbackData::overRect(QRectF r)
{
    return r.contains(mouse_pos);
}

void ListItemCallbackData::drawIconBorder(QPainter* painter, QRectF r)
{
    // Highlight shape icon border
    painter->setPen(Qt::black);
    painter->drawRect(r.adjusted(-3, -3, 1, 1));
    painter->setPen(Qt::white);
    painter->drawRect(r.adjusted(-4, -4, 2, 2));
}

void ListItemCallbackData::drawXIcon(QPainter* painter, QRectF r)
{
    // Draw "missing" X icon
    QPen pen(Qt::red, 2);
    painter->setPen(pen);
    painter->drawLine(r.topLeft(), r.bottomRight());
    painter->drawLine(r.topRight(), r.bottomLeft());
}

void ListItemCallbackData::drawIcon(QPainter* painter, QRectF r, const QByteArray& icon)
{
    painter->save();

    // Disable anti-aliasing
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, false);

    QSvgRenderer renderer;
    renderer.load(icon);
    renderer.render(painter, r);

    painter->restore();
}

void ListItemCallbackData::drawRowText(QPainter* painter, QString txt, int x, bool center_text, QColor color_default, QColor color_selected)
{
    QFontMetrics fm(style_option_view.font);

    auto selected_indexes = list_view->selectionModel()->selectedIndexes();
    int selected_row = selected_indexes.empty() ? -1 : selected_indexes.at(0).row();
    bool selected = (selected_row == model_index.row());
    int txt_y = style_option_view.rect.y() + (style_option_view.rect.height() - fm.height()) / 2;

    QRect txt_rect(style_option_view.rect);
    txt_rect.setLeft(style_option_view.rect.x() + x - (center_text ? fm.horizontalAdvance(txt) / 2 : 0));
    txt_rect.setY(txt_y);

    painter->setPen(selected ? color_selected : color_default);
    painter->drawText(txt_rect, txt);
}
