#ifndef CSVPARSER_H
#define CSVPARSER_H

#include <string>
#include <vector>

/**
 * CSV parser
 */
class CSVParser
{
public:
  /** Parse the contents of a CSV encoded string with quote escapes */
  void Parse(const std::string &str);

  int GetNumberOfColumns() const;

  const std::vector<std::string> &GetParsedStrings()  const;

protected:

  int m_Columns;
  std::vector<std::string> m_Data;

};

#endif // CSVPARSER_H
