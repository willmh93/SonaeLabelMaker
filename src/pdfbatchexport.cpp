#include "pdfbatchexport.h"
#include "ui_pdfbatchexport.h"

PDFBatchExport::PDFBatchExport(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PDFBatchExport)
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    ui->exportLog->setModel(&model);
    ui->exportLog->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(ui->beginBtn, &QPushButton::clicked, this, [this]()
    {
        emit beginExport();
    });
}

PDFBatchExport::~PDFBatchExport()
{
    delete ui;
}

void PDFBatchExport::setProgress(float v)
{
    ui->progressBar->setValue((int)(v * 100));
}

bool PDFBatchExport::isSinglePDF()
{
    return ui->singlepdf_checkBox->isChecked();
}

void PDFBatchExport::clearLog()
{
    model.clear();
}

void PDFBatchExport::addLogMessage(const QString& msg)
{
    QStandardItem* item = new QStandardItem(msg);
    model.appendRow(item);

    QModelIndex lastIndex = model.index(model.rowCount() - 1, 0);
    ui->exportLog->scrollTo(lastIndex);
}

void PDFBatchExport::addLogError(const QString& msg)
{
    QStandardItem* item = new QStandardItem(msg);
    item->setForeground(QBrush(QColor(255,50,50)));
    model.appendRow(item);

    QModelIndex lastIndex = model.index(model.rowCount() - 1, 0);
    ui->exportLog->scrollTo(lastIndex);
}
