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

  // TODO: this should not be hard-coded
  typename InputImageType::SizeType radius; radius.Fill(2);

  for( itk::InputDataObjectIterator it( this ); !it.IsAtEnd(); it++ )
    {
    // Check whether the input is an image of the appropriate dimension
    InputImageType *input = dynamic_cast< InputImageType * >( it.GetInput() );
    InputVectorImageType *vecInput = dynamic_cast< InputVectorImageType * >( it.GetInput() );
    if (input)
      {
      InputImageRegionType inputRegion;
      this->CallCopyOutputRegionToInputRegion( inputRegion, this->GetOutput()->GetRequestedRegion() );
      inputRegion.PadByRadius(radius);
      inputRegion.Crop(input->GetLargestPossibleRegion());
      input->SetRequestedRegion(inputRegion);
      }
    else if(vecInput)
      {
      InputImageRegionType inputRegion;
      this->CallCopyOutputRegionToInputRegion( inputRegion, this->GetOutput()->GetRequestedRegion() );
      inputRegion.PadByRadius(radius);
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

  // TODO: this should not be hard-coded
  typename InputImageType::SizeType radius; radius.Fill(2);

  // Adjust the output region so that we don't touch image boundaries.
  OutputImageRegionType crop_region = outputPtr->GetLargestPossibleRegion();
  crop_region.ShrinkByRadius(radius);
  OutputImageRegionType out_region = outputRegionForThread;
  out_region.Crop(crop_region);

  // Create an iterator for the output
  typedef itk::ImageRegionIterator<TOutputImage> OutputIter;
  OutputIter it_out(outputPtr, out_region);

  // Create a collection iterator for the inputs
  typedef ImageCollectionConstRegionIteratorWithIndex<
      TInputImage, TInputVectorImage> CollectionIter;

  // Configure the input collection iterator
  CollectionIter cit(out_region);
  for( itk::InputDataObjectIterator it( this ); !it.IsAtEnd(); it++ )
    cit.AddImage(it.GetInput());

  // TODO: This is hard-coded
  cit.SetRadius(radius);

  // Get the number of components
  int nComp = cit.GetTotalComponents();
  int nPatch = cit.GetNeighborhoodSize();
  int nColumns = nComp * nPatch;

  // Get the number of classes
  int nClass = m_Classifier->GetClassToLabelMapping().size();

  // Get the current class
  int activeClass = (int) m_Classifier->GetForegroundClass();

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

    // Perform classification on this data
    m_Classifier->GetForest()->ApplyFast(testData, testResult, vIndex, vResult);

    // Add up the predictions made by each tree for each class
    double p = 0;
    for(int i = 0; i < testResult.Size(); i++)
      {
      for(int j = 0; j < nClass; j++)
        {
        if(j == activeClass)
          p += testResult[i][0]->prob_[j];
        else
          p -= testResult[i][0]->prob_[j];
        }
      }
    p /= testResult.Size();

    // Presumably, at this point p stores the (p_fore - p_back) value
    it_out.Set((OutputPixelType)(p * 0x7fff));
    }
}


#endif
