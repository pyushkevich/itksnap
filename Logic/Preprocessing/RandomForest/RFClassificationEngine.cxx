#include "RFClassificationEngine.h"
#include "RandomForestClassifier.h"

#include "SNAPImageData.h"
#include "ImageWrapper.h"
#include "ImageCollectionToImageFilter.h"
#include "itkImageRegionIterator.h"

// Includes from the random forest library
typedef GreyType data_t;
typedef LabelType label_t;

#include "Library/classification.h"
#include "Library/data.h"

RFClassificationEngine::RFClassificationEngine()
{
  m_DataSource = NULL;
  m_Sample = NULL;
  m_Classifier = RandomForestClassifier::New();
  m_ForestSize = 50;
}

RFClassificationEngine::~RFClassificationEngine()
{
  if(m_Sample)
    delete m_Sample;
}

void RFClassificationEngine::SetDataSource(SNAPImageData *imageData)
{
  if(m_DataSource != imageData)
    {
    // Copy the data source
    m_DataSource = imageData;

    // Reset the classifier
    m_Classifier->Reset();
    }
}

void RFClassificationEngine::ResetClassifier()
{
  m_Classifier->Reset();
}

void RFClassificationEngine:: TrainClassifier()
{
  assert(m_DataSource && m_DataSource->IsMainLoaded());

  // TODO: in the future, we should only recompute the sample when we know
  // that the data has changed. However, currently, we are just going to
  // compute a new sample every time

  // Delete the sample
  if(m_Sample)
    delete m_Sample;

  // Get the segmentation image - which determines the samples
  LabelImageWrapper *wrpSeg = m_DataSource->GetSegmentation();
  LabelImageWrapper::ImagePointer imgSeg = wrpSeg->GetImage();
  typedef itk::ImageRegionConstIterator<LabelImageWrapper::ImageType> LabelIter;

  // We need to iterate throught the label image once to determine the
  // number of samples to allocate.
  unsigned long nSamples = 0;
  for(LabelIter lit(imgSeg, imgSeg->GetBufferedRegion()); !lit.IsAtEnd(); ++lit)
    if(lit.Value())
      nSamples++;

  // Create an iterator for going over all the anatomical image data
  typedef ImageCollectionConstRegionIteratorWithIndex<
      AnatomicScalarImageWrapper::ImageType,
      AnatomicImageWrapper::ImageType> CollectionIter;

  CollectionIter cit(imgSeg->GetBufferedRegion());

  // Add all the anatomical images to this iterator
  for(LayerIterator it = m_DataSource->GetLayers(MAIN_ROLE | OVERLAY_ROLE);
      !it.IsAtEnd(); ++it)
    {
    cit.AddImage(it.GetLayer()->GetImageBase());
    }

  // Get the number of components
  int nComp = cit.GetTotalComponents();

  // Create a new sample
  m_Sample = new SampleType(nSamples, nComp);

  // Now fill out the samples
  int iSample = 0;
  for(LabelIter lit(imgSeg, imgSeg->GetBufferedRegion()); !lit.IsAtEnd(); ++lit, ++cit)
    {
    LabelType label = lit.Value();
    if(label)
      {
      // Fill in the data
      std::vector<GreyType> &column = m_Sample->data[iSample];
      for(int i = 0; i < nComp; i++)
        column[i] = cit.Value(i);

      // Fill in the label
      m_Sample->label[iSample] = label;

      ++iSample;
      }
    }

  // Check that the sample has at least two distinct labels
  bool isValidSample = false;
  for(int iSample = 1; iSample < m_Sample->Size(); iSample++)
    if(m_Sample->label[iSample] != m_Sample->label[iSample-1])
      { isValidSample = true; break; }

  // Now there is a valid sample. The text task is to train the classifier
  if(!isValidSample)
    throw IRISException("A classifier cannot be trained because the training "
                        "data contain fewer than two classes. Please label "
                        "examples of two or more tissue classes in the image.");

  // Set up the classifier parameters
  TrainingParameters params;
  params.treeDepth = 10;
  params.treeNum = m_ForestSize;
  params.candidateNodeClassifierNum = 10;
  params.candidateClassifierThresholdNum = 10;
  params.subSamplePercent = 0;
  params.splitIG = 0.1;
  params.leafEntropy = 0.05;
  params.verbose = true;

  // Create the classification engine
  typedef RandomForestClassifier::RFAxisClassifierType RFAxisClassifierType;
  typedef Classification<GreyType, LabelType, RFAxisClassifierType> ClassificationType;
  ClassificationType classification;

  // Before resetting the classifier, we want to retain whatever the
  // foreground label was.
  bool isOldForegroundLabelValid = m_Classifier->IsValidClassifier();
  LabelType oldForegroundLabel = 0;

  if(isOldForegroundLabelValid)
    oldForegroundLabel = m_Classifier->GetForegroundClassLabel();

  // Prepare the classifier
  m_Classifier->Reset();

  // Perform classifier training
  classification.Learning(
        params, *m_Sample,
        *m_Classifier->m_Forest,
        m_Classifier->m_ValidLabel,
        m_Classifier->m_ClassToLabelMapping);

  // Assign the foreground index to zero (default)
  m_Classifier->m_ForegroundClass = 0;

  // Now maybe re-assign the old foreground label
  if(isOldForegroundLabelValid)
    {
    for(RandomForestClassifier::MappingType::const_iterator it =
        m_Classifier->m_ClassToLabelMapping.begin();
        it != m_Classifier->m_ClassToLabelMapping.end(); ++it)
      {
      if(it->second == oldForegroundLabel)
        m_Classifier->m_ForegroundClass = it->first;
      }
    }
}

void RFClassificationEngine::SetClassifier(RandomForestClassifier *rf)
{
  // Set the classifier
  m_Classifier = rf;

  // Update the forest size
  m_ForestSize = m_Classifier->GetForest()->GetForestSize();
}

int RFClassificationEngine::GetNumberOfComponents() const
{
  assert(m_DataSource);

  int ncomp = 0;

  for(LayerIterator it = m_DataSource->GetLayers(MAIN_ROLE | OVERLAY_ROLE);
      !it.IsAtEnd(); ++it)
    ncomp += it.GetLayer()->GetNumberOfComponents();

  return ncomp;
}


