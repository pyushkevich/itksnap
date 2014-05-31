/**
 * Define different traning context inherited from abstract TrainingContext class
 * by instantiating different statistics and classifiers.
 */

#ifndef TRAININGCONTEXT_H
#define TRAININGCONTEXT_H

#include "classifier.h"
#include "statistics.h"

struct TrainingParameters
{
  size_t treeNum;
  size_t treeDepth;
  size_t candidateNodeClassifierNum;
  size_t candidateClassifierThresholdNum;
  std::vector<double> weights;
  double subSamplePercent;
  double splitIG;
  double leafEntropy;
  bool verbose;
};

template<class S, class C>
class TrainingContext
{
public:
  // randomly get a classifier
  virtual C RandomClassifier(Random& random) = 0;

  // get an object of statistics
  virtual S Statistics() = 0;

  // compute information gain
  virtual double ComputeIG(S& parent, S& leftChild, S& rightChild,
                           const std::vector<double>& weights) = 0;
};

template<class C, class dataT, class labelT>
class ClassificationContext : public TrainingContext<Histogram<dataT, labelT>, C>
{
public:
  ClassificationContext(int featureDim, int classNum): classNum_(classNum)
  {
    classifier_ = new C(featureDim);
  }
  ~ClassificationContext()
  {
    delete classifier_;
  }

  //// law of four here???

  C RandomClassifier(Random &random)
  {
    return classifier_->RandomClassifier(random);
  }

  Histogram<dataT, labelT> Statistics()
  {
    return Histogram<dataT, labelT>(classNum_);
  }

  double ComputeIG(Histogram<dataT, labelT>& parent,
                   Histogram<dataT, labelT>& leftChild,
                   Histogram<dataT, labelT>& rightChild,
                   const std::vector<double>& weights)
  {
    std::size_t pSampleNum = parent.sampleNum_;
    std::size_t lSampleNum = leftChild.sampleNum_;
    std::size_t rSampleNum = rightChild.sampleNum_;
    if ((lSampleNum == 0) || (rSampleNum == 0))
      {
        return 0.0;
      }
    else if ((lSampleNum + rSampleNum) != pSampleNum) {
        throw std::runtime_error("ComputeIG sampleNum error!");
      }
    else
      {
        return (parent.Entropy(weights) -
                (lSampleNum * leftChild.Entropy(weights) + rSampleNum * rightChild.Entropy(weights))/pSampleNum);
      }
  }

  C* classifier_;
  std::size_t classNum_;
};

template<class C, class dataT, class labelT>
class DensityEstimationContext : public TrainingContext<GaussianStat<dataT, labelT>, C>
{
public:
  DensityEstimationContext(int featureDim): featureDim_(featureDim)
  {
    classifier_ = new C(featureDim);
  }
  ~DensityEstimationContext()
  {
    delete classifier_;
  }

  C RandomClassifier(Random &random)
  {
    return classifier_->RandomClassifier(random);
  }

  GaussianStat<dataT, labelT> Statistics()
  {
    return GaussianStat<dataT, labelT>(featureDim_);
  }

  double ComputeIG(Histogram<dataT, labelT>& parent,
                   Histogram<dataT, labelT>& leftChild,
                   Histogram<dataT, labelT>& rightChild,
                   const std::vector<double>& weights)
  {
    std::size_t pSampleNum = parent.sampleNum_;
    std::size_t lSampleNum = leftChild.sampleNum_;
    std::size_t rSampleNum = rightChild.sampleNum_;
    if ((lSampleNum == 0) || (rSampleNum == 0))
      {
        return 0.0;
      }
    else if ((lSampleNum + rSampleNum) != pSampleNum) {
        throw std::runtime_error("ComputeIG sampleNum error!");
      }
    else
      {
        return (parent.Entropy(weights) -
                (lSampleNum * leftChild.Entropy(weights) + rSampleNum * rightChild.Entropy(weights))/pSampleNum);
      }
  }

  C* classifier_;
  std::size_t featureDim_;
};

#endif // TRAININGCONTEXT_H
