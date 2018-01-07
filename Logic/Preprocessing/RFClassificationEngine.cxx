#include "RFClassificationEngine.h"
#include "RandomForestClassifier.h"

#include "SNAPImageData.h"
#include "ImageWrapper.h"
#include "ImageCollectionToImageFilter.h"
#include "RLEImageRegionIterator.h"

// Includes from the random forest library
#include "Library/classification.h"
#include "Library/data.h"

template <class TPixel, class TLabel, int VDim>
RFClassificationEngine<TPixel,TLabel,VDim>::RFClassificationEngine()
{
  m_DataSource = NULL;
  m_Sample = NULL;
  m_Classifier = ClassifierType::New();
  m_ForestSize = 50;
  m_TreeDepth = 30;
  m_PatchRadius.Fill(0);
  m_UseCoordinateFeatures = false;
}

template <class TPixel, class TLabel, int VDim>
RFClassificationEngine<TPixel,TLabel,VDim>::~RFClassificationEngine()
{
  if(m_Sample)
    delete m_Sample;
}

template <class TPixel, class TLabel, int VDim>
void RFClassificationEngine<TPixel,TLabel,VDim>::SetDataSource(SNAPImageData *imageData)
{
  if(m_DataSource != imageData)
    {
    // Copy the data source
    m_DataSource = imageData;

    // Reset the classifier
    m_Classifier->Reset();
    }
}

template <class TPixel, class TLabel, int VDim>
void RFClassificationEngine<TPixel,TLabel,VDim>::ResetClassifier()
{
  m_Classifier->Reset();
}

template <class TPixel, class TLabel, int VDim>
void RFClassificationEngine<TPixel,TLabel,VDim>:: TrainClassifier()
{
  assert(m_DataSource && m_DataSource->IsMainLoaded());

  typedef ImageCollectionConstRegionIteratorWithIndex<
      AnatomicScalarImageWrapper::ImageType,
      AnatomicImageWrapper::ImageType> CollectionIter;

  // TODO: in the future, we should only recompute the sample when we know
  // that the data has changed. However, currently, we are just going to
  // compute a new sample every time

  // Delete the sample
  if(m_Sample)
    delete m_Sample;

  // Get the segmentation image - which determines the samples
  // TODO: this is defaulting to the first image - is this correct?
  LabelImageWrapper *wrpSeg = m_DataSource->GetFirstSegmentationLayer();
  LabelImageWrapper::ImagePointer imgSeg = wrpSeg->GetImage();
  typedef itk::ImageRegionConstIteratorWithIndex<LabelImageWrapper::ImageType> LabelIter;

  // Shrink the buffered region by radius because we can't handle BCs
  itk::ImageRegion<3> reg = imgSeg->GetBufferedRegion();
  reg.ShrinkByRadius(m_PatchRadius);

  // We need to iterate throught the label image once to determine the
  // number of samples to allocate.
  unsigned long nSamples = 0;
  for(LabelIter lit(imgSeg, reg); !lit.IsAtEnd(); ++lit)
    if(lit.Value())
      nSamples++;

  // Create an iterator for going over all the anatomical image data
  CollectionIter cit(reg);
  cit.SetRadius(m_PatchRadius);

  // Add all the anatomical images to this iterator
  for(LayerIterator it = m_DataSource->GetLayers(MAIN_ROLE | OVERLAY_ROLE);
      !it.IsAtEnd(); ++it)
    {
    cit.AddImage(it.GetLayer()->GetImageBase());
    }

  // Get the number of components
  int nComp = cit.GetTotalComponents();
  int nPatch = cit.GetNeighborhoodSize();
  int nColumns = nComp * nPatch;

  // Are we using coordinate informtion
  if(m_UseCoordinateFeatures)
    nColumns += 3;

  // Create a new sample
  m_Sample = new SampleType(nSamples, nColumns);

  // Now fill out the samples
  int iSample = 0;
  for(LabelIter lit(imgSeg, reg); !lit.IsAtEnd(); ++lit, ++cit)
    {
    LabelType label = lit.Value();
    if(label)
      {
      // Fill in the data
      std::vector<GreyType> &column = m_Sample->data[iSample];
      int k = 0;
      for(int i = 0; i < nComp; i++)
        for(int j = 0; j < nPatch; j++)
          column[k++] = cit.NeighborValue(i,j);

      // Add the coordinate features if used
      if(m_UseCoordinateFeatures)
        for(int d = 0; d < 3; d++)
          column[k++] = lit.GetIndex()[d];

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
  // TODO:
  params.treeDepth = m_TreeDepth;
  params.treeNum = m_ForestSize;
  params.candidateNodeClassifierNum = 10;
  params.candidateClassifierThresholdNum = 10;
  params.subSamplePercent = 0;
  params.splitIG = 0.1;
  params.leafEntropy = 0.05;
  params.verbose = true;

  // Cap the number of training voxels at some reasonable number
  if(m_Sample->Size() > 10000)
    params.subSamplePercent = 100 * 10000.0 / m_Sample->Size();
  else
    params.subSamplePercent = 0;

  // Create the classification engine
  typedef typename ClassifierType::RFAxisClassifierType RFAxisClassifierType;
  typedef Classification<GreyType, LabelType, RFAxisClassifierType> ClassificationType;
  ClassificationType classification;

  // Before resetting the classifier, we want to retain whatever the
  // weighting of the classes was
  std::map<LabelType, double> old_label_weights;
  if(m_Classifier->IsValidClassifier())
    {
    // Get the class weights
    const typename ClassifierType::WeightArray &class_weights = m_Classifier->GetClassWeights();

    // Convert them to label weights (since class to label mapping may change)
    for(typename ClassifierType::MappingType::const_iterator it =
        m_Classifier->GetClassToLabelMapping().begin();
        it != m_Classifier->GetClassToLabelMapping().end(); ++it)
      {
      old_label_weights[it->second] = class_weights[it->first];
      }
    }

  // Prepare the classifier
  m_Classifier->Reset();

  // Perform classifier training
  classification.Learning(
        params, *m_Sample,
        *m_Classifier->GetForest(),
        m_Classifier->GetValidLabel(),
        m_Classifier->GetClassToLabelMapping());

  // Reset the class weights to the number of classes and assign default
  int n_classes = m_Classifier->GetClassToLabelMapping().size(), n_fore = 0, n_back = 0;
  m_Classifier->GetClassWeights().resize(n_classes, -1.0);

  // Apply the old weight assignments if possible. Keep track of the number of fore and back classes
  for(typename ClassifierType::MappingType::iterator it =
      m_Classifier->GetClassToLabelMapping().begin();
      it != m_Classifier->GetClassToLabelMapping().end(); ++it)
    {
    if(old_label_weights.find(it->second) != old_label_weights.end())
      {
      m_Classifier->GetClassWeights()[it->first] = old_label_weights[it->second];
      }
    if(m_Classifier->GetClassWeights()[it->first] < 0.0)
      n_back++;
    else if(m_Classifier->GetClassWeights()[it->first] > 0.0)
      n_fore++;
    }

  // Make sure that we have at least one foreground class and at least one background class
  if(n_classes >= 2)
    {
    if(n_fore == 0)
      m_Classifier->GetClassWeights().front() = 1.0;
    if(n_back == 0)
      m_Classifier->GetClassWeights().back() = -1.0;
    }

  // Store the patch radius in the classifier - this remains fixed until
  // training is repeated
  m_Classifier->SetPatchRadius(m_PatchRadius);
  m_Classifier->SetUseCoordinateFeatures(m_UseCoordinateFeatures);
}

template <class TPixel, class TLabel, int VDim>
void RFClassificationEngine<TPixel,TLabel,VDim>::SetClassifier(ClassifierType *rf)
{
  // Set the classifier
  m_Classifier = rf;

  // Update the forest size
  m_ForestSize = m_Classifier->GetForest()->GetForestSize();
}

template <class TPixel, class TLabel, int VDim>
int RFClassificationEngine<TPixel,TLabel,VDim>::GetNumberOfComponents() const
{
  assert(m_DataSource);

  int ncomp = 0;

  for(LayerIterator it = m_DataSource->GetLayers(MAIN_ROLE | OVERLAY_ROLE);
      !it.IsAtEnd(); ++it)
    ncomp += it.GetLayer()->GetNumberOfComponents();

  return ncomp;
}

// Template instantiation
template class RFClassificationEngine<GreyType, LabelType, 3>;
