#include "UnsupervisedClustering.h"
#include "KMeansPlusPlus.h"
#include "EMGaussianMixtures.h"
#include "GaussianMixtureModel.h"
#include "GenericImageData.h"
#include "ImageWrapper.h"
#include "ImageWrapperTraits.h"

UnsupervisedClustering::UnsupervisedClustering()
{
  m_ClusteringEM = NULL;
  m_NumberOfClusters = 3;
  m_DataArray = NULL;
}

UnsupervisedClustering::~UnsupervisedClustering()
{
  if(m_ClusteringEM)
    {
    delete m_ClusteringEM;
    delete m_ClusteringInitializer;
    }

  if(m_DataArray)
    delete m_DataArray;
}


void UnsupervisedClustering::SetDataSource(GenericImageData *imageData)
{
  m_DataSource = imageData;

  // Figure out the number of data components
  unsigned int nComp = 0;
  for(LayerIterator lit = imageData->GetLayers(
        LayerIterator::MAIN_ROLE | LayerIterator::OVERLAY_ROLE);
      !lit.IsAtEnd(); ++lit)
    {
    nComp += lit.GetLayer()->GetNumberOfComponents();
    }

  // Size the data array
  int nvox = imageData->GetMain()->GetNumberOfVoxels();
  m_DataArray = new double *[nvox];
  double *buffer = new double[nvox * nComp];
  for(int i = 0; i < nvox; i++, buffer+=nComp)
    m_DataArray[i] = buffer;

  // Allocate the data array
  int iOffset = 0;
  for(LayerIterator lit = imageData->GetLayers(
        LayerIterator::MAIN_ROLE | LayerIterator::OVERLAY_ROLE);
      !lit.IsAtEnd(); ++lit)
    {
    AnatomicImageWrapper *aiw = dynamic_cast<AnatomicImageWrapper *>(lit.GetLayer());
    int pVoxel = 0;
    int iComp = aiw->GetNumberOfComponents();
    for(AnatomicImageWrapper::Iterator it = aiw->GetImageIterator();
        !it.IsAtEnd(); ++it, ++pVoxel)
      {
      AnatomicImageWrapper::PixelType pixel = it.Get();
      for(int j = 0; j < iComp; j++)
        m_DataArray[pVoxel][iOffset + j] = pixel[j];
      }

    iOffset += iComp;
    }

  m_NumberOfVoxels = nvox;
  m_NumberOfComponents = nComp;

  // Initialize
  this->InitializeEM();
}

void UnsupervisedClustering::SetNumberOfClusters(int nClusters)
{
  if(m_NumberOfClusters != nClusters)
    {
    m_NumberOfClusters = nClusters;
    if(m_DataArray)
      {
      this->InitializeEM();
      }
    }
}

void UnsupervisedClustering::InitializeEM()
{
  if(m_ClusteringEM)
    {
    delete m_ClusteringEM;
    delete m_ClusteringInitializer;
    }

  // Allocate the EM algorithm
  m_ClusteringEM = new EMGaussianMixtures(
        m_DataArray, m_NumberOfVoxels,
        m_NumberOfComponents, m_NumberOfClusters);

  // Allocate the K means ++
  m_ClusteringInitializer = new KMeansPlusPlus(
        m_DataArray, m_NumberOfVoxels,
        m_NumberOfComponents, m_NumberOfClusters);

  m_ClusteringInitializer->Initialize();

  m_ClusteringEM->SetGaussianMixtureModel(
        m_ClusteringInitializer->GetGaussianMixtureModel());
  m_ClusteringEM->SetMaxIteration(10);

  // Get the GMM
  m_MixtureModel = m_ClusteringEM->GetGaussianMixtureModel();
}


void UnsupervisedClustering::Iterate()
{
  long start = clock();
  m_ClusteringEM->UpdateOnce();
  m_MixtureModel->PrintParameters();
  long end = clock();
  std::cout << "spending " << (end-start)/1000 << std::endl;
}

