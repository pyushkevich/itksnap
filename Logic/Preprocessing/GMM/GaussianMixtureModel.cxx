#include "GaussianMixtureModel.h"
#include <iostream>
#include <algorithm>

GaussianMixtureModel::GaussianMixtureModel()
  :m_dimOfGaussian(0), m_numOfGaussian(0)
{
}

GaussianMixtureModel::~GaussianMixtureModel()
{
  // Delete all of the existing Gaussians
  for(GaussianVectorIterator iter = m_gaussian.begin(); iter != m_gaussian.end(); ++iter)
    delete *iter;
}

void GaussianMixtureModel::Initialize(int dimOfGaussian, int numOfGaussian)
{
  m_dimOfGaussian = dimOfGaussian;
  m_numOfGaussian = numOfGaussian;

  // Delete all of the existing Gaussians
  for(GaussianVectorIterator iter = m_gaussian.begin(); iter != m_gaussian.end(); ++iter)
    delete *iter;

  // Allocate the new clusters
  m_gaussian.resize(numOfGaussian, NULL);
  m_weight.resize(numOfGaussian, 0);
  m_foreground_state.resize(numOfGaussian, 0);

  for (int i = 0; i < numOfGaussian; i++)
    m_gaussian[i] = new Gaussian(dimOfGaussian);

  // Mark the first cluster as foreground
  if(numOfGaussian > 0)
    m_foreground_state[0] = 1;
}

Gaussian * GaussianMixtureModel::GetGaussian(int index)
{
  assert(index < m_numOfGaussian);
  return m_gaussian[index];
}

const GaussianMixtureModel::VectorType &
GaussianMixtureModel::GetMean(int index)
{
  assert(index < m_numOfGaussian);
  return m_gaussian[index]->GetMean();
}

const GaussianMixtureModel::MatrixType &
GaussianMixtureModel::GetCovariance(int index)
{
  assert(index < m_numOfGaussian);
  return m_gaussian[index]->GetCovariance();
}

double GaussianMixtureModel::GetWeight(int index)
{
  assert(index < m_numOfGaussian);
  return m_weight[index];
}

void GaussianMixtureModel::SetGaussian(int index, const VectorType &mean, const MatrixType &cov)
{
  assert(index < m_numOfGaussian);
  m_gaussian[index]->SetMean(mean);
  m_gaussian[index]->SetCovariance(cov);
}

void GaussianMixtureModel::SetMean(int index, const VectorType &mean)
{
  assert(index < m_numOfGaussian);
  m_gaussian[index]->SetMean(mean);
}

void GaussianMixtureModel::SetCovariance(int index, const MatrixType &cov)
{
  assert(index < m_numOfGaussian);
  m_gaussian[index]->SetCovariance(cov);
}

void GaussianMixtureModel::SetWeight(int index, double weight)
{
  assert(index < m_numOfGaussian);
  m_weight[index] = weight;
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
  double w_curr = m_weight[index];
  double w_clamp = std::max(std::min(weight, 1.0), 0.0);
  double w_scale = w_curr == 1.0 ? 0.0 : (1.0 - w_clamp) / (1.0 - w_curr);
  double w_sum = 0.0;

  // Update all the weights so the sum is still one
  for(int i = 0; i < m_numOfGaussian; i++)
    {
    if(i == index)
      {
      m_weight[i] = w_clamp;
      }
    else
      {
      m_weight[i] *= w_scale;
      }

    // Keet track of the sum of weights
    w_sum += m_weight[i];
    }

  // There is still a possibility that the sum of weights is not 1. In that
  // case assign the difference to the next cluster
  if(w_sum < 1.0)
    m_weight[(index % m_numOfGaussian)] += 1.0 - w_sum;
}

double GaussianMixtureModel::EvaluatePDF(int index, double *x)
{
  if (index < m_numOfGaussian)
  {
    return m_gaussian[index]->EvaluatePDF(x);
  }
  else
  {
    std::cout << "index out of boundary at " << __FILE__ << " : " << __LINE__  <<std::endl;
    exit(0);
  }
}

double GaussianMixtureModel::EvaluateLogPDF(int index, double *x)
{
  assert(index < m_numOfGaussian);
  return m_gaussian[index]->EvaluateLogPDF(x);
}


double GaussianMixtureModel::EvaluateLogPDF(
    int index, vnl_vector<double> &x, VectorType &xscratch)
{
  assert(index < m_numOfGaussian);
  return m_gaussian[index]->EvaluateLogPDF(x, xscratch);
}


void GaussianMixtureModel::PrintParameters()
{
  int i = 0;
  GaussianVectorIterator gaussianIter;
  WeightVectorIterator weightIter;
  for(gaussianIter = m_gaussian.begin(), weightIter = m_weight.begin();
      gaussianIter != m_gaussian.end(); ++gaussianIter, ++weightIter)
  {
    std::cout << std::endl << "Gaussian Component " << ++i << ":" << std::endl;
    std::cout << "weight:" << std::endl << *weightIter << std::endl;
    (*gaussianIter)->PrintParameters();
    }
}

bool GaussianMixtureModel::IsForeground(int index)
{
  return m_foreground_state[index];
}

void GaussianMixtureModel::SetForeground(int index)
{
  m_foreground_state[index] = true;
}

void GaussianMixtureModel::SetBackground(int index)
{
  m_foreground_state[index] = false;
}
