#include <regex>
#include "csv_reader.h"


void CSVReader::open(const char* txt)
{
    stream.str(txt);
    col = row = 0;
    ch_index = 0;
    max_header_row = 0;
    data_loaded = false;
    file_opened = true;

    read_raw_table();
}

void CSVReader::read_raw_table()
{
    CSVCellInfo cell_info;

    CSVRow row_cells;
    while (read_cell(cell_info))
    {
        if (cell_info.row_start)
        {
            table.emplace_back(row_cells);
            row_cells.clear();
        }

        CSVCellPtr cell_ptr = make_shared<CSVCell>(nullptr, cell_info.row, cell_info.col, cell_info.txt);
        row_cells.push_back(cell_ptr);
    }
    if (row_cells.size())
        table.emplace_back(row_cells);
}

bool CSVReader::read_cell(CSVCellInfo& info)
{
    info.col = col;
    info.row = row;

    bool in_quote = false;
    char ch;
    std::string full_txt;

    while (stream.get(ch))
    {
        if (ch == '\"')
            in_quote = !in_quote;
        
        if (!in_quote && ch == ',')
            break;

        full_txt += ch;
    }
    info.txt = full_txt;

    if (stream.eof())
    {
        info.txt = "";
        info.exists = false;
        info.row_start = false;
        return false;
    }


    info.exists = true;

    if (info.txt.starts_with_nl())
    {
        row++;
        col = 0;

        info.row_start = true;
        info.txt = info.txt.trimmed().unquoted();
    }
    else
    {
        col++;

        info.row_start = false;
        info.txt = info.txt.unquoted();
    }

    info.col = col;
    info.row = row;

    return true;
}

CSVCellPtr CSVReader::findCell(std::string txt, CSVRect r)
{
    int row_last = (r.row_last == CSVRect::END) ? (table.size() - 1) : r.row_last;
    for (int _row = r.row_first; _row <= row_last; _row++)
    {
        int col_last = (r.col_last == CSVRect::END) ? (table[_row].size() - 1) : r.col_last;
        for (int _col = r.col_first; _col <= col_last; _col++)
        {
            CSVCellPtr raw_cell = table[_row][_col];
            if (raw_cell->txt == txt)
                return raw_cell;
        }
    }
    return nullptr;
}

CSVCellPtr CSVReader::findCellIf(std::function<bool(string_ex&)> valid, CSVRect r)
{
    int row_last = (r.row_last == CSVRect::END) ? (table.size() - 1) : r.row_last;
    for (int _row = r.row_first; _row <= row_last; _row++)
    {
        int col_last = (r.col_last == CSVRect::END) ? (table[_row].size() - 1) : r.col_last;
        for (int _col = r.col_first; _col <= col_last; _col++)
        {
            CSVCellPtr raw_cell = table[_row][_col];
            if (valid(raw_cell->txt))
                return raw_cell;
        }
    }
    return nullptr;
}

std::vector<CSVCellPtr> CSVReader::findCellsWith(std::string txt, CSVRect r)
{
    std::vector<CSVCellPtr> ret;

    int row_last = (r.row_last == CSVRect::END) ? (table.size() - 1) : r.row_last;
    for (int _row = r.row_first; _row <= row_last; _row++)
    {
        int col_last = (r.col_last == CSVRect::END) ? (table[_row].size() - 1) : r.col_last;
        for (int _col = r.col_first; _col <= col_last; _col++)
        {
            CSVCellPtr raw_cell = table[_row][_col];
            if (raw_cell->txt.contains(txt))
            {
                ret.push_back(raw_cell);
            }
        }
    }
    return ret;
}

CSVHeaderPtr CSVReader::setHeader(CSVCellPtr cell)
{
    CSVHeaderPtr header = nullptr;

    // Do we already have this column as a header?
    for (CSVHeaderPtr existing_header : headers)
    {
        if (existing_header->col == cell->col)
        {
            header = existing_header;
            break;
        }
    }

    if (!header)
        header = make_shared<CSVHeader>(cell->col);

    header->addSubHeader(cell);
    headers.push_back(header);

    if (header->max_row > max_header_row)
        max_header_row = header->max_row;

    return header;
}

void CSVReader::readData()
{
    size_t row_count = table.size();
    for (int header_index = 0; header_index < headers.size(); header_index++)
    {
        CSVHeaderPtr header = headers[header_index];
        int col = header->col;
        for (int row = header->max_row+1; row < row_count; row++)
        {
            CSVCellPtr cell = table[row][col];
            cell->header = header;
        }
    }

    data_loaded = true;

    //size_t row_count = table.size();
    //for (size_t row = 0; row < row_count; row++)
    //{
    //    size_t col_count = table[row].size();
    //    for (size_t col = 0; col <= col_count; col++)
    //    {
    //        CSVCellPtr cell = table[row][col];
    //        cell->header
    //    }
    //}
}

/*
void CSVReader::next_row_first_cell(CSVCellInfo& info)
{
    while (read_cell(info))
    {
        if (info.row_start)
            break;
    }
}

CSVHeaderPtr CSVReader::setHeader(std::string custom_id, std::string header_txt)
{
    for (CSVHeaderPtr setHeader : headers)
    {
        if (setHeader->txt == header_txt)
        {
            setHeader->custom_id = custom_id;
            return setHeader;
        }
    }
    return nullptr;
}

//CSVHeaderPtr CSVReader::header_regex(std::string pattern)
//{
//    parsing_header = true;
//
//    CSVCellInfo cell;
//    while (read_cell(cell))
//    {
//        std::regex regexPattern(pattern);
//
//        if (std::regex_match(cell.txt, regexPattern))
//        {
//            CSVHeaderPtr header_ptr = make_shared<CSVHeader>(col, cell.txt);
//            headers.push_back(header_ptr);
//            return header_ptr;
//        }
//    }
//    return nullptr;
//}
//
//CSVHeaderPtr CSVReader::header_with(std::string substr)
//{
//    parsing_header = true;
//
//    CSVCellInfo cell;
//    while (read_cell(cell))
//    {
//        if (cell.txt.contains(substr))
//        {
//            CSVHeaderPtr header_ptr = make_shared<CSVHeader>(col, cell.txt);
//            headers.push_back(header_ptr);
//            return header_ptr;
//        }
//    }
//    return nullptr;
//}

CSVHeaderPtr CSVReader::header_if(
    std::string custom_id, 
    std::function<bool(string_ex&)> valid,
    CSVHeaderPtr after
)
{
    for (CSVHeaderPtr setHeader : headers)
    {
        if (after && setHeader->col <= after->col)
            continue;

        if (valid(setHeader->txt))
        {
            setHeader->custom_id = custom_id;
            return setHeader;
        }
    }

    return nullptr;
}

std::vector<CSVHeaderPtr> CSVReader::headers_if(std::function<bool(string_ex&)> valid)
{
    return std::vector<CSVHeaderPtr>();
}

CSVHeaderPtr CSVReader::add_header(int col, string_ex header_txt)
{
    CSVHeaderPtr header_ptr = make_shared<CSVHeader>(col, header_txt);
    headers.push_back(header_ptr);
    return header_ptr;
}

void CSVReader::headers_start(std::string header_item)
{
    parsing_header = true;

    bool found_first_header = false;
    CSVCellInfo cell;
    while (read_cell(cell))
    {
        if (!found_first_header)
        {
            if (cell.txt == header_item)
            {
                first_header_col = cell.col;

                // Found first setHeader item
                add_header(cell.col, cell.txt);
                found_first_header = true;
            }
        }
        else
        {
            add_header(cell.col, cell.txt);

            // Find remaining headers until new row
            if (cell.row_start)
                break;
        }
    }
}

bool CSVReader::read_row(CSVRow* dest)
{
    size_t header_index = 0;
    size_t header_count = headers.size();
    dest->clear();

    // Grab first data cell (is just finished setHeader)
    if (parsing_header)
    {
        next_row_first_cell(cell);
        parsing_header = false;
    }

    // Skip to first valid setHeader column
    for (int i=0; i<first_header_col-1; i++)
        read_cell(cell);

    // Loop cells in row...
    while (cell)
    {
        if (header_index < header_count)
        {
            auto setHeader = headers[header_index];
            if (col == setHeader->col)
            {
                dest->emplace_back(make_shared<CSVCell>(setHeader, cell.row, cell.col, cell.txt));
                header_index++;
            }
        }

        read_cell(cell);

        if (cell.row_start)
            return true; // ... Until end of row
    }

    return false; // End of file
}

void CSVReader::read_data(vector<CSVRow> &data)
{
    size_t header_index = 0;
    size_t header_count = headers.size();

    data.clear();

    CSVRow data_row;

    // Skip to first valid setHeader column
    for (int i = 0; i < first_header_col - 1; i++)
        read_cell(cell);

    // Loop cells in row...
    while (cell)
    {
        if (header_index < header_count)
        {
            auto setHeader = headers[header_index];
            if (col == setHeader->col)
            {
                dest->emplace_back(make_shared<CSVCell>(setHeader, cell.row, cell.col, cell.txt));
                header_index++;
            }
        }

        read_cell(cell);

        if (cell.row_start)
            return true; // ... Until end of row
    }
}*/

CSVHeaderPtr CSVHeader::setCustomId(const std::string& id)
{
    custom_id = id;
    return shared_from_this();
}

CSVHeaderPtr CSVHeader::addSubHeader(CSVCellPtr cell)
{
    subheaders.push_back(cell);
    if (cell->row > max_row)
        max_row = cell->row;
    return shared_from_this();
}
