#ifndef FORMATTEDTABLE_H
#define FORMATTEDTABLE_H

#include <sstream>
#include <vector>
#include <list>
#include <string>
#include <algorithm>

/**
 * A helper class for formatting tables. You just specify the number of columns
 * and then add elements with the << operator (header and data elements are
 * added the same way). Then pipe it to cout
 */
class FormattedTable
{
public:
  /**
   * Construct the table with a predefined number of columns
   */
  FormattedTable(int n_col);

  /**
   * Construct the table without a predefined number of columns. Instead
   * the table will be constructed using calls to EndRow()
   */
  FormattedTable();

  template <class TAtomic> FormattedTable& operator << (const TAtomic &datum)
    {
    // Convert the datum to a string and measure its length
    std::ostringstream oss;
    oss << datum;
    int w = (int) oss.str().length();

    // We need to add a new row in two cases: there is currently no row, or the
    // current row has been filled to m_Columns
    if(m_RowEnded)
      {
      m_Data.push_back(RowType());
      if(m_Columns > 0)
        m_Data.back().reserve(m_Columns);
      m_RowEnded = false;
      }

    // Now we have a row to add to that is guaranteed to be under m_Columns
    m_Data.back().push_back(oss.str());

    // If the number of columns is fixed, then call EndRow() automatically
    if(m_Columns > 0 && m_Data.back().size() >= m_Columns)
      m_RowEnded = true;

    // Now update the column width information
    if(m_Width.size() < m_Data.back().size())
      m_Width.push_back(w);
    else
      m_Width[m_Data.back().size() - 1] = std::max(w, m_Width[m_Data.back().size() - 1]);

    return *this;
    }

  void AppendString(const std::string str)
    {

    }

  void EndRow();

  void PrintRow(
      std::ostream &os, int iRow, const std::string &prefix = "",
      std::vector<bool> col_filter = std::vector<bool>()) const;

  void Print(std::ostream &os, const std::string &prefix = "",
             std::vector<bool> col_filter = std::vector<bool>()) const;

  int Rows() const;

  int Columns() const;

  const std::string &operator() (int iRow, int iCol) const;

  /** Parse the contents of a CSV encoded string with quote escapes */
  void ParseCSV(const std::string &str);

protected:
  typedef std::vector<std::string> RowType;
  typedef std::vector<RowType> TableType;

  // The data in the table
  TableType m_Data;

  // The string width measurements in the table
  std::vector<int> m_Width;

  // The number of columns (may change dynamically)
  int m_Columns;

  // Was row just ended
  bool m_RowEnded;
};

#endif // FORMATTEDTABLE_H
