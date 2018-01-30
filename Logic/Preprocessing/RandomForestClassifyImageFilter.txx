#ifndef RANDOMFORESTCLASSIFYIMAGEFILTER_TXX
#define RANDOMFORESTCLASSIFYIMAGEFILTER_TXX

#include "RandomForestClassifyImageFilter.h"
#include "itkImageRegionConstIterator.h"
#include "RandomForestClassifier.h"
#include "ImageCollectionToImageFilter.h"
#include <itkProgressReporter.h>

#include "Library/data.h"
#include "Library/forest.h"
#include "Library/statistics.h"
#include "Library/classifier.h"


template <class TInputImage, class TInputVectorImage, class TOutputImage>
RandomForestClassifyImageFilter<TInputImage, TInputVectorImage, TOutputImage>
::RandomForestClassifyImageFilter()
{
  // m_MixtureModel = NULL;
}

template <class TInputImage, class TInputVectorImage, class TOutputImage>
RandomForestClassifyImageFilter<TInputImage, TInputVectorImage, TOutputImage>
::~RandomForestClassifyImageFilter()
{
}


template <class TInputImage, class TInputVectorImage, class TOutputImage>
void
RandomForestClassifyImageFilter<TInputImage, TInputVectorImage, TOutputImage>
::AddScalarImage(InputImageType *image)
{
  this->AddInput(image);
}

template <class TInputImage, class TInputVectorImage, class TOutputImage>
void
RandomForestClassifyImageFilter<TInputImage, TInputVectorImage, TOutputImage>
::AddVectorImage(InputVectorImageType *image)
{
  this->AddInput(image);
}

template <class TInputImage, class TInputVectorImage, class TOutputImage>
void
RandomForestClassifyImageFilter<TInputImage, TInputVectorImage, TOutputImage>
::SetClassifier(RandomForestClassifier *classifier)
{
  m_Classifier = classifier;
  this->Modified();
}


template <class TInputImage, class TInputVectorImage, class TOutputImage>
void
RandomForestClassifyImageFilter<TInputImage, TInputVectorImage, TOutputImage>
::GenerateInputRequestedRegion()
{
  itk::ImageSource<TOutputImage>::GenerateInputRequestedRegion();

  for( itk::InputDataObjectIterator it( this ); !it.IsAtEnd(); it++ )
    {
    // Check whether the input is an image of the appropriate dimension
    InputImageType *input = dynamic_cast< InputImageType * >( it.GetInput() );
    InputVectorImageType *vecInput = dynamic_cast< InputVectorImageType * >( it.GetInput() );
    if (input)
      {
      InputImageRegionType inputRegion;
      this->CallCopyOutputRegionToInputRegion( inputRegion, this->GetOutput()->GetRequestedRegion() );
      inputRegion.PadByRadius(m_Classifier->GetPatchRadius());
      inputRegion.Crop(input->GetLargestPossibleRegion());
      input->SetRequestedRegion(inputRegion);
      }
    else if(vecInput)
      {
      InputImageRegionType inputRegion;
      this->CallCopyOutputRegionToInputRegion( inputRegion, this->GetOutput()->GetRequestedRegion() );
      inputRegion.PadByRadius(m_Classifier->GetPatchRadius());
      inputRegion.Crop(vecInput->GetLargestPossibleRegion());
      vecInput->SetRequestedRegion(inputRegion);
      }
    }
}


template <class TInputImage, class TInputVectorImage, class TOutputImage>
void
RandomForestClassifyImageFilter<TInputImage, TInputVectorImage, TOutputImage>
::PrintSelf(std::ostream &os, itk::Indent indent) const
{
  os << indent << "RandomForestClassifyImageFilter" << std::endl;
}

template <class TInputImage, class TInputVectorImage, class TOutputImage>
void
RandomForestClassifyImageFilter<TInputImage, TInputVectorImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType &outputRegionForThread,
                       itk::ThreadIdType threadId)
{
  assert(m_Classifier);

  OutputImagePointer outputPtr = this->GetOutput(0);

  // Fill the output region with zeros
  itk::ImageRegionIterator<OutputImageType> zit(outputPtr, outputRegionForThread);
  for(; !zit.IsAtEnd(); ++zit)
    zit.Set((OutputPixelType) 0);

  // Adjust the output region so that we don't touch image boundaries.
  OutputImageRegionType crop_region = outputPtr->GetLargestPossibleRegion();
  crop_region.ShrinkByRadius(m_Classifier->GetPatchRadius());
  OutputImageRegionType out_region = outputRegionForThread;
  bool can_crop = out_region.Crop(crop_region);

  if(!can_crop)
    return;

  // Create an iterator for the output
  typedef itk::ImageRegionIteratorWithIndex<TOutputImage> OutputIter;
  OutputIter it_out(outputPtr, out_region);

  // Create a collection iterator for the inputs
  typedef ImageCollectionConstRegionIteratorWithIndex<
      TInputImage, TInputVectorImage> CollectionIter;

  // Configure the input collection iterator
  CollectionIter cit(out_region);
  for( itk::InputDataObjectIterator it( this ); !it.IsAtEnd(); it++ )
    cit.AddImage(it.GetInput());

  // TODO: This is hard-coded
  cit.SetRadius(m_Classifier->GetPatchRadius());

  // Get the number of components
  int nComp = cit.GetTotalComponents();
  int nPatch = cit.GetNeighborhoodSize();
  int nColumns = nComp * nPatch;

  // Are coordinate features used?
  if(m_Classifier->GetUseCoordinateFeatures())
    nColumns += 3;

  // Get the number of classes
  int nClass = m_Classifier->GetClassToLabelMapping().size();

  // Get the class weights (as they are assigned to foreground/background)
  const RandomForestClassifier::WeightArray &class_weights = m_Classifier->GetClassWeights();

  // Create the MLdata representing each voxel (?)
  typedef Histogram<InputPixelType,LabelType> HistogramType;
  typedef MLData<InputPixelType,HistogramType *> TestingDataType;
  TestingDataType testData(1, nColumns);

  // Get the number of trees
  int nTrees = m_Classifier->GetForest()->trees_.size();

  // Create and allocate the test result vector
  typedef Vector<Vector<HistogramType *> > TestingResultType;
  TestingResultType testResult;
  testResult.Resize(nTrees);
  for(int i = 0; i < nTrees; i++)
    testResult[i].Resize(1);

  // Some vectors that are allocated for speed
  std::vector<size_t> vIndex(1);
  std::vector<bool> vResult(1);

  // Iterate through all the voxels
  for(; !it_out.IsAtEnd(); ++it_out, ++cit)
    {
    // Assign the data to the testData vector
    int k = 0;
    for(int i = 0; i < nComp; i++)
      for(int j = 0; j < nPatch; j++)
        testData.data[0][k++] = cit.NeighborValue(i,j);

    // Add the coordinate features
    if(m_Classifier->GetUseCoordinateFeatures())
      for(int d = 0; d < 3; d++)
        testData.data[0][k++] = it_out.GetIndex()[d];

    // Perform classification on this data
    m_Classifier->GetForest()->ApplyFast(testData, testResult, vIndex, vResult);

    // New code: compute output map with a bias parameter. The bias parameter q is such
    // that p_fore = q maps to 0 speed value. For the time being we just shift the linear
    // mapping from p_fore to speed and cap speed between -1 and 1

    // First we compute p_fore - for some reason not all trees in the forest have probabilities
    // summing up to one (some are zero), so we need to use division
    double p_fore_total = 0, p_total = 0;
    for(int i = 0; i < testResult.Size(); i++)
      {
      HistogramType *hist = testResult[i][0];
      for(int j = 0; j < nClass; j++)
        {
        double p = hist->prob_[j];
        if(class_weights[j] > 0.0)
          p_fore_total += p;
        p_total += p;
        }
      }

    // Set output only if the total probability is non-zero
    if(p_total > 0)
      {
      double q = m_Classifier->GetBiasParameter();
      double p_fore = p_fore_total / p_total;
      double speed = 2 * (p_fore - q);
      if(speed < -1.0)
        speed = -1.0;
      else if(speed > 1.0)
        speed = 1.0;

      it_out.Set((OutputPixelType)(speed * 0x7fff));
      }
    }
}


#endif
