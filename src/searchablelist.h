#ifndef SEARCHABLELIST_H
#define SEARCHABLELIST_H

#include <QWidget>
#include <QListView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QSortFilterProxyModel>
#include <any>

namespace Ui {
class SearchableList;
}


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
};

Q_DECLARE_METATYPE(SearchableListItem)

class CustomListModel : public QAbstractListModel
{
    Q_OBJECT

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


class SearchableList : public QWidget
{
    Q_OBJECT

    std::string field_id;
    //std::vector<SearchableListItem> all_items;

public:

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

    /*SearchableListItem *findItem(QString txt)
    {
        return model.find(txt);
    }*/

    void setCurrentItem(int i);
    void setCurrentItem(const QString &txt);
    void setCurrentItem(SearchableListItem *item);

private:

    Ui::SearchableList *ui;

    CustomListModel  model;
    CustomFilterProxyModel proxyModel;

};

#endif // SEARCHABLELIST_H
