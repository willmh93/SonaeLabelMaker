#include <QPushButton>
#include <QSvgRenderer>


#include <string>
#include <algorithm>

#include "pageoptions.h"
#include "ui_pageoptions.h"
#include "fieldtoolbar.h"

#include "file_uploader.h"

#include "mainwindow.h"

//#define STRICTZIP
//#include <minizip/zip.h>
//#include <minizip/ioapi.h>


QByteArray generatePDF(QGraphicsScene* scene, float src_margin)
{
    QByteArray pdfData;
    QBuffer buffer(&pdfData);
    buffer.open(QIODevice::WriteOnly);

    QPdfWriter pdfWriter(&buffer);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    pdfWriter.setResolution(300);  // High quality

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

void PdfSceneWriter::start(float _src_margin)
{
    src_margin = _src_margin;

    pdfBuffer.open(QIODevice::WriteOnly);
    pdfWriter = new QPdfWriter(&pdfBuffer);
    pdfWriter->setPageSize(QPageSize(QPageSize::A4));
    pdfWriter->setResolution(300);

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
void tarSetOctal(char *field, size_t size, qint64 value) {
    snprintf(field, size, "%*llo", static_cast<int>(size - 1), value);
}

// Creates a TAR archive from multiple QByteArrays
QByteArray createTarArchive(const QVector<QPair<QString, QByteArray>> &files) {
    QByteArray tarData;

    for (const auto &file : files) {
        const QString &fileName = file.first;
        const QByteArray &fileContent = file.second;

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
        const unsigned char *rawHeader = reinterpret_cast<const unsigned char *>(&header);
        for (size_t i = 0; i < sizeof(TarHeader); i++)
            checksum += rawHeader[i];
        tarSetOctal(header.chksum, sizeof(header.chksum), checksum);

        // Append header to TAR archive
        tarData.append(reinterpret_cast<const char *>(&header), sizeof(TarHeader));

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

PageOptions::PageOptions(QStatusBar* _statusBar, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageOptions)
{
    ui->setupUi(this);
    statusBar = _statusBar;

    project_file_manager = new FileManager(this);
    csv_picker = new FileManager(this);
    svg_picker = new FileManager(this);
    pdf_exporter = new FileManager(this);

    field_merged_code = addSearchableListRow1("mat_gen_code", "Material / Generic Code");
    field_products    = addSearchableListRow1("products", "Available Products");

    field_shape       = addSearchableListRow2("shape", "Shape");
    field_shape_col   = addSearchableListRow2("shape_col", "Shape Color");
    field_back_col    = addSearchableListRow2("back_col", "Background Color");

    // Bigger rows for icons
    field_shape->setRowHeight(24);
    field_shape_col->setRowHeight(24);
    field_back_col->setRowHeight(24);
    
    // Set which fields are selectable
    field_shape->setSelectable(false);
    field_shape_col->setSelectable(false);
    field_back_col->setSelectable(false);

    //field_merged_code->setSelectable(false);

    // Set which fields are filterable
    field_shape->setFilterable(false);
    field_shape_col->setFilterable(false);
    field_back_col->setFilterable(false);

    // Prepare radio button logic
    radio_lists = std::make_shared<std::vector<SearchableList*>>();
    radio_lists->push_back(field_merged_code);
    radio_lists->push_back(field_products);

    field_merged_code->setRadioGroup(radio_lists);
    field_products->setRadioGroup(radio_lists);
    field_merged_code->setRadioChecked(true);

    // Field toolbars
    field_shape->setCustomWidget(new FieldToolbar());

    // Provide icon painter callbacks
    field_shape->setIconPainter([this](SearchableListItem& item, QPainter* painter, QRect& r)
    {
        if (composerGenerator.shapeMap().count(item.txt) == 0)
        {
            QPen pen(Qt::red, 2);
            painter->setPen(pen);
            painter->drawLine(r.topLeft(), r.bottomRight());
            painter->drawLine(r.topRight(), r.bottomLeft());
        }
        else
        {
            painter->save();

            // Disable anti-aliasing
            painter->setRenderHint(QPainter::Antialiasing, false);
            painter->setRenderHint(QPainter::SmoothPixmapTransform, false);


            ShapeInfo shape = composerGenerator.getShapeInfo(item.txt);
            QSvgRenderer renderer;
            renderer.load(shape.svg_icon);
            renderer.render(painter, r.adjusted(-2, -2, 2, 2));

            painter->restore();
        }
    });

    field_shape_col->setIconPainter([this](SearchableListItem& item, QPainter* painter, QRect& r)
    {
        if (composerGenerator.shapeColorMap().count(item.txt) == 0)
        {
            QPen pen(Qt::red, 2);
            painter->setPen(pen);
            painter->drawLine(r.topLeft(), r.bottomRight());
            painter->drawLine(r.topRight(), r.bottomLeft());
        }
        else
        {
            QColor color = composerGenerator.getShapeColor(item.txt);
            QBrush brush(color);
            painter->fillRect(r, brush);
        }
    });

    field_back_col->setIconPainter([this](SearchableListItem& item, QPainter* painter, QRect& r)
    {
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
        }
    });

    // Define callbacks
    auto toggleShowAllProducts = [this](bool)
    {
        field_merged_code->setSelectable(field_merged_code->getRadioChecked());
        field_merged_code->clearFilter();
        field_products->clearFilter();
        //field_products->setSelectable(field_products->getRadioChecked());
        repopulateLists();
    };

    auto changeShapeItem = [this](SearchableListItem& item)
    {
        svg_picker->doLoad("Select SVG File", "SVG File (*.svg);", [this, item](
            const QString& filename,
            const QByteArray& data)
        {
            composerGenerator.setShapeInfo(item.txt, ShapeInfo::fromData(data));
            field_shape->refresh();
            recomposePage(getSelectedProduct(), selected_entry);
        });
    };

    auto changeShapeColorItem = [this](SearchableListItem& item)
    {
        color_picker = new QColorDialog(this);
        color_picker->setCurrentColor(composerGenerator.getShapeColor(item.txt));
        connect(color_picker, &QColorDialog::colorSelected, this, [this, item](const QColor& color) {
            composerGenerator.setShapeColor(item.txt, color);
            field_shape_col->refresh();
            recomposePage(getSelectedProduct(), selected_entry);
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
            recomposePage(getSelectedProduct(), selected_entry);
        });
        color_picker->open();
    };

    // Hook up callbacks for detecting Radio toggle change
    field_merged_code->onRadioToggled(toggleShowAllProducts);
    field_products->onRadioToggled(toggleShowAllProducts);

    // Hook up callbacks for inputting new shapes/colours
    field_shape->onClickIcon(changeShapeItem);
    field_shape->onDoubleClickItem(changeShapeItem);
    field_shape_col->onClickIcon(changeShapeColorItem);
    field_shape_col->onDoubleClickItem(changeShapeColorItem);
    field_back_col->onClickIcon(changeBackColorItem);
    field_back_col->onDoubleClickItem(changeBackColorItem);

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

        field_shape->setCurrentItem(nullableFieldPropTxt(selected_entry->cell_shape->txt));
        field_shape_col->setCurrentItem(nullableFieldPropTxt(selected_entry->cell_shape_color->txt));
        field_back_col->setCurrentItem(nullableFieldPropTxt(selected_entry->cell_back_color->txt));

        recomposePage(getSelectedProduct(), selected_entry);
    });

    // Detect product list selection change
    connect(field_products, &SearchableList::onChangedSelected, this, [this](SearchableListItem& item)
    {
        selected_entry = product_oiltype_entries[item.txt.toStdString()];

        {
            QSignalBlocker blocker(field_merged_code);
            field_merged_code->setCurrentItem(selected_entry->mergedCode().c_str());
        }

        field_shape->setCurrentItem(nullableFieldPropTxt(selected_entry->cell_shape->txt));
        field_shape_col->setCurrentItem(nullableFieldPropTxt(selected_entry->cell_shape_color->txt));
        field_back_col->setCurrentItem(nullableFieldPropTxt(selected_entry->cell_back_color->txt));

        recomposePage(getSelectedProduct(), selected_entry);
    });

    // Load Project button
    connect(ui->load_btn, &QPushButton::clicked, this, [this]()
    {
        project_file_manager->doLoad("Select Project File", "Project File (*.json);",
            [this](const QString& filename, const QByteArray& data)
        {
            selected_entry = nullptr;

            deserialize(data);
            rebuildDatabaseAndPopulateUI();

            statusBar->showMessage("Project:   " + filename);
        });
    });

    // Save Project button
    connect(ui->save_btn, &QPushButton::clicked, this, [this]()
    {
        QString save_data = serialize();
        project_file_manager->doSave("Select Project File", "label_maker_project.bin", "Project File (*.json);", save_data.toUtf8());
    });

    // Load CSV button
    connect(ui->csv_btn, &QPushButton::clicked, this, [this]()
    {
        selected_entry = nullptr;

        csv_picker->doLoad("Select CSV File", "CSV File (*.csv);", [this](
            const QString& filename,
            const QByteArray& data)
        {
            openCSV(data.toStdString().c_str());
            rebuildDatabaseAndPopulateUI();
        });
    });

    // Single PDF button
    connect(ui->pdf_btn, &QPushButton::clicked, this, [this]()
    {
        recomposePage(getSelectedProduct(), selected_entry, [this](QGraphicsScene* scene, ComposerResultInt result)
        {
            if (result == ComposerResult::SUCCEEDED)
            {
                QByteArray pdf = generatePDF(scene, pagePreview->page_margin);
                pdf_exporter->doSave("Select PDF Path", "label.pdf", "PDF File (*.pdf);", pdf);
            }
            else
            {
                // Handle error
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
                pdf_writer.start(pagePreview->page_margin);
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
            /*QByteArray composite_pdf = *pdfData;

            if (pdfPainter)
            {
                pdfPainter->end();
                delete pdfPainter;
                pdfPainter = nullptr;
            }

            if (pdfWriter)
            {
                delete pdfWriter;
                pdfWriter = nullptr;
            }

            if (pdfBuffer)
            {
                delete pdfBuffer;
                pdfBuffer = nullptr;
            }

            if (pdfData)
            {
                delete pdfData;
                pdfData = nullptr;
            }*/

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
    recomposePage(pair.first.c_str(), pair.second, [this, pair](QGraphicsScene* scene, ComposerResultInt result)
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

                QByteArray pdf = generatePDF(scene, pagePreview->page_margin);
                pdfs.append({ filename, pdf });
            }
        }

        if (result == ComposerResult::SUCCEEDED)
            batch_dialog->addLogMessage(("Composed:  " + pair.first).c_str());
        else
        {
            //batch_dialog->addLogError("");
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
            //batch_dialog->addLogError("");
        }

        ++export_index;
        float progress = (float)export_index / (float)product_oiltype_entries.size();
        batch_dialog->setProgress(progress);

        // Schedule the next file processing without blocking the UI
        QTimer::singleShot(50, this, &PageOptions::processExportPDF);
    });
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

ComposerResultInt PageOptions::recomposePage(QString product_name, OilTypeEntryPtr entry, std::function<void(QGraphicsScene *, ComposerResultInt)> callback)
{
    ComposerResultInt ret = 0;

    if (entry)
    {
        composeInfo.shape = composerGenerator.getShapeInfo(entry->cell_shape->txt.c_str());
        composeInfo.shape_color = composerGenerator.getShapeColor(entry->cell_shape_color->txt.c_str());
        composeInfo.tag_background_color = composerGenerator.getBackColor(entry->cell_back_color->txt.c_str());

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


        if (ret == ComposerResult::SUCCEEDED)
        {
            composeInfo.autoDetermineStroke();
            QGraphicsScene* scene = pagePreview->composeScene(composeInfo);
            if (callback)
                callback(scene, ret);

            return ret;
        }
    }

    if (callback)
        callback(nullptr, ret);

    return ret;
}

void PageOptions::openCSV(const char* text)
{
    csv.open(text);
    rebuildDatabaseAndPopulateUI();
}

void PageOptions::rebuildDatabaseAndPopulateUI()
{
    rebuildDatabase();
    repopulateLists();
}

void PageOptions::rebuildDatabase()
{
    // Clear maps
    merged_code_entries.clear();
    product_oiltype_entries.clear();

    if (csv.opened())
    {
        std::vector<CSVHeaderPtr> vendor_headers;

        // Find headers and assign id's
        {
            CSVRect headers_r(
                CSVRect::BEG, CSVRect::BEG,
                CSVRect::END, 3
            );

            csv.setHeader(csv.findCellFuzzy("CORRECT MATERIAL CODE", headers_r))
                ->setCustomId("material_code");

            csv.setHeader(csv.findCellFuzzy("GENERIC CODE", headers_r))
                ->setCustomId("generic_code");

            // Find all manufacturer headers
            auto vendor_header_cells = csv.findCellsWith("VENDOR", headers_r);
            for (size_t i = 0; i < vendor_header_cells.size(); i++)
                vendor_headers.push_back(csv.setHeader(vendor_header_cells[i])->setCustomId("vendor_" + QString::number(i).toStdString()));

            csv.setHeader(csv.findCellIf([](string_ex& txt) {
                return txt.contains("Shape") && txt.contains("Visc Grade");
            }, headers_r))->setCustomId("shape");

            csv.setHeader(csv.findCellIf([](string_ex& txt) {
                return txt.contains("Shape Colour");
            }, headers_r))->setCustomId("shape_color");

            csv.setHeader(csv.findCellIf([](string_ex& txt) {
                return txt.contains("Shape Colour");
            }, headers_r))->setCustomId("shape_color");

            csv.setHeader(csv.findCellIf([](string_ex& txt) {
                return txt.contains("Background Colour");
            }, headers_r))->setCustomId("back_color");
        }

        csv.readData();

        for (int i = csv.dataRowFirst(); i < csv.dataRowLast(); i++)
        {
            CSVRow& row = csv.getRow(i);

            // Read basic values
            auto entry = std::make_shared<OilTypeEntry>();
            entry->cell_material_code = row.findByHeaderCustomID("material_code");
            entry->cell_generic_code = row.findByHeaderCustomID("generic_code");
            entry->cell_shape = row.findByHeaderCustomID("shape");
            entry->cell_shape_color = row.findByHeaderCustomID("shape_color");
            entry->cell_back_color = row.findByHeaderCustomID("back_color");

            // Skipping entries with missing generic code
            if (entry->cell_generic_code->txt.size() == 0)
                continue;

            // Look at each manufacturer, see if product available for this oil type
            for (auto vendor_header_cell : vendor_headers)
            {
                // Is a product defined?
                CSVCellPtr vendor_cell = row.findByHeader(vendor_header_cell);
                if (vendor_cell->txt.size())
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
}

void PageOptions::repopulateLists()
{
    // Clear lists
    field_merged_code->clear();
    field_products->clear();
    field_shape->clear();
    field_shape_col->clear();
    field_back_col->clear();

    // Add placeholder <none> for missing entry fields
    field_shape->addUniqueListItem("<none>");
    field_shape_col->addUniqueListItem("<none>");
    field_back_col->addUniqueListItem("<none>");

    // Populate lists with database values
    for (const auto& [merged_code, entry] : merged_code_entries)
    {
        // Merged Oil-Type Code
        field_merged_code->addUniqueListItem(merged_code.c_str(), entry);

        // Shape
        QString shape_txt = nullableFieldPropTxt(entry->cell_shape->txt);
        field_shape->addUniqueListItem(shape_txt);

        // Shape Color
        QString shape_col_txt = nullableFieldPropTxt(entry->cell_shape_color->txt);
        field_shape_col->addUniqueListItem(shape_col_txt);

        // Back Color
        QString back_col_txt = nullableFieldPropTxt(entry->cell_back_color->txt);
        field_back_col->addUniqueListItem(back_col_txt);
    }

    // Show all vendors
    if (field_products->getRadioChecked())
    {
        for (const auto& [vendor, entry] : product_oiltype_entries)
            field_products->addListItem(vendor.c_str());
    }

    // Set project-saved relationships, even if CSV doesn't use them or they already exist
    {
        // Shape
        for (const auto& [key, v] : composerGenerator.shapeMap())
            field_shape->addUniqueListItem(key);

        // Shape Color
        for (const auto& [key, v] : composerGenerator.shapeColorMap())
            field_shape_col->addUniqueListItem(key);

        // Back Color
        for (const auto& [key, v] : composerGenerator.backColorMap())
            field_back_col->addUniqueListItem(key);
    }

    //field_merged_code->setSelectable(false);
}

QByteArray PageOptions::serialize()
{ 
    QJsonObject jsonObj;

    QJsonObject composerInfoGeneratorObj;
    composerGenerator.serialize(composerInfoGeneratorObj);
    jsonObj["composer_info_generator"] = composerInfoGeneratorObj;
    jsonObj["version"] = 1;

    QJsonDocument jsonDoc(jsonObj);
    return jsonDoc.toJson();
}

void PageOptions::deserialize(const QByteArray &json)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(json);

    if (jsonDoc.isNull() || !jsonDoc.isObject())
        return;

    QJsonObject jsonObj = jsonDoc.object();
    qDebug() << "Version:" << jsonObj["version"].toInt();

    QJsonObject composerInfoGeneratorObj = jsonObj["composer_info_generator"].toObject();
    composerGenerator.deserialize(composerInfoGeneratorObj);

}

SearchableList* PageOptions::addSearchableListRow1(QString field_id, QString field_name)
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
}
