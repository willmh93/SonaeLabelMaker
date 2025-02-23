#include "file_uploader.h"

#ifndef __EMSCRIPTEN__
#include <QFileDialog>
#endif

FileUploader* FileUploader::current_instance = nullptr;


#ifdef __EMSCRIPTEN__

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
using namespace emscripten;

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

EMSCRIPTEN_BINDINGS(localfileaccess) {
    function("qtReadFiles", &readFiles);
    function("qtReadFileContent", &readFileContent);
};

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
            FileUploader::current_instance,
            "fileRecieved",
            Qt::QueuedConnection,
            Q_ARG(QString, fileNameStr),
            Q_ARG(QByteArray, fileData)
        );

        free(contentPointer);
    }, context);
}

void FileUploader::openDialog()
{
    openFileDialog();
}

#else


void FileUploader::openDialog()
{
    QString fileName = QFileDialog::getOpenFileName(nullptr,
        tr("Open CSV File"),
        "",
        tr("CSV Files (*.csv);;All Files (*)"));

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
            FileUploader::current_instance,
            "fileRecieved",
            Qt::QueuedConnection,
            Q_ARG(QString, fileName),
            Q_ARG(QByteArray, fileData)
        );
    }
}

#endif
