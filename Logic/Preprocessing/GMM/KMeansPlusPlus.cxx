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

  m_gmm = GaussianMixtureModel::New();
  m_gmm->Initialize(dataDim, numOfClusters);
}

KMeansPlusPlus::~KMeansPlusPlus()
{
  delete m_centers;
  delete m_xCenter;
  delete m_xCounter;
  delete m_distance;
}

double KMeansPlusPlus::Distance(const double *x, const double *y)
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

  Gaussian::VectorType tmpMean(m_dataDim, 0.0);
  for (int i = 0; i < m_numOfClusters; i++)
    {
    m_gmm->SetMean(i, tmpMean);
    }

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
        m_gmm->SetMean(j, tmpMean);
        break;
        }
      }
    }
  // m_gmm->PrintParameters();
  // getchar();
  for (int i = 0; i < m_numOfClusters; i++)
    {
    tmpMean = m_gmm->GetMean(i);

    if(m_xCounter[i] > 0)
      {
      // If this class is not empty, we set its mean
      tmpMean /= m_xCounter[i];
      }
    else
      {
      // If it is empty, we set the mean to -infinity (rather than nan)
      tmpMean.fill(-std::numeric_limits<double>::infinity());
      }

    m_gmm->SetMean(i, tmpMean);
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
        double dist = Distance(m_x[i], m_gmm->GetMean(j).data_block());
        if (radius[j] < dist)
          {
          radius[j] = dist;
          }
        break;
        }
      }
    }

  Gaussian::MatrixType tmpcovar(m_dataDim, m_dataDim, 0);
  for (int i = 0; i < m_numOfClusters; i++)
    {
    for (int j = 0; j < m_dataDim; j++)
      {
      tmpcovar(j,j) = radius[i];
      }
    m_gmm->SetCovariance(i, tmpcovar);
    }

  delete radius;

  for (int i = 0; i < m_numOfClusters; i++)
    {
    m_gmm->SetWeight(i, 1.0/(double) m_numOfClusters);
    }
}

GaussianMixtureModel * KMeansPlusPlus::GetGaussianMixtureModel(void)
{
  return m_gmm;
}
