#include "GaussianMixtureModel.h"
#include <iostream>

GaussianMixtureModel::GaussianMixtureModel(int dimOfGaussian, int numOfGaussian)
  :m_dimOfGaussian(dimOfGaussian), m_numOfGaussian(numOfGaussian)
{
  m_gaussian = new GaussianVector();
  m_weight = new WeightVector();
  m_foreground_state = new BoolVector();
  Gaussian *gaussian;
  for (int i = 0; i < numOfGaussian; i++)
  {
    gaussian = new Gaussian(dimOfGaussian);
    m_gaussian->push_back(gaussian);
    m_weight->push_back(0);
    m_foreground_state->push_back(i == 0);
  }
}

GaussianMixtureModel::~GaussianMixtureModel()
{
  for(GaussianVectorIterator iter = m_gaussian->begin(); iter != m_gaussian->end(); ++iter)
  {
    delete *iter;
  }
  delete m_gaussian;
  delete m_weight;
  delete m_foreground_state;
}

Gaussian * GaussianMixtureModel::GetGaussian(int index)
{
  if (index < m_numOfGaussian)
  {
    return (*m_gaussian)[index];
  }
  else
  {
    std::cout << "index out of boundary at " << __FILE__ << " : " << __LINE__  <<std::endl;
    exit(0);
  }
}

double * GaussianMixtureModel::GetMean(int index)
{
  if (index < m_numOfGaussian)
  {
    return (*m_gaussian)[index]->GetMean();
  }
  else
  {
    std::cout << "index out of boundary at " << __FILE__ << " : " << __LINE__  <<std::endl;
    exit(0);
  }
}

double * GaussianMixtureModel::GetCovariance(int index)
{
  if (index < m_numOfGaussian)
  {
    return (*m_gaussian)[index]->GetCovariance();
  }
  else
  {
    std::cout << "index out of boundary at " << __FILE__ << " : " << __LINE__  <<std::endl;
    exit(0);
  }
}

double GaussianMixtureModel::GetWeight(int index)
{
  if (index < m_numOfGaussian)
  {
    return (*m_weight)[index];
  }
  else
  {
    std::cout << "index out of boundary at " << __FILE__ << " : " << __LINE__  <<std::endl;
    exit(0);
  }
}

void GaussianMixtureModel::SetGaussian(int index, double *mean, double *covariance)
{
  if (index < m_numOfGaussian)
  {
    (*m_gaussian)[index]->SetMean(mean);
    (*m_gaussian)[index]->SetCovariance(covariance);
  }
  else
  {
    std::cout << "index out of boundary at " << __FILE__ << " : " << __LINE__  <<std::endl;
    exit(0);
  }
}

void GaussianMixtureModel::SetMean(int index, double *mean)
{
  if (index < m_numOfGaussian)
  {
    (*m_gaussian)[index]->SetMean(mean);
  }
  else
  {
    std::cout << "index out of boundary at " << __FILE__ << " : " << __LINE__  <<std::endl;
    exit(0);
  }
}

void GaussianMixtureModel::SetCovariance(int index, double *covariance)
{
  if (index < m_numOfGaussian)
  {
    (*m_gaussian)[index]->SetCovariance(covariance);
  }
  else
  {
    std::cout << "index out of boundary at " << __FILE__ << " : " << __LINE__  <<std::endl;
    exit(0);
  }
}

void GaussianMixtureModel::SetWeight(int index, double weight)
{
  if (index < m_numOfGaussian)
    {
    (*m_weight)[index] = weight;
    }
  else
    {
    std::cout << "index out of boundary at " << __FILE__ << " : " << __LINE__  <<std::endl;
    exit(0);
    }
}

void GaussianMixtureModel::SetWeightAndRenormalize(int index, double weight)
{
  // Check the range
  if (index >= m_numOfGaussian)
    {
    std::cout << "index out of boundary at " << __FILE__ << " : " << __LINE__  <<std::endl;
    exit(0);
    }

  // The weight should be clamped to the range [0 1]
  double w_curr = (*m_weight)[index];
  double w_clamp = std::max(std::min(weight, 1.0), 0.0);
  double w_scale = w_curr == 1.0 ? 0.0 : (1.0 - w_clamp) / (1.0 - w_curr);
  double w_sum = 0.0;

  // Update all the weights so the sum is still one
  for(int i = 0; i < m_numOfGaussian; i++)
    {
    if(i == index)
      {
      (*m_weight)[i] = w_clamp;
      }
    else
      {
      (*m_weight)[i] *= w_scale;
      }

    // Keet track of the sum of weights
    w_sum += (*m_weight)[i];
    }

  // There is still a possibility that the sum of weights is not 1. In that
  // case assign the difference to the next cluster
  if(w_sum < 1.0)
    (*m_weight)[(index % m_numOfGaussian)] += 1.0 - w_sum;
}

double GaussianMixtureModel::EvaluatePDF(int index, double *x)
{
  if (index < m_numOfGaussian)
  {
    return (*m_gaussian)[index]->EvaluatePDF(x);
  }
  else
  {
    std::cout << "index out of boundary at " << __FILE__ << " : " << __LINE__  <<std::endl;
    exit(0);
  }
}

double GaussianMixtureModel::EvaluateLogPDF(int index, double *x)
{
  if (index < m_numOfGaussian)
  {
    return (*m_gaussian)[index]->EvaluateLogPDF(x);
  }
  else
  {
    std::cout << "index out of boundary at " << __FILE__ << " : " << __LINE__  <<std::endl;
    exit(0);
  }
}


double GaussianMixtureModel::EvaluatePDF(int index, vnl_vector<double> &x, vnl_vector<double> &xscratch)
{
  if (index < m_numOfGaussian)
  {
    return (*m_gaussian)[index]->EvaluatePDF(x, xscratch); }
  else
  {
    std::cout << "index out of boundary at " << __FILE__ << " : " << __LINE__  <<std::endl;
    exit(0);
  }
}

double GaussianMixtureModel::EvaluateLogPDF(int index, vnl_vector<double> &x, vnl_vector<double> &xscratch)
{
  if (index < m_numOfGaussian)
  {
    return (*m_gaussian)[index]->EvaluateLogPDF(x, xscratch); }
  else
  {
    std::cout << "index out of boundary at " << __FILE__ << " : " << __LINE__  <<std::endl;
    exit(0);
  }
}


void GaussianMixtureModel::PrintParameters()
{
  int i = 0;
  GaussianVectorIterator gaussianIter;
  WeightVectorIterator weightIter;
  for(gaussianIter = m_gaussian->begin(), weightIter = m_weight->begin();
      gaussianIter != m_gaussian->end(); ++gaussianIter, ++weightIter)
  {
    std::cout << std::endl << "Gaussian Component " << ++i << ":" << std::endl;
    std::cout << "weight:" << std::endl << *weightIter << std::endl;
    (*gaussianIter)->PrintParameters();
    }
}

bool GaussianMixtureModel::IsForeground(int index)
{
  return (*m_foreground_state)[index];
}

void GaussianMixtureModel::SetForeground(int index)
{
  (*m_foreground_state)[index] = true;
}

void GaussianMixtureModel::SetBackground(int index)
{
  (*m_foreground_state)[index] = false;
}
