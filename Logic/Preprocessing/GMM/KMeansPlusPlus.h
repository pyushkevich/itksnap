#ifndef KMEANS_PLUS_PLUS_H
#define KMEANS_PLUS_PLUS_H

#include "GaussianMixtureModel.h"
#include "SNAPCommon.h"
#include <vector>

class KMeansPlusPlus
{
public:
  KMeansPlusPlus(const vnl_matrix<double> &x, int numOfClusters);
  ~KMeansPlusPlus();

  double DistanceSq(const double *x, const double *y);
  void Initialize(void);
  GaussianMixtureModel * GetGaussianMixtureModel(void);
private:
  std::vector<int> m_Centroids;
  std::vector<double> m_DistanceSqToNearestCentroid;
  std::vector<int> m_ClusterMembership;
  std::vector<int> m_ClusterSize;

  const vnl_matrix<double> &m_x;
  int m_dataSize;
  int m_dataDim;
  int m_numOfClusters;
  SmartPtr<GaussianMixtureModel> m_gmm;
};

#endif
