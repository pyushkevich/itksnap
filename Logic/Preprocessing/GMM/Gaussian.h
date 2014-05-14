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

  const VectorType &GetMean() const;
  const MatrixType &GetCovariance() const;
  double GetTotalVariance();

  void SetMean(const VectorType &mean);
  void SetCovariance(const MatrixType &cov);

  double EvaluatePDF(double *x);
  double EvaluateLogPDF(double *x);

  // Evaluate log PDF with user-provided scratch buffer
  double EvaluateLogPDF(VectorType &x, VectorType &xscratch);

  void PrintParameters();

  // Tests whether the Gaussian is a delta function (i.e., has zero total variance)
  bool isDeltaFunction();


private:
  int m_dimension;

  MatrixType m_covariance_matrix;
  VectorType m_mean_vector;

  // SVD of the covariance matrix
  MatrixType m_V, m_Vt;
  vnl_diag_matrix<double> m_Lambda;
  VectorType m_DiagNormFac;

  // Mean-subtracted and rotated x vector; PCA-normalized z-vector
  // these vectors are used to avoid memory allocation
  VectorType m_x_vector;
};

#endif
