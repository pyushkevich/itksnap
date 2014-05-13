#ifndef UNSUPERVISEDCLUSTERING_H
#define UNSUPERVISEDCLUSTERING_H

#include <itkObject.h>
#include <itkObjectFactory.h>
#include <SNAPCommon.h>

class KMeansPlusPlus;
class EMGaussianMixtures;
class GaussianMixtureModel;
class SNAPImageData;

class UnsupervisedClustering : public itk::Object
{
public:

  irisITKObjectMacro(UnsupervisedClustering, itk::Object)

  void SetDataSource(SNAPImageData *imageData);

  irisGetMacro(NumberOfClusters, int)

  irisGetMacro(NumberOfSamples, int)

  irisGetMacro(MixtureModel, GaussianMixtureModel *)

  void SetMixtureModel(GaussianMixtureModel *model);

  void SetNumberOfClusters(int nClusters);

  void SetNumberOfSamples(int nSamples);

  void InitializeClusters();

  void Iterate();


protected:

  UnsupervisedClustering();
  virtual ~UnsupervisedClustering();


  void InitializeEM();
  void SampleDataSource();
  void SortClustersByRelevance();

  EMGaussianMixtures *m_ClusteringEM;
  KMeansPlusPlus *m_ClusteringInitializer;
  GaussianMixtureModel *m_MixtureModel;
  SNAPImageData *m_DataSource;

  int m_NumberOfClusters, m_NumberOfComponents, m_NumberOfVoxels, m_NumberOfSamples;

  bool m_SamplesDirty;

  // TODO: probably double is larger than we need
  double **m_DataArray;

  // A set of samples located near the center of the image, used to sort
  // initial clusters in terms of relevance to the user
  std::vector<int> m_CenterSamples;
};

#endif // UNSUPERVISEDCLUSTERING_H
