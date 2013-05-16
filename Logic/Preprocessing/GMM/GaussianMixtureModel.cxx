#include "GaussianMixtureModel.h"
#include <iostream>

GaussianMixtureModel::GaussianMixtureModel(int dimOfGaussian)
  :m_dimOfGaussian(dimOfGaussian), m_numOfGaussian(0)
{
  m_gaussian = new GaussianVector();
  m_weight = new WeightVector();
}

GaussianMixtureModel::GaussianMixtureModel(int dimOfGaussian, int numOfGaussian)
  :m_dimOfGaussian(dimOfGaussian), m_numOfGaussian(numOfGaussian)
{
  m_gaussian = new GaussianVector();
  m_weight = new WeightVector();
  Gaussian *gaussian;
  for (int i = 0; i < numOfGaussian; i++)
  {
    gaussian = new Gaussian(dimOfGaussian);
    m_gaussian->push_back(gaussian);
    m_weight->push_back(0);
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

void GaussianMixtureModel::AddGaussian(double *mean, double *covariance, double weight)
{
  Gaussian *gaussian = new Gaussian(m_dimOfGaussian, mean, covariance);
  m_gaussian->push_back(gaussian);
  m_weight->push_back(weight);
  ++m_numOfGaussian;
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
