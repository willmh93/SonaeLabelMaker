#include "pageoptions.h"
#include "ui_pageoptions.h"

PageOptions::PageOptions(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageOptions)
{
    ui->setupUi(this);
}

PageOptions::~PageOptions()
{
    delete ui;
}
