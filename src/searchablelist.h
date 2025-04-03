#ifndef SEARCHABLELIST_H
#define SEARCHABLELIST_H

#include <QWidget>
#include <QScrollbar>
#include <QListView>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QMouseEvent>
#include <QPointer>

#include <any>

namespace Ui {
class SearchableList;
}

class SearchableListView;
class StyledItemDelegate;
class SearchableList;

struct ListItemCallbackData
{
    SearchableListView* list_view;
    const StyledItemDelegate* item_delegate;
    const QStyleOptionViewItem& style_option_view;

    QAbstractItemModel* model;
    const QModelIndex& model_index;

    QPoint mouse_pos;
    QMouseEvent* mouse_event;

    //

    void drawSelectionHighlightRect(QPainter* painter);

    bool overIcon(QRectF r);
    bool overRect(QRectF r);
    void drawIconBorder(QPainter* painter, QRectF r);

    void drawXIcon(QPainter* painter, QRectF r);
    void drawIcon(QPainter* painter, QRectF r, const QByteArray &icon);

    void drawRowText(
        QPainter* painter, 
        QString txt, 
        int x, 
        bool center_text=false, 
        QColor color_default= Qt::white, 
        QColor color_selected=Qt::black
    );
};

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
    std::any meta;

public:
    int uid;
    int sort_index = 0;

    QString txt;
    QString old_txt;
    bool editable;


    SearchableListItem(int _uid=-1)
    {
        uid = _uid;
        editable = false;
    }
    SearchableListItem(int _uid, const QString& txt) : txt(txt)
    {
        uid = _uid;
        editable = false;
    }
    SearchableListItem(int _uid, const QString &txt, const std::any &data) : txt(txt), data(data)
    {
        uid = _uid;
        editable = false;
    }
    SearchableListItem(const SearchableListItem& rhs)
    {
        data = rhs.data;
        meta = rhs.meta;
        uid = rhs.uid;
        sort_index = rhs.sort_index;
        txt = rhs.txt;
        editable = rhs.editable;
    }

    QRect itemRect()
    {

    }

    template<typename T> T as()
    {
        return std::any_cast<T>(data);
    }

    void setData(const std::any& _data)
    {
        data = _data;
    }

    void setEditable(bool b=true)
    {
        editable = b;
    }
};

Q_DECLARE_METATYPE(SearchableListItem)

class CustomListModel : public QAbstractListModel
{
    Q_OBJECT;

    

public:
    explicit CustomListModel(QObject* parent = nullptr)
        : QAbstractListModel(parent) 
    {}

    void setItems(const QList<SearchableListItem>& items) {
        beginResetModel();
        m_items = items;

        endResetModel();
    }

    QList<SearchableListItem> getItems() const
    {
        return m_items;
    }

    SearchableListItem& itemAt(int index)
    {
        Q_ASSERT(index >= 0 && index < m_items.size());
        return m_items[index];
    }

    //int itemCount() const { return m_items.size(); }

    std::unordered_map<QString,int> uid_indexes;
    void lockSortOrder()
    {
        uid_indexes.clear();
        for (int i = 0; i < m_items.size(); i++)
            uid_indexes[m_items[i].txt] = i;
    }

    void unlockSortOrder()
    {
        beginResetModel();
        std::sort(m_items.begin(), m_items.end(),
            [this](const SearchableListItem& a, const SearchableListItem& b)
        {
            return uid_indexes[a.txt] < uid_indexes[b.txt];
        });
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
            if (m_items[i].uid == item->uid)
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

    Qt::ItemFlags flags(const QModelIndex& index) const
    {
        if (!index.isValid())
            return Qt::NoItemFlags;

        const SearchableListItem& item = m_items.at(index.row());

        if (item.editable)
            return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
        else
            return QAbstractItemModel::flags(index);

    }


    QVariant data(const QModelIndex& index, int role) const override 
    {
        if (!index.isValid() || index.row() >= m_items.size())
            return QVariant();

        const SearchableListItem& item = m_items.at(index.row());

        switch (role) {
        case Qt::DisplayRole: return item.txt;// +" (" + (item.editable ? "editable)" : "fixed)");
        case Qt::UserRole: return QVariant::fromValue(item);  // Custom role
        //case Qt::BackgroundRole: return item.color;
        default: return QVariant();
        }
    }

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override
    {
        if (!index.isValid() || index.row() >= m_items.size())
            return false;

        if (role == Qt::EditRole)
        {
            m_items[index.row()].txt = value.toString();
            emit dataChanged(index, index, { role });
            return true;
        }

        return false;
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
    explicit CustomFilterProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) 
    {
        //setSortRole(Qt::UserRole);
        //setDynamicSortFilter(true);
    }

    //bool lessThan(const QModelIndex& left, const QModelIndex& right) const override 
    //{
    //    int left_uid = left.data(Qt::UserRole).value<SearchableListItem>().uid;
    //    int right_uid = right.data(Qt::UserRole).value<SearchableListItem>().uid;
    //    return left_uid < right_uid;
    //}

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

class CommittingLineEdit : public QLineEdit 
{
    Q_OBJECT;

    bool committed = false;

public:
    explicit CommittingLineEdit(QWidget* parent = nullptr) : QLineEdit(parent) {}

signals:
    void commitAndClose(QWidget* editor);  // This will be connected in the delegate

protected:
    void focusOutEvent(QFocusEvent* event) override {
        QLineEdit::focusOutEvent(event);
        triggerCommit();
    }

    void keyPressEvent(QKeyEvent* event) override {
        if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && !committed) {
            triggerCommit();
        }
        else {
            QLineEdit::keyPressEvent(event);
        }
    }

private:
    void triggerCommit() {
        if (!committed) {
            committed = true;
            emit commitAndClose(this);
        }
    }
};


class StyledItemDelegate : public QStyledItemDelegate 
{
    Q_OBJECT;

    int row_size = 16;

    mutable QPointer<QWidget> current_editor;


public:
    StyledItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent)
    {

    }

    void setRowHeight(int height)
    {
        row_size = height;
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    bool eventFilter(QObject* obj, QEvent* event);

    bool editorEvent(
        QEvent* event, 
        QAbstractItemModel* model,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) override;

    QRectF getIconRect(
        const QStyleOptionViewItem& option,
        const QModelIndex& index,
        int iconIndex,
        bool from_end=false,
        qreal icon_margin=0,
        qreal icon_spacing=0,
        qreal start_spacing=4,
        qreal overrideSide=-1) const;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const;

    void closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint);

    // Default text when text-edit shown
    void setEditorData(QWidget* editor, const QModelIndex& index) const;

    void updateEditorGeometry(QWidget* editor,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const;


    void commitData(QWidget* editor)
    {
        qDebug() << "commitData called on" << editor;
        QStyledItemDelegate::commitData(editor);
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
    {
        if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor))
        {
            QString newText = lineEdit->text();
            qDebug() << "User entered:" << newText;

            model->setData(index, newText, Qt::EditRole);

            const SearchableListItem& item = index.data(Qt::UserRole).value<SearchableListItem>();
        }
    }

signals:

    //void iconClicked(const QModelIndex& index);
    //void dataChanged(const QModelIndex& index, QString txt);
};

class SearchableListView : public QListView
{
    Q_OBJECT;

    friend class StyledItemDelegate;

public:
    explicit SearchableListView(QWidget* parent = nullptr) : QListView(parent) 
    {
        setMouseTracking(true);
    }

    //void mouseMoveEvent(QMouseEvent* event) override;

    QPoint getMousePos() const 
    {
        return mouse_pos; 
    }

private:
    QPoint mouse_pos;

protected:
    //void resizeEvent(QResizeEvent* event) override
    //{
    //    QListView::resizeEvent(event);
    //    int scrollbarWidth = verticalScrollBar()->isVisible() ? verticalScrollBar()->width() : 0;
    //    setViewportMargins(0, 0, scrollbarWidth, 0);
    //}
};

class SearchableList : public QWidget
{
    Q_OBJECT

    std::string field_id;
    QString name;

    bool filterable = true;
    std::shared_ptr<std::vector<SearchableList*>> other_lists = nullptr;

    //std::vector<SearchableListItem> all_items;

    int editor_margin_left = 0;
    int editor_margin_right = 0;

    int SearchableListItem_UID_Counter = 0;

protected:

    void _setRadioChecked(bool b, bool blockSignals=false);

public:

    std::function<void(bool)> on_radio_toggle = nullptr;

    std::function<bool(SearchableListItem&, QPainter*, ListItemCallbackData&)>
        item_paint_callback = nullptr;
    //std::function<void(SearchableListItem&)> 
    //    icon_click_callback = nullptr;
    
    std::function<void(SearchableListItem&, ListItemCallbackData&)>
        item_mousemove_callback = nullptr;
    std::function<void(SearchableListItem&, ListItemCallbackData&)>
        item_click_callback = nullptr;
    std::function<void(SearchableListItem&, ListItemCallbackData&)>
        item_doubleclick_callback = nullptr;
    

signals:

    void onChangedSelected(SearchableListItem&);
    void itemModified(SearchableListItem&);

    //void onChangedSelectedIndex(const QModelIndex&);

public:

    explicit SearchableList(QWidget* parent = nullptr);

    ~SearchableList();

    SearchableList* init(const std::string field_id, const QString name);

    void setName(const QString& name);
    const std::string &fieldId() { return field_id; }

    //template<typename T>
    void setCustomWidget(QWidget *w);

    void refreshLayoutUI();
    void setFilterable(bool b);

    void setRadioGroup(std::shared_ptr<std::vector<SearchableList*>> other_lists=nullptr);
    void setRadioChecked(bool b);
    bool getRadioChecked();

    void onRadioToggled(std::function<void(bool b)> _onToggle)
    {
        on_radio_toggle = _onToggle;
    }

    void onItemPaint(std::function<bool(SearchableListItem&, QPainter*, ListItemCallbackData&)> _item_paint_callback)
    {
        item_paint_callback = _item_paint_callback;
    }

    /*void onClickIcon(std::function<void(SearchableListItem&)> _icon_click_callback)
    {
        icon_click_callback = _icon_click_callback;
    }*/
    
    void onItemClick(std::function<void(SearchableListItem&, ListItemCallbackData &)> _item_click_callback)
    {
        item_click_callback = _item_click_callback;
    }

    void onItemDoubleClick(std::function<void(SearchableListItem&, ListItemCallbackData&)> _item_doubleclick_callback)
    {
        item_doubleclick_callback = _item_doubleclick_callback;
    }

    void onItemMouseMove(std::function<void(SearchableListItem&, ListItemCallbackData&)> _item_mousemove_callback)
    {
        item_mousemove_callback = _item_mousemove_callback;
    }

    void lockSortOrder()
    {
        model.lockSortOrder();
    }

    void unlockSortOrder()
    {
        model.unlockSortOrder();
    }

    void clear()
    {
        model.clear();
        SearchableListItem_UID_Counter = 0;
    }

    void addListItem(QString txt)
    {
        model.appendRow(SearchableListItem(SearchableListItem_UID_Counter++, txt));
    }

    template<typename T>
    void addListItem(QString txt, const T& data)
    {
        model.appendRow(SearchableListItem(SearchableListItem_UID_Counter++, txt, data));
    }

    SearchableListItem *addUniqueListItem(QString txt)
    {
        if (model.find(txt) < 0)
            model.appendRow(SearchableListItem(SearchableListItem_UID_Counter++, txt));
        return findItem(txt);
    }

    template<typename T>
    void addUniqueListItem(QString txt, const T& data)
    {
        if (model.find(txt) < 0)
            model.appendRow(SearchableListItem(SearchableListItem_UID_Counter++, txt, data));
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

    CustomListModel* getModel()
    {
        return &model;
    }

    SearchableListItem* findItem(QString txt)
    {
        return model.findItem(txt);
    }

    void refresh();
    void repaint();

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
    void beginEdit(SearchableListItem* item);

    void setRowHeight(int height);
    int editorMarginLeft() { return editor_margin_left; }
    int editorMarginRight() { return editor_margin_right; }
    void setEditorMarginLeft(int margin) { editor_margin_left = margin; }
    void setEditorMarginRight(int margin) { editor_margin_right = margin; }
    void setEditorMargins(int left, int right) { 
        editor_margin_left = left;
        editor_margin_right = right;
    }

    const SearchableListView* getListView() const;

private:

    Ui::SearchableList *ui;
    QWidget* custom_widget = nullptr;

    CustomListModel  model;
    CustomFilterProxyModel proxyModel;

    StyledItemDelegate* item_delegate;

};

#endif // SEARCHABLELIST_H
