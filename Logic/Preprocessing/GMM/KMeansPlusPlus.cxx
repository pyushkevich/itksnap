#include "KMeansPlusPlus.h"
#include "math.h"
#include "time.h"
#include "stdlib.h"

KMeansPlusPlus::KMeansPlusPlus(double **x, int dataSize, int dataDim, int numOfClusters)
  :m_dataSize(dataSize), m_dataDim(dataDim), m_numOfClusters(numOfClusters)
{
  m_x = x;
  m_xCenter = new int[dataSize];
  m_centers = new int[numOfClusters];
  m_xCounter = new int[numOfClusters];
  m_distance = new double[dataSize];
  m_gmm = new GaussianMixtureModel(dataDim, numOfClusters);
}

KMeansPlusPlus::~KMeansPlusPlus()
{
  delete m_centers;
  delete m_xCenter;
  delete m_xCounter;
  delete m_distance;
  delete m_gmm;
}

double KMeansPlusPlus::Distance(double *x, double *y)
{
  double tmp = 0;
  for (int i = 0; i < m_dataDim; i++)
  {
    tmp += (x[i] - y[i]) * (x[i] - y[i]);
  }
  return sqrt(tmp);
}

void KMeansPlusPlus::Initialize(void)
{
  // for (int i = 0; i < numOfClusters; i++)
  // {
  //   m_xCounter[i] = 0;
  // }
  
  srand(time(0));

  m_centers[0] = (int)(((double) rand() / (double) RAND_MAX) * m_dataSize);
  double distSum = 0;
  for (int i = 0; i < m_dataSize; i++)
  {
    m_xCenter[i] = m_centers[0];
    m_distance[i] = Distance(m_x[i], m_x[m_centers[0]]);
    distSum += m_distance[i];
  }
  m_xCounter[0] = m_dataSize;

  double probDist = 0;
  double currentSum = 0;
  int idx = 0;
  for (int i = 1; i < m_numOfClusters; i++)
  {
    m_xCounter[i] = 0;
    probDist = ((double) rand() / (double) RAND_MAX) * distSum;
    currentSum = 0;
    for (idx = 0; idx < m_dataSize; idx++)
    {
      currentSum += m_distance[idx];
      if (currentSum >= probDist)
      {
	break;
      }
    }

    m_centers[i] = idx;

    distSum = 0;
    for (int j = 0; j < m_dataSize; j++)
    {
      if (m_distance[j] > Distance(m_x[j], m_x[m_centers[i]]))
      {
	++m_xCounter[i];
	for (int k = 0; k < i; k++)
	{
	  if (m_centers[k] == m_xCenter[j])
	  {
	    --m_xCounter[k];
	    break;
	  }
	}
	m_distance[j] = Distance(m_x[j], m_x[m_centers[i]]);
	m_xCenter[j] = m_centers[i];
      }
      distSum += m_distance[j];
    }
  }

  double *tmpMean = new double[m_dataDim];
  for (int j = 0; j < m_dataDim; j++)
  {
    tmpMean[j] = 0;
  }
  for (int i = 0; i < m_numOfClusters; i++)
  {
    m_gmm->SetMean(i, tmpMean);
  }
  delete tmpMean;
  // m_gmm->PrintParameters();
  // getchar();
  
  for (int i = 0; i < m_dataSize; i++)
  {
    for (int j = 0; j < m_numOfClusters; j++)
    {
      if (m_xCenter[i] == m_centers[j])
      {
	tmpMean = m_gmm->GetMean(j);
	for (int k = 0; k < m_dataDim; k++)
	{
	  tmpMean[k] = tmpMean[k] + m_x[i][k];
	}
	break;
      }
    }
  }
  // m_gmm->PrintParameters();
  // getchar();
  for (int i = 0; i < m_numOfClusters; i++)
  {
    tmpMean = m_gmm->GetMean(i);
    for (int j = 0; j < m_dataDim; j++)
    {
      tmpMean[j] = tmpMean[j] / m_xCounter[i];
    }
  }
  
  double *radius = new double[m_numOfClusters];
  for (int i = 0; i < m_numOfClusters; i++)
  {
    radius[i] = 0;
  }
  for (int i = 0; i < m_dataSize; i++)
  {
    for (int j = 0; j < m_numOfClusters; j++)
    {
      if (m_xCenter[i] == m_centers[j])
      {
	if (radius[j] < Distance(m_x[i], m_gmm->GetMean(j)))
	{
	  radius[j] = Distance(m_x[i], m_gmm->GetMean(j));
	}
	break;
      }
    }
  }

  double *tmpcovar = new double[m_dataDim*m_dataDim];
  for (int i = 0; i < m_numOfClusters; i++)
  {
    for (int j = 0; j < m_dataDim; j++)
    {
      for (int k = 0; k < m_dataDim; k++)
      {
	if (j == k)
	{
	  tmpcovar[j*m_dataDim+k] = radius[i];
	}
	else
	{
	  tmpcovar[j*m_dataDim+k] = 0;
	}
      }
    }
    m_gmm->SetCovariance(i, tmpcovar);
  }

  delete radius;
  delete tmpcovar;

  for (int i = 0; i < m_numOfClusters; i++)
  {
    m_gmm->SetWeight(i, 1.0/(double) m_numOfClusters);
  }
}

GaussianMixtureModel * KMeansPlusPlus::GetGaussianMixtureModel(void)
{
  return m_gmm;
}
