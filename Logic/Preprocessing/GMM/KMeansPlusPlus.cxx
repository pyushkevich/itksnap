#include "KMeansPlusPlus.h"
#include "math.h"
#include "stdlib.h"
#include <random>

KMeansPlusPlus::KMeansPlusPlus(const vnl_matrix<double> &x, int numOfClusters)
  : m_x(x), m_dataSize(x.rows()), m_dataDim(x.cols()), m_numOfClusters(numOfClusters)
{
  m_gmm = GaussianMixtureModel::New();
  m_gmm->Initialize(m_dataDim, numOfClusters);
}

KMeansPlusPlus::~KMeansPlusPlus()
{
}

double KMeansPlusPlus::DistanceSq(const double *x, const double *y)
{
  double sum_sq = 0;
  for (int i = 0; i < m_dataDim; i++)
  {
    sum_sq += (x[i] - y[i]) * (x[i] - y[i]);
  }
  return sum_sq;
}

void KMeansPlusPlus::Initialize(void)
{
  // Create a distribution in range [0, m_dataSize - 1]
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> distr_ds(0, m_dataSize - 1);
  std::uniform_real_distribution<double> distr_01(0.0, 1.0);

  // Clear the centroids
  m_Centroids.clear();
  m_DistanceSqToNearestCentroid.resize(m_dataSize, 0.);
  m_ClusterMembership.resize(m_dataSize, 0);
  m_ClusterSize.resize(m_numOfClusters, 0);

  // Randomly pick the first centroid
  m_Centroids.push_back(distr_ds(gen));

  // Pick the rest of the centroids
  while(m_Centroids.size() <= m_numOfClusters)
  {
    // Compute distance to closest centroid from each data point
    double sum_dist_sq = 0;
    std::fill(m_ClusterSize.begin(), m_ClusterSize.end(), 0);
    for(unsigned int i = 0; i < m_dataSize; i++)
    {
      int nearest_center = 0;
      double min_dist_sq = DistanceSq(m_x[i], m_x[m_Centroids[0]]);
      for(unsigned int j = 1; j < m_Centroids.size(); j++)
      {
        double d_ij = DistanceSq(m_x[i], m_x[m_Centroids[j]]);
        if(d_ij < min_dist_sq)
        {
          min_dist_sq = d_ij;
          nearest_center = j;
        }
      }

      sum_dist_sq += min_dist_sq;
      m_DistanceSqToNearestCentroid[i] = min_dist_sq;
      m_ClusterMembership[i] = nearest_center;
      m_ClusterSize[nearest_center]++;
    }

    // Break if all the centroids are found
    if(m_Centroids.size() == m_numOfClusters)
      break;

    // Choose next centroid with probability proportional to D(x)^2
    double thresh = distr_01(gen) * sum_dist_sq;
    double cumulative = 0;
    for(unsigned int i = 0; i < m_dataSize; i++)
    {
      cumulative += m_DistanceSqToNearestCentroid[i] * m_DistanceSqToNearestCentroid[i];
      if(cumulative >= thresh)
      {
        m_Centroids.push_back(i);
        break;
      }
    }
  }

  // Compute the initial means and variances of the clusters
  std::vector<Gaussian::VectorType> sum_x(m_numOfClusters, Gaussian::VectorType(m_dataDim, 0.0));
  std::vector<Gaussian::MatrixType> sum_xy(m_numOfClusters, Gaussian::MatrixType(m_dataDim, m_dataDim, 0.0));

  for (int i = 0; i < m_dataSize; i++)
  {
    int j = m_ClusterMembership[i];
    for (int k = 0; k < m_dataDim; k++)
    {
      sum_x[j][k] += m_x[i][k];
      for (int m = 0; m < m_dataDim; m++)
      {
        sum_xy[j](k, m) += m_x[i][k] * m_x[i][m];
      }
    }
  }

  for(int j = 0; j < m_numOfClusters; j++)
  {
    double n = m_ClusterSize[j];
    m_gmm->SetMean(j, sum_x[j] / n);
    m_gmm->SetCovariance(j, (sum_xy[j] - outer_product(sum_x[j], sum_x[j]) / n) / (n - 1));
    m_gmm->SetWeight(j, n / m_dataSize);
    /*
    std::cout << "KM++ Cluster: " << j << std::endl;
    std::cout << "  Centroid: " << m_Centroids[j] << std::endl;
    std::cout << "  Size: " << m_ClusterSize[j] << std::endl;
    std::cout << "  Mean: " << m_gmm->GetMean(j) << std::endl;
    std::cout << "  Cov: " << m_gmm->GetCovariance(j) << std::endl;
    std::cout << "  Weight: " << m_gmm->GetWeight(j) << std::endl;
    */
  }
}

GaussianMixtureModel * KMeansPlusPlus::GetGaussianMixtureModel(void)
{
  return m_gmm;
}
