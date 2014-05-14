#ifndef GAUSSIAN_MIXTURE_MODEL_H
#define GAUSSIAN_MIXTURE_MODEL_H

#include "SNAPCommon.h"
#include "itkDataObject.h"
#include "itkObjectFactory.h"
#include "Gaussian.h"
#include <vector>

class GaussianMixtureModel : public itk::DataObject
{
public:
  irisITKObjectMacro(GaussianMixtureModel, itk::DataObject)

  typedef std::vector<Gaussian *> GaussianVector;
  typedef std::vector<double> WeightVector;
  typedef std::vector<bool> BoolVector;
  typedef GaussianVector::iterator GaussianVectorIterator;
  typedef WeightVector::iterator WeightVectorIterator;

  typedef Gaussian::VectorType VectorType;
  typedef Gaussian::MatrixType MatrixType;

  void Initialize(int dimOfGaussian, int numOfGaussian);
  
  Gaussian * GetGaussian(int index);
  const VectorType &GetMean(int index);
  const MatrixType &GetCovariance(int index);
  double GetWeight(int index);

  void SetGaussian(int index, const VectorType &mean, const MatrixType &cov);
  void SetMean(int index, const VectorType &mean);
  void SetCovariance(int index, const MatrixType &cov);
  void SetWeight(int index, double weight);
  void SetWeightAndRenormalize(int index, double weight);

  double EvaluateLogPDF(int index, double *x);
  double EvaluateLogPDF(int index, vnl_vector<double> &x, VectorType &xscratch);
  double EvaluatePDF(int index, double *x);


  void PrintParameters();

  int GetNumberOfGaussians() { return m_numOfGaussian; }
  int GetNumberOfComponents() { return m_dimOfGaussian; }

  /**
   * Is the cluster an object or background?
   */
  bool IsForeground(int index);

  void SetForeground(int index);
  void SetBackground(int index);
  
protected:

  GaussianMixtureModel();
  ~GaussianMixtureModel();

  int m_numOfGaussian;
  int m_dimOfGaussian;
  GaussianVector m_gaussian;
  WeightVector m_weight;
  BoolVector m_foreground_state;
};

#endif
