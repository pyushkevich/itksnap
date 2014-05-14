#include "EMGaussianMixtures.h"
#include <iostream>
#include <ctime>

EMGaussianMixtures::EMGaussianMixtures(double **x, int dataSize, int dataDim, int numOfClass)
  :m_x(x), m_numOfData(dataSize), m_dimOfGaussian(dataDim), m_numOfGaussian(numOfClass), m_setPriorFlag(0), m_numOfIteration(0), m_fail(0)
{
  m_latent = new double*[dataSize];
  m_probs = new double[dataSize*numOfClass];
  for (int i = 0; i < dataSize; i++)
    {
    m_latent[i] = &m_probs[i*numOfClass];
    }
  m_log_pdf = new double*[dataSize];
  m_probs2 = new double[dataSize*numOfClass];
  for (int i = 0; i < dataSize; i++)
    {
    m_log_pdf[i] = &m_probs2[i*numOfClass];
    }
  m_tmp1 = new double[numOfClass];
  m_tmp2 = new double[dataDim];
  m_tmp3 = new double[dataDim*dataDim];
  m_sum = new double[numOfClass];
  m_weight = new double[numOfClass];

  m_gmm = GaussianMixtureModel::New();
  m_gmm->Initialize(dataDim, numOfClass);

  m_maxIteration = 30;
  m_precision = 1.0e-7;
  m_logLikelihood = std::numeric_limits<double>::infinity();
}

EMGaussianMixtures::~EMGaussianMixtures()
{
  delete m_probs;
  delete m_probs2;
  delete m_latent;
  delete m_log_pdf;
  delete m_tmp1;
  delete m_tmp2;
  delete m_tmp3;
  delete m_sum;
  delete m_weight;
}

void EMGaussianMixtures::Reset(void)
{
  m_numOfIteration = 0;
  m_fail = 0;
  m_logLikelihood = std::numeric_limits<double>::infinity();
  for (int i = 0; i < m_numOfData*m_numOfGaussian; i++)
    {
    m_probs[i] = 0;
    m_probs2[i] = 0;
    }
}

void EMGaussianMixtures::SetMaxIteration(int maxIteration)
{
  m_maxIteration = maxIteration;
}

void EMGaussianMixtures::SetPrecision(double precision)
{
  m_precision = precision;
}

void EMGaussianMixtures::SetParameters(int index, const VectorType &mean, const MatrixType &covariance, double weight)
{
  m_gmm->SetGaussian(index, mean, covariance);
  m_gmm->SetWeight(index, weight);
}

void EMGaussianMixtures::SetGaussianMixtureModel(GaussianMixtureModel *gmm)
{
  for (int i = 0; i < m_numOfGaussian; i++)
    {
    m_gmm->SetGaussian(i, gmm->GetMean(i), gmm->GetCovariance(i));
    m_gmm->SetWeight(i, gmm->GetWeight(i));
    if(gmm->IsForeground(i))
      m_gmm->SetForeground(i);
    else
      m_gmm->SetBackground(i);
    }
}

void EMGaussianMixtures::SetPrior(double **prior)
{
  m_prior = prior;
  m_setPriorFlag = 1;
}

void EMGaussianMixtures::RemovePrior(void)
{
  m_prior = 0;
  m_setPriorFlag = 0;
}

int EMGaussianMixtures::GetMaxIteration(void)
{
  return m_maxIteration;
}

double ** EMGaussianMixtures::Update(void)
{
  double currentLogLikelihood = 0;
  m_numOfIteration = 0;
  m_fail = 0;
  while ((fabs(m_logLikelihood - currentLogLikelihood) > m_precision) && (m_numOfIteration < m_maxIteration))
    {
    if (m_logLikelihood < currentLogLikelihood)
      {
      m_fail = 1;
      std::cout << "!!!!!! Log Likelihood increase, EM fails" << std::endl;
      std::cout << "old=" <<m_logLikelihood << std::endl << "new=" << currentLogLikelihood << std::endl;
      // break;
      }
    ++m_numOfIteration;
    m_logLikelihood = currentLogLikelihood;
    EvaluatePDF();
    currentLogLikelihood = EvaluateLogLikelihood();
    UpdateLatent();
    UpdateMean();
    UpdateCovariance();
    if (m_setPriorFlag == 0)
      {
      UpdateWeight();
      }

    std::cout << std::endl <<"=====================" << std::endl;
    std::cout << "After " << m_numOfIteration << " Iteration:" << std::endl;
    std::cout << "log likelihood:" << std::endl << m_logLikelihood << std::endl;
    PrintParameters();
    //getchar();
    }
  return m_latent;
}

double ** EMGaussianMixtures::UpdateOnce(void)
{
  long start = 0;
  long end = 0;
  start = clock();
  EvaluatePDF();
  end = clock();
  std::cout << "evaluate pdf spending " << (end-start)/1000 << std::endl;
  start = clock();
  double currentLogLikelihood = EvaluateLogLikelihood();
  end = clock();
  std::cout << "evaluate likelihood spending " << (end-start)/1000 << std::endl;
  if (m_logLikelihood < currentLogLikelihood)
    {
    m_fail = 1;
    std::cout << "!!!!!! Log Likelihood increase, EM fails" << std::endl;
    std::cout << "old=" <<m_logLikelihood << std::endl << "new=" << currentLogLikelihood << std::endl;
    }
  if (fabs(m_logLikelihood - currentLogLikelihood) <= m_precision)
    {
    std::cout << "Log Likelihood converged" << std::endl;
    }
  if (m_numOfIteration >= m_maxIteration)
    {
    std::cout << "Reach the maximum iteration number" << std::endl;
    }
  ++m_numOfIteration;
  m_logLikelihood = currentLogLikelihood;
  
  start = clock();
  UpdateLatent();
  end = clock();
  std::cout << "latent spending " << (end-start)/1000 << std::endl;
  start = clock();
  UpdateMean();
  end = clock();
  std::cout << "mean spending " << (end-start)/1000 << std::endl;
  start = clock();
  UpdateCovariance();
  end = clock();
  std::cout << "covariance spending " << (end-start)/1000 << std::endl;
  if (m_setPriorFlag == 0)
    {
    start = clock();
    UpdateWeight();
    end = clock();
    std::cout << "weight spending " << (end-start)/1000 << std::endl;
    }

  std::cout << std::endl <<"=====================" << std::endl;
  std::cout << "After " << m_numOfIteration << " Iteration:" << std::endl;
  std::cout << "log likelihood:" << std::endl << m_logLikelihood << std::endl;
  PrintParameters();
  //getchar();
  return m_latent;
}

void EMGaussianMixtures::EvaluatePDF(void)
{
  for (int i = 0; i < m_numOfData; i++)
    {
    for (int j = 0; j < m_numOfGaussian; j++)
      {
      m_log_pdf[i][j] = m_gmm->EvaluateLogPDF(j, m_x[i]);
      }
    }
  if (m_setPriorFlag == 0)
    {
    for (int j = 0; j < m_numOfGaussian; j++)
      {
      m_weight[j] = m_gmm->GetWeight(j);
      }
    }
}

#include <vnl/vnl_math.h>

double EMGaussianMixtures::ComputePosterior(int nGauss, double *log_pdf, double *w, double *log_w, int j)
{
  // Instead of directly computing the expression
  //   latent[i][j] = w[j] * N(x_i; m_j, Sigma_j) / Sum_k[w[k] * N(x_i; m_k, Sigma_k)]
  // which is equivalently
  //   latent[i][j] = exp(a_j) / Sum_k[ exp(a_k) ]
  // where
  //   a_j = log( w[j] * N(x_i; m_j, Sigma_j) )
  // we compute
  //   latent[i][j] = 1 / (1 + Sum_(k!=j)[ exp(a_k - a_j) ])
  // which is numerically stable

  // If the weight of the class is zero, the posterior is automatically zero
  if(w[j] == 0)
    return 0;

  // We are computing m_latent[i][j]
  double denom = 1.0;

  // The log of w[j] * pdf[j];
  double exp_j = (log_w[j] + log_pdf[j]);
  for (int k = 0; k < nGauss; k++)
    {
    if(j != k && w[k] > 0)
      {
      // The log of (w[k] * pdf[k]) / (w[j] * pdf[j])
      double exponent = (log_w[k] + log_pdf[k]) - exp_j;
      if(exponent < -20)
        {
        // (w[k] * pdf[k]) / (w[j] * pdf[j]) is effectively zero
        continue;
        }
      else if(exponent > 20)
        {
        // latent[i][j] is effectively zero
        denom = vnl_huge_val(1.0);
        break;
        }
      else
        {
        denom += exp(exponent);
        }
      }
    }

  // Now compute 1/denom
  double post = 1.0 / denom;
  return post;
}

void EMGaussianMixtures::UpdateLatent(void)
{
  double sum = 0;
  for (int i = 0; i < m_numOfGaussian; i++)
    {
    m_sum[i] = 0;
    }

  // Compute log of the weights and store in logw
  vnl_vector<double> logw(m_numOfGaussian);
  for(int i = 0; i < m_numOfGaussian; i++)
    logw(i) = log(m_weight[i]);
  
  if (m_setPriorFlag == 0)
    {
    for (int i = 0; i < m_numOfData; i++)
      {
      for (int j = 0; j < m_numOfGaussian; j++)
        {
        m_latent[i][j] = ComputePosterior(m_numOfGaussian, m_log_pdf[i], m_weight, logw.data_block(), j);
        m_sum[j] += m_latent[i][j];
        }


      /*
      sum = 0;
      for (int j = 0; j < m_numOfGaussian; j++)
        {
        m_tmp1[j] = m_weight[j] * m_pdf[i][j];
        sum += m_tmp1[j];
        }
      if(sum == 0)
        {
        for (int j = 0; j < m_numOfGaussian; j++)
          {
          m_latent[i][j] = 1.0 / m_numOfGaussian;
          m_sum[j] += m_latent[i][j];
          }
        }
      else
        {
        for (int j = 0; j < m_numOfGaussian; j++)
          {
          m_latent[i][j] = m_tmp1[j] / sum;
          m_sum[j] += m_latent[i][j];
          }
        }
        */
      }
    }
  else
    {
    /**= THIS IS NOT USED
    for (int i = 0; i < m_numOfData; i++)
      {
      sum = 0;
      for (int j = 0; j < m_numOfGaussian; j++)
        {
        m_tmp1[j] = m_prior[i][j] * m_pdf[i][j];
        sum += m_tmp1[j];
        }
      for (int j = 0; j < m_numOfGaussian; j++)
        {
        m_latent[i][j] = m_tmp1[j] / sum;
        m_sum[j] += m_latent[i][j];
        }
      }
      */
    }
}

void EMGaussianMixtures::UpdateMean(void)
{
  for (int i = 0; i < m_numOfGaussian; i++)
    {
    for (int j = 0; j < m_dimOfGaussian; j++)
      {
      m_tmp2[j] = 0;
      }
    
    for (int j = 0; j < m_numOfData; j++)
      {
      for (int k = 0; k < m_dimOfGaussian; k++)
        {
        m_tmp2[k] += m_latent[j][i] * m_x[j][k];
        }
      }

    // This can lead to a possible divide by zero situation. In case the sum
    // of latent variables for class i is zero, we set the mean of that class
    // to infinity
    if(m_sum[i] > 0)
      {
      for (int j = 0; j < m_dimOfGaussian; j++)
        {
        m_tmp2[j] = m_tmp2[j] / m_sum[i];
        }
      }
    else
      {
      for (int j = 0; j < m_dimOfGaussian; j++)
        {
        m_tmp2[j] = - std::numeric_limits<double>::infinity();
        }
      }


    m_gmm->SetMean(i, VectorType(m_tmp2, m_dimOfGaussian));
    }
}

void EMGaussianMixtures::UpdateCovariance(void)
{
  for (int i = 0; i < m_numOfGaussian; i++)
    {
    const VectorType &current_mean = m_gmm->GetMean(i);
    
    for (int j = 0; j < m_dimOfGaussian; j++)
      {
      for (int k = 0; k < m_dimOfGaussian; k++)
        {
        m_tmp3[j*m_dimOfGaussian+k] = 0;
        }
      }
    
    for (int j = 0; j < m_numOfData; j++)
      {
      for (int k = 0; k < m_dimOfGaussian; k++)
        {
        m_tmp2[k] = m_x[j][k] - current_mean[k];
        }
      for (int k = 0; k < m_dimOfGaussian; k++)
        {
        for (int l = 0; l < m_dimOfGaussian; l++)
          {
          m_tmp3[k*m_dimOfGaussian+l] += m_tmp2[k] * m_tmp2[l] * m_latent[j][i];
          }
        }
      }

    if(m_sum[i] > 0)
      {
      for (int j = 0; j < m_dimOfGaussian*m_dimOfGaussian; j++)
        {
        m_tmp3[j] = m_tmp3[j] / m_sum[i];
        }
      }
    else
      {
      for (int j = 0; j < m_dimOfGaussian*m_dimOfGaussian; j++)
        {
        m_tmp3[j] = 0.0;
        }
      }


    m_gmm->SetCovariance(i, MatrixType(m_tmp3, m_dimOfGaussian, m_dimOfGaussian));
    }
}

void EMGaussianMixtures::UpdateWeight(void)
{
  for (int i = 0; i < m_numOfGaussian; i++)
    {
    m_gmm->SetWeight(i, m_sum[i]/m_numOfData);
    }
}

double EMGaussianMixtures::EvaluateLogLikelihood(void)
{
  double tmp1 = 0;
  double tmp2 = 0;
  if (m_setPriorFlag == 0)
    {
    for (int i = 0; i < m_numOfData; i++)
      {
      tmp1 = 0;
      for (int j = 0; j < m_numOfGaussian; j++)
        {
        if(!m_gmm->GetGaussian(j)->isDeltaFunction())
          tmp1 += m_weight[j] * exp(m_log_pdf[i][j]);
        }
      tmp2 += log(tmp1);
      }
    }
  else
    {
    for (int i = 0; i < m_numOfData; i++)
      {
      tmp1 = 0;
      for (int j = 0; j < m_numOfGaussian; j++)
        {
        if(!m_gmm->GetGaussian(j)->isDeltaFunction())
          tmp1 += m_prior[i][j] * exp(m_log_pdf[i][j]);
        }
      tmp2 += log(tmp1);
      }
    }
  return tmp1;
}

void EMGaussianMixtures::PrintParameters(void)
{
  m_gmm->PrintParameters();
}
