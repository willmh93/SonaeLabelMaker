#ifndef PAGEOPTIONS_H
#define PAGEOPTIONS_H

#include <QWidget>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "searchablelist.h"
#include "pagepreview.h"
#include "csv_reader.h"

class FileUploader;

namespace Ui {
class PageOptions;
}



class ComposerInfoGenerator
{
    // Description to Usable Shape/Color Map
    std::unordered_map<QString, ShapeInfo> shape_map;
    std::unordered_map<QString, QColor> shape_color_map;
    std::unordered_map<QString, QColor> back_color_map;

public:

    void addShapeMapping(QString desc, ShapeInfo shape_info)
    {
        shape_map[desc] = shape_info;
    }

    void addShapeColorMapping(QString desc, QColor color)
    {
        shape_color_map[desc] = color;
    }

    void addBackColor(QString desc, QColor color)
    {
        back_color_map[desc] = color;
    }

    ShapeInfo toShapeInfo(QString desc)
    {
        return shape_map[desc];
    }

    QColor toShapeColor(QString desc)
    {
        return shape_color_map[desc];
    }

    QColor toBackColor(QString desc)
    {
        return back_color_map[desc];
    }

    void serialize(QJsonObject &info) {}
    void deserialize() {}
};

struct OilTypeEntry
{
    CSVCellPtr cell_material_code;
    CSVCellPtr cell_generic_code;
    CSVCellPtr cell_shape;
    CSVCellPtr cell_shape_color;
    CSVCellPtr cell_back_color;

    std::vector<CSVCellPtr> vendor_cells;
};
typedef std::shared_ptr<OilTypeEntry> OilTypeEntryPtr;

class PageOptions : public QWidget
{
    Q_OBJECT;

    //CSVTable data;
    std::vector<SearchableList*> field_widgets;

    SearchableList* field_merged_code;
    SearchableList* field_shape;
    SearchableList* field_shape_col;
    SearchableList* field_back_col;
    
    SearchableList* field_products;

    PagePreview* pagePreview;

    ComposerInfoGenerator composerGenerator;
    ComposerInfo composeInfo;

public:
    explicit PageOptions(QWidget *parent = nullptr);
    ~PageOptions();

    void setPagePreview(PagePreview* _pagePreview)
    {
        pagePreview = _pagePreview;
    }

    CSVReader csv;
    FileUploader *csv_picker;


    QString nullableFieldPropTxt(std::string txt)
    {
        if (txt.size() == 0)
            return "<none>";
        return txt.c_str();
    };

    SearchableList* addOilTypeField(QString field_id, QString field_name);
    SearchableList* addProductField(QString field_id, QString field_name);
    //void populateOilTypeFields();

    void openCSV(const char *text);
    void buildDatabase();

    void serialize();
    void deserialize(const QString& json);

    QObject* dialogObject = nullptr;

public slots:
    void openColorPicker() {
        if (dialogObject) {
            QMetaObject::invokeMethod(dialogObject, "openDialog");
        }
        else {
            qDebug() << "Failed to find QML dialog object.";
        }
    }

    void onColorSelected(const QColor &color) {
        qDebug() << "Color chosen:" << color;
        // Apply the color to UI elements (e.g., change a label background)
    }

private:
    QQmlApplicationEngine engine;

private:
    Ui::PageOptions *ui;
};

#endif // PAGEOPTIONS_H
