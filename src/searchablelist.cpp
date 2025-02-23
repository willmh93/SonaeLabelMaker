#include "searchablelist.h"
#include "ui_searchablelist.h"

#include <QLineEdit.h>

SearchableList::SearchableList(
    const std::string _field_id,
    const QString name, 
    QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SearchableList)
{
    ui->setupUi(this);
    field_id = _field_id;
    setName(name);

    proxyModel.setSourceModel(&model);
    ui->list->setModel(&proxyModel);

    connect(ui->filter, &QLineEdit::textChanged, this, [this](const QString& filter)
    {
        proxyModel.setFilterString(filter);
    });
    
    connect(ui->list->selectionModel(), &QItemSelectionModel::currentChanged, this,
        [this](const QModelIndex& current, const QModelIndex& previous)
    {
        if (current.isValid())
        {
            QVariant data = current.data(Qt::UserRole); // Get custom data
            SearchableListItem item = data.value<SearchableListItem>();
            emit onChangedSelected(item);
            //qDebug() << "Selected item value:" << item.data;
        }

        //if (!selected.indexes().isEmpty()) 
        {
            //QVariant data = current.data(Qt::UserRole); // Get custom data

            //QModelIndex index = selected.indexes().first();

            //ui->list->item index.row
            //emit onChangedSelected(index.data().toString());
        }
    });

    
    //filteredPopulate("");
}

SearchableList::~SearchableList()
{
    delete ui;
}

void SearchableList::setName(const QString &name)
{
    ui->nameLabel->setText(name);
}

void SearchableList::setCurrentItem(int i)
{
    QModelIndex modelIndex = proxyModel.index(i, 0);
    ui->list->setCurrentIndex(modelIndex);
    ui->list->selectionModel()->select(modelIndex, QItemSelectionModel::Select);

}

void SearchableList::setCurrentItem(const QString& txt)
{
    int i = proxyModel.find(txt);
    QModelIndex modelIndex = proxyModel.index(i, 0);
    ui->list->setCurrentIndex(modelIndex);
    ui->list->selectionModel()->select(modelIndex, QItemSelectionModel::Select);
}

void SearchableList::setCurrentItem(SearchableListItem* item)
{
    int i = model.find(item);
    QModelIndex modelIndex = proxyModel.index(i, 0);
    ui->list->setCurrentIndex(modelIndex);
    ui->list->selectionModel()->select(modelIndex, QItemSelectionModel::Select);
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