#ifndef FILE_UPLOADED_H
#define FILE_UPLOADED_H

#include <QObject>
#include <QMetaObject>
#include <QFile>
#include <functional>

class FileManager : public QObject
{
    Q_OBJECT

public:
    static FileManager *current_instance;
    std::function<void(const QString& filename, const QByteArray& data)> file_recieved_callback = nullptr;

    explicit FileManager(QObject *parent=nullptr);

signals:
    void fileRecieved(const QString &filename, const QByteArray &data);

public:
    void doLoad(
        QString caption,
        QString filter,
        std::function<void(const QString& filename, const QByteArray& data)> callback);

    void doSave(QString caption, QString filename_hint, QString filter, const char* data, size_t length);
    void doSave(QString caption, QString filename_hint, QString filter, const QByteArray &data);
};

#endif


