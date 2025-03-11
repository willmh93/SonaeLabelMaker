#ifndef PAGEOPTIONS_H
#define PAGEOPTIONS_H

#include <QWidget>
#include <QStatusBar>
#include <QPdfWriter>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QColorDialog>

#include <QStandardItemModel>

#include "pdfbatchexport.h"
#include "searchablelist.h"
#include "pagepreview.h"
#include "csv_reader.h"

class FileManager;

namespace Ui {
class PageOptions;
}

class PdfSceneWriter
{
public:
    PdfSceneWriter();
    ~PdfSceneWriter();

    void start(float src_margin);
    void addPage(QGraphicsScene* scene);
    QByteArray finalize();

private:

    float src_margin;
    QRectF page_rect = QRectF(0, 0, 2480, 3508);

    QByteArray pdfData;
    QBuffer pdfBuffer;
    QPdfWriter* pdfWriter = nullptr;
    QPainter* pdfPainter = nullptr;
    bool firstPage = true;
};

class ComposerInfoGenerator
{
    // Description to Usable Shape/Color Map
    std::unordered_map<QString, ShapeInfo> shape_map;
    std::unordered_map<QString, QColor> shape_color_map;
    std::unordered_map<QString, QColor> back_color_map;
    std::unordered_map<QString, QColor> inner_back_color_map;

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

    const std::unordered_map<QString, QColor> innerBackColorMap() const
    {
        return inner_back_color_map;
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

    void setInnerBackColor(QString desc, QColor color)
    {
        inner_back_color_map[desc] = color;
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

    QColor getInnerBackColor(QString desc)
    {
        if (inner_back_color_map.count(desc))
            return inner_back_color_map[desc];
        else
            return QColor(0, 0, 0, 0);
    }

    void serialize(QJsonObject &info) const
    {
        QJsonObject map_shape_obj, map_shape_color_obj, map_back_color_obj, map_inner_back_color_obj;

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

        for (const auto& [key, value] : inner_back_color_map)
            map_inner_back_color_obj[key] = value.name(QColor::HexArgb);


        info["shape"] = map_shape_obj;
        info["shape_color"] = map_shape_color_obj;
        info["back_color"] = map_back_color_obj;
        info["inner_back_color"] = map_inner_back_color_obj;
    }

    void deserialize(const QJsonObject& info)
    {
        QJsonObject map_shape_obj = info["shape"].toObject();
        QJsonObject map_shape_color_obj = info["shape_color"].toObject();
        QJsonObject map_back_color_obj = info["back_color"].toObject();
        QJsonObject map_inner_back_color_obj = info["inner_back_color"].toObject();

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

        for (auto it = map_inner_back_color_obj.begin(); it != map_inner_back_color_obj.end(); ++it)
            setInnerBackColor(it.key(), QColor(it->toString()));
    }
};

enum ComposerResult
{
    SUCCEEDED=0ull,
    MISSING_MATERIAL_CODE=1ull,
    MISSING_GENERIC_CODE=2ull,
    MISSING_PRODUCT_NAME=4ull,
    MISSING_SHAPE=8ull,
    MISSING_SHAPE_COLOR=16ull,
    MISSING_BACK_COLOR=32ull
};
typedef unsigned long long ComposerResultInt;

struct OilTypeEntry
{
    CSVCellPtr cell_material_code;
    CSVCellPtr cell_generic_code;
    CSVCellPtr cell_shape;
    CSVCellPtr cell_shape_color;
    CSVCellPtr cell_back_color;
    CSVCellPtr cell_inner_back_color;

    std::vector<CSVCellPtr> vendor_cells;

    //bool detected_userflag_vendor_notdefined = false;

    bool missingData()
    {
        if (cell_material_code->txt.size() == 0) return true;
        if (cell_generic_code->txt.size() == 0) return true;
        if (cell_shape->txt.size() == 0) return true;
        if (cell_shape_color->txt.size() == 0) return true;
        if (cell_back_color->txt.size() == 0) return true;
        //if (cell_inner_back_color->txt.size() == 0) return true;
        return false;

    }

    std::string mergedCode()
    {
        return cell_material_code->txt + "    " + cell_generic_code->txt;
    }
};

typedef std::shared_ptr<OilTypeEntry> OilTypeEntryPtr;

template<typename K, typename V>
class flat_map : public std::vector<std::pair<K, V>>
{
public:

    V &get(const K& k)
    {
        auto it = std::find_if(this->begin(), this->end(), [k](const auto &entry) {
            return entry.first == k;
        });

        if (it == this->end())
            return this->emplace_back(std::make_pair(k, V())).second;
        
        return it->second;
    }

    void set(const K& k, const V& v)
    {
        auto it = std::find_if(this->begin(), this->end(), [k](const auto& entry) {
            return entry.first == k;
        });

        if (it == this->end())
            this->emplace_back(std::make_pair(k, v));
        else
            it->second = v;
    }

    V& operator[](const K& k)
    {
        return get(k);
    }

    template<typename Comparator>
    void sort(Comparator comp)
    {
        std::sort(this->begin(), this->end(), comp);
    }
};

struct TokenDescriptionMap
{
    QString name;
    flat_map<string_ex, string_ex> lookup;
    QStandardItemModel* model = nullptr;

    TokenDescriptionMap()
    {
        model = new QStandardItemModel();
    }

    void set(QString token, QString decription)
    {
        lookup.set(token.toStdString(), decription.toStdString());
    }

    void sortByDescendingLength()
    {
        lookup.sort([](const auto &pair_a, const auto& pair_b)
        {
            return pair_a.first.size() < pair_b.first.size();
        });
    }

    void populateModel()
    {
        if (model)
        {
            for (size_t i = 0; i < lookup.size(); i++)
            {
                QList<QStandardItem*> rowItems;

                rowItems.push_back(new QStandardItem(lookup.at(i).first.c_str()));
                rowItems.push_back(new QStandardItem(lookup.at(i).second.c_str()));
                model->insertRow(0, rowItems);
            }
        }
    }
};

class PageOptions : public QWidget
{
    Q_OBJECT;

    QByteArray warning_icon_data;

    std::shared_ptr<std::vector<SearchableList*>> radio_lists;

    SearchableList* field_merged_code;
    SearchableList* field_shape;
    SearchableList* field_shape_col;
    SearchableList* field_back_col;
    SearchableList* field_inner_back_col;
    SearchableList* field_products;

    PagePreview* pagePreview = nullptr;
    QStatusBar* statusBar;

    flat_map<std::string, OilTypeEntryPtr> merged_code_entries;
    flat_map<std::string, OilTypeEntryPtr> product_oiltype_entries;

    OilTypeEntryPtr selected_entry = nullptr;

    ComposerInfoGenerator composerGenerator;
    ComposerInfo composeInfo;

    CSVReader csv;
    QString project_filename;
    QString csv_filename;

    FileManager* csv_picker = nullptr;
    FileManager* svg_picker = nullptr;
    FileManager* project_file_manager = nullptr;
    FileManager* pdf_exporter = nullptr;

    PDFBatchExport* batch_dialog = nullptr;
    QVector<QPair<QString, QByteArray>> pdfs;
    int export_index = 0;
    bool batch_export_busy = false;

    PdfSceneWriter pdf_writer;

    QStandardItemModel csv_model;
    QStandardItemModel description_model;

    std::vector<TokenDescriptionMap> description_maps;

    void setTokenIndexName(int col, QString name)
    {
        if (col >= description_maps.size())
        {
            description_maps.resize(col + 1);
        }
        description_maps[col].name = name;
    }

    void setTokenDescription(int col, QString token, QString desc)
    {
        if (col >= description_maps.size())
        {
            description_maps.resize(col + 1);
        }
        description_maps[col].set(token, desc);
    }

    void populateTokenDescriptionModels()
    {
        // First, sort tokens in descending order of length, so greedy algorithm
        // prioritizes matches with the longest length
        for (size_t i = 0; i < description_maps.size(); i++)
            description_maps[i].sortByDescendingLength();

        for (size_t i = 0; i < description_maps.size(); i++)
            description_maps[i].populateModel();
    }

    void onChangeSelectedMaterialEntry();

public:
    explicit PageOptions(QStatusBar* statusBar, QWidget *parent = nullptr);
    ~PageOptions();

    void setPagePreview(PagePreview* _pagePreview)
    {
        pagePreview = _pagePreview;
    }

    

    QString nullableFieldPropTxt(std::string txt)
    {
        if (txt.size() == 0)
            return "<none>";
        return txt.c_str();
    };

    //SearchableList* addSearchableListRow1(QString field_id, QString field_name);
    //SearchableList* addSearchableListRow2(QString field_id, QString field_name);
    
    //SearchableList* initSearchableList(QString field_id, QString field_name);
    
    
    //void populateOilTypeFields();

    void updateStatusBar();

    void readCSV(const char *text);

    void rebuildDatabaseAndPopulateUI();
    bool rebuildDatabase();
    void repopulateLists();

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

    void processExportPDF();

private:
    //QQmlApplicationEngine *engine;

    QString getSelectedProduct();
    ComposerResultInt recomposePage(
        QString product_name, 
        OilTypeEntryPtr entry,
        bool allow_errors,
        std::function<void(QGraphicsScene*, ComposerResultInt)> callback=nullptr);
    
    
    void updateGenericCodeDescriptionTable();

private:
    Ui::PageOptions *ui;
};

#endif // PAGEOPTIONS_H
