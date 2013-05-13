#ifndef UNSUPERVISEDCLUSTERING_H
#define UNSUPERVISEDCLUSTERING_H

#include <itkObject.h>
#include <itkObjectFactory.h>
#include <SNAPCommon.h>

class KMeansPlusPlus;
class EMGaussianMixtures;
class GaussianMixtureModel;
class GenericImageData;

class UnsupervisedClustering : public itk::Object
{
public:

  irisITKObjectMacro(UnsupervisedClustering, itk::Object)

  void SetDataSource(GenericImageData *imageData);

  irisGetMacro(NumberOfClusters, int)

  irisGetMacro(MixtureModel, GaussianMixtureModel *)

  void SetNumberOfClusters(int nClusters);

  void ReinitializeClusters();

  void Iterate();


protected:

  UnsupervisedClustering();
  virtual ~UnsupervisedClustering();


  void InitializeEM();

  EMGaussianMixtures *m_ClusteringEM;
  KMeansPlusPlus *m_ClusteringInitializer;
  GaussianMixtureModel *m_MixtureModel;
  GenericImageData *m_DataSource;

  int m_NumberOfClusters, m_NumberOfComponents, m_NumberOfVoxels;

  // TODO: this is duplication of data and also double is larger than we need
  double **m_DataArray;
};

#endif // UNSUPERVISEDCLUSTERING_H
