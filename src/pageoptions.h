#ifndef PAGEOPTIONS_H
#define PAGEOPTIONS_H

#include <QWidget>
#include <QStatusBar>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QColorDialog>

#include "searchablelist.h"
#include "pagepreview.h"
#include "csv_reader.h"

class FileManager;

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

    void makeShapeIcon(QString desc)
    {
        shape_map[desc].makeIcon();
    }

    const std::unordered_map<QString, ShapeInfo> shapeMap() const
    {
        return shape_map;
    }

    const std::unordered_map<QString, QColor> shapeColorMap() const
    {
        return shape_color_map;
    }

    const std::unordered_map<QString, QColor> backColorMap() const
    {
        return back_color_map;
    }

    void setShapeInfo(QString desc, ShapeInfo shape_info)
    {
        shape_map[desc] = shape_info;
        makeShapeIcon(desc);
    }

    void setShapeColor(QString desc, QColor color)
    {
        shape_color_map[desc] = color;
    }

    void setBackColor(QString desc, QColor color)
    {
        back_color_map[desc] = color;
    }

    bool containsShapeInfo(QString desc)
    {
        return shape_map.count(desc) > 0;
    }

    bool containsShapeColor(QString desc)
    {
        return shape_color_map.count(desc) > 0;
    }

    bool containsBackColor(QString desc)
    {
        return back_color_map.count(desc) > 0;
    }

    ShapeInfo getShapeInfo(QString desc)
    {
        if (shape_map.count(desc))
            return shape_map[desc];
        else
            return ShapeInfo();
    }

    QColor getShapeColor(QString desc)
    {
        if (shape_color_map.count(desc))
            return shape_color_map[desc];
        else
            return QColor(0, 0, 0, 0);
    }

    QColor getBackColor(QString desc)
    {
        if (back_color_map.count(desc))
            return back_color_map[desc];
        else
            return QColor(0, 0, 0, 0);
    }

    void serialize(QJsonObject &info) const
    {
        QJsonObject map_shape_obj, map_shape_color_obj, map_back_color_obj;

        for (const auto& [key, value] : shape_map)
        {
            QJsonObject shape_info_obj;
            value.serialize(shape_info_obj);
            map_shape_obj[key] = shape_info_obj;
        }

        for (const auto& [key, value] : shape_color_map)
            map_shape_color_obj[key] = value.name(QColor::HexArgb);

        for (const auto& [key, value] : back_color_map)
            map_back_color_obj[key] = value.name(QColor::HexArgb);


        info["back_color"] = map_back_color_obj;
        info["shape_color"] = map_shape_color_obj;
        info["shape"] = map_shape_obj;
    }

    void deserialize(const QJsonObject& info)
    {
        QJsonObject map_shape_obj = info["shape"].toObject();
        QJsonObject map_shape_color_obj = info["shape_color"].toObject();
        QJsonObject map_back_color_obj = info["back_color"].toObject();

        qDebug() << map_shape_obj;

        for (auto it = map_shape_obj.begin(); it != map_shape_obj.end(); ++it)
        {
            ShapeInfo shape_info;
            shape_info.deserialize(it->toObject());
            setShapeInfo(it.key(), shape_info);
        }
        
        for (auto it = map_shape_color_obj.begin(); it != map_shape_color_obj.end(); ++it)
            setShapeColor(it.key(), QColor(it->toString()));

        for (auto it = map_back_color_obj.begin(); it != map_back_color_obj.end(); ++it)
            setBackColor(it.key(), QColor(it->toString()));
    }
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
    //std::vector<SearchableList*> field_widgets;

    SearchableList* field_merged_code;
    SearchableList* field_shape;
    SearchableList* field_shape_col;
    SearchableList* field_back_col;
    
    SearchableList* field_products;

    PagePreview* pagePreview;
    QStatusBar* statusBar;

    OilTypeEntryPtr selected_entry = nullptr;

    ComposerInfoGenerator composerGenerator;
    ComposerInfo composeInfo;

public:
    explicit PageOptions(QStatusBar* statusBar, QWidget *parent = nullptr);
    ~PageOptions();

    void setPagePreview(PagePreview* _pagePreview)
    {
        pagePreview = _pagePreview;
    }

    CSVReader csv;
    FileManager* csv_picker;
    FileManager* svg_picker;
    FileManager* project_file_manager;

    QString nullableFieldPropTxt(std::string txt)
    {
        if (txt.size() == 0)
            return "<none>";
        return txt.c_str();
    };

    SearchableList* addSearchableListRow1(QString field_id, QString field_name);
    SearchableList* addSearchableListRow2(QString field_id, QString field_name);
    //void populateOilTypeFields();

    void openCSV(const char *text);
    void refreshData();
    void rebuildDatabase();

    QByteArray serialize();
    void deserialize(const QByteArray& json);

    QColorDialog *color_picker;
    //QObject* dialogObject = nullptr;

public slots:

    /*void openDialog(const QString& qmlFile);
    void onColorSelected(const QColor &color) {
        qDebug() << "Color chosen:" << color;
        // Apply the color to UI elements (e.g., change a label background)
    }*/

private:
    //QQmlApplicationEngine *engine;
    void recomposePage();

private:
    Ui::PageOptions *ui;
};

#endif // PAGEOPTIONS_H
