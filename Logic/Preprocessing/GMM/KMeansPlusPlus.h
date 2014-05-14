#ifndef KMEANS_PLUS_PLUS_H
#define KMEANS_PLUS_PLUS_H

#include "GaussianMixtureModel.h"
#include "SNAPCommon.h"

class KMeansPlusPlus
{
public:
  KMeansPlusPlus(double **x, int dataSize, int dataDim, int numOfClusters);
  ~KMeansPlusPlus();

  double Distance(const double *x, const double *y);
  void Initialize(void);
  GaussianMixtureModel * GetGaussianMixtureModel(void);
private:
  double **m_x;
  int *m_xCenter;
  int *m_centers;
  int *m_xCounter;
  double *m_distance;
  int m_dataSize;
  int m_dataDim;
  int m_numOfClusters;
  SmartPtr<GaussianMixtureModel> m_gmm;
};

#endif
