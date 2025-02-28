#ifndef SEARCHABLELIST_H
#define SEARCHABLELIST_H

#include <QWidget>
#include <QListView>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QPainter>


#include <any>

namespace Ui {
class SearchableList;
}

class FocusableLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    FocusableLineEdit(QWidget* parent = nullptr) : QLineEdit(parent) {}

protected:
    void focusInEvent(QFocusEvent* event) override {
        QLineEdit::focusInEvent(event);  // Call base class implementation
        qDebug() << "Line edit focused";
        // Emit custom signal if needed
        emit focusGained();
    }

    void focusOutEvent(QFocusEvent* event) override {
        QLineEdit::focusOutEvent(event);  // Call base class implementation
        qDebug() << "Line edit lost focus";
        // Emit custom signal if needed
        emit focusLost();
    }

signals:
    void focusGained();
    void focusLost();
};

struct SearchableListItem
{
private:
    std::any data;
public:
    QString txt;

    SearchableListItem()
    {}
    SearchableListItem(const QString& txt) : txt(txt)
    {}
    SearchableListItem(const QString &txt, const std::any &data) : txt(txt), data(data)
    {}

    template<typename T> T as()
    {
        return std::any_cast<T>(data);
    }

    void setData(const std::any& _data)
    {
        data = _data;
    }
};

Q_DECLARE_METATYPE(SearchableListItem)

class CustomListModel : public QAbstractListModel
{
    Q_OBJECT;

    

public:
    explicit CustomListModel(QObject* parent = nullptr)
        : QAbstractListModel(parent) {}

    void setItems(const QList<SearchableListItem>& items) {
        beginResetModel();
        m_items = items;
        endResetModel();
    }

    void appendRow(const SearchableListItem& item)
    {
        beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
        m_items.append(item);
        endInsertRows();
    }

    int find(QString txt)
    {
        for (qsizetype i = 0; i < m_items.size(); i++)
        {
            if (m_items[i].txt == txt)
                return i;
        }
        return -1;
    }

    int find(SearchableListItem *item)
    {
        for (qsizetype i = 0; i < m_items.size(); i++)
        {
            if (&m_items[i] == item)
                return i;
        }
        return -1;
    }

    SearchableListItem *findItem(QString txt)
    {
        for (qsizetype i = 0; i < m_items.size(); i++)
        {
            if (m_items[i].txt == txt)
                return &m_items[i];
        }
        return nullptr;
    }

    /*int indexOf(SearchableListItem* item)
    {
        for (qsizetype i = 0; i < m_items.size(); i++)
        {
            if (&m_items[i] == item)
                return i;
        }
        return -1;
    }*/

    void clear()
    {
        beginResetModel();
        m_items.clear();
        endResetModel();
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        Q_UNUSED(parent);
        return m_items.size();
    }

    QVariant data(const QModelIndex& index, int role) const override {
        if (!index.isValid() || index.row() >= m_items.size())
            return QVariant();

        const SearchableListItem& item = m_items.at(index.row());

        

        switch (role) {
        case Qt::DisplayRole: return item.txt;
        case Qt::UserRole: return QVariant::fromValue(item);  // Custom role
        //case Qt::BackgroundRole: return item.color;
        default: return QVariant();
        }
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
            return QStringLiteral("Items");
        return QVariant();
    }

private:

    QList<SearchableListItem> m_items;
};

class CustomFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit CustomFilterProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

    void setFilterString(const QString& filter) {
        m_filterString = filter;
        invalidateFilter();  // Triggers re-filtering
    }

    int find(QString txt) const
    {
        for (qsizetype row = 0; row < rowCount(); row++)
        {
            QModelIndex proxyIndex = index(row, 0);
            const auto &item = data(proxyIndex, Qt::UserRole).value<SearchableListItem>();
            if (item.txt == txt)
                return row;
        }
        return -1;
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
        if (m_filterString.isEmpty()) {
            return true;
        }

        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        QString itemText = sourceModel()->data(index, Qt::DisplayRole).toString();

        return itemText.contains(m_filterString, Qt::CaseInsensitive);
    }

private:
    QString m_filterString;
};

class SearchableList;

class XItemDelegate : public QStyledItemDelegate 
{
    Q_OBJECT;

    int row_size = 16;

public:
    XItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    void setRowHeight(int height)
    {
        row_size = height;
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    bool editorEvent(
        QEvent* event, 
        QAbstractItemModel* model,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) override;

signals:
    void iconClicked(const QModelIndex& index);

private:
    QRect getIconRect(
        const QStyleOptionViewItem& option,
        const QModelIndex& index, 
        const QRect& itemRect) const
    {
        int yOffset = option.rect.height() / 4; // Center it vertically
        int size = option.rect.height() / 2;  // Scale with row height

        // Get the text and calculate its bounding box
        QString text = index.data(Qt::DisplayRole).toString();
        QFontMetrics fm(option.font);
        int textWidth = fm.horizontalAdvance(text);  // Get text width

        int xOffset = textWidth + 10;
        QRect xRect(xOffset, option.rect.y() + yOffset, size, size);
        return xRect;

    }
};

class SearchableList : public QWidget
{
    Q_OBJECT

    std::string field_id;
    QString name;

    bool filterable = true;
    std::shared_ptr<std::vector<SearchableList*>> other_lists = nullptr;

    //std::vector<SearchableListItem> all_items;

protected:

    void _setRadioChecked(bool b, bool blockSignals=false);

public:

    std::function<void(bool)> on_radio_toggle = nullptr;
    std::function<void(SearchableListItem&, QPainter* painter, QRect&r)> icon_painter = nullptr;
    std::function<void(SearchableListItem&)> icon_click_callback = nullptr;
    std::function<void(SearchableListItem&)> item_doubleclick_callback = nullptr;
    

signals:

    void onChangedSelected(SearchableListItem&);
    //void onChangedSelectedIndex(const QModelIndex&);

public:

    explicit SearchableList(
        const std::string field_id,
        const QString name,
        QWidget* parent = nullptr);

    ~SearchableList();

    void setName(const QString& name);
    const std::string &fieldId() { return field_id; }

    void refreshLayoutUI();
    void setFilterable(bool b);

    void setRadioGroup(std::shared_ptr<std::vector<SearchableList*>> other_lists=nullptr);
    void setRadioChecked(bool b);
    bool getRadioChecked();

    void onRadioToggled(std::function<void(bool b)> _onToggle)
    {
        on_radio_toggle = _onToggle;
    }

    void setIconPainter(std::function<void(SearchableListItem&, QPainter* painter, QRect& r)> _icon_painter)
    {
        icon_painter = _icon_painter;
    }

    void onClickIcon(std::function<void(SearchableListItem&)> _icon_click_callback)
    {
        icon_click_callback = _icon_click_callback;
    }

    void onDoubleClickItem(std::function<void(SearchableListItem&)> _item_doubleclick_callback)
    {
        item_doubleclick_callback = _item_doubleclick_callback;
    }

    void clear()
    {
        model.clear();
    }

    void addListItem(QString txt)
    {
        model.appendRow(SearchableListItem(txt));
    }

    template<typename T>
    void addListItem(QString txt, const T& data)
    {
        model.appendRow(SearchableListItem(txt, data));
    }

    void addUniqueListItem(QString txt)
    {
        if (model.find(txt) < 0)
            model.appendRow(SearchableListItem(txt));
    }

    template<typename T>
    void addUniqueListItem(QString txt, const T& data)
    {
        if (model.find(txt) < 0)
            model.appendRow(SearchableListItem(txt, data));
    }

    template<typename T>
    bool attachItemData(QString txt, const T& data)
    {
        SearchableListItem *item = model.findItem(txt);
        if (item)
        {
            item->setData(data);
            return true;
        }
        return false;
    }

    SearchableListItem* findItem(QString txt)
    {
        return model.findItem(txt);
    }

    void refresh();

    /*SearchableListItem *findItem(QString txt)
    {
        return model.find(txt);
    }*/

    void updateSelectionModel();

    void clearFilter();

    void setSelectable(bool b);
    void setCurrentItem(int i);
    void setCurrentItem(const QString &txt);
    void setCurrentItem(SearchableListItem* item);

    bool getCurrentItem(SearchableListItem* item);

    void setRowHeight(int height);


private:

    Ui::SearchableList *ui;

    CustomListModel  model;
    CustomFilterProxyModel proxyModel;

    XItemDelegate* item_delegate;

};

#endif // SEARCHABLELIST_H
