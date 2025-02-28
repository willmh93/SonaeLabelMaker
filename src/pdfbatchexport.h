#ifndef PDFBATCHEXPORT_H
#define PDFBATCHEXPORT_H

#include <QDialog>
#include <QStandardItem>
#include <QStandardItemModel>

namespace Ui {
class PDFBatchExport;
}

class PDFBatchExport : public QDialog
{
    Q_OBJECT;

    QStandardItemModel model;

public:
    explicit PDFBatchExport(QWidget *parent = nullptr);
    ~PDFBatchExport();

    void setProgress(float v);
    bool isSinglePDF();

    void clearLog();
    void addLogMessage(const QString& msg);
    void addLogError(const QString& msg);

signals:

    void beginExport();


private:
    Ui::PDFBatchExport *ui;
};

#endif // PDFBATCHEXPORT_H
