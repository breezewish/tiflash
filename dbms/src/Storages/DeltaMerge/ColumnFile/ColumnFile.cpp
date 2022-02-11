#include <IO/MemoryReadWriteBuffer.h>
#include <Storages/DeltaMerge/ColumnFile/ColumnFile.h>
#include <Storages/DeltaMerge/ColumnFile/ColumnFileBig.h>
#include <Storages/DeltaMerge/ColumnFile/ColumnFileDeleteRange.h>
#include <Storages/DeltaMerge/ColumnFile/ColumnFileInMemory.h>
#include <Storages/DeltaMerge/ColumnFile/ColumnFileTiny.h>
#include <Storages/DeltaMerge/RowKeyFilter.h>


namespace DB
{
namespace DM
{
/// ======================================================
/// Helper methods.
/// ======================================================
size_t copyColumnsData(
    const Columns & from,
    const ColumnPtr & pk_col,
    MutableColumns & to,
    size_t rows_offset,
    size_t rows_limit,
    const RowKeyRange * range)
{
    if (range)
    {
        RowKeyColumnContainer rkcc(pk_col, range->is_common_handle);
        if (rows_limit == 1)
        {
            if (range->check(rkcc.getRowKeyValue(rows_offset)))
            {
                for (size_t col_index = 0; col_index < to.size(); ++col_index)
                    to[col_index]->insertFrom(*from[col_index], rows_offset);
                return 1;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            auto [actual_offset, actual_limit] = RowKeyFilter::getPosRangeOfSorted(*range, pk_col, rows_offset, rows_limit);
            for (size_t col_index = 0; col_index < to.size(); ++col_index)
                to[col_index]->insertRangeFrom(*from[col_index], actual_offset, actual_limit);
            return actual_limit;
        }
    }
    else
    {
        if (rows_limit == 1)
        {
            for (size_t col_index = 0; col_index < to.size(); ++col_index)
                to[col_index]->insertFrom(*from[col_index], rows_offset);
        }
        else
        {
            for (size_t col_index = 0; col_index < to.size(); ++col_index)
                to[col_index]->insertRangeFrom(*from[col_index], rows_offset, rows_limit);
        }
        return rows_limit;
    }
}

ColumnFileInMemory * ColumnFile::tryToInMemoryFile()
{
    return !isInMemoryFile() ? nullptr : static_cast<ColumnFileInMemory *>(this);
}

ColumnFileTiny * ColumnFile::tryToTinyFile()
{
    return !isTinyFile() ? nullptr : static_cast<ColumnFileTiny *>(this);
}

ColumnFileDeleteRange * ColumnFile::tryToDeleteRange()
{
    return !isDeleteRange() ? nullptr : static_cast<ColumnFileDeleteRange *>(this);
}

ColumnFileBig * ColumnFile::tryToBigFile()
{
    return !isBigFile() ? nullptr : static_cast<ColumnFileBig *>(this);
}

String columnFilesToString(const ColumnFiles & column_files)
{
    String column_files_info = "[";
    for (const auto & f : column_files)
    {
        if (f->isInMemoryFile())
            column_files_info += "B_" + DB::toString(f->getRows());
        else if (f->isTinyFile())
            column_files_info += "B_" + DB::toString(f->getRows());
        else if (f->isBigFile())
            column_files_info += "F_" + DB::toString(f->getRows());
        else if (auto * f_delete = f->tryToDeleteRange(); f_delete)
            column_files_info += "D_" + f_delete->getDeleteRange().toString();
        column_files_info += (f->isSaved() ? "_S," : "_N,");
    }

    if (!column_files.empty())
        column_files_info.erase(column_files_info.size() - 1);
    column_files_info += "]";
    return column_files_info;
}

} // namespace DM
} // namespace DB