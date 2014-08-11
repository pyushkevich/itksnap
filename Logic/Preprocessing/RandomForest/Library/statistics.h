/**
 * Define statistics inherited from abstract Statistics class.
 */

#ifndef STATISTICS_H
#define STATISTICS_H

#define _USE_MATH_DEFINES

#include <vector>
#include <math.h>
#include "data.h"
#include "linearalgebra.h"

class Statistics
{
public:
//  Statistics();
//  Statistics(const Statistics& s);

  // aggregate a data with current statistics
  virtual void Aggregate(const DataSet& data, index_t index) = 0;

  // aggregate two statistics
  virtual void Aggregate(const Statistics& s) = 0;

  // clear current statistics
  virtual void Clear() = 0;

  virtual void Read(std::istream& is) = 0;
  virtual void Write(std::ostream& os) = 0;
};

template<class dataT, class labelT>
class Histogram: public Statistics
{
public:
  typedef MLData<dataT, labelT> MLDataT;

  Histogram()
  {
    bins_.resize(0);
    prob_.resize(0);
    sampleNum_ = 0;
  }
  Histogram(size_t classNum)
  {
    bins_.resize(classNum);
    prob_.resize(classNum);
    for(int i = 0; i < classNum; ++i)
      {
        bins_[i] = 0;
        prob_[i] = 0;
      }
    sampleNum_ = 0;
  }

  void Aggregate(const DataSet& data, index_t index)
  {
    // label must start at 0
    bins_[((const MLDataT&)data).label[index]]++;
    sampleNum_++;
  }

  void Aggregate(const Statistics& s)
  {
    const Histogram& hist = (const Histogram&)s;
    if (hist.bins_.size() != bins_.size())
      {
        throw std::runtime_error("Aggregate two histgram have different bins!");
      }
    for(int i = 0; i < bins_.size(); ++i)
      {
        bins_[i] += hist.bins_[i];
      }
    sampleNum_ += hist.sampleNum_;
  }

  void Clear()
  {
    for(int i = 0; i < bins_.size(); ++i)
      {
        bins_[i] = 0;
        prob_[i] = 0;
      }
    sampleNum_ = 0;
  }

  double Probability(index_t index) const
  {
    return (double)bins_[index] / (double)sampleNum_;
  }

  // never call this function before calling Entropy(weights) to get prob_ set
  double Entropy()
  {
    static const double ONE_OVER_LOG_2 = 3.3219280945;
    if (sampleNum_ == 0)
      {
        return 0.0;
      }
    else
      {
        double entropy = 0.0;
        for (int i = 0; i < prob_.size(); ++i)
          {
            if (prob_[i] != 0)
              {
                entropy -= prob_[i] * log(prob_[i]) * ONE_OVER_LOG_2;
              }
          }
        return entropy;
      }
  }

  double Entropy(const std::vector<double>& weights)
  {
    static const double ONE_OVER_LOG_2 = 3.3219280945;
    if (sampleNum_ == 0)
      {
        return 0.0;
      }
    else
      {
        double entropy = 0.0;
        if ( weights.empty() )
          {
            for(int i = 0; i < bins_.size(); ++i)
              {
                if (bins_[i] != 0)
                  {
                    prob_[i] = Probability(i);
                    entropy -= prob_[i] * log(prob_[i]) * ONE_OVER_LOG_2;
                  }
              }
          }
        else
          {
            double sum = 0.0;
            for(int i = 0; i < bins_.size(); ++i)
              {
                prob_[i] = weights[i] * bins_[i];
                sum += prob_[i];
              }
            if (sum != 0.0)
              {
                for(int i = 0; i < bins_.size(); ++i)
                  {
                    if (prob_[i] != 0)
                      {
                        prob_[i] = prob_[i] / sum;
                        entropy -= prob_[i] * log(prob_[i]);
                      }
                  }
              }
          }

        return entropy;
      }
  }

  void Print(int level)
  {
    static int colNum = 8;
    int i = 0;
    std::cout << "+ Statistics: hist"
              << "    sample# = " << sampleNum_;
    if ((level/10 - (level/100)*10)  == 2)
      {
        std::cout << "    [Addr: " << this << "]";
      }
    std::cout << std::endl << "  bins: ";
    for (i = 0; i < bins_.size(); ++i)
      {
        std::cout << bins_[i] << "  ";
        if ((i % colNum) == (colNum - 1))
          {
            std::cout << std::endl << "        ";
          }
      }
    if ((i % colNum) != 0)
      {
        std::cout << std::endl;
      }
  }

  bool Valid()
  {
    size_t total = 0;
    for (size_t i = 0; i < bins_.size(); ++i)
      {
        total += bins_[i];
      }
    if (total == sampleNum_)
      {
        return true;
      }
    else
      {
        return false;
      }
  }

  virtual void Read(std::istream& is)
  {
    readBasicType(is, sampleNum_);
    size_t binSize = 0;
    readBasicType(is, binSize);
    bins_.resize(binSize);
    sampleNum_ = 0;
    for (size_t i = 0; i < binSize; ++i)
      {
        readBasicType(is, bins_[i]);
        readBasicType(is, prob_[i]);
      }
  }

  virtual void Write(std::ostream& os)
  {
    if (! Valid())
      {
        std::runtime_error("Write statistics, sampleNum_ error!");
        return;
      }
    writeBasicType(os, sampleNum_);
    size_t binSize = bins_.size();
    writeBasicType(os, binSize);
    for (size_t i = 0; i < binSize; ++i)
      {
        writeBasicType(os, bins_[i]);
        writeBasicType(os, prob_[i]);
      }
  }

  std::vector<size_t> bins_;
  std::vector<double> prob_;
  size_t sampleNum_;
};

template<class dataT, class labelT>
class GaussianStat: public Statistics
{
public:
  typedef MLData<dataT, labelT> MLDataT;

  GaussianStat()
  {
    x_.resize(0);
    xSquare_.resize(0);
    meanValid_ = false;
    covValid_ = false;
    featureDim_ = 0;
    sampleNum_ = 0;
    a_ = 0.0000001;
    b_ = 1.0;
  }
  GaussianStat(size_t featureDim, double a = 0.0000001, double b = 1.0)
  {
    x_.resize(featureDim);
    xSquare_.resize(featureDim);
    for(index_t i = 0; i < featureDim; ++i)
      {
        x_[i] = 0;
        xSquare_[i].resize(featureDim);
        for (index_t j = 0; j < featureDim; ++j)
          {
            xSquare_[i][j] = 0;
          }
      }
    meanValid_ = false;
    covValid_ = false;
    featureDim_ = featureDim;
    sampleNum_ = 0;
    a_ = a < 0.0000001 ? 0.0000001 : a;
    b_ = b < 1 ? 1.0 : b;
  }

  void Aggregate(const DataSet& data, index_t index)
  {
    dataT tmp = 0;
    const std::vector<dataT>& vData = ((const MLDataT&)data).data[index];
    for (index_t i = 0; i < featureDim_; ++i)
      {
        x_[i] += vData[i];
        for (index_t j = i; j < featureDim_; ++j)
          {
            tmp = vData[i] * vData[j];
            xSquare_[i][j] += tmp;
          }
        for (index_t j = 0; j < i; ++j)
          {
            xSquare_[i][j] = xSquare_[j][i];
          }
      }
    meanValid_ = false;
    covValid_ = false;
    sampleNum_++;
  }

  void Aggregate(const Statistics& s)
  {
    const GaussianStat& gaussian = (const GaussianStat&)s;
    if (gaussian.featureDim_ != featureDim_)
      {
        throw std::runtime_error("Aggregate two GaussianStat have different dimension!");
      }
    for(index_t i = 0; i < featureDim_; ++i)
      {
        x_[i] += gaussian.x_[i];
        for(index_t j = 0; j < featureDim_; ++j)
          {
            xSquare_[i][j] += gaussian.xSquare_[i][j];
          }
      }
    meanValid_ = false;
    covValid_ = false;
    sampleNum_ += gaussian.sampleNum_;
  }

  void CalculateMean()
  {
    mean_.resize(featureDim_);
    for (index_t i = 0; i < featureDim_; ++i)
      {
        mean_[i] = x_[i] / sampleNum_;
      }
    meanValid_ = true;
  }

  void CalculateCov()
  {
    double alpha = sampleNum_ / (sampleNum_ + a_);
    double beta = (1 - alpha) * b_;
    cov_.resize(featureDim_);
    L_.resize(featureDim_);
    for (index_t i = 0; i < featureDim_; ++i)
      {
        cov_[i].resize(featureDim_);
        L_[i].resize(featureDim_);
        for (index_t j = i; j < featureDim_; ++j)
          {
            cov_[i][j] = xSquare_[i][j] / sampleNum_
                - (x_[i] * x_[j]) / (sampleNum_ * sampleNum_);
//            cov_[i][j] = xSquare_[i][j] / (sampleNum_ - 1)
//                - (x_[i] * x_[j]) / (sampleNum_ * (sampleNum_ - 1));
            if (i == j)
              {
                cov_[i][j] = alpha * cov_[i][j] + beta;
              }
          }
        for (index_t j = 0; j < i; ++j)
          {
            cov_[i][j] = cov_[j][i];
          }
      }
    cholesky(cov_, L_);
    covValid_ = true;
  }

  double Entropy(const std::vector<double>& weights)
  {
    if (sampleNum_ == 0)
      {
        return 0.0;
      }
    else
      {
        if ( ! covValid_ )
          {
            CalculateCov();
          }
        return (0.5 * log(pow(2*M_PI*M_E, featureDim_)
                          * spddeterminant(L_, true)));
      }
  }

  double Pdf(std::vector<dataT> X)
  {
    if ( ! meanValid_ )
      {
        CalculateMean();
      }
    if ( ! covValid_ )
      {
        CalculateCov();
      }
    if (X.size() != mean_.size())
      {
        throw std::runtime_error("GaussianStat::Pdf: XSize != meanSize\n");
      }
    for (index_t i = 0; i < X.size(); ++i)
      {
        X[i] -= mean_[i];
      }
    return (sqrt(1.0 / (pow(2 * M_PI, featureDim_) * spddeterminant(L_, true)))
            * exp(-0.5 * multiplyXTspdAinvX(X, L_, true)));
  }

  void Clear()
  {
    bool hasMean = (mean_.size() != 0);
    bool hasCov = (cov_.size() != 0);
    for(index_t i = 0; i < featureDim_; ++i)
      {
        x_[i] = 0;
        if (hasMean)
          {
            mean_[i] = 0;
          }
        for (index_t j = 0; j < featureDim_; ++j)
          {
            xSquare_[i][j] = 0;
            if (hasCov)
              {
                cov_[i][j] = 0;
                L_[i][j] = 0;
              }
          }
      }
    meanValid_ = false;
    covValid_ = false;
    sampleNum_ = 0;
  }

  void Print(int level)
  {
    std::cout << "+ Statistics: gaussian"
              << "    sample# = " << sampleNum_;
    if ((level/10 - (level/100)*10)  == 2)
      {
        std::cout << "    [Addr: " << this << "]";
      }
    std::cout << std::endl << "a = " << a_ << " , b = " << b_ << std::endl;
    if (meanValid_)
      {
        std::cout << "  mean: " << std::endl;
        for (index_t i = 0; i < featureDim_; ++i)
          {
            std::cout << mean_[i] << "  ";
          }
        std::cout << std::endl;
      }
    else
      {
        std::cout << "  x: " << std::endl;
        for (index_t i = 0; i < featureDim_; ++i)
          {
            std::cout << x_[i] << "  ";
          }
        std::cout << std::endl;
      }
    if (covValid_)
      {
        std::cout << "  covariance: " << std::endl;
        for (index_t i = 0; i < featureDim_; ++i)
          {
            for (index_t j = 0; j < featureDim_; ++j)
              {
                std::cout << cov_[i][j] << "  ";
              }
            std::cout << std::endl;
          }
      }
    else
      {
        std::cout << "  xSquare: " << std::endl;
        for (index_t i = 0; i < featureDim_; ++i)
          {
            for (index_t j = 0; j < featureDim_; ++j)
              {
                std::cout << xSquare_[i][j] << "  ";
              }
            std::cout << std::endl;
          }
      }
  }

  virtual void Read(std::istream& is)
  {
    readBasicType(is, featureDim_);
    readBasicType(is, sampleNum_);
    readBasicType(is, a_);
    readBasicType(is, b_);
    readBasicType(is, meanValid_);
    readBasicType(is, covValid_);
    for (index_t i = 0; i < featureDim_; ++i)
      {
        readBasicType(is, x_[i]);
      }
    for (index_t i = 0; i < featureDim_; ++i)
      {
        for (index_t j = 0; j < featureDim_; ++j)
          {
            readBasicType(is, xSquare_[i]);
          }
      }
    if (meanValid_)
      {
        for (index_t i = 0; i < featureDim_; ++i)
          {
            readBasicType(is, mean_[i]);
          }
      }
    if (covValid_)
      {
        for (index_t i = 0; i < featureDim_; ++i)
          {
            for (index_t j = 0; j < featureDim_; ++j)
              {
                readBasicType(is, cov_[i]);
              }
          }
      }
  }

  virtual void Write(std::ostream& os)
  {
    writeBasicType(os, featureDim_);
    writeBasicType(os, sampleNum_);
    writeBasicType(os, a_);
    writeBasicType(os, b_);
    writeBasicType(os, meanValid_);
    writeBasicType(os, covValid_);
    for (index_t i = 0; i < featureDim_; ++i)
      {
        writeBasicType(os, x_[i]);
      }
    for (index_t i = 0; i < featureDim_; ++i)
      {
        for (index_t j = 0; j < featureDim_; ++j)
          {
            writeBasicType(os, xSquare_[i]);
          }
      }
    if (meanValid_)
      {
        for (index_t i = 0; i < featureDim_; ++i)
          {
            writeBasicType(os, mean_[i]);
          }
      }
    if (covValid_)
      {
        for (index_t i = 0; i < featureDim_; ++i)
          {
            for (index_t j = 0; j < featureDim_; ++j)
              {
                writeBasicType(os, cov_[i]);
              }
          }
      }
  }

  std::vector<dataT> x_;
  std::vector<std::vector<dataT> > xSquare_;
  std::vector<dataT> mean_;
  std::vector<std::vector<dataT> > cov_;
  std::vector<std::vector<dataT> > L_;
  bool meanValid_;
  bool covValid_;
  size_t featureDim_;
  size_t sampleNum_;
  double a_;
  double b_;
};

#endif // STATISTICS_H
