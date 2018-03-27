#include "CSVParser.h"
#include <vector>
#include <sstream>

using namespace std;

void CSVParser::Parse(const std::string &str)
{
  m_Data.clear();
  m_Columns = 0;

  istringstream iss(str);
  string line, word;
  while(getline(iss, line))
    {
    int iCol = 0;

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
        m_Data.push_back(word);
        word = string();
        iCol++;
        continue;
        }

      // Normal character
      word.push_back(c);
      }

    // Update the column count
    if(m_Columns < iCol)
      m_Columns = iCol;
    }
}

int CSVParser::GetNumberOfColumns() const
{
  return m_Columns;
}

const vector<string> &CSVParser::GetParsedStrings() const
{
  return m_Data;
}
