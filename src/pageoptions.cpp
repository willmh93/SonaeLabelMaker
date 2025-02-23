#include <QToolButton>
#include <string>
#include <algorithm>

#include "pageoptions.h"
#include "ui_pageoptions.h"
#include "file_uploader.h"

#include "mainwindow.h"


PageOptions::PageOptions(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageOptions)
{
    ui->setupUi(this);
    csv_picker = new FileUploader();

    composerGenerator.addShapeMapping("Square ISO-VG32", ShapeInfo::fromPath(":/shapes/SQUARE.svg"));
    composerGenerator.addShapeMapping("Hexagon ISO-VG 68", ShapeInfo::fromPath(":/shapes/HEXAGON.svg"));
    composerGenerator.addShapeMapping("Pentagon ISO-VG 46", ShapeInfo::fromPath(":/shapes/PENTAGON.svg"));
    composerGenerator.addShapeMapping("Oval ISO-VG 460", ShapeInfo::fromPath(":/shapes/ELIPSE.svg"));
    composerGenerator.addShapeMapping("Parallelogram ISO-VG 320", ShapeInfo::fromPath(":/shapes/PARALLELOGRAM.svg"));
    composerGenerator.addShapeMapping("Tear Drop ISO-VG 220", ShapeInfo::fromPath(":/shapes/TEARDROP.svg"));
    composerGenerator.addShapeMapping("Triangle ISO-VG 22", ShapeInfo::fromPath(":/shapes/TRIANGLE.svg"));
    composerGenerator.addShapeMapping("Square ISO-VG 32", ShapeInfo::fromPath(":/shapes/SQUARE.svg"));
    composerGenerator.addShapeMapping("Diamond SAE-VG 90", ShapeInfo::fromPath(":/shapes/DIAMOND.svg"));
    composerGenerator.addShapeMapping("Hexagon SAE-VG 40", ShapeInfo::fromPath(":/shapes/HEXAGON.svg"));
    composerGenerator.addShapeMapping("Square SAE-VG 20W-20", ShapeInfo::fromPath(":/shapes/SQUARE.svg"));
    composerGenerator.addShapeMapping("Egg-Timer ISO-VG 1000", ShapeInfo::fromPath(":/shapes/HOURGLASS_V.svg"));
    composerGenerator.addShapeMapping("Octagon ISO-VG 100", ShapeInfo::fromPath(":/shapes/OCTAGON.svg"));
    composerGenerator.addShapeMapping("Grease Gun_Grease", ShapeInfo::fromPath(":/shapes/GREASE_GUN.svg"));

    composerGenerator.addShapeColorMapping("Gray-Mineral", QColor("#5f5f5f"));
    composerGenerator.addShapeColorMapping("Black-Mineral - EP - WD", QColor("#000000"));
    composerGenerator.addShapeColorMapping("Blue-Mineral (GrpII) - H2 Food Safe", QColor("#0000ff"));
    composerGenerator.addShapeColorMapping("White-Synthetic - POE", QColor("#ffffff"));
    composerGenerator.addShapeColorMapping("Green-Mineral - AW", QColor("#00ff00"));
    composerGenerator.addShapeColorMapping("Yellow-Mineral", QColor("#ffff00"));
    composerGenerator.addShapeColorMapping("Orange-Synthetic - POE", QColor("#ff7f00"));
    composerGenerator.addShapeColorMapping("Red-Mineral - WS - ASHLESS", QColor("#ff0000"));
    composerGenerator.addShapeColorMapping("Red-Mineral", QColor("#ff0000"));
    composerGenerator.addShapeColorMapping("Purple-Synthetic", QColor("#ff00ff"));
    composerGenerator.addShapeColorMapping("Black-Mineral", QColor("#000000"));
    composerGenerator.addShapeColorMapping("Green-Synthetic (PAO)", QColor("#00ff00"));
    composerGenerator.addShapeColorMapping("Red-Synthetic - ASHLESS", QColor("#ff0000"));
    composerGenerator.addShapeColorMapping("Orange-Mineral", QColor("#ff7f00"));
    composerGenerator.addShapeColorMapping("Yellow-Synthetic(PG) - EP", QColor("#ffff00"));
    composerGenerator.addShapeColorMapping("Gray-Synthetic(PAO) - AW", QColor("#ffff00"));
    composerGenerator.addShapeColorMapping("Purple-Synthetic(PAG)", QColor("#ff00ff"));
    composerGenerator.addShapeColorMapping("Black-Synthetic(PAO) - EP", QColor("#000000"));
    composerGenerator.addShapeColorMapping("Black-Synthetic (PAO)", QColor("#000000"));
    composerGenerator.addShapeColorMapping("White-Mineral - AW - ASHLESS", QColor("#ffffff"));
    composerGenerator.addShapeColorMapping("Yellow", QColor("#ffff00"));
    composerGenerator.addShapeColorMapping("Dark Green", QColor("#007f00"));
    composerGenerator.addShapeColorMapping("Purple", QColor("#ff00ff"));
    composerGenerator.addShapeColorMapping("Blue", QColor("#0000ff"));
    composerGenerator.addShapeColorMapping("Red", QColor("#ff0000"));
    composerGenerator.addShapeColorMapping("White", QColor("#ffffff"));
    composerGenerator.addShapeColorMapping("Brown", QColor("#6f1010"));
    composerGenerator.addShapeColorMapping("Black", QColor("#000000"));
    composerGenerator.addShapeColorMapping("Gray", QColor("#5f5f5f"));
    
    composerGenerator.addBackColor("Gray-Thermal Oils", QColor("#5f5f5f"));
    composerGenerator.addBackColor("Yellow-Other Oils", QColor("#e2e5c5"));
    composerGenerator.addBackColor("Red-Transmission Oils", QColor("#ff0000"));
    composerGenerator.addBackColor("Black-Engine Oils", QColor("#000000"));
    composerGenerator.addBackColor("Green-Hydraulic Oils", QColor("#00ff00"));
    composerGenerator.addBackColor("Orange-Gear Oils", QColor("#ff7f00"));
    composerGenerator.addBackColor("Blue-Compressor Oils", QColor("#0000ff"));
    composerGenerator.addBackColor("Red-PolyUrea", "#ff0000");
    composerGenerator.addBackColor("Black-PTFE/PFPE", "#000000");
    composerGenerator.addBackColor("Green-Lithium", "#00ff00");
    composerGenerator.addBackColor("Light Green-Lithium Complex", "#4fff4f");
    composerGenerator.addBackColor("Brown-Clay Bentonite", "#7f1f1f");
    composerGenerator.addBackColor("White-Barium Complex", "#ffffff");
    composerGenerator.addBackColor("Gray-Aluminium Complex", "#7f7f7f");
    composerGenerator.addBackColor("Puple-Polymer", "#ff00ff");

    serialize();

    field_merged_code = addOilTypeField("mat_gen_code", "Material / Generic Code");
    field_shape       = addOilTypeField("shape", "Shape");
    field_shape_col   = addOilTypeField("shape_col", "Shape Color");
    field_back_col    = addOilTypeField("back_col", "Background Color");

    field_products    = addProductField("products", "Available Products");

    /*connect(field_shape, &SearchableList::onChangedSelected, this, [this](const QString& value)
    {
        composeInfo.shape = value;
    });

    connect(field_shape_col, &SearchableList::onChangedSelected, this, [this](const QString& value)
    {
        composeInfo.shape_color = value;
    });

    connect(field_back_col, &SearchableList::onChangedSelected, this, [this](const QString& value)
    {
        composeInfo.tag_background_color = value;
    });*/


    connect(field_merged_code, &SearchableList::onChangedSelected, this, [this](SearchableListItem& item)
    {
        auto entry = item.as<OilTypeEntryPtr>();

        field_products->clear();
        for (CSVCellPtr vendor_cell : entry->vendor_cells)
            field_products->addListItem(QString(vendor_cell->txt.c_str()));

        field_shape->setCurrentItem(nullableFieldPropTxt(entry->cell_shape->txt));
        field_shape_col->setCurrentItem(nullableFieldPropTxt(entry->cell_shape_color->txt));
        field_back_col->setCurrentItem(nullableFieldPropTxt(entry->cell_back_color->txt));

        composeInfo.shape = composerGenerator.toShapeInfo(entry->cell_shape->txt.c_str());
        composeInfo.shape_color = composerGenerator.toShapeColor(entry->cell_shape_color->txt.c_str());
        composeInfo.tag_background_color = composerGenerator.toBackColor(entry->cell_back_color->txt.c_str());

        pagePreview->composeScene(composeInfo);
    });

    connect(ui->csv_btn, &QToolButton::clicked, this, [this]()
    {
        //openColorPicker();
        csv_picker->openDialog();
    });

    connect(csv_picker, &FileUploader::fileRecieved, this, [this](
        const QString& filename,
        const QByteArray& data)
    {
        openCSV(data.toStdString().c_str());
    });

    engine.load(QUrl(QStringLiteral("qrc:/src/colorpicker.qml")));
    if (engine.rootObjects().isEmpty())
        return;

    // Get reference to the QML dialog object
    dialogObject = engine.rootObjects().first();
}

PageOptions::~PageOptions()
{
    delete ui;
    delete csv_picker;
}

void PageOptions::openCSV(const char* text)
{
    csv.open(text);

    buildDatabase();

    

    //populateOilTypeFields();
}

void PageOptions::buildDatabase()
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

    field_shape->addUniqueListItem("<none>");
    field_shape_col->addUniqueListItem("<none>");
    field_back_col->addUniqueListItem("<none>");

    for (int i = csv.dataRowFirst(); i < csv.dataRowLast(); i++)
    {
        CSVRow& row = csv.getRow(i);

        // Read basic values
        auto entry = std::make_shared<OilTypeEntry>();
        entry->cell_material_code = row.findByHeaderCustomID("material_code");
        entry->cell_generic_code  = row.findByHeaderCustomID("generic_code");
        entry->cell_shape         = row.findByHeaderCustomID("shape");
        entry->cell_shape_color   = row.findByHeaderCustomID("shape_color");
        entry->cell_back_color    = row.findByHeaderCustomID("back_color");

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

        // Shape
        QString shape_txt = nullableFieldPropTxt(entry->cell_shape->txt);
        field_shape->addUniqueListItem(shape_txt, entry);

        // Shape Color
        QString shape_col_txt = nullableFieldPropTxt(entry->cell_shape_color->txt);
        field_shape_col->addUniqueListItem(shape_col_txt, entry);

        // Back Color
        QString back_col_txt = nullableFieldPropTxt(entry->cell_back_color->txt);
        field_back_col->addUniqueListItem(back_col_txt, entry);
    }
}

void PageOptions::serialize()
{ 
    QJsonObject jsonObj;
    jsonObj["version"] = 1;

    QJsonObject composerInfoGeneratorObj;
    composerGenerator.serialize(composerInfoGeneratorObj);
    jsonObj["composer_info_generator"] = composerInfoGeneratorObj;
}

void PageOptions::deserialize(const QString &json)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(json.toUtf8());

    if (!jsonDoc.isNull() && jsonDoc.isObject()) {
        QJsonObject jsonObj = jsonDoc.object();
        qDebug() << "Name:" << jsonObj["name"].toString();
        qDebug() << "Version:" << jsonObj["version"].toInt();

        QJsonArray features = jsonObj["features"].toArray();
        for (const QJsonValue& feature : features) {
            qDebug() << "Feature:" << feature.toString();
        }
    }
}

SearchableList* PageOptions::addOilTypeField(QString field_id, QString field_name)
{
    SearchableList* field_widget = new SearchableList(field_id.toStdString(), field_name);
    
    field_widgets.push_back(field_widget);
    ui->field_list->addWidget(field_widget);
    return field_widget;
}

SearchableList* PageOptions::addProductField(QString field_id, QString field_name)
{
    SearchableList* field_widget = new SearchableList(field_id.toStdString(), field_name);

    field_widgets.push_back(field_widget);
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

