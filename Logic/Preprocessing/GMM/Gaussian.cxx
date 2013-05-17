#include "Gaussian.h"
#include <iostream>

Gaussian::Gaussian(int dimension)
  :m_dimension(dimension), m_setMeanFlag(0), m_setCovarianceFlag(0), m_normalization(0), m_log_normalization(0)
{
  m_mean = new double[dimension];
  m_covariance = new double[dimension*dimension];
//  m_mean_matrix = new MatrixType(dimension, 1);
  m_mean_vector = new VectorType(dimension);
  m_covariance_matrix = new MatrixType(dimension, dimension);
  m_precision_matrix = 0;
//  m_x_matrix = new MatrixType(dimension, 1);
  m_x_vector = new VectorType(dimension);
  m_precision_matrix = new MatrixType(dimension, dimension);
}

Gaussian::Gaussian(int dimension, double *mean, double *covariance)
  :m_dimension(dimension), m_setMeanFlag(1), m_setCovarianceFlag(1)
{
  m_mean = new double[dimension];
  m_covariance = new double[dimension*dimension];
//  m_mean_matrix = new MatrixType(dimension, 1);
  m_mean_vector = new VectorType(dimension);
  m_covariance_matrix = new MatrixType(dimension, dimension);

  m_is_delta = true;
  for (int i = 0; i < dimension; i++)
  {
    m_mean[i] = mean[i];
    (*m_mean_vector)(i) = mean[i];
    for (int j = 0; j < dimension; j++)
    {
      m_covariance[i*dimension+j] = covariance[i*dimension+j];
      (*m_covariance_matrix)(i,j) = covariance[i*dimension+j];
      if(fabs(covariance[i*m_dimension+j]) > 1.e-10)
        m_is_delta = false;
    }
  }

  m_precision_matrix = new MatrixType(dimension, dimension);
  if(!m_is_delta)
    {
    (*m_precision_matrix) = vnl_matrix_inverse<double>(*m_covariance_matrix);
    m_normalization = sqrt(pow(2*M_PI, m_dimension) * vnl_determinant(*m_covariance_matrix));
    m_log_normalization = log(m_normalization);
    }

//  m_x_matrix = new MatrixType(dimension, 1);
  m_x_vector = new VectorType(dimension);
}

Gaussian::~Gaussian()
{
  delete m_mean;
  delete m_covariance;
//  delete m_mean_matrix;
  delete m_mean_vector;
  delete m_covariance_matrix;
  if (m_precision_matrix != 0)
  {
    delete m_precision_matrix;
  }
//  delete m_x_matrix;
  delete m_x_vector;
}

double * Gaussian::GetMean()
{
  if (m_setMeanFlag != 0)
  {
    return m_mean;
  }
  else
  {
    std::cout << "mean hasn't setup yet at " << __FILE__ << " : " << __LINE__  <<std::endl;
    exit(0);
  }
}

double * Gaussian::GetCovariance()
{
  if (m_setCovarianceFlag != 0)
  {
    return m_covariance;
  }
  else
  {
    std::cout << "covariance hasn't setup yet at " << __FILE__ << " : " << __LINE__  <<std::endl;
    exit(0);
  }
}

void Gaussian::SetMean(double *mean)
{
  for(int i = 0; i < m_dimension; i++)
  {
    m_mean[i] = mean[i];
    (*m_mean_vector)(i) = mean[i];
  }
  m_setMeanFlag = 1;
}

void Gaussian::SetCovariance(double *covariance)
{
  m_is_delta = true;
  for(int i = 0; i < m_dimension; i++)
  {
    for(int j = 0; j < m_dimension; j++)
    {
      m_covariance[i*m_dimension+j] = covariance[i*m_dimension+j];
      (*m_covariance_matrix)(i,j) = covariance[i*m_dimension+j];
      if(fabs(covariance[i*m_dimension+j]) > 1.e-10)
        m_is_delta = false;
    }
  }
//  m_precision_matrix_generator = new MatrixInverseType(*m_covariance_matrix);
//  m_precision_matrix = new MatrixType(m_dimension, m_dimension);

  if(!m_is_delta)
    {
    (*m_precision_matrix) = vnl_matrix_inverse<double>(*m_covariance_matrix);
    m_normalization = sqrt(pow(2*M_PI, m_dimension) * vnl_determinant(*m_covariance_matrix));
    m_log_normalization = log(m_normalization);
    m_setCovarianceFlag = 1;
    }
  else
    {
    // the Gaussian is a delta function. The code should not use the precision matrix or normalization
    }
}

#include <vnl/vnl_math.h>
double Gaussian::EvaluatePDF(double *x)
{
  for(int i = 0; i < m_dimension; i++)
  {
    (*m_x_vector)(i) = x[i] - (*m_mean_vector)(i);
    // (*m_x_matrix)(i,0) = x[i] - (*m_mean_matrix)(i,0);
  }

  if(!m_is_delta)
    {
    double t = dot_product((*m_precision_matrix) * (*m_x_vector), (*m_x_vector));
    double z = exp(-0.5 * t) / m_normalization;
    return z;
    }
  else
    {
    // Behave like a delta function
    return m_x_vector->inf_norm() == 0.0 ? vnl_huge_val(1.0) : 0.0;
    }

  /*
  return (exp(( -0.5*(*m_x_matrix).transpose()
    * (*m_precision_matrix)
		* (*m_x_matrix) )(0,0))
	  / m_normalization);
    */
}

double Gaussian::EvaluateLogPDF(double *x)
{
  for(int i = 0; i < m_dimension; i++)
  {
    (*m_x_vector)(i) = x[i] - (*m_mean_vector)(i);
  }

  if(!m_is_delta)
    {
    double t = dot_product((*m_precision_matrix) * (*m_x_vector), (*m_x_vector));
    return -0.5 * t - m_log_normalization;
    }
  else
    {
    return m_x_vector->inf_norm() == 0.0 ? vnl_huge_val(1.0) : -vnl_huge_val(1.0);
    }
}

double Gaussian::EvaluateLogPDF(VectorType &x, VectorType &xscratch)
{
  for(int i = 0; i < m_dimension; i++)
  {
    xscratch[i] = x[i] - (*m_mean_vector)(i);
  }

  if(!m_is_delta)
    {
    double t = dot_product((*m_precision_matrix) * xscratch, xscratch);
    return -0.5 * t - m_log_normalization;
    }
  else
    {
    return xscratch.inf_norm() == 0.0 ? vnl_huge_val(1.0) : -vnl_huge_val(1.0);
    }
}

double Gaussian::EvaluatePDF(VectorType &x, VectorType &xscratch)
{
  for(int i = 0; i < m_dimension; i++)
  {
    xscratch[i] = x[i] - (*m_mean_vector)(i);
  }

  if(!m_is_delta)
    {
    double t = dot_product((*m_precision_matrix) * xscratch, xscratch);
    double z = exp(-0.5 * t) / m_normalization;
    return z;
    }
  else
    {
    // Behave like a delta function
    return m_x_vector->inf_norm() == 0.0 ? vnl_huge_val(1.0) : 0.0;
    }
}


void Gaussian::PrintParameters()
{
  if (m_setMeanFlag != 0)
  {
    std::cout << "mean:" << std::endl;
    for (int i = 0; i < m_dimension; i++)
    {
      std::cout << m_mean[i] << std::endl;
    }
  }
  else
  {
    std::cout << "mean:" << std::endl << "NA" << std::endl;;
  }
  
  if (m_setCovarianceFlag != 0)
  {
    std::cout << "covariance:" << std::endl;
    for (int i = 0; i < m_dimension; i++)
    {
      for (int j = 0; j < m_dimension; j++)
      {
	std::cout << m_covariance[i*m_dimension+j];
	if (j != (m_dimension-1))
	{
	  std::cout << "  ";
	}
      }
      std::cout << std::endl;
    }
  }
  else
  {
    std::cout << "covariance:" << std::endl << "NA" << std::endl;
  }
}
