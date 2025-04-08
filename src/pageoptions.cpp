#include <QPushButton>
#include <QSvgRenderer>
#include <QMessageBox>
#include <QTableView>
#include <QToolTip>

#include <string>
#include <algorithm>

#include "instructions.h"
#include "pageoptions.h"
#include "ui_pageoptions.h"
#include "fieldtoolbar.h"

#include "file_uploader.h"

#include "mainwindow.h"

//#define STRICTZIP
//#include <minizip/zip.h>
//#include <minizip/ioapi.h>


QByteArray generatePDF(QGraphicsScene* scene, int dpi, float src_margin)
{
    QByteArray pdfData;
    QBuffer buffer(&pdfData);
    buffer.open(QIODevice::WriteOnly);

    QPdfWriter pdfWriter(&buffer);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    pdfWriter.setResolution(dpi);  // High quality

    QRectF print_rect = pdfWriter.pageLayout().paintRectPixels(pdfWriter.resolution());
    QRectF src_rect = scene->sceneRect().adjusted(
        src_margin - print_rect.left(),
        src_margin - print_rect.top(),
        -src_margin,
        -src_margin
    );
    QRectF dest_rect(
        print_rect.left(),
        print_rect.top(),
        src_rect.width(),
        src_rect.height()
    );

    QPainter painter(&pdfWriter);
    scene->render(&painter, dest_rect, src_rect);
    painter.end();

    return pdfData;  // Return in-memory PDF
}

PdfSceneWriter::PdfSceneWriter()
    : pdfBuffer(&pdfData)
{}

void PdfSceneWriter::start(int _dpi, float _src_margin)
{
    dpi = _dpi;
    src_margin = _src_margin;

    pdfBuffer.open(QIODevice::WriteOnly);
    pdfWriter = new QPdfWriter(&pdfBuffer);
    pdfWriter->setPageSize(QPageSize(QPageSize::A4));
    pdfWriter->setResolution(dpi);

    pdfPainter = new QPainter();
    if (!pdfPainter->begin(pdfWriter)) {
        qWarning() << "Failed to begin painter on PDF writer.";
    }
}

PdfSceneWriter::~PdfSceneWriter()
{
    if (pdfPainter) {
        pdfPainter->end();
        delete pdfPainter;
    }
    delete pdfWriter;
}

void PdfSceneWriter::addPage(QGraphicsScene* scene)
{
    if (!pdfPainter || !pdfWriter) return;

    if (!firstPage) {
        pdfWriter->newPage();  // only after the first page
    }
    else {
        firstPage = false;
    }


    QRectF print_rect = pdfWriter->pageLayout().paintRectPixels(pdfWriter->resolution());
    QRectF src_rect = scene->sceneRect().adjusted(
        src_margin - print_rect.left(),
        src_margin - print_rect.top(),
        -src_margin,
        -src_margin
    );
    QRectF dest_rect(
        print_rect.left(),
        print_rect.top(),
        src_rect.width(),
        src_rect.height()
    );

    scene->render(pdfPainter, dest_rect, src_rect);
}

QByteArray PdfSceneWriter::finalize()
{
    if (pdfPainter) {
        pdfPainter->end();
        delete pdfPainter;
        pdfPainter = nullptr;
    }

    delete pdfWriter;
    pdfWriter = nullptr;

    pdfBuffer.close();

    qDebug() << "Final PDF size:" << pdfData.size();
    return pdfData;
}

/*
// Struct to hold the buffer being written to
struct MemoryZipBuffer {
    QByteArray data;
    qint64 pos = 0;
};

// Open handler - nothing to do, just return success
void* ZCALLBACK memOpen(void*, const char*, int) {
    return new MemoryZipBuffer();  // Allocate zip buffer
}

// Write handler - append data to QByteArray
unsigned long ZCALLBACK memWrite(void* opaque, void* stream, const void *buf, unsigned long size) {
    auto buffer = static_cast<MemoryZipBuffer*>(stream);
    buffer->data.append(static_cast<const char*>(buf), size);
    return size;
}

// Close handler - delete buffer
int ZCALLBACK memClose(void* opaque, void* stream) {
    delete static_cast<MemoryZipBuffer*>(stream);
    return ZIP_OK;
}

// Seek handler - not needed for sequential writes, return error if called
long ZCALLBACK memSeek(void*, void*, unsigned long, int) {
    return -1;  // Not supported
}

// Tell handler - just return the current position
long ZCALLBACK memTell(void*, void* stream) {
    auto buffer = static_cast<MemoryZipBuffer*>(stream);
    return buffer->data.size();
}

// Set up zlib_filefunc_def to use the above functions
void setupMemoryFileFunc(zlib_filefunc_def *pzlib_filefunc_def) {
    pzlib_filefunc_def->zopen_file = memOpen;
    pzlib_filefunc_def->zwrite_file = memWrite;
    pzlib_filefunc_def->zclose_file = memClose;
    pzlib_filefunc_def->zseek_file = memSeek;
    pzlib_filefunc_def->ztell_file = memTell;
    pzlib_filefunc_def->zread_file = nullptr;  // No reading
    pzlib_filefunc_def->opaque = nullptr;
}

QByteArray createZipInMemory(const QVector<QByteArray> &pdfs) {
    zlib_filefunc_def fileFunc;
    setupMemoryFileFunc(&fileFunc);

    zipFile zf = zipOpen2(nullptr, APPEND_STATUS_CREATE, nullptr, &fileFunc);
    if (!zf) {
        qWarning() << "Failed to create in-memory ZIP!";
        return QByteArray();
    }

    for (int i = 0; i < pdfs.size(); ++i) {
        QString fileName = QString("file_%1.pdf").arg(i + 1);

        if (zipOpenNewFileInZip(zf, fileName.toUtf8().constData(), nullptr, nullptr, 0, nullptr, 0, nullptr, 0, 0) != ZIP_OK) {
            qWarning() << "Failed to add file to ZIP:" << fileName;
            zipClose(zf, nullptr);
            return QByteArray();
        }

        const QByteArray &data = pdfs[i];
        if (zipWriteInFileInZip(zf, data.constData(), data.size()) != ZIP_OK) {
            qWarning() << "Failed to write file to ZIP:" << fileName;
            zipClose(zf, nullptr);
            return QByteArray();
        }

        zipCloseFileInZip(zf);
    }

    // Extract the final QByteArray
    auto buffer = reinterpret_cast<MemoryZipBuffer*>(zf->pfile);
    QByteArray result = buffer->data;

    zipClose(zf, nullptr);

    return result;
}

bool createZip(const QString &zipPath, const std::vector<QByteArray> &pdfs)
{
    zipFile zf = zipOpen(zipPath.toUtf8().constData(), APPEND_STATUS_CREATE);
    if (!zf) {
        qWarning() << "Failed to create ZIP file!";
        return false;
    }

    for (size_t i = 0; i < pdfs.size(); ++i) {
        QString fileName = QString("label_%1.pdf").arg(i + 1);

        zipOpenNewFileInZip(zf, fileName.toUtf8().constData(), nullptr, nullptr, 0, nullptr, 0, nullptr, 0, 0);
        zipWriteInFileInZip(zf, pdfs[i].constData(), pdfs[i].size());
        zipCloseFileInZip(zf);
    }

    zipClose(zf, nullptr);
    return true;
}
*/

// TAR Header Structure (512 bytes)
struct TarHeader {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];  // To make it 512 bytes
};

// Converts a number to an octal string (for TAR headers)
void tarSetOctal(char* field, size_t size, qint64 value) {
    snprintf(field, size, "%*llo", static_cast<int>(size - 1), value);
}

// Creates a TAR archive from multiple QByteArrays
QByteArray createTarArchive(const QVector<QPair<QString, QByteArray>>& files) {
    QByteArray tarData;

    for (const auto& file : files) {
        const QString& fileName = file.first;
        const QByteArray& fileContent = file.second;

        // Create TAR header
        TarHeader header;
        memset(&header, 0, sizeof(TarHeader));
        strncpy(header.name, fileName.toUtf8().constData(), sizeof(header.name) - 1);
        tarSetOctal(header.mode, sizeof(header.mode), 0644);  // File permissions
        tarSetOctal(header.uid, sizeof(header.uid), 0);
        tarSetOctal(header.gid, sizeof(header.gid), 0);
        tarSetOctal(header.size, sizeof(header.size), fileContent.size());
        tarSetOctal(header.mtime, sizeof(header.mtime), 0);
        memset(header.chksum, ' ', sizeof(header.chksum));  // Checksum placeholder
        strncpy(header.magic, "ustar", 6);  // Standard TAR format
        strncpy(header.version, "00", 2);

        // Calculate checksum
        unsigned int checksum = 0;
        const unsigned char* rawHeader = reinterpret_cast<const unsigned char*>(&header);
        for (size_t i = 0; i < sizeof(TarHeader); i++)
            checksum += rawHeader[i];
        tarSetOctal(header.chksum, sizeof(header.chksum), checksum);

        // Append header to TAR archive
        tarData.append(reinterpret_cast<const char*>(&header), sizeof(TarHeader));

        // Append file data
        tarData.append(fileContent);

        // Pad to 512-byte block
        int paddingSize = 512 - (fileContent.size() % 512);
        if (paddingSize < 512)
            tarData.append(QByteArray(paddingSize, '\0'));
    }

    // Append two empty 512-byte blocks to mark end of archive
    tarData.append(QByteArray(1024, '\0'));

    return tarData;
}

PageOptions::PageOptions(QStatusBar* _statusBar, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::PageOptions)
{
    ui->setupUi(this);
    statusBar = _statusBar;

    project_file_manager = new FileManager(this);
    csv_picker = new FileManager(this);
    svg_picker = new FileManager(this);
    pdf_exporter = new FileManager(this);

    field_merged_code = ui->merged_code_list->init("mat_gen_code", "Material / Generic Code");
    field_products = ui->product_list->init("products", "Available Products");

    field_shape = ui->shape_list->init("shape", "Shape");
    field_shape_col = ui->shape_color_list->init("shape_col", "Shape Colour");
    field_back_col = ui->back_color_list->init("back_col", "Background Colour");
    field_inner_back_col = ui->inner_back_color_list->init("inner_back_col", "Inner-Background Colour");

    // Bigger rows for icons
    field_shape->setRowHeight(24);
    field_shape_col->setRowHeight(24);
    field_back_col->setRowHeight(24);
    field_inner_back_col->setRowHeight(24);

    // Set which fields are selectable
    field_shape->setSelectable(false);
    field_shape_col->setSelectable(false);
    field_back_col->setSelectable(false);
    field_inner_back_col->setSelectable(false);

    //field_merged_code->setSelectable(false);

    // Set which fields are filterable
    field_shape->setFilterable(false);
    field_shape_col->setFilterable(false);
    field_back_col->setFilterable(false);
    field_inner_back_col->setFilterable(false);

    // Set editor margins
    field_shape->setEditorMargins(48, 0);
    field_shape_col->setEditorMargins(48, 0);
    field_back_col->setEditorMargins(48, 0);
    field_inner_back_col->setEditorMargins(48, 0);

    // Prepare radio button logic
    radio_lists = std::make_shared<std::vector<SearchableList*>>();
    radio_lists->push_back(field_merged_code);
    radio_lists->push_back(field_products);

    field_merged_code->setRadioGroup(radio_lists);
    field_products->setRadioGroup(radio_lists);
    field_merged_code->setRadioChecked(true);

    FieldToolbar* shape_toolbar = new FieldToolbar(field_shape);
    FieldToolbar* shape_col_toolbar = new FieldToolbar(field_shape_col);
    FieldToolbar* back_col_toolbar = new FieldToolbar(field_back_col);
    FieldToolbar* inner_back_col_toolbar = new FieldToolbar(field_inner_back_col);

    // Field toolbars
    field_shape->setCustomWidget(shape_toolbar);
    field_shape_col->setCustomWidget(shape_col_toolbar);
    field_back_col->setCustomWidget(back_col_toolbar);
    field_inner_back_col->setCustomWidget(inner_back_col_toolbar);

    ShapeItem warning_icon;
    warning_icon.load(":/res/warning.svg");
    warning_icon.setFillStyle(QColor(255,0,0,100));
    warning_icon_red_data = warning_icon.data();

    warning_icon.setFillStyle(QColor(255,255,0,100));
    warning_icon_yellow_data = warning_icon.data();

    ShapeItem delete_icon;
    delete_icon.load(":/res/bin.svg");

    delete_icon.setFillStyle(QColor(Qt::white));
    delete_inactive_data = delete_icon.data();

    delete_icon.setFillStyle(QColor(Qt::red));
    delete_active_data = delete_icon.data();

    /// Define callbacks

    // Add item button functions

    connect(shape_toolbar, &FieldToolbar::onAddBtn, this, [this]()
    {
        SearchableListItem* item = field_shape->addUniqueListItem("");
        item->setEditable(true);
        field_shape->beginEdit(item);
    });

    connect(shape_col_toolbar, &FieldToolbar::onAddBtn, this, [this]()
    {
        SearchableListItem* item = field_shape_col->addUniqueListItem("");
        item->setEditable(true);
        field_shape_col->beginEdit(item);
    });

    connect(back_col_toolbar, &FieldToolbar::onAddBtn, this, [this]()
    {
        SearchableListItem* item = field_back_col->addUniqueListItem("");
        item->setEditable(true);
        field_back_col->beginEdit(item);
    });

    connect(inner_back_col_toolbar, &FieldToolbar::onAddBtn, this, [this]()
    {
        SearchableListItem* item = field_inner_back_col->addUniqueListItem("");
        item->setEditable(true);
        field_inner_back_col->beginEdit(item);
    });

    // Row deletion functions

    auto deleteShapeRow = [this](QString shape_desc)
    {
        composerGenerator.removeShapeInfo(shape_desc);
        repopulateLists(false, false, true, false, false, false, false);
    };

    auto deleteShapeColorRow = [this](QString shape_desc)
    {
        composerGenerator.removeShapeColor(shape_desc);
        repopulateLists(false, false, false, true, false, false, false);
    };

    auto deleteBackColorRow = [this](QString shape_desc)
    {
        composerGenerator.removeBackColor(shape_desc);
        repopulateLists(false, false, false, false, true, false, false);
    };

    auto deleteInnerBackColorRow = [this](QString shape_desc)
    {
        composerGenerator.removeInnerBackColor(shape_desc);
        repopulateLists(false, false, false, false, false, true, false);
    };

    // Row data edit functions

    auto changeShapeItem = [this](SearchableListItem& item)
    {
        svg_picker->doLoad("Select SVG File", "*.svg", [this, item](
            const QString& filename,
            const QByteArray& data)
        {
            composerGenerator.setShapeInfo(item.txt, ShapeInfo::fromData(data));
            field_shape->refresh();
            recomposePage(getSelectedProduct(), selected_entry, true);
        });
    };

    auto changeShapeColorItem = [this](SearchableListItem& item)
    {
        color_picker = new QColorDialog(this);
        color_picker->setCurrentColor(composerGenerator.getShapeColor(item.txt));
        connect(color_picker, &QColorDialog::colorSelected, this, [this, item](const QColor& color) {
            composerGenerator.setShapeColor(item.txt, color);
            field_shape_col->refresh();
            recomposePage(getSelectedProduct(), selected_entry, true);
        });
        color_picker->open();
    };

    auto changeBackColorItem = [this](SearchableListItem& item)
    {
        color_picker = new QColorDialog(this);
        color_picker->setCurrentColor(composerGenerator.getBackColor(item.txt));
        connect(color_picker, &QColorDialog::colorSelected, this, [this, item](const QColor& color) {
            composerGenerator.setBackColor(item.txt, color);
            field_back_col->refresh();
            recomposePage(getSelectedProduct(), selected_entry, true);
        });
        color_picker->open();
    };

    auto changeInnerBackColorItem = [this](SearchableListItem& item)
    {
        color_picker = new QColorDialog(this);
        color_picker->setCurrentColor(composerGenerator.getInnerBackColor(item.txt));
        connect(color_picker, &QColorDialog::colorSelected, this, [this, item](const QColor& color) {
            composerGenerator.setInnerBackColor(item.txt, color);
            field_inner_back_col->refresh();
            recomposePage(getSelectedProduct(), selected_entry, true);
        });
        color_picker->open();
    };

    // Mouse-Move callbacks (tooltips, cursors, etc)

    auto itemMouseMoveCallback = [this](
        SearchableList *field,
        std::unordered_map<std::string, int> users_map,
        SearchableListItem& item,
        ListItemCallbackData& info,
        QString data_icon_tooltip,
        QString users_tooltip_append)
    {
        QRectF r1 = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0);
        QRectF r2 = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0, true, 2);
        QRectF users_r = r1.adjusted(24, 0, 24, 0);

        bool no_users = noUsers(users_map, item.txt);

        if (info.overIcon(r1))
        {
            QPointF tr = field->mapToGlobal(r1.toRect().topRight());
            QToolTip::showText(tr.toPoint(), data_icon_tooltip, field, r1.toRect(), 10000);

            info.list_view->setCursor(Qt::PointingHandCursor);
        }
        else if (info.overRect(users_r))
        {
            QPointF tr = field->mapToGlobal(users_r.toRect().topRight());
            QString users_txt = QString::number(users_map[item.txt.toStdString()]) + users_tooltip_append;
            QToolTip::showText(tr.toPoint(), users_txt, field, users_r.toRect(), 10000);
        }
        else if (no_users && info.overIcon(r2))
        {
            info.list_view->setCursor(Qt::PointingHandCursor);
        }
        else
        {
            info.list_view->unsetCursor();
            QToolTip::hideText();
        }
    };

    field_merged_code->onItemMouseMove([this](SearchableListItem& item, ListItemCallbackData& info)
    {
        // todo: Make helper function for this as it's a duplicate of the painter
        auto entry = item.as<OilTypeEntryPtr>();
        bool shape_assigned = composerGenerator.containsShapeInfo(entry->cell_shape->txt.c_str());
        bool shape_color_assigned = composerGenerator.containsShapeColor(entry->cell_shape_color->txt.c_str());
        bool back_color_assigned = composerGenerator.containsBackColor(entry->cell_back_color->txt.c_str());
        bool valid = !entry->missingData() && shape_assigned && shape_color_assigned && back_color_assigned;

        int icon_index = 0;

        if (!valid)
        {
            QRectF r = info.item_delegate->getIconRect(info.style_option_view, info.model_index, icon_index++, true, 3, 4, 2);
            if (info.overIcon(r))
            {
                QPointF tr = field_merged_code->getListView()->mapToGlobal(r.toRect().topRight());
                QToolTip::showText(tr.toPoint(), "missing styles",
                    field_merged_code, r.toRect(), 10000);
            }
        }

        if (entry->token_parse_errors)
        {
            QRectF r = info.item_delegate->getIconRect(info.style_option_view, info.model_index, icon_index, true, 3, 4, 2);
            if (info.overIcon(r))
            {
                QPointF tr = field_merged_code->getListView()->mapToGlobal(r.toRect().topRight());
                QToolTip::showText(tr.toPoint(), "unparsable code (or missing description)", 
                    field_merged_code, r.toRect(), 10000);
            }
        }
    });

    field_shape->onItemMouseMove([this, &itemMouseMoveCallback](SearchableListItem& item, ListItemCallbackData& info)
    {
        itemMouseMoveCallback(field_shape, users_shape, item, info, "change shape", " products use this shape");
    });

    field_shape_col->onItemMouseMove([this, &itemMouseMoveCallback](SearchableListItem& item, ListItemCallbackData& info)
    {
        itemMouseMoveCallback(field_shape_col, users_shape_color, item, info, "change shape colour", " products use this shape colour");
    });

    field_back_col->onItemMouseMove([this, &itemMouseMoveCallback](SearchableListItem& item, ListItemCallbackData& info)
    {
        itemMouseMoveCallback(field_back_col, users_back_color, item, info, "change background colour", " products use this background colour");
    });

    field_inner_back_col->onItemMouseMove([this, &itemMouseMoveCallback](SearchableListItem& item, ListItemCallbackData& info)
    {
        itemMouseMoveCallback(field_inner_back_col, users_inner_back_color, item, info, "change inner-background colour", " products use this inner-background colour");
    });

    // Single-Click callbacks (edit/delete icon callbacks)

    field_shape->onItemClick([this, changeShapeItem, deleteShapeRow](SearchableListItem& item, ListItemCallbackData& info)
    {
        QRectF dataIconRect = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0);
        QRectF deleteIconRect = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0, true, 2);
        bool no_users = noUsers(users_shape, item.txt);

        if (info.overIcon(dataIconRect))
        {
            changeShapeItem(item);
        }
        else if (no_users && info.overIcon(deleteIconRect))
        {
            // Grab source model
            QString desc = item.txt;
            QTimer::singleShot(0, [deleteShapeRow, desc]()
            {
                deleteShapeRow(desc);
            });
        }
    });

    field_shape_col->onItemClick([this, changeShapeColorItem, deleteShapeColorRow](SearchableListItem& item, ListItemCallbackData& info)
    {
        QRectF dataIconRect = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0);
        QRectF deleteIconRect = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0, true, 2);

        bool no_users = noUsers(users_shape_color, item.txt);

        if (info.overIcon(dataIconRect))
        {
            changeShapeColorItem(item);
        }
        else if (no_users && info.overIcon(deleteIconRect))
        {
            // Grab source model
            QString desc = item.txt;
            QTimer::singleShot(0, [deleteShapeColorRow, desc]()
            {
                deleteShapeColorRow(desc);
            });
        }
    });

    field_back_col->onItemClick([this, changeBackColorItem, deleteBackColorRow](SearchableListItem& item, ListItemCallbackData& info)
    {
        QRectF dataIconRect = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0);
        QRectF deleteIconRect = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0, true, 2);

        bool no_users = noUsers(users_back_color, item.txt);

        if (info.overIcon(dataIconRect))
        {
            changeBackColorItem(item);
        }
        else if (no_users && info.overIcon(deleteIconRect))
        {
            // Grab source model
            QString desc = item.txt;
            QTimer::singleShot(0, [deleteBackColorRow, desc]()
            {
                deleteBackColorRow(desc);
            });
        }
    });

    field_inner_back_col->onItemClick([this, changeInnerBackColorItem, deleteInnerBackColorRow](SearchableListItem& item, ListItemCallbackData& info)
    {
        QRectF dataIconRect = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0);
        QRectF deleteIconRect = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0, true, 2);

        bool no_users = noUsers(users_back_color, item.txt);

        if (info.overIcon(dataIconRect))
        {
            changeInnerBackColorItem(item);
        }
        else if (no_users && info.overIcon(deleteIconRect))
        {
            // Grab source model
            QString desc = item.txt;
            QTimer::singleShot(0, [deleteInnerBackColorRow, desc]()
            {
                deleteInnerBackColorRow(desc);
            });
        }
    });

    // Rename Item callbacks
    connect(field_shape, &SearchableList::itemModified, this, [this](SearchableListItem& item)
    {
        if (item.txt == item.old_txt)
            return;

        bool already_exists = composerGenerator.containsShapeInfo(item.txt);
        if (!already_exists)
            composerGenerator.replaceShapeInfo(item.old_txt, item.txt);

        repopulateLists(false, false, true, false, false, false, false);

        if (already_exists)
        {
            QMessageBox* box = new QMessageBox(QMessageBox::Critical,
                "Error",
                "A shape already exists with this name",
                QMessageBox::NoButton,
                this);
            box->show();
        }
    });

    connect(field_shape_col, &SearchableList::itemModified, this, [this](SearchableListItem& item)
    {
        if (item.txt == item.old_txt)
            return;

        bool already_exists = composerGenerator.containsShapeColor(item.txt);
        if (!already_exists)
            composerGenerator.replaceShapeColor(item.old_txt, item.txt);

        repopulateLists(false, false, false, true, false, false, false);

        if (already_exists)
        {
            QMessageBox* box = new QMessageBox(QMessageBox::Critical,
                "Error",
                "A shape colour already exists with this name",
                QMessageBox::NoButton,
                this);
            box->show();
        }
    });

    connect(field_back_col, &SearchableList::itemModified, this, [this](SearchableListItem& item)
    {
        if (item.txt == item.old_txt)
            return;

        bool already_exists = composerGenerator.containsBackColor(item.txt);
        if (!already_exists)
            composerGenerator.replaceBackColor(item.old_txt, item.txt);

        repopulateLists(false, false, false, false, true, false, false);

        if (already_exists)
        {
            QMessageBox* box = new QMessageBox(QMessageBox::Critical,
                "Error",
                "A background colour already exists with this name",
                QMessageBox::NoButton,
                this);
            box->show();
        }
    });

    connect(field_inner_back_col, &SearchableList::itemModified, this, [this](SearchableListItem& item)
    {
        if (item.txt == item.old_txt)
            return;

        bool already_exists = composerGenerator.containsInnerBackColor(item.txt);
        if (!already_exists)
            composerGenerator.replaceInnerBackColor(item.old_txt, item.txt);

        repopulateLists(false, false, false, false, false, true, false);

        if (already_exists)
        {
            QMessageBox* box = new QMessageBox(QMessageBox::Critical,
                "Error",
                "An inner-background colour already exists with this name",
                QMessageBox::NoButton,
                this);
            box->show();
        }
    });

    // Callbacks for detecting Radio toggle change
    auto toggleShowAllProducts = [this](bool)
    {
        field_merged_code->setSelectable(field_merged_code->getRadioChecked());
        field_merged_code->clearFilter();
        field_products->clearFilter();
        //field_products->setSelectable(field_products->getRadioChecked());
        repopulateLists();
    };

    field_merged_code->onRadioToggled(toggleShowAllProducts);
    field_products->onRadioToggled(toggleShowAllProducts);

    /// Painters
    field_merged_code->onItemPaint([this](SearchableListItem& item, QPainter* painter, ListItemCallbackData& info)
    {
        auto entry = item.as<OilTypeEntryPtr>();
        bool shape_assigned = composerGenerator.containsShapeInfo(entry->cell_shape->txt.c_str());
        bool shape_color_assigned = composerGenerator.containsShapeColor(entry->cell_shape_color->txt.c_str());
        bool back_color_assigned = composerGenerator.containsBackColor(entry->cell_back_color->txt.c_str());
        bool valid = !entry->missingData() && shape_assigned && shape_color_assigned && back_color_assigned;

        info.drawSelectionHighlightRect(painter);

        // Draw item text
        info.drawRowText(painter, item.txt, 4);

        int icon_index = 0;
        if (!valid)
        {
            QRectF r = info.item_delegate->getIconRect(info.style_option_view, info.model_index, icon_index++, true, 3, 4, 2);
            info.drawIcon(painter, r, warning_icon_red_data);
        }

        if (entry->token_parse_errors)
        {
            QRectF r2 = info.item_delegate->getIconRect(info.style_option_view, info.model_index, icon_index, true, 3, 4, 2);
            info.drawIcon(painter, r2, warning_icon_yellow_data);
        }

        return true;
    });

    /// Shape / Colours
    field_shape->onItemPaint([this](SearchableListItem& item, QPainter* painter, ListItemCallbackData& info)
    {
        QRectF dataIconRect = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0, false, 0, 0, 6);
        QRectF deleteIconRect = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0, true, 2);
        auto& users_map = users_shape;

        info.drawSelectionHighlightRect(painter);

        //if (composerGenerator.shapeMap().count(item.txt) > 0)
        if (composerGenerator.containsValidShapeInfo(item.txt))
        {
            ShapeInfo shape = composerGenerator.getShapeInfo(item.txt);
            info.drawIcon(painter, dataIconRect.adjusted(-2, -2, 2, 2), shape.svg_icon);

            // Highlight shape icon border
            if (info.overIcon(dataIconRect))
                info.drawIconBorder(painter, dataIconRect);
        }
        else
        {
            // Draw "missing" X icon
            info.drawXIcon(painter, dataIconRect);
        }

        // "Delete" icon
        bool no_users = noUsers(users_map, item.txt);
        bool unassigned = item.txt == "<unassigned>";

        if (no_users)
            info.drawIcon(painter, deleteIconRect, info.overIcon(deleteIconRect) ? delete_active_data : delete_inactive_data);

        // Draw users text
        if (!unassigned)
        {
            info.drawRowText(painter,
                "[" + QString::number(users_map[item.txt.toStdString()]) + "]",
                dataIconRect.width() + 24, true,
                no_users ? QColor(255, 50, 50) : QColor(180, 180, 180),
                no_users ? QColor(255, 50, 50) : QColor(70, 70, 70)
            );
        }

        // Draw main text
        info.drawRowText(painter, item.txt, dataIconRect.width() + (unassigned ? 16 : 40));
        return true;
    });

    field_shape_col->onItemPaint([this](SearchableListItem& item, QPainter* painter, ListItemCallbackData& info)
    {
        QRectF dataIconRect = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0, false, 0, 0, 6);
        QRectF deleteIconRect = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0, true, 2);
        auto& users_map = users_shape_color;

        info.drawSelectionHighlightRect(painter);

        //if (composerGenerator.shapeColorMap().count(item.txt) > 0)
        if (composerGenerator.containsValidShapeColor(item.txt))
        {
            QColor color = composerGenerator.getShapeColor(item.txt);
            painter->fillRect(dataIconRect.adjusted(-1, -1, 0, 0), color);

            // Highlight shape icon border
            if (info.overIcon(dataIconRect))
                info.drawIconBorder(painter, dataIconRect);
        }
        else
        {
            // Draw "missing" X icon
            info.drawXIcon(painter, dataIconRect);
        }

        // "Delete" icon
        bool no_users = noUsers(users_map, item.txt);
        bool unassigned = item.txt == "<unassigned>";

        if (no_users)
            info.drawIcon(painter, deleteIconRect, info.overIcon(deleteIconRect) ? delete_active_data : delete_inactive_data);

        // Draw users text
        if (!unassigned)
        {
            info.drawRowText(painter,
                "[" + QString::number(users_map[item.txt.toStdString()]) + "]",
                dataIconRect.width() + 24, true,
                no_users ? QColor(255, 50, 50) : QColor(180, 180, 180),
                no_users ? QColor(255, 50, 50) : QColor(70, 70, 70)
            );
        }

        // Draw main text
        info.drawRowText(painter, item.txt, dataIconRect.width() + (unassigned ? 16 : 40));
        return true;

        /*QRectF r1 = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0);
        QRectF r2 = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0, true).adjusted(0.5, 0.5, -0.5, -0.5);

        auto selected_indexes = info.list_view->selectionModel()->selectedIndexes();
        int selected_row = selected_indexes.empty() ? -1 : selected_indexes.at(0).row();
        bool selected = (selected_row == info.model_index.row());
        if (selected)
            painter->fillRect(info.style_option_view.rect, QColor(55, 138, 221));

        // Disable anti-aliasing
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->setRenderHint(QPainter::SmoothPixmapTransform, false);

        if (composerGenerator.shapeColorMap().count(item.txt) == 0)
        {
            QPen pen(Qt::red, 2);
            painter->setPen(pen);
            painter->drawLine(r1.topLeft(), r1.bottomRight());
            painter->drawLine(r1.topRight(), r1.bottomLeft());
        }
        else
        {
            QColor color = composerGenerator.getShapeColor(item.txt);
            painter->fillRect(r1, color);
            if (r1.contains(info.mouse_pos))
            {
                painter->setPen(Qt::black);
                painter->drawRect(r1);
                painter->setPen(Qt::white);
                painter->drawRect(r1.adjusted(-1, -1, 1, 1));
            }
        }

        // "Delete" icon
        bool no_users = users_shape_color[item.txt.toStdString()] == 0;
        if (no_users)
        {
            QSvgRenderer renderer;
            bool over_delete = r2.adjusted(-2, -2, 2, 2).contains(info.mouse_pos);
            renderer.load(over_delete ? delete_active_data : delete_inactive_data);
            renderer.render(painter, r2.adjusted(-2, -2, 2, 2));
        }

        // Text
        {
            // Determine colors
            QColor col_item = selected ? Qt::black : Qt::white;
            QColor col_users = selected ? QColor(70, 70, 70) : QColor(180, 180, 180);
            if (no_users) col_users = QColor(255, 50, 50);

            QFontMetrics fm(info.style_option_view.font);
            int txt_y = info.style_option_view.rect.y() + (info.style_option_view.rect.height() - fm.height()) / 2;

            // Draw users text
            {
                QString users_txt = "[" + QString::number(users_shape_color[item.txt.toStdString()]) + "]";
                int users_width = fm.horizontalAdvance(users_txt);

                QRect users_rect(info.style_option_view.rect);
                users_rect.setLeft(info.style_option_view.rect.x() + r1.width() + 24 - users_width / 2);
                users_rect.setY(txt_y);

                painter->setPen(col_users);
                painter->drawText(users_rect, users_txt);
            }

            // Draw item text
            {
                QRect txt_rect(info.style_option_view.rect);
                txt_rect.setLeft(info.style_option_view.rect.x() + r1.width() + 40);
                txt_rect.setY(txt_y);

                painter->setPen(col_item);
                painter->drawText(txt_rect, item.txt);
            }
        }

        painter->restore();

        return true;*/
    });

    field_back_col->onItemPaint([this](SearchableListItem& item, QPainter* painter, ListItemCallbackData& info)
    {
        QRectF dataIconRect = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0, false, 0, 0, 6);
        QRectF deleteIconRect = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0, true, 2);
        auto& users_map = users_back_color;

        info.drawSelectionHighlightRect(painter);

        //if (composerGenerator.backColorMap().count(item.txt) > 0)
        if (composerGenerator.containsValidBackColor(item.txt))
        {
            QColor color = composerGenerator.getBackColor(item.txt);
            painter->fillRect(dataIconRect.adjusted(-1, -1, 0, 0), color);

            // Highlight shape icon border
            if (info.overIcon(dataIconRect))
                info.drawIconBorder(painter, dataIconRect);
        }
        else
        {
            // Draw "missing" X icon
            info.drawXIcon(painter, dataIconRect);
        }

        // "Delete" icon
        bool no_users = noUsers(users_map, item.txt);
        bool unassigned = item.txt == "<unassigned>";

        if (no_users)
            info.drawIcon(painter, deleteIconRect, info.overIcon(deleteIconRect) ? delete_active_data : delete_inactive_data);

        // Draw users text
        if (!unassigned)
        {
            info.drawRowText(painter,
                "[" + QString::number(users_map[item.txt.toStdString()]) + "]",
                dataIconRect.width() + 24, true,
                no_users ? QColor(255, 50, 50) : QColor(180, 180, 180),
                no_users ? QColor(255, 50, 50) : QColor(70, 70, 70)
            );
        }

        // Draw main text
        info.drawRowText(painter, item.txt, dataIconRect.width() + (unassigned ? 16 : 40));
        return true;

        /*QRectF r = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0);

        if (composerGenerator.backColorMap().count(item.txt) == 0)
        {
            QPen pen(Qt::red, 2);
            painter->setPen(pen);
            painter->drawLine(r.topLeft(), r.bottomRight());
            painter->drawLine(r.topRight(), r.bottomLeft());
        }
        else
        {
            QColor color = composerGenerator.getBackColor(item.txt);
            QBrush brush(color);
            painter->fillRect(r, brush);
            if (r.contains(info.mouse_pos))
            {
                painter->setPen(Qt::black);
                painter->drawRect(r);
                painter->setPen(Qt::white);
                painter->drawRect(r.adjusted(-1, -1, 1, 1));
            }
        }*/

        return false;
    });

    field_inner_back_col->onItemPaint([this](SearchableListItem& item, QPainter* painter, ListItemCallbackData& info)
    {
        QRectF dataIconRect = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0, false, 0, 0, 6);
        QRectF deleteIconRect = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0, true, 2);
        auto& users_map = users_inner_back_color;

        info.drawSelectionHighlightRect(painter);

        //if (composerGenerator.innerBackColorMap().count(item.txt) > 0)
        if (composerGenerator.containsValidInnerBackColor(item.txt))
        {
            QColor color = composerGenerator.getInnerBackColor(item.txt);
            painter->fillRect(dataIconRect.adjusted(-1, -1, 0, 0), color);

            // Highlight shape icon border
            if (info.overIcon(dataIconRect))
                info.drawIconBorder(painter, dataIconRect);
        }
        else
        {
            // Draw "missing" X icon
            info.drawXIcon(painter, dataIconRect);
        }

        // "Delete" icon
        bool no_users = noUsers(users_map, item.txt);
        bool unassigned = item.txt == "<unassigned>";

        if (no_users)
            info.drawIcon(painter, deleteIconRect, info.overIcon(deleteIconRect) ? delete_active_data : delete_inactive_data);

        // Draw users text
        if (!unassigned)
        {
            info.drawRowText(painter,
                "[" + QString::number(users_map[item.txt.toStdString()]) + "]",
                dataIconRect.width() + 24, true,
                no_users ? QColor(255, 50, 50) : QColor(180, 180, 180),
                no_users ? QColor(255, 50, 50) : QColor(70, 70, 70)
            );
        }

        // Draw main text
        info.drawRowText(painter, item.txt, dataIconRect.width() + (unassigned ? 16 : 40));
        return true;

        /*QRectF r = info.item_delegate->getIconRect(info.style_option_view, info.model_index, 0);

        if (composerGenerator.innerBackColorMap().count(item.txt) == 0)
        {
            QPen pen(Qt::red, 2);
            painter->setPen(pen);
            painter->drawLine(r.topLeft(), r.bottomRight());
            painter->drawLine(r.topRight(), r.bottomLeft());
        }
        else
        {
            QColor color = composerGenerator.getInnerBackColor(item.txt);
            QBrush brush(color);
            painter->fillRect(r, brush);
            if (r.contains(info.mouse_pos))
            {
                painter->setPen(Qt::black);
                painter->drawRect(r);
                painter->setPen(Qt::white);
                painter->drawRect(r.adjusted(-1, -1, 1, 1));
            }
        }*/

        return false;
    });


    // Detect [material/generic] list selection change
    connect(field_merged_code, &SearchableList::onChangedSelected, this, [this](SearchableListItem& item)
    {
        selected_entry = item.as<OilTypeEntryPtr>();

        // List available products for this oil (unless we're already showing them all)
        bool show_all_products = field_products->getRadioChecked();
        if (!show_all_products)
        {
            field_products->clear();
            for (CSVCellPtr vendor_cell : selected_entry->vendor_cells)
                field_products->addListItem(QString(vendor_cell->txt.c_str()));
        }

        // Select first vendor
        if (selected_entry->vendor_cells.size())
        {
            QSignalBlocker blocker(field_products);
            field_products->setCurrentItem(selected_entry->vendor_cells[0]->txt.c_str());
        }

        onChangeSelectedMaterialEntry();
    });

    // Detect product list selection change
    connect(field_products, &SearchableList::onChangedSelected, this, [this](SearchableListItem& item)
    {
        selected_entry = product_oiltype_entries[item.txt.toStdString()];

        {
            QSignalBlocker blocker(field_merged_code);
            field_merged_code->setCurrentItem(selected_entry->mergedCode().c_str());
        }

        onChangeSelectedMaterialEntry();
    });

    // Load Project button
    connect(ui->load_btn, &QPushButton::clicked, this, [this]()
    {
        project_file_manager->doLoad("Select Project File", "*.json",
            [this](const QString& filename, const QByteArray& data)
        {
            selected_entry = nullptr;

            deserialize(data);
            rebuildDatabaseAndPopulateUI();

            project_filename = filename;
            updateStatusBar();
        });
    });

    // Save Project button
    connect(ui->save_btn, &QPushButton::clicked, this, [this]()
    {
        QString save_data = serialize();
        project_file_manager->doSave("Save Project File", "project.json", "Project File (*.json);", save_data.toUtf8());
    });

    // Load CSV button
    connect(ui->csv_btn, &QPushButton::clicked, this, [this]()
    {
        selected_entry = nullptr;

        csv_picker->doLoad("Select CSV File", "CSV File (*.csv);", [this](
            const QString& filename,
            const QByteArray& data)
        {
            readCSV(data.toStdString().c_str());
            rebuildDatabaseAndPopulateUI();

            csv_filename = filename;
            updateStatusBar();
        });
    });

    // Single PDF button
    connect(ui->pdf_btn, &QPushButton::clicked, this, [this]()
    {
        recomposePage(getSelectedProduct(), selected_entry, false, [this](QGraphicsScene* scene, ComposerResultInt result)
        {
            if (result == ComposerResult::SUCCEEDED)
            {
                QByteArray pdf = generatePDF(scene, pagePreview->targetPageDPI(), pagePreview->targetPageMargin());
                pdf_exporter->doSave("Select PDF Path", "label.pdf", "PDF File (*.pdf);", pdf);
            }
            else
            {
                QString err;
                if (result & ComposerResult::NO_SELECTED_ITEM)
                    err += "No product selected.\n";
                if (result & ComposerResult::MISSING_MATERIAL_CODE)
                    err += "Invalid:  Material Code\n";
                if (result & ComposerResult::MISSING_GENERIC_CODE)
                    err += "Invalid:  Generic Code\n";
                if (result & ComposerResult::MISSING_SHAPE)
                    err += "Invalid:  Shape\n";
                if (result & ComposerResult::MISSING_SHAPE_COLOR)
                    err += "Invalid:  Shape Color\n";
                if (result & ComposerResult::MISSING_BACK_COLOR)
                    err += "Invalid:  Background Color\n";

                // Handle error
                QMessageBox* box = new QMessageBox(QMessageBox::Critical,
                    "Error",
                    err,
                    QMessageBox::NoButton,
                    this);
                box->show();
            }
        });
    });

    // Batch PDF button
    connect(ui->batch_pdf_btn, &QPushButton::clicked, this, [this]()
    {
        batch_dialog = new PDFBatchExport(this);
        connect(batch_dialog, &PDFBatchExport::beginExport, this, [this]()
        {
            if (batch_export_busy)
                return;

            batch_export_busy = true;
            export_index = 0;
            pdfs.clear();
            batch_dialog->clearLog();

            if (batch_dialog->isSinglePDF())
            {
                pdf_writer.start(pagePreview->targetPageDPI(), pagePreview->targetPageMargin());
            }

            //pdfData = new QByteArray();
            //pdfBuffer = new QBuffer(pdfData);
            //pdfBuffer->open(QIODevice::WriteOnly);
            //pdfWriter = new QPdfWriter(pdfBuffer);
            //pdfWriter->setPageSize(QPageSize(QPageSize::A4));
            //pdfWriter->setResolution(300);
            //pdfPainter = new QPainter(pdfWriter);

            QTimer::singleShot(0, this, &PageOptions::processExportPDF);
            batch_dialog->setDisabled(true);
        });

        batch_dialog->open();
    });

    // Help button
    connect(ui->help_btn, &QPushButton::clicked, this, [this]()
    {
        Instructions* instructions = new Instructions(this);
        instructions->setFixedSize(instructions->size());
        instructions->show();
    });

    // Detect selected token-description table cell edit
    connect(&selected_product_description_model, &QStandardItemModel::dataChanged, this,
        [this](const QModelIndex& topLeft,
               const QModelIndex& bottomRight,
               const QVector<int>& roles)
    {
        if (roles.contains(Qt::EditRole) || roles.isEmpty())
        {
            onEditSelectedProductTokenDescription(topLeft.row(), topLeft.column(), topLeft.data().toString());
            //qDebug() << "Cell edited at row" << topLeft.row() << "column" << topLeft.column();
        }
    });
    
    description_maps.resize(6);
    setTokenColumnName(0, "Lubricant Type");
    setTokenColumnName(1, "VISC / NLGI");
    setTokenColumnName(2, "Base Oil / Thickener Type");
    setTokenColumnName(3, "Additive Type /\nGrease Base Oil Type");
    setTokenColumnName(4, "Grease Viscosity / Additive");
    setTokenColumnName(5, "Additive Type 3 / Application");

    // Detect setting token-description tables cell edit
    token_tables = {ui->A_tbl, ui->B_tbl, ui->C_tbl, ui->D_tbl, ui->E_tbl, ui->F_tbl};

    for (size_t i = 0; i < token_tables.size(); i++)
    {
        QStandardItemModel* model = description_maps[i].model.get();

        connect(model, &QStandardItemModel::dataChanged, this,
            [this, i](const QModelIndex& topLeft,
            const QModelIndex& bottomRight,
            const QVector<int>& roles)
        {
            onEditSettingsProductTokenDescription(i, topLeft.row(), topLeft.column(), topLeft.data().toString());
        });
    }

    connect(ui->add_row_btn, &QPushButton::clicked, this, [this]()
    {
        int table_index = ui->token_defs_tabs->currentIndex();
        TokenDescriptionMap& map = description_maps[table_index];
        QStandardItemModel* model = map.model.get();
        QTableView* table = token_tables[table_index];

        map.lookup.push_back(std::pair<std::string, std::string>("", ""));
        
        QList<QStandardItem*> newRow;
        newRow.push_back(new QStandardItem(""));
        newRow.push_back(new QStandardItem(""));
        
        model->appendRow(newRow);
        auto tokenIndex = model->index(map.lookup.size() - 1, 0);
        table->setCurrentIndex(tokenIndex);
        table->edit(tokenIndex);
    });

    connect(ui->remove_row_btn, &QPushButton::clicked, this, [this]()
    {
        int table_index = ui->token_defs_tabs->currentIndex();
        TokenDescriptionMap& map = description_maps[table_index];
        QStandardItemModel* model = map.model.get();
        QTableView* table = token_tables[table_index];

        auto currentIndex = table->currentIndex();
        if (currentIndex.isValid())
        {
            int row = currentIndex.row();
            map.lookup.erase(map.lookup.begin() + row);
            model->removeRow(row);

            scanForAllTokenParseErrors();
            updateSelectedProductTokenTable();
        }
    });

    /*composerGenerator.setShapeInfo("Square ISO-VG32", ShapeInfo::fromPath(":/shapes/SQUARE.svg"));
    composerGenerator.setShapeInfo("Hexagon ISO-VG 68", ShapeInfo::fromPath(":/shapes/HEXAGON.svg"));
    composerGenerator.setShapeInfo("Pentagon ISO-VG 46", ShapeInfo::fromPath(":/shapes/PENTAGON.svg"));
    composerGenerator.setShapeInfo("Oval ISO-VG 460", ShapeInfo::fromPath(":/shapes/ELIPSE.svg"));
    composerGenerator.setShapeInfo("Parallelogram ISO-VG 320", ShapeInfo::fromPath(":/shapes/PARALLELOGRAM.svg"));
    composerGenerator.setShapeInfo("Tear Drop ISO-VG 220", ShapeInfo::fromPath(":/shapes/TEARDROP.svg"));
    composerGenerator.setShapeInfo("Triangle ISO-VG 22", ShapeInfo::fromPath(":/shapes/TRIANGLE.svg"));
    composerGenerator.setShapeInfo("Square ISO-VG 32", ShapeInfo::fromPath(":/shapes/SQUARE.svg"));
    composerGenerator.setShapeInfo("Diamond SAE-VG 90", ShapeInfo::fromPath(":/shapes/DIAMOND.svg"));
    composerGenerator.setShapeInfo("Hexagon SAE-VG 40", ShapeInfo::fromPath(":/shapes/HEXAGON.svg"));
    composerGenerator.setShapeInfo("Square SAE-VG 20W-20", ShapeInfo::fromPath(":/shapes/SQUARE.svg"));
    composerGenerator.setShapeInfo("Egg-Timer ISO-VG 1000", ShapeInfo::fromPath(":/shapes/HOURGLASS_V.svg"));
    composerGenerator.setShapeInfo("Octagon ISO-VG 100", ShapeInfo::fromPath(":/shapes/OCTAGON.svg"));
    composerGenerator.setShapeInfo("Grease Gun_Grease", ShapeInfo::fromPath(":/shapes/GREASE_GUN.svg"));

    composerGenerator.setShapeColor("Gray-Mineral", QColor("#7f7f7f"));
    composerGenerator.setShapeColor("Black-Mineral - EP - WD", QColor("#000000"));
    composerGenerator.setShapeColor("Blue-Mineral (GrpII) - H2 Food Safe", QColor("#0000ff"));
    composerGenerator.setShapeColor("White-Synthetic - POE", QColor("#ffffff"));
    composerGenerator.setShapeColor("Green-Mineral - AW", QColor("#00ff00"));
    composerGenerator.setShapeColor("Yellow-Mineral", QColor("#ffff00"));
    composerGenerator.setShapeColor("Orange-Synthetic - POE", QColor("#ff7f00"));
    composerGenerator.setShapeColor("Red-Mineral - WD - ASHLESS", QColor("#ff0000"));
    composerGenerator.setShapeColor("Red-Mineral", QColor("#ff0000"));
    composerGenerator.setShapeColor("Purple-Synthetic", QColor("#ff00ff"));
    composerGenerator.setShapeColor("Black-Mineral", QColor("#000000"));
    composerGenerator.setShapeColor("Green-Synthetic (PAO)", QColor("#00ff00"));
    composerGenerator.setShapeColor("Red-Mineral - ASHLESS", QColor("#ff0000"));
    composerGenerator.setShapeColor("Orange-Mineral", QColor("#ff8000"));
    composerGenerator.setShapeColor("Yellow-Synthetic(PG) - EP", QColor("#ffff00"));
    composerGenerator.setShapeColor("Gray-Synthetic(PAO) - AW", QColor("#7f7f7f"));
    composerGenerator.setShapeColor("Purple-Synthetic (PAG)", QColor("#ff00ff"));
    composerGenerator.setShapeColor("Black-Synthetic(PAO) - EP", QColor("#000000"));
    composerGenerator.setShapeColor("Black-Synthetic (PAO)", QColor("#000000"));
    composerGenerator.setShapeColor("White-Mineral - AW - ASHLESS", QColor("#ffffff"));
    composerGenerator.setShapeColor("Yellow", QColor("#ffff00"));
    composerGenerator.setShapeColor("Dark Green", QColor("#007f00"));
    composerGenerator.setShapeColor("Purple", QColor("#ff00ff"));
    composerGenerator.setShapeColor("Blue", QColor("#0000ff"));
    composerGenerator.setShapeColor("Red", QColor("#ff0000"));
    composerGenerator.setShapeColor("White", QColor("#ffffff"));
    composerGenerator.setShapeColor("Brown", QColor("#6f1010"));
    composerGenerator.setShapeColor("Black", QColor("#000000"));
    composerGenerator.setShapeColor("Gray", QColor("#5f5f5f"));

    composerGenerator.setBackColor("Gray-Thermal Oils", QColor("#7f7f7f"));
    composerGenerator.setBackColor("Yellow-Other Oils", QColor("#ffff00"));
    composerGenerator.setBackColor("Red-Transmission Oils", QColor("#ff0000"));
    composerGenerator.setBackColor("Black-Engine Oils", QColor("#000000"));
    composerGenerator.setBackColor("Green-Hydraulic Oils", QColor("#00ff00"));
    composerGenerator.setBackColor("Orange-Gear Oils", QColor("#ff8000"));
    composerGenerator.setBackColor("Blue-Compressor Oils", QColor("#0000ff"));
    composerGenerator.setBackColor("Red-PolyUrea", "#ff0000");
    composerGenerator.setBackColor("Black-PTFE/PFPE", "#000000");
    composerGenerator.setBackColor("Green-Lithium", "#00ff00");
    composerGenerator.setBackColor("Light Green-Lithium Complex", "#00ff00");
    composerGenerator.setBackColor("Brown-Clay Bentonite", "#7f1f1f");
    composerGenerator.setBackColor("White-Barium Complex", "#ffffff");
    composerGenerator.setBackColor("Gray-Aluminium Complex", "#7f7f7f");
    composerGenerator.setBackColor("Purple-Polymer", "#ff00ff");*/

    /*{
        // Lubricant Type
        setTokenColumnName(0, "Lubricant Type");
        setTokenDescription(0, "GB", "Bearing Grease");
        setTokenDescription(0, "GB/GC", "Bearing Grease High Performance"); //setTokenDescription(0, "GC", "Chain Grease");
        setTokenDescription(0, "GG", "Grease Food Grade");
        setTokenDescription(0, "GP", "Paste Grease");
        setTokenDescription(0, "OC", "Compressor Oil");
        setTokenDescription(0, "OE", "Electrical Oil");
        setTokenDescription(0, "OG", "Gear Oil");
        setTokenDescription(0, "OH", "Hydraulic Oil");
        setTokenDescription(0, "OM", "Motor / Engine Oil"); /// CLASH. Resolve by some threadhold?  SAE[0W-50W] = Engine. SAE[80W-140] 
        setTokenDescription(0, "OO", "Other Oil");
        setTokenDescription(0, "OT", "Thermal Oil");

        // Viscosity / NLGI type
        setTokenColumnName(1, "VISC / NLGI");
        //setTokenDescription(1, "10",    "VG10");  /// e.g. OE-10-M     = "Electrical Insulating Oil"
        setTokenDescription(1, "32", "ISO (VG) 32 Light hydraulic oil");  /// e.g. OT-32-M     = "Heat Transfer Oil, VG32, Mineral"
        setTokenDescription(1, "46", "ISO (VG) 46 Medium hydraulic & compressor oil");  /// e.g. OG-100-M-EP = "Compressor Oil, VG46, Mineral, AW,  ISO 6743/3 - DAA and DAB for reciprocating air compressors, DAG for rotary air compressors"
        setTokenDescription(1, "50", "ISO (VG) 50 Similar to VG 46 but slightly thicker");
        setTokenDescription(1, "68", "ISO (VG) 68 Heavier hydraulic & gear oil");
        setTokenDescription(1, "100", "ISO (VG) 100 Heavy-duty gear & bearing oil");  //CLASH WITH    e.g. "Gear Oil, VG100, Mineral, EP, DIN 51517-3 for CLP type gear oils"
        setTokenDescription(1, "150", "ISO (VG) 150 Industrial gear oil");
        setTokenDescription(1, "220", "ISO (VG) 220 Heavy industrial gear oil");
        setTokenDescription(1, "260", "ISO (VG) 260 Similar to VG 280"); // Mistake? Should be VG260?    e.g. "Chain Oil, VG150, Synthetic (POE), AW, suitable for high temperature"
        setTokenDescription(1, "280", "ISO (VG) Higher viscosity industrial gear oil"); // Mistake? Should be VG260?    e.g. "Chain Oil, VG150, Synthetic (POE), AW, suitable for high temperature"
        setTokenDescription(1, "320", "ISO (VG) 320 High-viscosity gear oil");
        setTokenDescription(1, "400", "ISO (VG) 400 High-load gear oil");
        setTokenDescription(1, "460", "ISO (VG) 460 Extra heavy-duty gear oil");
        setTokenDescription(1, "680", "ISO (VG) 680 Extra high viscosity oil");
        setTokenDescription(1, "1000", "ISO (VG) 1000 Extreme conditions, open gears");
        //setTokenDescription(1, "1500", "VG1500");

        setTokenDescription(1, "5W-30", "SAE Gear Oil Viscosity 5W Winter-grade oil for cold starts");
        setTokenDescription(1, "10W-40", "SAE Gear Oil Viscosity 10W Light winter-grade oil");
        setTokenDescription(1, "15W-40", "SAE Gear Oil Viscosity 15W Moderate winter-grade oil");
        setTokenDescription(1, "20W-40", "SAE Gear Oil Viscosity 20W Heavier winter-grade oil");
        setTokenDescription(1, "30", "SAE Gear Oil Viscosity 30 Medium-weight engine oil");
        //setTokenDescription(1, "40",      "SAE 40"); // MISSING FROM GLOSSARY
        setTokenDescription(1, "80W-90", "UNDEFINED");
        setTokenDescription(1, "80W", "SAE Gear Oil Viscosity 80W Light gear oil");
        setTokenDescription(1, "85W-140", "SAE Gear Oil Viscosity 85W Medium gear oil");
        setTokenDescription(1, "90", "SAE Gear Oil Viscosity 90 Standard gear oil");

        setTokenDescription(1, "ATF32", "Automatic Transmission Fluid (ATF) ATF 32 Transmission fluid for automatics");

        setTokenDescription(1, "000", "NLGI Grease Grade 0 Very soft, semi-fluid grease");
        setTokenDescription(1, "00", "NLGI Grease Grade 0 Soft, nearly fluid grease");
        setTokenDescription(1, "0", "NLGI Grease Grade 0 Soft grease");
        setTokenDescription(1, "1", "NLGI Grease Grade 1 Semi-soft grease");
        setTokenDescription(1, "1.5", "NLGI Grease Grade 1.5 Between NLGI 1 and 2");
        setTokenDescription(1, "2", "NLGI Grease Grade 2 Standard multi-purpose grease");
        setTokenDescription(1, "2.5", "NLGI Grease Grade 2.5 Slightly firmer than NLGI 2");
        setTokenDescription(1, "3", "NLGI Grease Grade 3 Firm grease");

        // Base Oil / Thickener Type
        setTokenColumnName(2, "Base Oil / Thickener Type");
        setTokenDescription(2, "M", "Mineral Oil (Base Oil)");
        setTokenDescription(2, "Na", "Sodium");
        setTokenDescription(2, "Li", "Lithium");
        setTokenDescription(2, "LiHS", "Lithium Hydroxystearate");
        setTokenDescription(2, "Li/Ca", "Lithium Calcium");
        setTokenDescription(2, "LiC", "Lithium Complex");
        setTokenDescription(2, "CaC", "Calcium Complex");
        setTokenDescription(2, "AlC", "Aluminum Complex");
        setTokenDescription(2, "BaC", "Barium Complex");
        setTokenDescription(2, "Clay", "Clay");
        setTokenDescription(2, "Ben", "Bentonite");
        setTokenDescription(2, "PU", "PolyUrea");
        setTokenDescription(2, "PTFE", "Polytetrafluoroethylene (Specialist)");
        setTokenDescription(2, "PFPE", "Perfluoropolyether (Specialist)");
        setTokenDescription(2, "P", "Polymer");

        // Additive Type / Grease Base Oil Type
        setTokenColumnName(3, "Additive Type /\nGrease Base Oil Type");
        setTokenDescription(3, "EP", "Extreme Pressure (Enhances load-carrying capacity)");
        setTokenDescription(3, "AW", "Anti-Wear (Reduces wear on metal surfaces)");
        setTokenDescription(3, "AW/EP", "Combined Anti-Wear and Extreme Pressure Additives");
        setTokenDescription(3, "HT", "High Temperature (Suitable for elevated temperature applications)");
        setTokenDescription(3, "HT PLUS", "Enhanced High-Temperature Performance");
        setTokenDescription(3, "H1", "Food-Grade Lubricant (Approved for incidental food contact)");
        setTokenDescription(3, "NC", "Non-Corrosive (Prevents rust and oxidation)");
        setTokenDescription(3, "WD", "Water-Displacing (Repels water, prevents corrosion)");
        setTokenDescription(3, "M", "Mineral Oil");
        setTokenDescription(3, "S", "Synthetic");
        setTokenDescription(3, "S(PAO)", "Synthetic (Polyalphaolefin - PAO)");
        setTokenDescription(3, "S(ESTER)", "Synthetic (Ester-Based)");
        setTokenDescription(3, "S(PFPE)", "Synthetic (Perfluoropolyether - PFPE)");
        setTokenDescription(3, "S(PAG)", "Synthetic (Polyalkylene Glycol - PAG)");
        setTokenDescription(3, "S/S", "Semi-Synthetic (Blend of Mineral & Synthetic Oils)");
        setTokenDescription(3, "PFPE", "Perfluoropolyether (High-performance synthetic base)");
        ///setTokenDescription(3, "PAO", "PolyAlphaOlefin");
        ///setTokenDescription(3, "PAG", "Polyalkylene Glycol");
        ///setTokenDescription(3, "PG", "Polyglycol");
        ///setTokenDescription(3, "POE", "Polyol ester");
        ///setTokenDescription(3, "PFPE", "Perfluoropolyether (Specialist)");

        // Grease Visc / Additive
        setTokenColumnName(4, "Grease Viscosity / Additive");
        setTokenDescription(4, "WD", "Water - Displacing(Repels water, prevents corrosion)");
        setTokenDescription(4, "EP", "Extreme Pressure(Enhances load - carrying capacity)");
        setTokenDescription(4, "HVI", "High Viscosity Index(Maintains viscosity across temperature changes)");
        setTokenDescription(4, "ASHLESS", "Ashless Additive(Burns cleanly, leaving no residue)");
        setTokenDescription(4, "HT", "High Temperature(Enhances performance at elevated temperatures)");
        setTokenDescription(4, "VG100", "ISO Viscosity Grade 100");
        setTokenDescription(4, "VG460", "ISO Viscosity Grade 460");
        setTokenDescription(4, "32", "ISO VG 32 (Low - viscosity lubricant)");
        setTokenDescription(4, "46", "ISO VG 46 (Common hydraulic oil viscosity)");
        setTokenDescription(4, "100", "ISO VG 100");
        setTokenDescription(4, "150", "ISO VG 150");
        setTokenDescription(4, "150/200", "Viscosity range between ISO VG 150 and 200");
        setTokenDescription(4, "180/220", "Viscosity range between ISO VG 180 and 220");
        setTokenDescription(4, "220", "ISO VG 220");
        setTokenDescription(4, "270", "Viscosity rating(typically centistokes - cSt)");
        setTokenDescription(4, "390", "Viscosity rating(cSt)");
        setTokenDescription(4, "400", "Viscosity rating(cSt)");
        setTokenDescription(4, "460", "ISO VG 460");
        setTokenDescription(4, "680", "ISO VG 680");
        setTokenDescription(4, "1000", "ISO VG 1000");
        setTokenDescription(4, "1350", "ISO VG 1350");
        setTokenDescription(4, "1500", "ISO VG 1500");
        ///
        ///setTokenDescription(4, "VG10", "Polyglycol");

        // Additive Type 3 / Application
        setTokenColumnName(5, "Additive Type 3 / Application");
        setTokenDescription(5, "ASHLESS", "Ashless Additive (Leaves no solid residue, commonly used in clean-burning lubricants)");
        setTokenDescription(5, "H1", "Food-Grade Lubricant (Approved for incidental food contact in food-processing equipment)");
        setTokenDescription(5, "EP", "Extreme Pressure (Enhances load-carrying capacity, prevents wear under high pressure)");
        setTokenDescription(5, "EP(Mo)", "Extreme Pressure with Molybdenum Disulfide (MoS₂) (Enhances anti-wear and load-bearing properties)");
        setTokenDescription(5, "EP(Grph)", "Extreme Pressure with Graphite (Provides solid lubrication, ideal for high-load applications)");
        setTokenDescription(5, "AW", "Anti-Wear (Reduces wear in high-friction applications)");
        setTokenDescription(5, "AW/EP", "Combination of Anti-Wear and Extreme Pressure properties");
        setTokenDescription(5, "HT", "High Temperature (Designed for use in high-temperature environments)");
        setTokenDescription(5, "180", "Likely refers to viscosity rating (e.g., cSt at 40°C)");
        setTokenDescription(5, "460", "ISO Viscosity Grade 460 (Industrial lubricant standard)");
        setTokenDescription(5, "1500", "ISO Viscosity Grade 1500 (Used in very high-load applications)");


        // Thickener Type
        //ui->A_tbl->setModel(description_maps[0].model);
        //ui->B_tbl->setModel(description_maps[1].model);
        //ui->C_tbl->setModel(description_maps[2].model);
        //ui->D_tbl->setModel(description_maps[3].model);
        //ui->E_tbl->setModel(description_maps[4].model);
        //ui->F_tbl->setModel(description_maps[5].model);

        populateTokenDescriptionModels();
        populateTokenDescriptionTables();

        //auto prepare_table = [](QTableView* table, QStandardItemModel* model)
        //{
        //    model->setHeaderData(0, Qt::Horizontal, "Token");
        //    model->setHeaderData(1, Qt::Horizontal, "Description");
        //    table->setModel(model);
        //};
        //prepare_table(ui->A_tbl, description_maps[0].model);
        //prepare_table(ui->B_tbl, description_maps[1].model);
        //prepare_table(ui->C_tbl, description_maps[2].model);
        //prepare_table(ui->D_tbl, description_maps[3].model);
        //prepare_table(ui->E_tbl, description_maps[4].model);
        //prepare_table(ui->F_tbl, description_maps[5].model);

        //ui->A_tbl->resizeColumnsToContents();
        //ui->B_tbl->resizeColumnsToContents();
        //ui->C_tbl->resizeColumnsToContents();
        //ui->D_tbl->resizeColumnsToContents();
        //ui->E_tbl->resizeColumnsToContents();
    }*/

    rebuildDatabaseAndPopulateUI();
}

PageOptions::~PageOptions()
{
    delete ui;
}

void PageOptions::processExportPDF()
{
    if (export_index >= product_oiltype_entries.size())
    {
        if (batch_dialog->isSinglePDF())
        {
            auto composite_pdf = pdf_writer.finalize();
            pdf_exporter->doSave("Select PDF Path", "label.pdf", "PDF File (*.pdf);", composite_pdf);
        }
        else
        {
            QByteArray tarFile = createTarArchive(pdfs);
            pdf_exporter->doSave("Select Export Path", "batch_labels.tar", "TAR File (*.tar);", tarFile);
        }

        batch_dialog->setDisabled(false);
        batch_export_busy = false;

        return;
    }

    auto pair = product_oiltype_entries.at(export_index);

    // Process current file
    recomposePage(pair.first.c_str(), pair.second, false, [this, pair](QGraphicsScene* scene, ComposerResultInt result)
    {
        if (scene && result == ComposerResult::SUCCEEDED)
        {
            // Success, write page to PDF...

            if (batch_dialog->isSinglePDF())
            {
                pdf_writer.addPage(scene);
            }
            else
            {
                QString prod_name = pair.first.c_str();
                prod_name = prod_name.replace('/', "__");
                QString filename = (pair.second->cell_material_code->txt + "___" + prod_name.toStdString() + ".pdf").c_str();

                QByteArray pdf = generatePDF(scene, pagePreview->targetPageDPI(), pagePreview->targetPageMargin());
                pdfs.append({ filename, pdf });
            }
        }

        if (result == ComposerResult::SUCCEEDED)
            batch_dialog->addLogMessage(("Composed:  " + pair.first).c_str());
        else
        {
            batch_dialog->addLogError(("Error:  " + pair.first).c_str());

            if (result & ComposerResult::MISSING_MATERIAL_CODE)
                batch_dialog->addLogError("   - Invalid:  Material Code");
            if (result & ComposerResult::MISSING_GENERIC_CODE)
                batch_dialog->addLogError("   - Invalid:  Generic Code");
            if (result & ComposerResult::MISSING_SHAPE)
                batch_dialog->addLogError("   - Invalid:  Shape");
            if (result & ComposerResult::MISSING_SHAPE_COLOR)
                batch_dialog->addLogError("   - Invalid:  Shape Color");
            if (result & ComposerResult::MISSING_BACK_COLOR)
                batch_dialog->addLogError("   - Invalid:  Background Color");
        }

        ++export_index;
        float progress = (float)export_index / (float)product_oiltype_entries.size();
        batch_dialog->setProgress(progress);

        // Schedule the next file processing without blocking the UI
        QTimer::singleShot(25, this, &PageOptions::processExportPDF);
    });
}

void PageOptions::onChangeSelectedMaterialEntry()
{
    field_shape->setCurrentItem(nullableFieldPropTxt(selected_entry->cell_shape->txt));
    field_shape_col->setCurrentItem(nullableFieldPropTxt(selected_entry->cell_shape_color->txt));
    field_back_col->setCurrentItem(nullableFieldPropTxt(selected_entry->cell_back_color->txt));
    field_inner_back_col->setCurrentItem(nullableFieldPropTxt(selected_entry->cell_inner_back_color->txt));

    recomposePage(getSelectedProduct(), selected_entry, true);
    updateSelectedProductTokenTable();
}

QString PageOptions::getSelectedProduct()
{
    SearchableListItem selected_product_item;
    if (field_products->getCurrentItem(&selected_product_item))
    {
        return selected_product_item.txt;
    }
    return "";
}

ComposerResultInt PageOptions::recomposePage(
    QString product_name,
    OilTypeEntryPtr entry,
    bool allow_errors,
    std::function<void(QGraphicsScene*, ComposerResultInt)> callback)
{
    ComposerResultInt ret = 0;

    if (entry)
    {
        composeInfo.shape = composerGenerator.getShapeInfo(entry->cell_shape->txt.c_str());
        composeInfo.shape_color = composerGenerator.getShapeColor(entry->cell_shape_color->txt.c_str());
        composeInfo.tag_background_color = composerGenerator.getBackColor(entry->cell_back_color->txt.c_str());
        composeInfo.tag_inner_background_color = composerGenerator.getInnerBackColor(entry->cell_inner_back_color->txt.c_str());

        composeInfo.material_code = entry->cell_material_code->txt.c_str();
        composeInfo.generic_code = entry->cell_generic_code->txt.c_str();
        composeInfo.product_name = product_name;

        if (composeInfo.product_name.size() == 0)
            ret |= ComposerResult::MISSING_PRODUCT_NAME;

        if (composeInfo.material_code.size() == 0)
            ret |= ComposerResult::MISSING_MATERIAL_CODE;

        if (composeInfo.generic_code.size() == 0)
            ret |= ComposerResult::MISSING_GENERIC_CODE;

        if (!composeInfo.shape.valid || entry->cell_shape->txt.size() == 0)
            ret |= ComposerResult::MISSING_SHAPE;

        if (entry->cell_shape_color->txt.size() == 0)
            ret |= ComposerResult::MISSING_SHAPE_COLOR;

        if (entry->cell_back_color->txt.size() == 0)
            ret |= ComposerResult::MISSING_BACK_COLOR;


        if (allow_errors || ret == ComposerResult::SUCCEEDED)
        {
            composeInfo.autoDetermineStroke();
            QGraphicsScene* scene = pagePreview->composeScene(composeInfo);
            if (callback)
                callback(scene, ret);

            return ret;
        }
    }
    else
    {
        ret |= ComposerResult::NO_SELECTED_ITEM;
    }

    if (callback)
        callback(nullptr, ret);

    return ret;
}
void PageOptions::updateStatusBar()
{
    if (!statusBar) return;
    statusBar->showMessage("Project:   " + project_filename + "       CSV:   " + csv_filename);
}

void PageOptions::readCSV(const char* text)
{
    csv.clear();
    csv.open(text);
}

void PageOptions::rebuildDatabaseAndPopulateUI()
{
    if (!rebuildDatabase())
    {
        // Handle error
        QMessageBox* box = new QMessageBox(QMessageBox::Critical,
            "Error",
            "Unable to parse CSV headers. Please assign a new valid CSV.",
            QMessageBox::NoButton,
            this);
        box->show();
    }

    repopulateLists();
    scanForAllTokenParseErrors();
}

bool PageOptions::rebuildDatabase()
{
    // Clear maps
    merged_code_entries.clear();
    product_oiltype_entries.clear();

    if (csv.opened())
    {
        std::vector<CSVHeaderPtr> vendor_headers;

        // Find headers and assign id's
        {
            auto header_identifier_cell = csv.findCellFuzzy("HEADERS");
            int header_row = header_identifier_cell->row;

            CSVRect headers_r(
                CSVRect::BEG, header_row,
                CSVRect::END, header_row
            );

            // Material header
            auto material_code_header_cell = csv.findCell("SA_Material_Code", headers_r);
            if (!material_code_header_cell) return false;
            csv.setHeader(material_code_header_cell)->setCustomId("material_code");

            // Generic Code
            auto generic_code_header_cell = csv.findCell("Generic_Code", headers_r);
            if (!generic_code_header_cell) return false;
            csv.setHeader(generic_code_header_cell)->setCustomId("generic_code");

            // Find all manufacturer headers
            auto vendor_header_cells = csv.findCellsFuzzy("Vendor", headers_r, true, false);
            for (size_t i = 0; i < vendor_header_cells.size(); i++)
            {
                auto header_cell = csv.setHeader(vendor_header_cells[i]);
                if (!header_cell) return false;
                vendor_headers.push_back(
                    header_cell->setCustomId("vendor_" + QString::number(i).toStdString())
                );
            }

            // Shape
            auto shape_header_cell = csv.findCell("Shape", headers_r);
            if (!shape_header_cell) return false;
            csv.setHeader(shape_header_cell)->setCustomId("shape");

            // Shape Color
            auto shape_color_header_cell = csv.findCell("ShapeColour", headers_r);
            if (!shape_color_header_cell) return false;
            csv.setHeader(shape_color_header_cell)->setCustomId("shape_color");

            // Back Color
            auto back_color_header_cell = csv.findCell("BackColour", headers_r);
            if (!back_color_header_cell) return false;
            csv.setHeader(back_color_header_cell)->setCustomId("back_color");

            // Inner Back Color
            auto inner_back_color_header_cell = csv.findCell("InnerBackColour", headers_r);
            if (!inner_back_color_header_cell) return false;
            csv.setHeader(inner_back_color_header_cell)->setCustomId("inner_back_color");
        }

        csv.readData();

        for (int i = csv.dataRowFirst(); i < csv.dataRowLast(); i++)
        {
            CSVRow& row = csv.getRow(i);

            // Read basic values
            auto entry = std::make_shared<OilTypeEntry>();
            entry->cell_material_code = row.findCellByHeaderCustomID("material_code");
            entry->cell_generic_code = row.findCellByHeaderCustomID("generic_code");
            entry->cell_shape = row.findCellByHeaderCustomID("shape");
            entry->cell_shape_color = row.findCellByHeaderCustomID("shape_color");
            entry->cell_back_color = row.findCellByHeaderCustomID("back_color");
            entry->cell_inner_back_color = row.findCellByHeaderCustomID("inner_back_color");

            // Skipping entries with missing generic code
            if (entry->cell_generic_code->txt.size() == 0)
                continue;

            // Look at each manufacturer, see if product available for this oil type
            for (auto vendor_header_cell : vendor_headers)
            {
                // Is a product defined?
                CSVCellPtr vendor_cell = row.findCellByHeader(vendor_header_cell);
                if (vendor_cell->txt.size() &&
                    !vendor_cell->txt.compare("not defined", true, false) &&
                    !vendor_cell->txt.compare("undefined", true, false))
                {
                    // Yes
                    entry->vendor_cells.push_back(vendor_cell);

                    // Map the product back to the entry
                    product_oiltype_entries[vendor_cell->txt] = entry;
                }
            }

            // Map [material/generic] code to entry
            merged_code_entries[entry->mergedCode()] = entry;
        }
    }

    merged_code_entries.sort([](
        const std::pair<std::string, OilTypeEntryPtr>& a,
        const std::pair<std::string, OilTypeEntryPtr>& b)
    {
        return a.second->cell_generic_code->txt < b.second->cell_generic_code->txt;
    });

    product_oiltype_entries.sort([](
        const std::pair<std::string, OilTypeEntryPtr>& a,
        const std::pair<std::string, OilTypeEntryPtr>& b)
    {
        return a.first < b.first;
    });

    return true;
}

void PageOptions::repopulateLists(
    bool bMergedCode,
    bool bProducts,
    bool bShape,
    bool bShapeCol,
    bool bBackCol,
    bool bInnerBackCol,
    bool bCSV)
{
    field_shape->lockSortOrder();
    field_shape_col->lockSortOrder();
    field_back_col->lockSortOrder();
    field_inner_back_col->lockSortOrder();

    // Clear lists
    if (bMergedCode) field_merged_code->clear();
    if (bProducts) field_products->clear();
    if (bShape) field_shape->clear();
    if (bShapeCol) field_shape_col->clear();
    if (bBackCol) field_back_col->clear();
    if (bInnerBackCol) field_inner_back_col->clear();

    // Add placeholder <unassigned> for missing entry fields
    if (bShape) field_shape->addUniqueListItem("<unassigned>");
    if (bShapeCol) field_shape_col->addUniqueListItem("<unassigned>");
    if (bBackCol) field_back_col->addUniqueListItem("<unassigned>");
    if (bInnerBackCol) field_inner_back_col->addUniqueListItem("<unassigned>");

    // Set project-saved relationships, even if CSV doesn't use them or they already exist
    {
        // Shape
        if (bShape)
        {
            for (const auto& [key, v] : composerGenerator.shapeMap())
                field_shape->addUniqueListItem(key);
        }

        // Shape Color
        if (bShapeCol)
        {
            for (const auto& [key, v] : composerGenerator.shapeColorMap())
                field_shape_col->addUniqueListItem(key);
        }

        // Back Color
        if (bBackCol)
        {
            for (const auto& [key, v] : composerGenerator.backColorMap())
                field_back_col->addUniqueListItem(key);
        }

        // Inner-Back Color
        if (bInnerBackCol)
        {
            for (const auto& [key, v] : composerGenerator.innerBackColorMap())
                field_inner_back_col->addUniqueListItem(key);
        }
    }

    // Populate lists with database values
    for (const auto& [merged_code, entry] : merged_code_entries)
    {
        // Merged Oil-Type Code
        if (bMergedCode)
            field_merged_code->addUniqueListItem(merged_code.c_str(), entry);

        // Shape
        if (bShape)
        {
            QString shape_txt = nullableFieldPropTxt(entry->cell_shape->txt);
            field_shape->addUniqueListItem(shape_txt);
        }

        // Shape Color
        if (bShapeCol)
        {
            QString shape_col_txt = nullableFieldPropTxt(entry->cell_shape_color->txt);
            field_shape_col->addUniqueListItem(shape_col_txt);
        }

        // Back Color
        if (bBackCol)
        {
            QString back_col_txt = nullableFieldPropTxt(entry->cell_back_color->txt);
            field_back_col->addUniqueListItem(back_col_txt);
        }

        // Inner-Back Color
        if (bInnerBackCol)
        {
            QString inner_back_col_txt = nullableFieldPropTxt(entry->cell_inner_back_color->txt);
            field_inner_back_col->addUniqueListItem(inner_back_col_txt);
        }
    }

    // Show all vendors
    if (bProducts && field_products->getRadioChecked())
    {
        for (const auto& [vendor, entry] : product_oiltype_entries)
            field_products->addListItem(vendor.c_str());
    }

    // Repopulate table
    if (bCSV && pagePreview)
    {
        QTableView* table = pagePreview->getTable();
        table->setModel(&csv_model);

        int row = 0;
        csv_model.clear();

        auto csv_headers = csv.getHeaders();

        QStringList headers;
        //for (auto header : csv_headers)
        //    headers.push_back(header->subheaders.back()->txt.c_str());

        std::vector<OilTypeEntryPtr> sorted_entries;
        for (const auto& [merged_code, entry] : merged_code_entries)
            sorted_entries.push_back(entry);

        std::sort(sorted_entries.begin(), sorted_entries.end(), [](OilTypeEntryPtr a, OilTypeEntryPtr b)
        {
            long long mat_a = a->cell_material_code->txt.size() ? std::stoll(a->cell_material_code->txt) : 0;
            long long mat_b = b->cell_material_code->txt.size() ? std::stoll(b->cell_material_code->txt) : 0;
            return mat_a < mat_b;
        });

        for (OilTypeEntryPtr entry : sorted_entries)
        {
            QList<QStandardItem*> rowItems;

            qsizetype col = 0;

            // Material code
            if (col >= headers.size()) headers.resize(col + 1);
            headers[col++] = "Material Code";

            rowItems.append(new QStandardItem(entry->cell_material_code->txt.c_str()));

            // Generic code
            if (col >= headers.size()) headers.resize(col + 1);
            headers[col++] = "Generic Code";

            rowItems.append(new QStandardItem(entry->cell_generic_code->txt.c_str()));

            // Shape
            if (col >= headers.size()) headers.resize(col + 1);
            headers[col++] = entry->cell_shape->header->subheaders.back()->txt.c_str();

            QString shape_txt = entry->cell_shape->txt.c_str();
            rowItems.append(new QStandardItem(shape_txt));

            // Shape Color
            if (col >= headers.size()) headers.resize(col + 1);
            headers[col++] = entry->cell_shape_color->header->subheaders.back()->txt.c_str();

            QString shape_col_txt = entry->cell_shape_color->txt.c_str();
            rowItems.append(new QStandardItem(shape_col_txt));

            // Back Color
            if (col >= headers.size()) headers.resize(col + 1);
            headers[col++] = entry->cell_back_color->header->subheaders.back()->txt.c_str();

            QString back_col_txt = entry->cell_back_color->txt.c_str();
            rowItems.append(new QStandardItem(back_col_txt));

            // Inner Back Color
            if (col >= headers.size()) headers.resize(col + 1);
            headers[col++] = entry->cell_inner_back_color->header->subheaders.back()->txt.c_str();

            QString inner_back_col_txt = entry->cell_inner_back_color->txt.c_str();
            rowItems.append(new QStandardItem(inner_back_col_txt));

            for (auto vendor_cell : entry->vendor_cells)
            {
                if (col >= headers.size()) headers.resize(col + 1);
                headers[col++] = vendor_cell->header->subheaders.back()->txt.c_str();

                QString vendor_txt = vendor_cell->txt.c_str();
                rowItems.append(new QStandardItem(vendor_txt));
            }

            csv_model.insertRow(row++, rowItems);
        }

        csv_model.setHorizontalHeaderLabels(headers);
        table->resizeColumnsToContents();
    }

    field_shape->unlockSortOrder();
    field_shape_col->unlockSortOrder();
    field_back_col->unlockSortOrder();
    field_inner_back_col->unlockSortOrder();

    countStyleUsers();
    updateItemsEditable();

    //field_merged_code->setSelectable(false);
}


void PageOptions::setTokenColumnName(int col, QString name)
{
    //if (col >= description_maps.size())
    //    description_maps.resize(col + 1);
    
    description_maps[col].name = name;
}

void PageOptions::setTokenDescription(int col, QString token, QString desc)
{
    //if (col >= description_maps.size())
    //    description_maps.resize(col + 1);
    
    description_maps[col].set(token, desc);
}



void PageOptions::onEditSelectedProductTokenDescription(int row, int col, QString new_txt)
{
    // Map "column" in this case is actually the table row because of the way data is presented
    TokenDescriptionMap& map = description_maps[row];

    std::vector<GenericCodeTokenInfo> existing_tokens = parseGenericCodeTokens(selected_entry->cell_generic_code->txt);
    const GenericCodeTokenInfo& existing_token_info = existing_tokens[row];
    std::string existing_token = existing_token_info.txt;
    std::string existing_desc = existing_token_info.desc;

    // Which token name/description are we changing?
    QModelIndex token_model_index = selected_product_description_model.index(row, 0);
    QModelIndex desc_model_index = selected_product_description_model.index(row, 1);

    std::string new_token = token_model_index.data(Qt::DisplayRole).toString().toStdString();
    std::string new_desc = desc_model_index.data(Qt::DisplayRole).toString().toStdString();

    auto finalize = [&map, this]()
    {
        map.sortByDescendingLength();

        // Multiple materials might have been affected. Recheck all for parse errors
        scanForAllTokenParseErrors();

        // Reparse "selected" generic code and repopulate table
        updateSelectedProductTokenTable();

        // Update tables in settings too
        populateTokenDescriptionModels();
        populateTokenDescriptionTables();
    };

    if (col == 0)
    {
        if (new_txt.isEmpty())
        {
            // Removing this token
            QMessageBox* box = new QMessageBox(QMessageBox::Warning,
                "Warning",
                "Erasing token/description for all matching items. Are you sure?",
                QMessageBox::StandardButton::Yes|QMessageBox::StandardButton::Abort,
                this);

            QObject::connect(box, &QMessageBox::finished, [&map, existing_token, finalize](int result)
            {
                if (result == QMessageBox::Yes)
                    map.lookup.remove(existing_token);

                finalize();
            });
            box->show();
        }
        else
        {
            // New token was entered (not empty)

            // Test to see if the entered token is parseable (without altering data)
            {
                bool parseable = isChangedTokenParsable(map, row, new_txt.toStdString());
                if (!parseable)
                {
                    updateSelectedProductTokenTable();

                    QMessageBox* box = new QMessageBox(QMessageBox::Critical,
                        "Error", "Not a valid token for this generic code",
                        QMessageBox::NoButton,
                        this);

                    box->show();
                    return;
                }
            }

            // New token entered IS parseable

            // Changing token name
            if (map.lookup.contains(new_txt.toStdString()))
            {
                /* A token/description already exists with this name... How should we proceed?

                 Example: Renaming "10W-40" to "10W" when 10W already has a description
                 Message:

                    A description already exists for the token "10W"

                    - Click "Apply" to apply the new description to all other products using the same token
                    - Click "Discard" to discard the new description and keep the existing description            */


                    //int affect_count = 1;
                    //QString warning = "Changing this token will affect " + QString::number(affect_count) + " other products. Proceed?";

                QString warning = "A description already exists for the token \"" + QString(new_token.c_str()) + "\"\n"\
                    "- Click \"Apply\" to overwrite the saved description\n"\
                    "- Click \"Discard\" to switch to the saved description";

                QMessageBox* box = new QMessageBox(QMessageBox::Critical,
                    "Warning", warning,
                        QMessageBox::StandardButton::Apply |
                        QMessageBox::StandardButton::Discard |
                        QMessageBox::StandardButton::Abort,
                    this);

                QObject::connect(box, &QMessageBox::finished, [this, row, &map, existing_token, finalize, new_token, new_desc, new_txt](int ret)
                {
                    // todo: The issue is subtle. It's more about if you're shortening the code
                    //       leaving the longer code active, EVEN if not in use. The warning needs
                    //       to explain this, and present the options:
                    // > "Not possible to shorten token without remove "10W-40" first, otherwise
                    //    that will get matched before the suggested alternative "10W".
                    //    (Discard) Remove 10W-40 (Currency in use by X other materials)
                    //    (Abort)

                    // Is the existing token in use anywhere else? If not, prompt for deletion
                    //if (countTokenUsers(row, existing_token) == 0)
                    //{
                    //    QMessageBox* remove_existing_question_box = new QMessageBox(QMessageBox::Question,
                    //        "Remove old token?", 
                    //        '"' + QString(existing_token.c_str()) + "\" is no longer used by any materials.\n\nRemove?",
                    //        QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                    //        this);
                    //
                    //    connect(remove_existing_question_box, &QMessageBox::finished,
                    //        [finalize, &map, existing_token](int ret)
                    //    {
                    //        map.lookup.remove(existing_token);
                    //        finalize();
                    //    });
                    //
                    //    remove_existing_question_box->show();
                    //}
                    //else
                    {
                        if (ret == QMessageBox::StandardButton::Apply)
                            map.lookup.set(new_txt.toStdString(), new_desc);

                        finalize();
                    }
                });

                box->show();

                /*QMessageBox::StandardButton ret = QMessageBox::question(this, "Warning", warning,
                    QMessageBox::StandardButton::Apply | QMessageBox::StandardButton::Discard | QMessageBox::StandardButton::Abort);

                if (ret == QMessageBox::StandardButton::Apply)
                {
                    map.lookup.remove(existing_token);
                    map.lookup.set(new_txt.toStdString(), new_desc);
                    map.sortByDescendingLength();
                }
                else if (ret == QMessageBox::StandardButton::Discard)
                {
                    std::string existing_desc = map.lookup.get(new_token);
                    map.lookup.remove(existing_token);
                }*/
            }
            else
            {
                // Setup a new token/description with a dummy description
                if (existing_token_info.found)
                    map.lookup.remove(existing_token);

                map.lookup.set(new_txt.toStdString(), existing_desc);
                finalize();
            }
        }
    }
    else if (col == 1)
    {
        // Changing token description
        
        if (!new_token.empty())
            map.lookup[new_token] = new_txt.toStdString();

        finalize();
    }

    //
}

void PageOptions::onEditSettingsProductTokenDescription(int table_index, int row, int col, QString txt)
{
    TokenDescriptionMap& map = description_maps[table_index];
    std::string existing_token = map.lookup.at(row).first;
    std::string existing_desc = map.lookup.at(row).second;

    if (col == 0)
    {
        map.lookup.remove(existing_token);
        map.lookup.set(txt.toStdString(), existing_desc);

        map.sortByDescendingLength();
    }
    else if (col == 1)
    {
        map.lookup.set(existing_token, txt.toStdString());
    }

    map.populateModel();
    map.model->setHeaderData(0, Qt::Horizontal, "Token");
    map.model->setHeaderData(1, Qt::Horizontal, "Description");

    if (selected_entry)
    {
        // Reparse "selected" generic code and repopulate table
        updateSelectedProductTokenTable();
    }

    scanForAllTokenParseErrors();
}

std::vector<PageOptions::GenericCodeTokenInfo> PageOptions::parseGenericCodeTokens(const string_ex& generic_code)
{
    int cur_generic_substr_i = 0;
    bool succesfully_finalized = false;

    std::vector<GenericCodeTokenInfo> tokens;

    for (int i = 0; ; i++)
    {
        if (i >= description_maps.size())
            break;

        QString name = description_maps[i].name; // e.g. "Application", "Grease Consistency"

        std::string matched_token;

        auto lookup = description_maps[i].lookup;
        bool found_match = false;

        // Greedy algorithm for matching next substring
        for (size_t j = 0; j < lookup.size(); j++)
        {
            // Do we have any tokens left to read?
            if (cur_generic_substr_i >= generic_code.size())
            {
                succesfully_finalized = true;
                break;
            }

            const string_ex& lookup_tok = lookup.at(j).first;
            int remaining_len = generic_code.size() - cur_generic_substr_i;
            int max_match_len = std::min(remaining_len, static_cast<int>(lookup_tok.size()));

            bool substr_matched = generic_code.substr(cur_generic_substr_i, max_match_len).tolower() == lookup_tok.tolower();

            bool proceeded_by_delim =
                ((cur_generic_substr_i + lookup_tok.size()) == generic_code.size()) // Followed by null-terminator?
                || (generic_code[cur_generic_substr_i + max_match_len] == '-');     // Followed by '-'?

            if (substr_matched && proceeded_by_delim)
            {
                // Found a match, consume substring
                cur_generic_substr_i += lookup_tok.size() + 1; // +1 for hyphen
                matched_token = lookup_tok;
                found_match = true;
                break;
            }
        }


        if (found_match)
        {
            GenericCodeTokenInfo token_info;
            token_info.txt = matched_token;
            token_info.desc = description_maps[i].lookup.get(matched_token).c_str();
            token_info.found = true;

            tokens.push_back(token_info);
        }
        else
        {
            if (!succesfully_finalized)
            {
                GenericCodeTokenInfo token_info;
                token_info.found = false;
                tokens.push_back(token_info);
            }
            break;
        }

        if (succesfully_finalized)
            break;
    }

    return tokens;
}

void PageOptions::updateSelectedProductTokenTable()
{
    OilTypeEntryPtr entry = selected_entry;
    const string_ex& generic_code = entry->cell_generic_code->txt;
    //int cur_generic_substr_i = 0;

    selected_product_description_model.removeRows(0, selected_product_description_model.rowCount());

    std::vector<GenericCodeTokenInfo> parsed_tokens = parseGenericCodeTokens(generic_code);
    for (size_t i = 0; i < parsed_tokens.size(); i++)
    {
        const GenericCodeTokenInfo& token_info = parsed_tokens[i];

        QString name = description_maps[i].name;
        QList<QStandardItem*> rowItems;

        if (token_info.found)
        {
            rowItems.push_back(new QStandardItem(token_info.txt.c_str()));
            rowItems.push_back(new QStandardItem(token_info.desc.c_str()));

            selected_product_description_model.insertRow(i, rowItems);
            selected_product_description_model.setHeaderData(i, Qt::Vertical, name);
        }
        else
        {
            rowItems.push_back(new QStandardItem("<no match>"));
            rowItems.push_back(new QStandardItem("<ended parsing>"));

            selected_product_description_model.insertRow(i, rowItems);
            selected_product_description_model.setHeaderData(i, Qt::Vertical, name);
        }
    }

    /*bool succesfully_finalized = false;

    for (int i = 0; ; i++)
    {
        if (i >= description_maps.size())
            break;

        QString name = description_maps[i].name; // e.g. "Application", "Grease Consistency"

        std::string matched_token;

        auto lookup = description_maps[i].lookup;
        bool found_match = false;

        // Greedy algorithm for matching next substring
        for (size_t j = 0; j < lookup.size(); j++)
        {
            // Do we have any tokens left to read?
            if (cur_generic_substr_i >= generic_code.size())
            {
                succesfully_finalized = true;
                break;
            }

            const string_ex& lookup_tok = lookup.at(j).first;
            int remaining_len = generic_code.size() - cur_generic_substr_i;
            int max_match_len = std::min(remaining_len, static_cast<int>(lookup_tok.size()));

            bool substr_matched = generic_code.substr(cur_generic_substr_i, max_match_len).tolower() == lookup_tok.tolower();

            bool proceeded_by_delim =
                ((cur_generic_substr_i + lookup_tok.size()) == generic_code.size()) // Followed by null-terminator?
                || (generic_code[cur_generic_substr_i + max_match_len] == '-');     // Followed by '-'?

            if (substr_matched && proceeded_by_delim)
            {
                // Found a match, consume substring
                cur_generic_substr_i += lookup_tok.size() + 1; // +1 for hyphen
                matched_token = lookup_tok;
                found_match = true;
                break;
            }
        }

        if (succesfully_finalized)
            break;

        QString desc = description_maps[i].lookup.get(matched_token).c_str();
        QList<QStandardItem*> rowItems;

        if (found_match)
        {
            rowItems.push_back(new QStandardItem(matched_token.c_str()));
            rowItems.push_back(new QStandardItem(desc));

            selected_product_description_model.insertRow(i, rowItems);
            selected_product_description_model.setHeaderData(i, Qt::Vertical, name);
        }
        else
        {
            rowItems.push_back(new QStandardItem("<no match>"));
            rowItems.push_back(new QStandardItem("<ended parsing>"));

            selected_product_description_model.insertRow(i, rowItems);
            selected_product_description_model.setHeaderData(i, Qt::Vertical, name);

            break;
        }
    }*/

    selected_product_description_model.setHeaderData(0, Qt::Horizontal, "Token");
    selected_product_description_model.setHeaderData(1, Qt::Horizontal, "Description");

    ui->description_tbl->setModel(&selected_product_description_model);
    ui->description_tbl->resizeRowsToContents();
    ui->description_tbl->resizeColumnsToContents();
    ui->description_tbl->setColumnWidth(0, 80);

    for (int row = 0; row < ui->description_tbl->model()->rowCount(); ++row)
    {
        int currentHeight = ui->description_tbl->rowHeight(row);
        if (currentHeight < 25)
            ui->description_tbl->setRowHeight(row, 25);
    }
}

bool PageOptions::isChangedTokenParsable(TokenDescriptionMap& map, int token_index, std::string new_token)
{
    std::vector<GenericCodeTokenInfo> existing_tokens = parseGenericCodeTokens(selected_entry->cell_generic_code->txt);
    const GenericCodeTokenInfo& existing_token_info = existing_tokens[token_index];
    std::string existing_token = existing_token_info.txt;
    std::string existing_desc = existing_token_info.desc;

    bool parseable = false;
    bool new_token_already_exists = map.lookup.contains(new_token);

    // Temporarily erase existing token (since we're trying to change it to something else)
    if (existing_token_info.found) map.lookup.remove(existing_token);

    if (new_token_already_exists)
    {
        std::vector<GenericCodeTokenInfo> parse_test = parseGenericCodeTokens(selected_entry->cell_generic_code->txt);
        parseable = (token_index < parse_test.size() && parse_test[token_index].txt == new_token);
    }
    else
    {
        map.lookup.set(new_token, "dummy");
        map.sortByDescendingLength();
        std::vector<GenericCodeTokenInfo> parse_test = parseGenericCodeTokens(selected_entry->cell_generic_code->txt);
        parseable = (token_index < parse_test.size() && parse_test[token_index].txt == new_token);
        map.lookup.remove(new_token);
    }

    // Recover temporarily erased existing token before proceeding
    if (existing_token_info.found)
    {
        map.lookup.set(existing_token, existing_desc);
        map.sortByDescendingLength();
    }

    return parseable;
}

inline void PageOptions::checkForParseError(OilTypeEntryPtr entry)
{
    auto parsed_tokens = parseGenericCodeTokens(entry->cell_generic_code->txt);

    bool any_missing_descriptions = false;
    for (auto info : parsed_tokens)
    {
        if (info.desc.empty())
        {
            any_missing_descriptions = true;
            break;
        }
    }

    entry->token_parse_errors = 
        parsed_tokens.empty() || 
        !parsed_tokens.back().found || 
        any_missing_descriptions; // Treat no description as a parse error
}

void PageOptions::scanForAllTokenParseErrors()
{
    for (const auto& [merged_code, entry] : merged_code_entries)
        checkForParseError(entry);

    field_merged_code->repaint();
}


void PageOptions::populateTokenDescriptionModels()
{
    // First, sort tokens in descending order of length, so greedy algorithm
    // prioritizes matches with the longest length
    for (size_t i = 0; i < description_maps.size(); i++)
        description_maps[i].sortByDescendingLength();

    for (size_t i = 0; i < description_maps.size(); i++)
        description_maps[i].populateModel();
}

void PageOptions::populateTokenDescriptionTables()
{
    auto prepare_table = [](QTableView* table, std::shared_ptr<QStandardItemModel> model)
    {
        model->setHeaderData(0, Qt::Horizontal, "Token");
        model->setHeaderData(1, Qt::Horizontal, "Description");
        table->setModel(model.get());
    };

    for (size_t i = 0; i < token_tables.size(); i++)
        prepare_table(token_tables[i], description_maps[i].model);

    //prepare_table(ui->A_tbl, description_maps[0].model);
    //prepare_table(ui->B_tbl, description_maps[1].model);
    //prepare_table(ui->C_tbl, description_maps[2].model);
    //prepare_table(ui->D_tbl, description_maps[3].model);
    //prepare_table(ui->E_tbl, description_maps[4].model);
    //prepare_table(ui->F_tbl, description_maps[5].model);
}

int PageOptions::countTokenUsers(int tok_i, std::string token)
{
    int users = 0;
    for (const auto& [merged_code, entry] : merged_code_entries)
    {
        auto parsed_tokens = parseGenericCodeTokens(entry->cell_generic_code->txt);
        bool match = (tok_i < parsed_tokens.size()) && (parsed_tokens[tok_i].txt == token);

        if (match)
            users++;
    }
    return users;
}

void PageOptions::countStyleUsers()
{
    users_shape.clear();
    users_shape_color.clear();
    users_back_color.clear();
    users_inner_back_color.clear();

    for (const auto& [merged_code, entry] : merged_code_entries)
    {
        int products = entry->vendor_cells.size();
        users_shape[entry->cell_shape->txt] += products;
        users_shape_color[entry->cell_shape_color->txt] += products;
        users_back_color[entry->cell_back_color->txt] += products;
        users_inner_back_color[entry->cell_inner_back_color->txt] += products;
    }
}

void PageOptions::updateItemsEditable()
{
    auto updateItemsEditable = [](
        SearchableList* field, 
        std::unordered_map<std::string, int>& users_map)
    {
        auto model = field->getModel();
        auto shape_items = model->getItems();
        for (qsizetype i = 0; i < shape_items.size(); i++)
        {
            SearchableListItem& item = model->itemAt(i);
            std::string txt = item.txt.toStdString();
            item.setEditable(users_map.count(txt) == 0);
        }
    };

    updateItemsEditable(field_shape, users_shape);
    updateItemsEditable(field_shape_col, users_shape_color);
    updateItemsEditable(field_back_col, users_back_color);
    updateItemsEditable(field_inner_back_col, users_inner_back_color);
}

QByteArray PageOptions::serialize()
{
    QJsonObject jsonObj;

    QJsonObject composerInfoGeneratorObj;
    composerGenerator.serialize(composerInfoGeneratorObj);
    jsonObj["composer_info_generator"] = composerInfoGeneratorObj;
    jsonObj["assigned_csv"] = csv.getText();
    jsonObj["version"] = 3;

    // Serialize description maps (by name)
    QJsonObject descriptionMapsObj;
    //for (const TokenDescriptionMap& map : description_maps)
    for (size_t i=0; i<description_maps.size(); i++)
    {
        const TokenDescriptionMap& map = description_maps.at(i);

        QJsonObject mapObj, mapDataObj;
        map.serialize(mapDataObj);

        mapObj["column"] = (int)i;
        mapObj["data"] = mapDataObj;

        descriptionMapsObj[map.name] = mapObj;
    }

    jsonObj["description_maps"] = descriptionMapsObj;

    QJsonDocument jsonDoc(jsonObj);
    return jsonDoc.toJson();
}

void PageOptions::deserialize(const QByteArray& json)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(json);

    if (jsonDoc.isNull() || !jsonDoc.isObject())
        return;

    QJsonObject jsonObj = jsonDoc.object();
    int version = jsonObj["version"].toInt();
    qDebug() << "Version:" << version;

    QString csv_text = jsonObj["assigned_csv"].toString();

    QJsonObject composerInfoGeneratorObj = jsonObj["composer_info_generator"].toObject();
    composerGenerator.deserialize(composerInfoGeneratorObj);

    if (csv_text.size())
    {
        readCSV(csv_text.toStdString().c_str());
        rebuildDatabaseAndPopulateUI();
    }

    if (version >= 3)
    {
        QJsonObject descriptionMapsObj = jsonObj["description_maps"].toObject();

        //description_maps.clear(); // Todo: Add warning about potential unsaved changes?
        //description_maps.resize(descriptionMapsObj.size());

        //int i = 0;
        for (auto it = descriptionMapsObj.begin(); it != descriptionMapsObj.end(); ++it)
        {
            QJsonObject mapObj = it->toObject();

            // Deserialize column index/name
            int column = mapObj["column"].toInt();
            TokenDescriptionMap& map = description_maps[column];
            QJsonObject mapDataObj = mapObj["data"].toObject();

            // Deserialize map name/entries
            map.name = it.key();
            map.deserialize(mapDataObj);

            // Sorted place into map list
            //description_maps[column] = map;
            //i++;
        }
        populateTokenDescriptionModels();
        populateTokenDescriptionTables();
    }
}

/*SearchableList* PageOptions::addSearchableListRow1(QString field_id, QString field_name)
{
    SearchableList* field_widget = new SearchableList(field_id.toStdString(), field_name);

    ui->field_list->addWidget(field_widget);
    return field_widget;
}

SearchableList* PageOptions::addSearchableListRow2(QString field_id, QString field_name)
{
    SearchableList* field_widget = new SearchableList(field_id.toStdString(), field_name);

    ui->product_lists->addWidget(field_widget);
    return field_widget;
}*/

/*SearchableList* PageOptions::initSearchableList(QString field_id, QString field_name)
{
    SearchableList* field_widget = ui->

    ui->product_lists->addWidget(field_widget);
    return field_widget;
}*/


