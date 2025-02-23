#ifndef FILE_UPLOADED_H
#define FILE_UPLOADED_H

#include <QObject>
#include <QMetaObject>
#include <QFile>

class FileUploader : public QObject
{
    Q_OBJECT

public:
    static FileUploader *current_instance;

    explicit FileUploader(QObject *parent=nullptr) : QObject(parent)
    {
        FileUploader::current_instance = this;
    }

signals:
    void fileRecieved(const QString &filename, const QByteArray &data);

public:
    void openDialog();
};

#endif


