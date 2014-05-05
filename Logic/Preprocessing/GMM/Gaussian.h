#ifndef GAUSSIAN_H
#define GAUSSIAN_H

#include <vnl/vnl_matrix.h>
#include <vnl/algo/vnl_matrix_inverse.h>
#include <vnl/algo/vnl_determinant.h>

class Gaussian
{
public:
  typedef vnl_matrix<double> MatrixType;
  typedef vnl_vector<double> VectorType;
  
  Gaussian(int dimension);
  Gaussian(int dimension, double *mean, double *covariance);
  ~Gaussian();
  double * GetMean();
  double * GetCovariance();
  double GetCovarianceComponent(int row, int col);
  double GetTotalVariance();
  void SetMean(double *mean);
  void SetCovariance(double *covariance);
  void SetParameters(double *mean, double *covariance);


  double EvaluateLogPDF(double *x);
  double EvaluateLogPDF(VectorType &x, VectorType &xscratch, VectorType &zscratch);

  double EvaluatePDF(double *x);
  void PrintParameters();

  bool isDeltaFunction();

  //To do random number, check SPD

private:
  int m_dimension;
  int m_setMeanFlag;
  int m_setCovarianceFlag;
  double *m_mean;
  double *m_covariance;
  double m_normalization, m_log_normalization;
  bool m_is_delta;
//  MatrixType *m_mean_matrix;
  MatrixType *m_covariance_matrix;
  MatrixType *m_precision_matrix;
//  MatrixType *m_x_matrix;
  VectorType *m_x_vector;
  VectorType *m_mean_vector;

  // SVD of the covariance matrix
  MatrixType m_V, m_Vt;
  vnl_diag_matrix<double> m_Lambda;
  VectorType m_DiagNormFac;

  // Mean-subtracted and rotated x vector
  VectorType m_z_vector;


};

#endif
