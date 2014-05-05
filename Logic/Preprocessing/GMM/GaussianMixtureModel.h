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

  void Initialize(int dimOfGaussian, int numOfGaussian);
  
  Gaussian * GetGaussian(int index);
  double * GetMean(int index);
  double * GetCovariance(int index);
  double GetWeight(int index);
  void SetGaussian(int index, double *mean, double *covariance);
  void SetMean(int index, double *mean);
  void SetCovariance(int index, double *covariance);
  void SetWeight(int index, double weight);
  void SetWeightAndRenormalize(int index, double weight);

  double EvaluateLogPDF(int index, double *x);
  double EvaluateLogPDF(int index, vnl_vector<double> &x,
                        vnl_vector<double> &xscratch,
                        vnl_vector<double> &zscratch);

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
