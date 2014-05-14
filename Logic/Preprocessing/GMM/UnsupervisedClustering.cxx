#include "UnsupervisedClustering.h"
#include "KMeansPlusPlus.h"
#include "EMGaussianMixtures.h"
#include "GaussianMixtureModel.h"
#include "SNAPImageData.h"
#include "ImageWrapper.h"
#include "ImageWrapperTraits.h"

#include <algorithm>

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
    {
    // Delete the main data buffer
    delete m_DataArray[0];

    // Delete the pointers into the buffer
    delete m_DataArray;
    }


}


void UnsupervisedClustering::SetDataSource(SNAPImageData *imageData)
{
  if(m_DataSource != imageData)
    {
    m_DataSource = imageData;
    m_SamplesDirty = true;

    int nvox = m_DataSource->GetMain()->GetNumberOfVoxels();
    m_NumberOfSamples = (nvox > 10000) ? 10000 : nvox;
    }
}

void UnsupervisedClustering::SetMixtureModel(GaussianMixtureModel *model)
{
  m_ClusteringEM->SetGaussianMixtureModel(model);
  m_MixtureModel = m_ClusteringEM->GetGaussianMixtureModel();
}

#include "itkImageRandomConstIteratorWithIndex.hxx"

void UnsupervisedClustering::SampleDataSource()
{
  if(m_DataArray)
    {
    // Delete the main data buffer
    delete m_DataArray[0];

    // Delete the pointers into the buffer
    delete m_DataArray;
    }

  // Figure out the number of data components
  unsigned int nComp = 0;
  for(LayerIterator lit = m_DataSource->GetLayers(
        MAIN_ROLE | OVERLAY_ROLE);
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

  // Create a random walk through the speed image, which should be initialized
  // at this point. We iterate over the speed image because we can easily access
  // its internal image (it's always a scalar image)
  assert(m_DataSource->IsSpeedLoaded());
  typedef SpeedImageWrapper::ImageType SpeedImage;

  SpeedImage *speed = m_DataSource->GetSpeed()->GetImage();
  typedef itk::ImageRandomConstIteratorWithIndex<SpeedImage> RandomIter;
  RandomIter itRand(speed, speed->GetBufferedRegion());
  itRand.SetNumberOfSamples(nsam);

  // Initialize the 'central' samples list
  m_CenterSamples.clear();
  m_CenterSamples.reserve(400);

  // Define the center region
  itk::ImageRegion<3> rcenter = speed->GetBufferedRegion();
  rcenter.ShrinkByRadius(to_itkSize(Vector3d(rcenter.GetSize()) * 0.2));

  // Do the random walk
  int pVoxel = 0;
  for(; !itRand.IsAtEnd(); ++itRand)
    {
    // Go to the next sample location
    itk::Index<3> idx = itRand.GetIndex();

    // TODO: this is a really slow way to collect samples!
    int iOffset = 0;
    for(LayerIterator lit = m_DataSource->GetLayers(MAIN_ROLE | OVERLAY_ROLE);
        !lit.IsAtEnd(); ++lit)
      {
      ImageWrapperBase *iw = lit.GetLayer();
      iw->GetVoxelAsDouble(idx, m_DataArray[pVoxel] + iOffset);
      iOffset += iw->GetNumberOfComponents();
      }

    // Store as a 'central' sample if in the central 60% of the image
    if(m_CenterSamples.size() < 400 && rcenter.IsInside(idx))
      {
      m_CenterSamples.push_back(pVoxel);
      }

    pVoxel++;
    }

  m_NumberOfVoxels = nsam;

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

  // Sort the clusters based on center samples
  SortClustersByRelevance();
}

void UnsupervisedClustering::SortClustersByRelevance()
{
  int ng = m_MixtureModel->GetNumberOfGaussians();
  vnl_vector<double> log_pdf(ng), log_w(ng), w(ng);

  // the array to sort
  typedef std::pair<double, int> RelevancePair;
  std::vector<RelevancePair> rel(ng);

  for(int k = 0; k < ng; k++)
    {
    log_w[k] = log(m_MixtureModel->GetWeight(k));
    w[k] = m_MixtureModel->GetWeight(k);
    rel[k].first = 0.0;
    rel[k].second = k;
    }

  for(int i = 0; i < m_CenterSamples.size(); i++)
    {
    int s = m_CenterSamples[i];
    for(int k = 0; k < ng; k++)
      {
      log_pdf[k] = m_MixtureModel->EvaluateLogPDF(k, m_DataArray[s]);
      }

    for(int k = 0; k < ng; k++)
      {
      // Accumulate the posterior
      rel[k].first -= EMGaussianMixtures::ComputePosterior(
            ng, log_pdf.data_block(), w.data_block(), log_w.data_block(), k);
      }
    }

  // Sort the relevance array
  std::sort(rel.begin(), rel.end());

  // Create a new mixture model
  SmartPtr<GaussianMixtureModel> gmmtemp = GaussianMixtureModel::New();
  gmmtemp->Initialize(m_MixtureModel->GetNumberOfComponents(), ng);
  for(int k = 0; k < ng; k++)
    {
    int j = rel[k].second;
    gmmtemp->SetWeight(k, m_MixtureModel->GetWeight(j));
    gmmtemp->SetGaussian(k, m_MixtureModel->GetMean(j), m_MixtureModel->GetCovariance(j));
    }

  // Assign it to the EM
  m_ClusteringEM->SetGaussianMixtureModel(gmmtemp);
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


