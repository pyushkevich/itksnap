#ifndef UTILITY_H
#define UTILITY_H

#include <fstream>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <time.h>
// #include "omp.h"
#include "data.h"

template<class T>
void readBasicType(std::istream& is, T& obj)
{
  is.read((char*)(&obj), sizeof(T));
}

template<class T>
void writeBasicType(std::ostream& os, const T& obj)
{
  os.write((const char*)(&obj), sizeof(T));
}

// paul: bug, this needs to be inline
inline int getLineNum(const std::string& name)
{
  std::ifstream inFile(name.c_str(), std::ios::in);
  return std::count(std::istreambuf_iterator<char>(inFile),
                    std::istreambuf_iterator<char>(), '\n');
  inFile.close();
}

template<class dataT, class labelT>
MLData<dataT, labelT>* readTextFile(const std::string& name)
{
  int dataNum = getLineNum(name);
  int dataDim = 0;
  bool first = true;
  MLData<dataT, labelT>* data;
  int tmpLabel = 0;
  double tmpData = 0;
  size_t cLineIdx = 0;
  std::string line;
  std::ifstream inFile(name.c_str(), std::ios::in);
  if (inFile.is_open())
    {
      while ( getline(inFile, line) )
        {
          std::istringstream iss(line);
          std::vector<std::string> tokens;
          std::copy(std::istream_iterator<std::string>(iss),
                    std::istream_iterator<std::string>(),
                    std::back_inserter<std::vector<std::string> >(tokens));
          if (first)
            {
              dataDim = tokens.size() - 1;
              data = new MLData<dataT, labelT>(dataNum, dataDim);
              first = false;
            }
          std::stringstream(tokens[0]) >> tmpLabel;
          data->label[cLineIdx] = tmpLabel;
          for (index_t i = 1; i < tokens.size(); ++i)
            {
              std::stringstream(tokens[i]) >> tmpData;
              data->data[cLineIdx][i-1] = tmpData;
            }
          cLineIdx++;
        }
      inFile.close();
    }
  else
    {
      throw std::runtime_error("open file error");
    }
  return data;
}

class Timer
{
public:
  void Reset(void)
  {
    start_ = 0;
    finish_ = 0;
  }

  void Start(void)
  {
    start_ = clock();
  }
  void Stop(void)
  {
    finish_ = clock();
  }
  double SpendSecond(void)
  {
    return (double)(finish_-start_)/CLOCKS_PER_SEC;
  }
  double StopAndSpendSecond(void)
  {
    Stop();
    return SpendSecond();
  }

private:
  clock_t start_, finish_;
};

class MPTimer
{
public:
  void Reset(void)
  {
    start_ = 0.0;
    finish_ = 0.0;
  }

  void Start(void)
  {
    // start_ = omp_get_wtime();
    start_ = clock();
  }
  void Stop(void)
  {
    // finish_ = omp_get_wtime();
    finish_ = clock();
  }
  double SpendSecond(void)
  {
    return (finish_-start_) / CLOCKS_PER_SEC;
  }
  double StopAndSpendSecond(void)
  {
    Stop();
    return SpendSecond();
  }

private:
  double start_, finish_;
};

#endif // UTILITY_H
