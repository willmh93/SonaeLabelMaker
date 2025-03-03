#include "fieldtoolbar.h"
#include "ui_fieldtoolbar.h"

FieldToolbar::FieldToolbar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FieldToolbar)
{
    ui->setupUi(this);
}

FieldToolbar::~FieldToolbar()
{
    delete ui;
}
