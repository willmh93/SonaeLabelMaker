#include "file_uploader.h"
#include <QDebug>

#ifndef __EMSCRIPTEN__
#include <QFileDialog>
#endif

FileManager* FileManager::current_instance = nullptr;

FileManager::FileManager(QObject* parent) : QObject(parent)
{
    connect(this, &FileManager::fileRecieved, this, [this](const QString& filename, const QByteArray& data) {
        qDebug() << "file_recieved_callback: start";
        if (file_recieved_callback)
            file_recieved_callback(filename, data);
        qDebug() << "file_recieved_callback: end";
    });
}

#ifdef __EMSCRIPTEN__

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
using namespace emscripten;

///// Loading /////
typedef void (*FileDataCallback)(void *context, char *data, size_t length, const char *name);

void readFileContent(val event)
{
    // Copy file content to WebAssembly memory and call the user file data handler
    val fileReader = event["target"];

    // Set up source typed array
    val result = fileReader["result"]; // ArrayBuffer
    val Uint8Array = val::global("Uint8Array");
    val sourceTypedArray = Uint8Array.new_(result);

    // Allocate and set up destination typed array
    size_t size = result["byteLength"].as<size_t>();
    void *buffer = malloc(size);
    val destinationTypedArray = Uint8Array.new_(val::module_property("HEAPU8")["buffer"], size_t(buffer), size);
    destinationTypedArray.call<void>("set", sourceTypedArray);

    // Call user file data handler
    FileDataCallback fileDataCallback = reinterpret_cast<FileDataCallback>(fileReader["data-filesReadyCallback"].as<size_t>());
    void *context = reinterpret_cast<void *>(fileReader["data-filesReadyCallback"].as<size_t>());
    fileDataCallback(context, static_cast<char *>(buffer), size, fileReader["data-name"].as<std::string>().c_str());
}

void readFiles(val event)
{
    // Read all selcted files using FileReader
    val target = event["target"];
    val files = target["files"];
    int fileCount = files["length"].as<int>();
    for (int i = 0; i < fileCount; i++) {
        val file = files[i];
        val fileReader = val::global("FileReader").new_();
        fileReader.set("onload", val::module_property("qtReadFileContent"));
        fileReader.set("data-filesReadyCallback", target["data-filesReadyCallback"]);
        fileReader.set("data-filesReadyCallbackContext", target["data-filesReadyCallbackContext"]);
        fileReader.set("data-name", file["name"]);
        fileReader.call<void>("readAsArrayBuffer", file);
    }
}

void loadFile(const char *accept, FileDataCallback callback, void *context)
{
    // Create file input element which will dislay a native file dialog.
    val document = val::global("document");
    val input = document.call<val>("createElement", std::string("input"));
    input.set("type", "file");
    input.set("style", "display:none");
    input.set("accept", val(accept));

    // Set JavaScript onchange callback which will be called on file(s) selected,
    // and also forward the user C callback pointers so that the onchange
    // callback can call it. (The onchange callback is actually a C function
    // exposed to JavaScript with EMSCRIPTEN_BINDINGS).
    input.set("onchange", val::module_property("qtReadFiles"));
    input.set("data-filesReadyCallback", val(size_t(callback)));
    input.set("data-filesReadyCallbackContext", val(size_t(context)));

    // Programatically activate input
    val body = document["body"];
    body.call<void>("appendChild", input);
    input.call<void>("click");
    body.call<void>("removeChild", input);
}


extern "C" EMSCRIPTEN_KEEPALIVE void openFileDialog()
{
    void *context = nullptr;
    loadFile("*", [](void *context, char *contentPointer, size_t contentSize, const char *fileName)
    {
        QString fileNameStr = QString::fromUtf8(fileName);
        QByteArray fileData(reinterpret_cast<const char*>(contentPointer), contentSize);

        QMetaObject::invokeMethod(
            FileManager::current_instance,
            "fileRecieved",
            Qt::QueuedConnection,
            Q_ARG(QString, fileNameStr),
            Q_ARG(QByteArray, fileData)
        );

        qDebug() << "contentPointer: " << contentPointer;
        free(contentPointer);
    }, context);
}

///// Saving /////
typedef void (*FileSaveCallback)(bool success, void *context);

void saveFile(const char *data, size_t length, std::wstring fileNameHint)
{
    // Create file data Blob
    val Blob = val::global("Blob");
    val contentArray = val::array();
    val content = val(typed_memory_view(length, data));
    contentArray.call<void>("push", content);
    val type = val::object();
    type.set("type","application/octet-stream");
    val fileBlob = Blob.new_(contentArray, type);

    // Create Blob download link
    val document = val::global("document");
    val link = document.call<val>("createElement", std::string("a"));
    link.set("download", fileNameHint);
    val window = val::global("window");
    val URL = window["URL"];
    link.set("href", URL.call<val>("createObjectURL", fileBlob));
    link.set("style", "display:none");

    // Programatically click link
    val body = document["body"];
    body.call<void>("appendChild", link);
    link.call<void>("click");
    body.call<void>("removeChild", link);
}

/*void saveFile(const char *data, size_t length, const char *fileExtension, const char *defaultFileName, FileSaveCallback callback, void *context)
{
    val document = val::global("document");
    val button = document.call<val>("createElement", std::string("button"));
    button.set("style", "display:none");

    // Store callback pointer as attributes on the button
    button.set("data-filesSaveCallback", val(size_t(callback)));
    button.set("data-filesSaveCallbackContext", val(size_t(context)));

    // Convert data to Uint8Array
    val uint8Array = val::global("Uint8Array").new_(length);
    val memoryView = val(typed_memory_view(length, data));
    uint8Array.call<void>("set", memoryView);

    // Construct JavaScript array properly
    val args = val::array();
    args.call<void>("push", uint8Array);
    args.call<void>("push", val(fileExtension));
    args.call<void>("push", val(defaultFileName));
    args.call<void>("push", button);

    // Call JavaScript function with correct array format
    val::global("qtWriteFile").call<void>("apply", val::null(), args);

    // Append and remove the button to mimic the load dialog approach
    val body = document["body"];
    body.call<void>("appendChild", button);
    body.call<void>("removeChild", button);
}*/

extern "C" EMSCRIPTEN_KEEPALIVE void callSaveCallback(size_t callbackPtr, int success, size_t contextPtr)
{
    FileSaveCallback callback = reinterpret_cast<FileSaveCallback>(callbackPtr);
    if (callback)
        callback(success, reinterpret_cast<void *>(contextPtr));
}

// Example usage
void onFileSaved(bool success, void *context)
{
    if (success)
        printf("File saved successfully!\n");
    else
        printf("File save failed or was cancelled.\n");
}

extern "C" EMSCRIPTEN_KEEPALIVE void saveFileContent(const char *data, size_t length, const std::wstring filename_hint)
{
    //saveFile(data, length, ".bin", "foofile.bin", onFileSaved, nullptr);
    //saveFile(data, length, filename_hint, onFileSaved, nullptr);
    saveFile(data, length, filename_hint);
}

/*extern "C" EMSCRIPTEN_KEEPALIVE void testSaveFile()
{
    const char *data = "foocontent";
    std::wstring name = L"foofile";
    saveFile(data, strlen(data), name);
}*/


EMSCRIPTEN_BINDINGS(localfileaccess) {
    function("qtReadFiles", &readFiles);
    function("qtReadFileContent", &readFileContent);
    //function("qtSaveFileContent", &saveFileContent);
};

////// Save & Load Wrapper //////

void FileManager::doLoad(
    QString caption,
    QString filter,
    std::function<void(const QString& filename, const QByteArray& data)> callback)
{
    FileManager::current_instance = this;
    file_recieved_callback = callback;

    openFileDialog();
}

void FileManager::doSave(
    QString caption, // Ignored on web
    QString filename_hint,
    QString filter, // Ignore on web
    const char* data,
    size_t length)
{
    FileManager::current_instance = this;

    saveFile(data, length, filename_hint.toStdWString());
}

void FileManager::doSave(
    QString caption, // Ignored on web
    QString filename_hint,
    QString filter, // Ignore on web
    const QByteArray &data)
{
    FileManager::current_instance = this;
    saveFile(data.data(), data.size(), filename_hint.toStdWString());
}

#else

void FileManager::doLoad(
    QString caption, 
    QString filter,
    std::function<void(const QString& filename, const QByteArray& data)> callback)
{
    FileManager::current_instance = this;
    file_recieved_callback = callback;

    QString fileName = QFileDialog::getOpenFileName(nullptr, caption, "", filter);

    if (!fileName.isEmpty())
    {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Could not open file:" << fileName;
            return;
        }

        QByteArray fileData = file.readAll();
        file.close();

        QMetaObject::invokeMethod(
            FileManager::current_instance,
            "fileRecieved",
            Qt::QueuedConnection,
            Q_ARG(QString, fileName),
            Q_ARG(QByteArray, fileData)
        );
    }
}

void FileManager::doSave(QString caption, QString filename_hint, QString filter, const char* data, size_t length)
{
    FileManager::current_instance = this;

    QString fileName = QFileDialog::getSaveFileName(nullptr, caption, "", filter);

    if (filename_hint.size())
    {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "Could not create file: " << fileName;
            return;
        }

        file.write(data, length);
        file.close();
    }
}

void FileManager::doSave(QString caption, QString filename_hint, QString filter, const QByteArray& data)
{
    doSave(caption, filename_hint, filter, data.data(), data.size());
}

#endif
