/**
 * Define some weak classifiers inherited from abstract Classifier class.
 */

#ifndef CLASSIFIER_H
#define CLASSIFIER_H

#include <math.h>
#include "data.h"
#include "random.h"

template<class C, class dataT, class labelT>
class Classifier
{
public:
  // randomly get a classifier
  virtual C RandomClassifier(Random& random) = 0;

  // calculate feature's response for this classifier
  virtual double FeatureResponse(const DataSet& data, index_t index) const = 0;

  // calculate boolean response for this classifier and threshold
  bool Response(const DataSet& data, index_t index)
  {
    return FeatureResponse(data, index) < threshold_? true:false;
  }

  virtual void Read(std::istream& is) = 0;
  virtual void Write(std::ostream& os) = 0;

  double threshold_;
};

template<class dataT, class labelT>
class AxisAlignedClassifier :
    public Classifier<AxisAlignedClassifier<dataT, labelT>, dataT, labelT>
{
public:
  typedef Classifier<AxisAlignedClassifier<dataT, labelT>, dataT, labelT> ClassifierT;
  typedef AxisAlignedClassifier<dataT, labelT> AxisAlignedClassifierT;
  typedef MLData<dataT, labelT> MLDataT;

  AxisAlignedClassifier(): featureDim_(-1), axis_(-1) {}
  AxisAlignedClassifier(int dimension): featureDim_(dimension), axis_(-1) {}
  AxisAlignedClassifier(int dimension, int axis): featureDim_(dimension), axis_(axis) {}
  AxisAlignedClassifierT RandomClassifier(Random &random)
  {
    return AxisAlignedClassifierT(featureDim_, random.RandI(0, featureDim_));
  }

  double FeatureResponse(const DataSet& data, index_t index) const
  {
    return ((const MLDataT&)data).data[index][axis_];
  }

  void Print(int level)
  {
    std::cout << "- Classifier: axis"
              << "    dim = " << featureDim_
              << "    axis = " << axis_
              << "    threshold = " << ClassifierT::threshold_;
    if ((level/100 - (level/1000)*10)  == 2)
      {
        std::cout << "    [Addr: " <<  this << "]";
      }
    std::cout << std::endl;
  }

  virtual void Read(std::istream& is)
  {
    readBasicType(is, featureDim_);
    readBasicType(is, axis_);
    readBasicType(is, ClassifierT::threshold_);
  }

  virtual void Write(std::ostream& os)
  {
    writeBasicType(os, featureDim_);
    writeBasicType(os, axis_);
    writeBasicType(os, ClassifierT::threshold_);
  }

  int featureDim_;
  int axis_;
};

template<class dataT, class labelT>
class LinearClassifier
    : public Classifier<LinearClassifier<dataT, labelT>, dataT, labelT>
{
public:
  typedef Classifier<LinearClassifier<dataT, labelT>, dataT, labelT> ClassifierT;
  typedef LinearClassifier<dataT, labelT> LinearClassifierT;
  typedef MLData<dataT, labelT> MLDataT;

  LinearClassifier(): featureDim_(-1) {}
  LinearClassifier(int dimension): featureDim_(dimension)
  {
    unitVector_.resize(dimension);
  }
  LinearClassifier(int dimension, std::vector<dataT>& unitVector)
    : featureDim_(dimension), unitVector_(unitVector) {}

  LinearClassifierT RandomClassifier(Random &random)
  {
    double length = 0;
    for (int i = 0; i < featureDim_; ++i)
      {
        unitVector_[i] = 2.0 * random.RandD() - 1.0;
        length += unitVector_[i] * unitVector_[i];
      }
    length = sqrt(length);
    for (int i = 0; i < featureDim_; ++i)
      {
        unitVector_[i] = unitVector_[i] / length;
      }
    return LinearClassifierT(featureDim_, unitVector_);
  }

  double FeatureResponse(const DataSet &data, index_t index) const
  {
    double dotProduct = 0;
    for (int i = 0; i < featureDim_; ++i)
      {
        dotProduct += ((const MLDataT&)data).data[index][i] * unitVector_[i];
      }
    return dotProduct;
  }

  void Print(int level)
  {
    std::cout << "- Classifier: linear"
              << "    dim = " << featureDim_
              << "    unit vector = [";
    for (int i = 0; i < featureDim_; ++i)
      {
        std::cout << unitVector_[i];
        if (i != (featureDim_-1))
          {
            std::cout << "  ";
          }
        else
          {
            std::cout << "]";
          }
      }
    std::cout << "    threshold = " << ClassifierT::threshold_;
    if ((level/100 - (level/1000)*10)  == 2)
      {
        std::cout << "    [Addr: " <<  this << "]";
      }
    std::cout << std::endl;
  }

  virtual void Read(std::istream& is)
  {
    readBasicType(is, featureDim_);
    unitVector_.resize(featureDim_);
    for (int i = 0; i < featureDim_; ++i)
      {
        readBasicType(is, unitVector_[i]);
      }
    readBasicType(is, ClassifierT::threshold_);
  }

  virtual void Write(std::ostream& os)
  {
    writeBasicType(os, featureDim_);
    for (int i = 0; i < featureDim_; ++i)
      {
        writeBasicType(os, unitVector_[i]);
      }
    writeBasicType(os, ClassifierT::threshold_);
  }

  int featureDim_;
  std::vector<dataT> unitVector_;
};

#endif // CLASSIFIER_H
