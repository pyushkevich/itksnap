#ifndef EM_GAUSSIAN_MIXTURES_H
#define EM_GAUSSIAN_MIXTURES_H

#include "GaussianMixtureModel.h"
#include "SNAPCommon.h"

class EMGaussianMixtures
{
public:
  EMGaussianMixtures(double **x, int dataSize, int dataDim, int numOfClass);
  ~EMGaussianMixtures();

  typedef Gaussian::MatrixType MatrixType;
  typedef Gaussian::VectorType VectorType;

  void Reset(void);
  void SetMaxIteration(int maxIteration);
  void SetPrecision(double precision);
  void SetParameters(int index,
                     const VectorType &mean,
                     const MatrixType &covariance,
                     double weight);
  void SetGaussianMixtureModel(GaussianMixtureModel *gmm);
  void SetPrior(double **prior);
  void RemovePrior(void);

  GaussianMixtureModel *GetGaussianMixtureModel() const { return m_gmm; }

  
  int GetMaxIteration(void);

  double ** Update(void);
  double ** UpdateOnce(void);
  double EvaluateLogLikelihood(void);
  void PrintParameters(void);

  static double ComputePosterior(int nGauss, double *log_pdf, double *w, double *log_w, int j);

private:
  void EvaluatePDF(void);
  void UpdateLatent(void);
  void UpdateMean(void);
  void UpdateCovariance(void);
  void UpdateWeight(void);
  
  double **m_latent;
  double **m_log_pdf;
  double **m_prior;
  double **m_x;
  double *m_probs;
  double *m_probs2;
  double *m_tmp1;
  double *m_tmp2;
  double *m_tmp3;
  double *m_sum;
  double *m_weight;
  double m_logLikelihood;
  int m_numOfGaussian;
  int m_dimOfGaussian;
  int m_maxIteration;
  int m_numOfIteration;
  int m_numOfData;
  int m_setPriorFlag;
  int m_fail;
  double m_precision;

  SmartPtr<GaussianMixtureModel> m_gmm;
};

#endif
