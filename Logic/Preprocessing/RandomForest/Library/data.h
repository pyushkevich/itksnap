/**
 * Define some datasets inherited from abstract DataSet class.
 */

#ifndef DATA_H
#define DATA_H

#include <vector>
#include "random.h"

typedef std::size_t size_t;
typedef std::size_t index_t;

class DataSet
{
public:
  virtual size_t Size() const = 0;
  virtual void Resize(size_t n) = 0;
};

template<class T>
class Vector : public DataSet
{
public:
  typedef typename std::vector<T>::iterator iterator;
  typedef typename std::vector<T>::const_iterator const_iterator;

  Vector(): usable_(0) {}
  Vector(size_t n): usable_(0)
  {
    Resize(n);
  }
  Vector(size_t n, const T& val=T()): usable_(n)
  {
    data_ = std::vector<T>(n,val);
  }

  size_t Size() const { return data_.size(); }
  size_t UsableSize() const { return usable_; }
  void Resize(size_t n)
  {
    data_.resize(n);
    if (n < usable_)
      {
        usable_ = n;
      }
  }

  void ResetUsable()
  {
    usable_ = 0;
  }

  void PushBack(const T& t)
  {
    data_.push_back(t);
    usable_ = data_.size();
  }

  void PutBack(const T& t)
  {
    if (usable_ < data_.size())
      {
        data_[usable_] = t;
        ++usable_;
      }
    else
      {
        throw std::runtime_error("PutBack out of range of Vector\n");
      }
  }

  const T& Get(index_t i) const
  {
    if (i < data_.size())
      {
        return data_[i];
      }
    else
      {
        throw std::runtime_error("Get out of range of Vector\n");
      }
  }

  void Set(index_t i, T& t)
  {
    if (i < data_.size())
      {
        data_[i] = t;;
      }
    else
      {
        throw std::runtime_error("Set out of range of Vector\n");
      }
  }

  T& operator[](index_t i)
  {
    if (i < data_.size())
      {
        return data_[i];
      }
    else
      {
        throw std::runtime_error("[] out of range of Vector\n");
      }
  }
  const T& operator[](index_t i) const
  {
    if (i < data_.size())
      {
        return data_[i];
      }
    else
      {
        throw std::runtime_error("[] out of range of Vector\n");
      }
  }

  iterator Begin() { return data_.begin(); }
  const_iterator Begin() const { return data_.begin(); }
  iterator End() { return data_.end(); }
  const_iterator End() const { return data_.end(); }

private:
  std::vector<T> data_;
  index_t usable_;
};

template<class T>
class Matrix : public DataSet
{
public:
  Matrix(): rowUsable_(0), colUsable_(0), rowNum_(0), colNum_(0) {}
  Matrix(size_t rowNum, size_t colNum): rowUsable_(0), colUsable_(0)
  {
    Resize(rowNum, colNum);
  }

  size_t Size() const { return RowSize(); }
  size_t RowSize() const { return rowNum_; }
  size_t ColumnSize() const { return colNum_; }
  size_t UsableSize() const { return RowUsableSize(); }
  size_t RowUsableSize() const { return rowUsable_; }
  size_t ColumnUsableSize() const { return colUsable_; }

  void Resize(size_t rowNum, size_t colNum)
  {
    data_.resize(rowNum);
    for (index_t i = 0; i < rowNum; ++i)
      {
        data_[i].resize(colNum);
      }
    if (rowNum < rowUsable_)
      {
        rowUsable_ = rowNum;
      }
    if (colNum < colUsable_)
      {
        colUsable_ = colNum;
      }
    rowNum_ = rowNum;
    colNum_ = colNum;
  }

  void Resize(size_t rowNum)
  {
    data_.resize(rowNum);
    for (index_t i = 0; i < rowNum; ++i)
      {
        data_[i].resize(colNum_);
      }
    if (rowNum < rowUsable_)
      {
        rowUsable_ = rowNum;
      }
    rowNum_ = rowNum;
  }

  void ResetUsable()
  {
    rowUsable_ = 0;
    colUsable_ = 0;
  }

  void PushBack(const std::vector<T>& t)
  {
    data_.push_back(t);
    rowUsable_ = data_.size();
    ++rowNum_;
  }

  void PutBack(const T& t)
  {
    if ((rowUsable_ < rowNum_) && (colUsable_ < colNum_))
      {
        data_[rowUsable_][colUsable_] = t;
        ++colUsable_;
        if (colUsable_ == colNum_)
          {
            ++rowUsable_;
            colUsable_ = 0;
          }
      }
    else
      {
        throw std::runtime_error("PutBack out of range of Matrix\n");
      }
  }

  const T& Get(index_t rowIdx, index_t colIdx) const
  {
    if ((rowIdx < rowNum_) && (colIdx < colNum_))
      {
        return data_[rowIdx][colIdx];
      }
    else
      {
        throw std::runtime_error("Get out of range of Matrix\n");
      }
  }

  std::vector<T>& GetRow(index_t rowIdx) const
  {
    if (rowIdx < rowNum_)
      {
        return data_[rowIdx];
      }
    else
      {
        throw std::runtime_error("GetRow out of range of Matrix\n");
      }
  }

  void Set(index_t rowIdx, index_t colIdx, T& t)
  {
    if ((rowIdx < rowNum_) && (colIdx < colNum_))
      {
        data_[rowIdx][colIdx] = t;
        if (rowIdx > rowUsable_)
          {
            rowUsable_ = rowIdx;
          }
        if (colIdx > colUsable_)
          {
            colUsable_ = colIdx;
          }
      }
    else
      {
        throw std::runtime_error("Set out of range of Matrix\n");
      }
  }

  std::vector<T>& operator[](index_t i)
  {
    if (i < rowNum_)
      {
        return data_[i];
      }
    else
      {
        throw std::runtime_error("[] out of range of Matrix\n");
      }
  }
  const std::vector<T>& operator[](index_t i) const
  {
    if (i < rowNum_)
      {
        return data_[i];
      }
    else
      {
        throw std::runtime_error("[] out of range of Matrix\n");
      }
  }
private:
  std::vector<std::vector<T> > data_;
  size_t rowNum_;
  size_t colNum_;
  size_t rowUsable_;
  size_t colUsable_;
};

template<class dataT, class labelT>
class MLData: public DataSet
{
public:
  MLData(): dataNum_(0), dataDim_(0) {}
  MLData(size_t rowNum, size_t colNum): dataNum_(rowNum), dataDim_(colNum)
  {
    data.Resize(rowNum, colNum);
    label.Resize(rowNum);
  }

  size_t Size() const
  {
    return dataNum_;
  }

  size_t Dimension() const
  {
    return dataDim_;
  }

  void Resize(size_t n)
  {
    data.Resize(n);
    label.Resize(n);
  }

  size_t LabelClassNum()
  {
    std::set<labelT> labelSet(label.Begin(), label.End());
    return labelSet.size();
  }

  MLData<dataT, labelT>* Sampling(size_t eachClassNum)
  {
    std::set<labelT> labelSet(label.Begin(), label.End());
    size_t classNum = labelSet.size();
    std::map<labelT, index_t> backMapping;
    typename std::set<labelT>::iterator setIter;
    index_t i = 0;
    for (setIter = labelSet.begin(); setIter != labelSet.end(); ++setIter, ++i)
      {
        backMapping.insert(typename std::map<labelT, index_t>::value_type(*setIter, i));
      }

    Random random;
    MLData<dataT, labelT>* sample = new MLData<dataT, labelT>(eachClassNum * classNum, dataDim_);
    std::vector<std::vector<index_t> > dataIndexSet;
    dataIndexSet.resize(classNum);
    for (index_t i = 0; i < dataNum_; ++i)
      {
        dataIndexSet[(backMapping.find(label[i]))->second].push_back(i);
      }
    for (index_t i = 0; i < classNum; ++i)
      {
        size_t maxNum = dataIndexSet[i].size();
        for (index_t j = 0; j < eachClassNum; ++j)
          {
            index_t tmpIdx = dataIndexSet[i][random.RandI(0, maxNum)];
            sample->label[eachClassNum*i+j] = label[tmpIdx];
            for (index_t k = 0; k < dataDim_; ++k)
              {
                sample->data[eachClassNum*i+j][k] = data[tmpIdx][k];
              }
          }
      }
    return sample;
  }

  Matrix<dataT> data;
  Vector<labelT> label;
  size_t dataNum_;
  size_t dataDim_;
};

#endif // DATA_H
