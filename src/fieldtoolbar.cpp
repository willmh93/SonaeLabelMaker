#include "fieldtoolbar.h"
#include "ui_fieldtoolbar.h"

FieldToolbar::FieldToolbar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FieldToolbar)
{
    ui->setupUi(this);

    connect(ui->add_btn, &QPushButton::clicked, this, [this]()
    {
        emit onAddBtn();
    });

    //connect(ui->remove_btn, &QPushButton::clicked, this, [this]()
    //{
    //    emit onRemoveBtn();
    //});
}

FieldToolbar::~FieldToolbar()
{
    delete ui;
}
