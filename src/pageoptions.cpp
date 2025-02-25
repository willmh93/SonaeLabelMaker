#include <QPushButton>
#include <QSvgRenderer>

#include <string>
#include <algorithm>

#include "pageoptions.h"
#include "ui_pageoptions.h"
#include "file_uploader.h"

#include "mainwindow.h"


PageOptions::PageOptions(QStatusBar* _statusBar, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageOptions)
{
    ui->setupUi(this);

    statusBar = _statusBar;

    csv_picker = new FileManager(this);
    project_file_manager = new FileManager(this);
    svg_picker = new FileManager(this);

    field_merged_code = addSearchableListRow1("mat_gen_code", "Material / Generic Code");
    field_products = addSearchableListRow1("products", "Available Products");

    field_shape       = addSearchableListRow2("shape", "Shape");
    field_shape_col   = addSearchableListRow2("shape_col", "Shape Color");
    field_back_col    = addSearchableListRow2("back_col", "Background Color");

    //field_merged_code->setRowHeight(24);
    //field_products->setRowHeight(24);

    field_shape->setRowHeight(24);
    field_shape_col->setRowHeight(24);
    field_back_col->setRowHeight(24);
    

    field_shape->setSelectable(false);
    field_shape_col->setSelectable(false);
    field_back_col->setSelectable(false);

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

    auto changeShapeItem = [this](SearchableListItem& item)
    {
        svg_picker->doLoad("Select SVG File", "SVG File (*.svg);", [this, item](
            const QString& filename,
            const QByteArray& data)
        {
            composerGenerator.setShapeInfo(item.txt, ShapeInfo::fromData(data));
            field_shape->refresh();
            recomposePage();
        });
    };

    auto changeShapeColorItem = [this](SearchableListItem& item)
    {
        color_picker = new QColorDialog(this);
        color_picker->setCurrentColor(composerGenerator.getShapeColor(item.txt));
        connect(color_picker, &QColorDialog::colorSelected, this, [this, item](const QColor& color) {
            composerGenerator.setShapeColor(item.txt, color);
            field_shape_col->refresh();
            recomposePage();
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
            recomposePage();
        });
        color_picker->open();
    };

    field_shape->onClickIcon(changeShapeItem);
    field_shape->onDoubleClickItem(changeShapeItem);
    field_shape_col->onClickIcon(changeShapeColorItem);
    field_shape_col->onDoubleClickItem(changeShapeColorItem);
    field_back_col->onClickIcon(changeBackColorItem);
    field_back_col->onDoubleClickItem(changeBackColorItem);

    connect(field_merged_code, &SearchableList::onChangedSelected, this, [this](SearchableListItem& item)
    {
        auto entry = item.as<OilTypeEntryPtr>();
        selected_entry = entry;

        field_products->clear();
        for (CSVCellPtr vendor_cell : entry->vendor_cells)
            field_products->addListItem(QString(vendor_cell->txt.c_str()));

        field_shape->setCurrentItem(nullableFieldPropTxt(entry->cell_shape->txt));
        field_shape_col->setCurrentItem(nullableFieldPropTxt(entry->cell_shape_color->txt));
        field_back_col->setCurrentItem(nullableFieldPropTxt(entry->cell_back_color->txt));

        recomposePage();
    });

    connect(ui->load_btn, &QPushButton::clicked, this, [this]()
    {
        project_file_manager->doLoad("Select Project File", "Project File (*.json);",
            [this](const QString& filename, const QByteArray& data)
        {
            qDebug() << "Project deserialize(data)";
            deserialize(data);

            rebuildDatabase();

            statusBar->showMessage("Project:   " + filename);
        });
    });

    //connect(project_file_manager, &FileManager::fileRecieved, this, [this](
    //    const QString& filename,
    //    const QByteArray& data)
    //{
    //    deserialize(data);
    //    rebuildDatabase();
    //});

    connect(ui->save_btn, &QPushButton::clicked, this, [this]()
    {
        QString save_data = serialize();
        project_file_manager->doSave("Select Project File", "label_maker_project.bin", "Project File (*.json);", save_data.toUtf8());
    });

    connect(ui->csv_btn, &QPushButton::clicked, this, [this]()
    {
        qDebug() << "<Open CSV btn>";
        csv_picker->doLoad("Select CSV File", "CSV File (*.csv);", [this](
            const QString& filename,
            const QByteArray& data)
        {
            openCSV(data.toStdString().c_str());
            refreshData();
        });
    });

    /*connect(csv_picker, &FileManager::fileRecieved, this, [this](
        const QString& filename,
        const QByteArray& data)
    {
        openCSV(data.toStdString().c_str());
        refreshData();
    });*/

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

    composerGenerator.setShapeColor("Gray-Mineral", QColor("#5f5f5f"));
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
    composerGenerator.setShapeColor("Orange-Mineral", QColor("#ff7f00"));
    composerGenerator.setShapeColor("Yellow-Synthetic(PG) - EP", QColor("#ffff00"));
    composerGenerator.setShapeColor("Gray-Synthetic(PAO) - AW", QColor("#ffff00"));
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

    composerGenerator.setBackColor("Gray-Thermal Oils", QColor("#5f5f5f"));
    composerGenerator.setBackColor("Yellow-Other Oils", QColor("#e2e5c5"));
    composerGenerator.setBackColor("Red-Transmission Oils", QColor("#ff0000"));
    composerGenerator.setBackColor("Black-Engine Oils", QColor("#000000"));
    composerGenerator.setBackColor("Green-Hydraulic Oils", QColor("#00ff00"));
    composerGenerator.setBackColor("Orange-Gear Oils", QColor("#ff7f00"));
    composerGenerator.setBackColor("Blue-Compressor Oils", QColor("#0000ff"));
    composerGenerator.setBackColor("Red-PolyUrea", "#ff0000");
    composerGenerator.setBackColor("Black-PTFE/PFPE", "#000000");
    composerGenerator.setBackColor("Green-Lithium", "#00ff00");
    composerGenerator.setBackColor("Light Green-Lithium Complex", "#4fff4f");
    composerGenerator.setBackColor("Brown-Clay Bentonite", "#7f1f1f");
    composerGenerator.setBackColor("White-Barium Complex", "#ffffff");
    composerGenerator.setBackColor("Gray-Aluminium Complex", "#7f7f7f");
    composerGenerator.setBackColor("Purple-Polymer", "#ff00ff");*/
    

    //engine = new QQmlApplicationEngine(this);

    rebuildDatabase();
}

PageOptions::~PageOptions()
{
    delete ui;
}

void PageOptions::recomposePage()
{
    if (selected_entry)
    {
        composeInfo.shape = composerGenerator.getShapeInfo(selected_entry->cell_shape->txt.c_str());
        composeInfo.shape_color = composerGenerator.getShapeColor(selected_entry->cell_shape_color->txt.c_str());
        composeInfo.tag_background_color = composerGenerator.getBackColor(selected_entry->cell_back_color->txt.c_str());

        pagePreview->composeScene(composeInfo);
    }
}

/*void PageOptions::openDialog(const QString& qmlFile)
{
    QQmlComponent component(engine, QUrl(qmlFile));

    if (component.status() == QQmlComponent::Ready) {
        QObject* dialogObject = component.create();


        qDebug() << dialogObject->objectName();
        qDebug() << dialogObject->children();
        qDebug() << dialogObject->children()[0];
        qDebug() << dialogObject->children()[0]->children();
        qDebug() << dialogObject->children()[0]->children()[1];
        qDebug() << dialogObject->children()[0]->children()[1]->objectName();
        qDebug() << dialogObject->children()[0]->children()[1]->children();
        qDebug() << dialogObject->children()[0]->children()[1]->children()[0]->objectName();
        qDebug() << dialogObject->children()[0]->children()[1]->children()[0]->children();

        //if (dialogObject->objectName() != "colorPickerDialog")
        //    dialogObject = dialogObject->findChild<QObject*>("colorPickerDialog");

        dialogObject = dialogObject->children()[0]->children()[1]->children()[0];
       

        if (dialogObject) {
            qDebug() << "Opening Dialog from" << qmlFile;

            // Open the dialog
            QMetaObject::invokeMethod(dialogObject, "openDialog");

            // Ensure it gets deleted after closing
            connect(dialogObject, &QObject::destroyed, [qmlFile]() {
                qDebug() << "Dialog from" << qmlFile << "closed and deleted.";
            });
        }
    }
    else {
        qDebug() << "Failed to load QML Dialog: " << component.errorString();
    }
}*/

void PageOptions::openCSV(const char* text)
{
    csv.open(text);

    rebuildDatabase();
}

void PageOptions::refreshData()
{
    /*if (csv.loaded())
    {
        for (int i = csv.dataRowFirst(); i < csv.dataRowLast(); i++)
        {
            CSVRow& row = csv.getRow(i);
            string material_code = row.findByHeaderCustomID("material_code")->txt;
            string generic_code = row.findByHeaderCustomID("generic_code")->txt;
            QString merged_code_txt = (material_code + "    " + generic_code).c_str();

            field_merged_code->attachItemData(merged_code_txt, entry);

            //field_shape->findItem(row.

            // Attach data to list items
            //auto entry = std::make_shared<OilTypeEntry>();
            //QString merged_code_txt = (entry->cell_material_code->txt + "    " + entry->cell_generic_code->txt).c_str();
            //field_merged_code->attachItemData(merged_code_txt, entry);
        }
    }*/

    rebuildDatabase();
    field_shape->refresh();
}

void PageOptions::rebuildDatabase()
{
    qDebug() << "rebuildDatabase() start";

    field_shape->clear();
    field_shape_col->clear();
    field_back_col->clear();

    field_shape->addUniqueListItem("<none>");
    field_shape_col->addUniqueListItem("<none>");
    field_back_col->addUniqueListItem("<none>");

    if (csv.opened())
    {
        CSVRect headers_r(
            CSVRect::BEG, CSVRect::BEG,
            CSVRect::END, 3
        );

        csv.setHeader(csv.findCell("Material.Code", headers_r))
            ->setCustomId("material_code");

        csv.setHeader(csv.findCell("GENERIC CODE", headers_r))
            ->setCustomId("generic_code");

        auto vendor_header_cells = csv.findCellsWith("VENDOR", headers_r);
        std::vector<CSVHeaderPtr> vendor_headers;

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

            if (entry->cell_generic_code->txt.size() == 0)
                continue;

            // Find available vendors
            for (auto vendor_header_cell : vendor_headers)
            {
                CSVCellPtr vendor_cell = row.findByHeader(vendor_header_cell);
                if (vendor_cell->txt.size())
                    entry->vendor_cells.push_back(vendor_cell);
            }


            // Merged Oil-Type Code
            QString merged_code_txt = (entry->cell_material_code->txt + "    " + entry->cell_generic_code->txt).c_str();
            field_merged_code->addUniqueListItem(merged_code_txt, entry);

            // Attach entry to primary list item even if list item already exists (from loading 
            //field_merged_code->attachItemData(merged_code_txt, entry);

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
    }

    // Add project relationships even if CSV doesn't use them

    // Shape
    for (const auto& [key, value] : composerGenerator.shapeMap())
        field_shape->addUniqueListItem(key);

    // Shape Color
    for (const auto& [key, value] : composerGenerator.shapeColorMap())
        field_shape_col->addUniqueListItem(key);

    // Back Color
    for (const auto& [key, value] : composerGenerator.backColorMap())
        field_back_col->addUniqueListItem(key);

    qDebug() << "rebuildDatabase() end";
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
    
    //field_widgets.push_back(field_widget);
    ui->field_list->addWidget(field_widget);
    return field_widget;
}

SearchableList* PageOptions::addSearchableListRow2(QString field_id, QString field_name)
{
    SearchableList* field_widget = new SearchableList(field_id.toStdString(), field_name);

    //field_widgets.push_back(field_widget);
    ui->product_lists->addWidget(field_widget);
    return field_widget;
}

/*void PageOptions::populateOilTypeFields()
{
    field_shape->addListItem("<none>", true);
    field_shape_col->addListItem("<none>", true);
    field_back_col->addListItem("<none>", true);

    for (int i=csv.dataRowFirst(); i<csv.dataRowLast(); i++)
    {
        CSVRow& row = csv.getRow(i);

        CSVCellPtr material_code_cell = row.findByHeaderCustomID("material_code");
        CSVCellPtr generic_code_cell = row.findByHeaderCustomID("generic_code");

        if (generic_code_cell->txt.size() == 0)
            continue;

        // Merged Oil-Type Code
        std::string merged_code_txt = material_code_cell->txt + "    " + generic_code_cell->txt;

        field_merged_code->addListItem<CSVCellPtr>(QString(merged_code_txt.c_str()), material_code_cell, true);

        // Shape
        std::string shape_txt = row.findByHeaderCustomID("shape")->txt;
        if (shape_txt.size() == 0)
            shape_txt = "<none>";
        field_shape->addListItem<CSVCellPtr>(QString(shape_txt.c_str()), true);

        // Shape Color
        std::string shape_col_txt = row.findByHeaderCustomID("shape_color")->txt;
        if (shape_col_txt.size() == 0)
            shape_col_txt = "<none>";
        field_shape_col->addListItem<CSVCellPtr>(QString(shape_col_txt.c_str()), true);

        // Back Color
        std::string back_col_txt = row.findByHeaderCustomID("back_color")->txt;
        if (back_col_txt.size() == 0)
            back_col_txt = "<none>";
        field_back_col->addListItem<CSVCellPtr>(QString(back_col_txt.c_str()), true);
    }
}*/

