#include "searchablelist.h"
#include "ui_searchablelist.h"


#include <QEvent>
#include <QMouseEvent>


SearchableList::SearchableList(QWidget *parent) : QWidget(parent), ui(new Ui::SearchableList)
{
    ui->setupUi(this);
    
    if (other_lists == nullptr)
        ui->combo->hide();

    item_delegate = new XItemDelegate(ui->list);

    proxyModel.setSourceModel(&model);
    ui->list->setModel(&proxyModel);
    ui->list->setItemDelegate(item_delegate);

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

    connect(ui->list, &QListView::doubleClicked, this, [this](const QModelIndex& index)
    {
        if (item_doubleclick_callback)
        {
            SearchableListItem item = index.data(Qt::UserRole).value<SearchableListItem>();
            item_doubleclick_callback(item);
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
    //if (custom_widget)
    {
        //ui->tools_frame->removeWidget(w);
        //delete custom_widget;
    }
    //custom_widget = w;

    //w->setParent(this);
    //w->setLayout(ui->tools_frame->layout());
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

void SearchableList::setRowHeight(int height)
{
    item_delegate->setRowHeight(height);
    //ui->list->setGridSize(QSize(0, height)); // Increase row height to 50 pixels
    //listView->setIconSize(QSize(height, height));
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

QSize XItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return QSize(option.rect.width(), row_size); // Set row height to 50px
}

void XItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();

    // Draw the default background and focus rectangle
    QStyledItemDelegate::paint(painter, option, index);

    // Define the size and position of the "X"
    //int xOffset = 5;  // Offset from left
    /*int yOffset = option.rect.height() / 4; // Center it vertically
    int size = option.rect.height() / 2;  // Scale with row height

    // Get the text and calculate its bounding box
    QString text = index.data(Qt::DisplayRole).toString();
    QFontMetrics fm(option.font);
    int textWidth = fm.horizontalAdvance(text);  // Get text width

    int xOffset = textWidth + 10;
    QRect xRect(xOffset, option.rect.y() + yOffset, size, size);*/

    QRect xRect = getIconRect(option, index, option.rect);

    SearchableListItem item = index.data(Qt::UserRole).value<SearchableListItem>();
    QListView* listView = qobject_cast<QListView*>(this->parent());
    SearchableList* searchableList = qobject_cast<SearchableList*>(listView->parent());

    if (searchableList->icon_painter)
    {
        searchableList->icon_painter(item, painter, xRect);
    }

    // Draw the red "X"
    /*QPen pen(Qt::red, 2);
    painter->setPen(pen);
    painter->drawLine(xRect.topLeft(), xRect.bottomRight());
    painter->drawLine(xRect.topRight(), xRect.bottomLeft());*/

    painter->restore();

    // Adjust text position by shifting right
    //QStyleOptionViewItem textOption(option);
    //textOption.rect.setX(option.rect.x() + xRect.width() + 10);  // Move text right
    //QStyledItemDelegate::paint(painter, textOption, index);
}

bool XItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (event->type() == QEvent::MouseButtonPress) 
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        QRect iconRect = getIconRect(option, index, option.rect);

        if (iconRect.contains(mouseEvent->pos()))
        {
            SearchableListItem item = index.data(Qt::UserRole).value<SearchableListItem>();
            QListView* listView = qobject_cast<QListView*>(this->parent());
            SearchableList* searchableList = qobject_cast<SearchableList*>(listView->parent());

            if (searchableList->icon_click_callback)
                searchableList->icon_click_callback(item);

            //emit iconClicked(index);  // Emit a signal when the icon is clicked
            return true;
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
