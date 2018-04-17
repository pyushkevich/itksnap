#include "FormattedTable.h"
#include <iomanip>

using namespace std;

FormattedTable::FormattedTable(int n_col)
{
  m_Columns = n_col;
  m_Width.resize(n_col, 0);
  m_RowEnded = true;
}

FormattedTable::FormattedTable()
{
  m_Columns = 0;
  m_RowEnded = true;
}

void FormattedTable::EndRow()
{
  m_RowEnded = true;
}

void FormattedTable::PrintRow(ostream &os, int iRow, const string &prefix, std::vector<bool> col_filter) const
{
  const RowType &row = m_Data[iRow];
  os << prefix << left;
  for(int iCol = 0; iCol < row.size(); iCol++)
    {
    if(col_filter.size() == 0 || col_filter[iCol])
      os << setw(m_Width[iCol] + 2) << row[iCol];
    }
  os << endl;
}

void FormattedTable::Print(ostream &os, const string &prefix, std::vector<bool> col_filter) const
{
  for(int iRow = 0; iRow < m_Data.size(); iRow++)
    this->PrintRow(os, iRow, prefix, col_filter);
}

int FormattedTable::Rows() const { return (int) m_Data.size(); }

int FormattedTable::Columns() const{ return (int) m_Width.size(); }

const std::string &FormattedTable::operator()(int iRow, int iCol) const { return m_Data[iRow][iCol]; }

void FormattedTable::ParseCSV(const std::string &str)
{
  istringstream iss(str);
  string line, word;
  while(getline(iss, line))
    {
    bool in_quoted = false;
    for(int k = 0; k < line.length(); k++)
      {
      // Get the next character
      char c = line[k];

      // If the character is a quote, it either starts/ends a quote or is
      // an escaped quite character
      if(c == '"')
        {
        if(k+1 < line.length() && line[k+1] == '"')
          {
          // This is just an escaped quote, treat it like a normal character
          ++k;
          }
        else
          {
          // Toggle the in_quoted state and continue to the next character
          in_quoted = !in_quoted;
          continue;
          }
        }

      else if((c == ',' && !in_quoted) || c == '\n' || c == '\r')
        {
        // This is the end of a field
        (*this) << word;
        word.clear();

        // Endrow
        if(c != ',' && m_Columns == 0)
          EndRow();

        continue;
        }

      // Normal character
      word.push_back(c);
      }
    }
}
