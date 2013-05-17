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
  m_NumberOfSamples = 0;
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
  if(m_DataSource != imageData)
    {
    m_DataSource = imageData;
    m_SamplesDirty = true;

    int nvox = m_DataSource->GetMain()->GetNumberOfVoxels();
    m_NumberOfSamples = (nvox > 100000) ? 100000 : nvox;
    }
}

#include "itkImageRandomConstIteratorWithIndex.hxx"

void UnsupervisedClustering::SampleDataSource()
{
  if(m_DataArray)
    {
    delete m_DataArray;
    }

  // Figure out the number of data components
  unsigned int nComp = 0;
  for(LayerIterator lit = m_DataSource->GetLayers(
        LayerIterator::MAIN_ROLE | LayerIterator::OVERLAY_ROLE);
      !lit.IsAtEnd(); ++lit)
    {
    nComp += lit.GetLayer()->GetNumberOfComponents();
    }

  // Size the data array
  int nvox = m_DataSource->GetMain()->GetNumberOfVoxels();
  int nsam = (m_NumberOfSamples == 0) ? nvox : m_NumberOfSamples;

  // Create data structure for the EM code
  m_DataArray = new double *[nsam];
  double *buffer = new double[nsam * nComp];
  for(int i = 0; i < nsam; i++, buffer+=nComp)
    m_DataArray[i] = buffer;

  // Are we randomly sampling?
  if(nvox == nsam)
    {
    // Allocate the data array
    int iOffset = 0;
    for(LayerIterator lit = m_DataSource->GetLayers(
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
    }
  else
    {
    // Create a random walk through the main image
    typedef AnatomicImageWrapper::ImageType AnatomicImage;
    AnatomicImage *main = m_DataSource->GetMain()->GetImage();
    typedef itk::ImageRandomConstIteratorWithIndex<AnatomicImage> RandomIter;
    RandomIter itRand(main, main->GetBufferedRegion());
    itRand.SetNumberOfSamples(nsam);

    // Do the random walk
    int pVoxel = 0;
    for(; !itRand.IsAtEnd(); ++itRand)
      {
      itk::Index<3> idx = itRand.GetIndex();
      AnatomicImage::OffsetValueType offset = main->ComputeOffset(idx);

      int iOffset = 0;
      for(LayerIterator lit = m_DataSource->GetLayers(
            LayerIterator::MAIN_ROLE | LayerIterator::OVERLAY_ROLE);
          !lit.IsAtEnd(); ++lit)
        {
        AnatomicImageWrapper *aiw = dynamic_cast<AnatomicImageWrapper *>(lit.GetLayer());
        int iComp = aiw->GetNumberOfComponents();
        AnatomicImageWrapper::InternalPixelType *pixel =
            aiw->GetImage()->GetBufferPointer() + offset * iComp;


        for(int j = 0; j < iComp; j++)
          m_DataArray[pVoxel][iOffset + j] = pixel[j];

        iOffset += iComp;
        }

      pVoxel++;
      }

    m_NumberOfVoxels = nsam;
    }

  m_NumberOfComponents = nComp;
  m_SamplesDirty = false;
}

void UnsupervisedClustering::SetNumberOfClusters(int nClusters)
{
  if(m_NumberOfClusters != nClusters)
    {
    m_NumberOfClusters = nClusters;
    }
}

void UnsupervisedClustering::SetNumberOfSamples(int nSamples)
{
  if(m_NumberOfSamples != nSamples)
    {
    m_NumberOfSamples = nSamples;
    m_SamplesDirty = true;
    }
}

void UnsupervisedClustering::InitializeClusters()
{
  this->InitializeEM();
}

void UnsupervisedClustering::InitializeEM()
{
  // Make sure the data source is specified
  assert(m_DataSource);

  // Make sure samples exist
  if(m_SamplesDirty || m_DataArray == NULL)
    this->SampleDataSource();

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
  double **post = m_ClusteringEM->UpdateOnce();
  m_MixtureModel->PrintParameters();
  long end = clock();
  std::cout << "spending " << (end-start)/1000 << std::endl;
}

