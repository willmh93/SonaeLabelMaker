#pragma once
#include <QString>
#include <QDebug>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <functional>
using namespace std;

const long long int ss_max = std::numeric_limits<std::streamsize>::max();


//bool read_cell()
//{
//    bool exists = (bool)std::getline(stream, cell, ',');
//    if (cell.findCell('\n') != std::string::npos)
//    {
//        //std::getline(stream, dummy, '\n')
//        //stream.ignore(ss_max, '\n');
//        row++;
//    }
//    return exists;
//}
// 
//std::getline(stream, dummy, '\n')
//stream.ignore(ss_max, '\n');
/*
* bool ends_with_nl()
    {
        size_t len = size();
        if (len == 0)
            return false;

        for (size_t i = 0; i < len; i++)
        {
            if (at(len-1) == '\n' ||
                (len >= 2 && at(len-2) == '\r' && at(len-1) == '\n'))
            {
                return true;
            }
        }
        return false;
    }*/

    //// Flush row
    //if (data_row.size())
    //{
    //    data.push_back(data_row);
    //    data_row.clear();
    //}
    //reading_header_index = 0;

struct string_ex : public std::string
{
    string_ex() {}
    string_ex(const char *str) : string(str) {}
    string_ex(const string &str) : string(str) {}
    string_ex(string::iterator it1, string::iterator it2) : string(it1, it2) {}

    std::vector<string_ex> splitByWhitespace()
    {
        std::istringstream iss(*this);
        std::vector<string_ex> tokens;
        string_ex word;

        while (iss >> word)
            tokens.push_back(word);

        return tokens;
    }

    bool compare(string_ex txt, bool fuzzy_space, bool case_sensitive)
    {
        if (fuzzy_space)
        {
            std::vector<string_ex> words_lhs = splitByWhitespace();
            std::vector<string_ex> words_rhs = txt.splitByWhitespace();

            if (words_lhs.size() != words_rhs.size())
                return false;

            if (case_sensitive)
            {
                for (size_t i = 0; i < words_lhs.size(); i++)
                {
                    if (words_lhs[i] != words_rhs[i])
                        return false;
                }
            }
            else
            {
                for (size_t i = 0; i < words_lhs.size(); i++)
                {
                    if (words_lhs[i].tolower() != words_rhs[i].tolower())
                        return false;
                }
            }
            return true;
        }
        else
        {
            if (case_sensitive)
            {
                return (*this) == txt;
            }
            else
            {
                return this->tolower() == txt.tolower();
            }
        }
        return false;
    }

    string_ex tolower()
    {
        string_ex ret(begin(), end());
        std::transform(ret.begin(), ret.end(), ret.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return ret;
    }

    string_ex trimmed()
    {
        auto start = std::find_if(begin(), end(), [](char ch) { return !std::isspace(ch); });
        auto end = std::find_if(rbegin(), rend(), [](char ch) { return !std::isspace(ch); }).base();
        return (start < end) ? string_ex(start, end) : "";
    }

    string_ex unquoted()
    {
        auto start = std::find_if(begin(), end(), [](char ch) { return (ch != '\"'); });
        auto end = std::find_if(rbegin(), rend(), [](char ch) { return (ch != '\"'); }).base();
        return (start < end) ? string_ex(start, end) : "";
    }

    bool contains(const std::string& txt) const
    {
        return find(txt) != std::string::npos;
    }

    bool is_quoted() const
    {
        return (size() >= 2 && at(0) == '\"' && at(size() - 1) == '\"');
    }

    bool starts_with_nl() const
    {
        size_t len = size();
        for (size_t i = 0; i < len; i++)
        {
            if (at(0) == '\n' || (at(0) == '\r' && at(1) == '\n'))
                return true;
        }
        return false;
    }
};

struct CSVPoint
{
    int col;
    int row;
};

struct CSVRect
{
    enum
    {
        BEG = 0,
        END = -1
    };

    int col_first = BEG;
    int row_first = BEG;
    int col_last = END;
    int row_last = END;

    CSVRect() {}
    CSVRect(int col_first, int row_first, int col_last, int row_last)
    :
        col_first(col_first),
        row_first(row_first),
        col_last(col_last),
        row_last(row_last)
    {}
};


struct CSVCellInfo
{
    bool row_start = false;
    bool exists = false;
    int col;
    int row;
    string_ex txt;

    operator bool()
    {
        return exists;
    }
};

struct CSVCell;
struct CSVHeader;

typedef shared_ptr<CSVCell> CSVCellPtr;
typedef shared_ptr<CSVHeader> CSVHeaderPtr;

struct CSVHeader : public std::enable_shared_from_this<CSVHeader>
{
    int col;
    int max_row = 0;

    string_ex custom_id;

    std::vector<CSVCellPtr> subheaders;

    CSVHeader(int col)
        : col(col)
    {}

    CSVHeaderPtr setCustomId(const std::string& id);
    CSVHeaderPtr addSubHeader(CSVCellPtr cell);
};

struct CSVCell 
{
    CSVHeaderPtr header;
    int row;
    int col;
    string_ex txt;

    CSVCell(CSVHeaderPtr header, int row, int col, std::string txt)
        : header(header), row(row), col(col), txt(txt)
    {}
};



struct CSVRow : public std::vector<CSVCellPtr>
{
    CSVCellPtr findByHeader(CSVHeaderPtr header)
    {
        return at(header->col);
    }

    CSVCellPtr findByHeaderCustomID(const std::string& id)
    {
        size_t count = size();
        for (size_t i = 0; i < count; i++)
        {
            CSVCellPtr& cell = at(i);
            if (cell->header && cell->header->custom_id == id)
                return at(i);
        }
        return nullptr; // make_shared<CSVCell>(nullptr, -1, -1, "");
    }

    //CSVCellPtr findByHeader(const std::string& setHeader)
    //{
    //    size_t count = 0;
    //    for (size_t i = 0; i < count; i++)
    //    {
    //        if (at(i)->setHeader->txt == setHeader)
    //            return at(i);
    //    }
    //    return nullptr;
    //}
};

struct CSVTable : public std::vector<CSVRow>
{
};

class CSVReader
{
    int col, row;
    int ch_index;
    bool data_loaded = false;
    bool file_opened = false;

    istringstream stream;
    vector<CSVRow> table;
    vector<CSVHeaderPtr> headers;
    int max_header_row;

public:

    //int first_header_col;
    //bool parsing_header = false;

    //int header_col_first;
    //int header_row_first;
    //int header_col_last;
    //int header_row_last;

    CSVReader() {}
    void open(const char* txt);

    void read_raw_table();
    bool read_cell(CSVCellInfo &info);

    // Adds columns setHeader identifier (multiple can exist)
    CSVCellPtr findCell(std::string txt, CSVRect r = CSVRect());
    CSVCellPtr findCellFuzzy(std::string txt, CSVRect r = CSVRect(), bool fuzzy_space=true, bool case_sensitive=false);
    CSVCellPtr findCellIf(std::function<bool(string_ex&)> valid, CSVRect r = CSVRect());
    std::vector<CSVCellPtr> findCells(std::string txt, CSVRect r = CSVRect());
    std::vector<CSVCellPtr> findCellsFuzzy(std::string txt, CSVRect r = CSVRect(), bool fuzzy_space = true, bool case_sensitive = false);
    std::vector<CSVCellPtr> findCellsWith(std::string txt, CSVRect r = CSVRect());

    CSVHeaderPtr setHeader(CSVCellPtr cell);
    void readData();

    bool opened()
    {
        return file_opened;
    }

    bool loaded()
    {
        return data_loaded;
    }

    CSVRow& getRow(int row)
    {
        return table[row];
    }
    
    int dataRowFirst()
    {
        return max_header_row + 1;
    }

    int dataRowLast()
    {
        return static_cast<int>(table.size());
    }

    //CSVHeaderPtr header_if(
    //    std::string custom_id,
    //    std::function<bool(string_ex&)> valid,
    //    CSVHeaderPtr after = nullptr
    //);

    //void next_row_first_cell(CSVCellInfo& info);

    //CSVHeaderPtr setHeader(std::string custom_id, std::string header_txt);
    ////CSVHeaderPtr header_regex(std::string custom_id, std::string pattern);
    ////CSVHeaderPtr header_with(std::string custom_id, std::string substr);
    //CSVHeaderPtr header_if(
    //    std::string custom_id, 
    //    std::function<bool(string_ex&)> valid,
    //    CSVHeaderPtr after=nullptr
    //);

    //std::vector<CSVHeaderPtr> headers_if(std::function<bool(string_ex&)> valid);

    //CSVHeaderPtr add_header(int col, string_ex header_txt);
    //void headers_start(std::string header_item);

    //CSVCellInfo cell;
    //bool read_row(CSVRow *data_row);

    //void read_data(vector<CSVRow>& data);
};
