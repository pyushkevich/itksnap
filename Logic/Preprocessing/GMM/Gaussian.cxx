#include "Gaussian.h"
#include <iostream>
#include <vnl/vnl_math.h>
#include <vnl/vnl_trace.h>
#include <vnl/algo/vnl_symmetric_eigensystem.h>
#include <limits>

Gaussian::Gaussian(int dimension)
  :m_dimension(dimension)
{
  // Initialize the scratch buffers
  m_x_vector = VectorType(dimension);
}

const Gaussian::VectorType &Gaussian::GetMean() const
{
  assert(m_mean_vector.size());
  return m_mean_vector;
}

const Gaussian::MatrixType &Gaussian::GetCovariance() const
{
  assert(m_covariance_matrix.size());
  return m_covariance_matrix;
}

double Gaussian::GetTotalVariance()
{
  return vnl_trace(m_covariance_matrix);
}

void Gaussian::SetMean(const VectorType &mean)
{
  assert(mean.size() == m_dimension);
  m_mean_vector = mean;
}

void Gaussian::SetCovariance(const MatrixType &cov)
{
  // Validate and assign covariance matrix
  assert(cov.rows() == m_dimension && cov.cols() == m_dimension);
  m_covariance_matrix = cov;

  // Perform SVD on the covariance matrix
  vnl_symmetric_eigensystem<double> eig(m_covariance_matrix);
  m_V = eig.V;
  m_Vt = eig.V.transpose();
  m_Lambda = eig.D;

  // Compute the normalization factors for each 1D Gaussian component
  m_DiagNormFac = VectorType(m_dimension);
  for(int i = 0; i < m_dimension; i++)
    m_DiagNormFac[i] = log(2 * vnl_math::pi * m_Lambda[i]);
}

double Gaussian::EvaluateLogPDF(VectorType &x, VectorType &xscratch)
{
  // Subtract the mean from x
  for(int i = 0; i < m_dimension; i++)
    xscratch[i] = x[i] - m_mean_vector[i];

  // Compute 2*log(p(z))
  double logz = 0;
  for (int i = 0; i < m_dimension; i++)
    {
    // Compute z[i], the projection of x on the i-th eigenvector of the covariance
    // matrix. Then z[i] is a 1D normally distributed variable with variance m_Lambda[i]
    double zi = 0;
    for(int j = 0; j < m_dimension; j++)
      zi += m_Vt(i,j) * xscratch[j];

    // Should there be a tolerance for this conditional and the next?
    if(m_Lambda[i] == 0)
      {
      if(zi != 0)
        {
        // Zero variance and z[i] != 0, which means p(x) = 0, log(p(x)) = -inf
        logz = -std::numeric_limits<double>::infinity();
        break;
        }
      else
        {
        // Zero variance and z[i] == 0, which means p(z[i]) = 1, log(p(z[i])) = 0;
        continue;
        }
      }
    else
      {
      // Compute the 1D Gaussian log-probability
      logz -= m_DiagNormFac[i] + (zi * zi / m_Lambda[i]);
      }
    }

  // Final value needs to be divided by two
  return 0.5 * logz;
}

double Gaussian::EvaluatePDF(double *x)
{
  // We got to exponentiate somewhere, so might as well do it here
  double logp = this->EvaluateLogPDF(x);

  // According to POSIX definition, exp(-inf) = +0 so this is safe
  return exp(logp);
}

// TODO: better to pass in VectorType here
double Gaussian::EvaluateLogPDF(double *x)
{
  VectorType xvec(x, m_dimension);
  return this->EvaluateLogPDF(xvec, m_x_vector);
}

void Gaussian::PrintParameters()
{
  std::cout << "mean:" << std::endl;
  if (m_mean_vector.size())
    std::cout << m_mean_vector << std::endl;
  else
    std::cout << "NA" << std::endl;;
  
  std::cout << "covariance:" << std::endl;
  if (m_covariance_matrix.size())
    std::cout << m_covariance_matrix << std::endl;
  else
    std::cout << "NA" << std::endl;;
}

bool Gaussian::isDeltaFunction()
{
  return GetTotalVariance() == 0.0;
}
