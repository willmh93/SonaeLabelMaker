#ifndef PAGEOPTIONS_H
#define PAGEOPTIONS_H

#include <QWidget>
#include <QStatusBar>
#include <QPdfWriter>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

//#include <QQmlApplicationEngine>
//#include <QQmlContext>
//#include <QQmlComponent>
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

    const std::unordered_map<QString, ShapeInfo>& shapeMap() const
    {
        return shape_map;
    }

    const std::unordered_map<QString, QColor>& shapeColorMap() const
    {
        return shape_color_map;
    }

    const std::unordered_map<QString, QColor>& backColorMap() const
    {
        return back_color_map;
    }

    const std::unordered_map<QString, QColor>& innerBackColorMap() const
    {
        return inner_back_color_map;
    }

    // 'set' methods

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

    // 'remove' methods

    void removeShapeInfo(QString desc)
    {
        auto it = shape_map.find(desc);
        if (it != shape_map.end())
            shape_map.erase(it);
    }

    void removeShapeColor(QString desc)
    {
        auto it = shape_color_map.find(desc);
        if (it != shape_color_map.end())
            shape_color_map.erase(it);
    }

    void removeBackColor(QString desc)
    {
        auto it = back_color_map.find(desc);
        if (it != back_color_map.end())
            back_color_map.erase(it);
    }

    void removeInnerBackColor(QString desc)
    {
        auto it = inner_back_color_map.find(desc);
        if (it != inner_back_color_map.end())
            inner_back_color_map.erase(it);
    }

    // 'replace' methods

    void replaceShapeInfo(QString desc, QString new_desc)
    {
        //if (containsShapeInfo(desc))
        {
            ShapeInfo info = getShapeInfo(desc);
            removeShapeInfo(desc);

            if (!new_desc.isEmpty())
                setShapeInfo(new_desc, info);
        }
    }

    void replaceShapeColor(QString desc, QString new_desc)
    {
        auto info = getShapeColor(desc);
        removeShapeColor(desc);
        if (!new_desc.isEmpty())
            setShapeColor(new_desc, info);
    }

    void replaceBackColor(QString desc, QString new_desc)
    {
        auto info = getBackColor(desc);
        removeBackColor(desc);
        if (!new_desc.isEmpty())
            setBackColor(new_desc, info);
    }

    void replaceInnerBackColor(QString desc, QString new_desc)
    {
        auto info = getInnerBackColor(desc);
        removeInnerBackColor(desc);
        if (!new_desc.isEmpty())
            setInnerBackColor(new_desc, info);
    }

    // 'contains' methods

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

    /// Not needed as of yet (since inner-background isn't considered essential)
    bool containsInnerBackColor(QString desc)
    {
        return inner_back_color_map.count(desc) > 0;
    }

    // 'contains valid data' methods
    bool containsValidShapeInfo(QString desc)
    {
        return containsShapeInfo(desc) && getShapeInfo(desc).valid;
    }

    bool containsValidShapeColor(QString desc)
    {
        return containsShapeColor(desc) && getShapeColor(desc).isValid();
    }

    bool containsValidBackColor(QString desc)
    {
        return containsBackColor(desc) && getBackColor(desc).isValid();
    }

    bool containsValidInnerBackColor(QString desc)
    {
        return containsInnerBackColor(desc) && getInnerBackColor(desc).isValid();
    }

    // todo: Switch to internal pointers. Entire byte-arrays being copied
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
            return QColor(); // null color
    }

    QColor getBackColor(QString desc)
    {
        if (back_color_map.count(desc))
            return back_color_map[desc];
        else
            return QColor(); // null color
    }

    QColor getInnerBackColor(QString desc)
    {
        if (inner_back_color_map.count(desc))
            return inner_back_color_map[desc];
        else
            return QColor(); // null color
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
    SUCCEEDED=0ul,
    MISSING_MATERIAL_CODE=1ul,
    MISSING_GENERIC_CODE=2ul,
    MISSING_PRODUCT_NAME=4ul,
    MISSING_SHAPE=8ul,
    MISSING_SHAPE_COLOR=16ul,
    MISSING_BACK_COLOR=32ul,
    NO_SELECTED_ITEM=64ul
};
typedef unsigned long ComposerResultInt;

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

    bool contains(const K& k)
    {
        auto it = std::find_if(this->begin(), this->end(), [k](const auto& entry) {
            return entry.first == k;
        });

        if (it == this->end())
            return false;

        return true;
    }

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

    void remove(const K& k)
    {
        auto it = std::find_if(this->begin(), this->end(), [k](const auto& entry) {
            return entry.first == k;
        });

        if (it != this->end())
            this->erase(it);
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
            return pair_a.first.size() > pair_b.first.size();
        });
    }

    void populateModel()
    {
        if (model)
        {
            model->clear();
            for (size_t i = 0; i < lookup.size(); i++)
            {
                QList<QStandardItem*> rowItems;

                rowItems.push_back(new QStandardItem(lookup.at(i).first.c_str()));
                rowItems.push_back(new QStandardItem(lookup.at(i).second.c_str()));
                model->insertRow(i, rowItems);
            }
        }
    }

    void serialize(QJsonObject& info) const
    {
        for (size_t i = 0; i < lookup.size(); i++)
        {
            auto& item = lookup.at(i);
            QString name = item.first.c_str();
            QString desc = item.second.c_str();
            info[name] = desc;
        }
    }

    void deserialize(const QJsonObject& info)
    {
        lookup.clear();
        for (auto it = info.constBegin(); it != info.constEnd(); ++it)
        {
            QString name = it.key();
            QString desc = it.value().toString();
            lookup[name.toStdString()] = desc.toStdString();
        }
    }
};

class SelectedProductDescriptionModel : public QStandardItemModel {
public:
    using QStandardItemModel::QStandardItemModel;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (!index.isValid())
            return QVariant();

        if (role == Qt::ForegroundRole) {
            int row = index.row();
            int col = index.column();
            QVariant cellValue = QStandardItemModel::data(index, Qt::DisplayRole);

            // Highlight red of cell missing
            if (col == 0 && cellValue.toString() == "<no match>") {
                return QBrush(Qt::red);
            }
            else if (col == 1)
            {
                QModelIndex first_cell_index = this->index(row, 0);
                if (first_cell_index.data(Qt::DisplayRole).toString() == "<no match>")
                    return QBrush(Qt::red);
            }
        }

        // For all other roles, use the default behavior
        return QStandardItemModel::data(index, role);
    }
};


class PageOptions : public QWidget
{
    Q_OBJECT;

    // Icon data
    QByteArray warning_icon_data;
    QByteArray delete_inactive_data;
    QByteArray delete_active_data;

    // Radio buttons
    std::shared_ptr<std::vector<SearchableList*>> radio_lists;

    // Fields
    SearchableList* field_merged_code;
    SearchableList* field_shape;
    SearchableList* field_shape_col;
    SearchableList* field_back_col;
    SearchableList* field_inner_back_col;
    SearchableList* field_products;

    PagePreview* pagePreview = nullptr;
    QStatusBar* statusBar; // todo: Remove?

    // Merged Code / Product vector-like maps
    flat_map<std::string, OilTypeEntryPtr> merged_code_entries;
    flat_map<std::string, OilTypeEntryPtr> product_oiltype_entries;
    OilTypeEntryPtr selected_entry = nullptr;
    void onChangeSelectedMaterialEntry();

    // "User count" maps for each Product 
    std::unordered_map<std::string, int> users_shape;
    std::unordered_map<std::string, int> users_shape_color;
    std::unordered_map<std::string, int> users_back_color;
    std::unordered_map<std::string, int> users_inner_back_color;

    // Helpers
    bool noUsers(std::unordered_map<std::string, int>& users_map, QString desc)
    {
        if (desc == "<unassigned>") return false;
        return users_map[desc.toStdString()] == 0;
    };

    QString nullableFieldPropTxt(std::string txt)
    {
        if (txt.size() == 0) return "<unassigned>";
        return txt.c_str();
    };


    // Composer info / generator
    ComposerInfo composeInfo;
    ComposerInfoGenerator composerGenerator;

    // Project
    QString project_filename;

    // CSV
    CSVReader csv;
    QString csv_filename;
    QStandardItemModel csv_model;

    // File Management
    FileManager* csv_picker = nullptr;
    FileManager* svg_picker = nullptr;
    FileManager* project_file_manager = nullptr;
    FileManager* pdf_exporter = nullptr;

    // PDF
    PDFBatchExport* batch_dialog = nullptr;
    QVector<QPair<QString, QByteArray>> pdfs;
    int export_index = 0;
    bool batch_export_busy = false;
    PdfSceneWriter pdf_writer;

    // Token-Description maps
    std::vector<TokenDescriptionMap> description_maps;
    void setTokenColumnName(int col, QString name);
    void setTokenDescription(int col, QString token, QString desc);

    // Selected product token description table model
    SelectedProductDescriptionModel selected_product_description_model;
    void onEditSelectedProductTokenDescription(int row, int col, QString txt);
    void updateGenericCodeDescriptionTable();

    // Settings "all tables" models
    void populateTokenDescriptionModels();
    void populateTokenDescriptionTables();


public:
    explicit PageOptions(QStatusBar* statusBar, QWidget *parent = nullptr);
    ~PageOptions();

    void setPagePreview(PagePreview* _pagePreview)
    {
        pagePreview = _pagePreview;
    }


    void updateStatusBar();

    void readCSV(const char *text);

    void rebuildDatabaseAndPopulateUI();
    bool rebuildDatabase();
    void repopulateLists(
        bool bMergedCode=true,
        bool bProducts = true,
        bool bShape = true,
        bool bShapeCol = true,
        bool bBackCol = true,
        bool bInnerBackCol = true,
        bool bCSV = true
    );
    void countStyleUsers();
    void updateItemsEditable();

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
    
   

private:
    Ui::PageOptions *ui;
};

#endif // PAGEOPTIONS_H
