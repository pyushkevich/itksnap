#ifndef THREADEDHISTOGRAMIMAGEFILTER_HXX
#define THREADEDHISTOGRAMIMAGEFILTER_HXX

#include "ThreadedHistogramImageFilter.h"
#include <itkProgressReporter.h>
#include <itkImageRegionConstIterator.h>

template <class TInputImage>
ThreadedHistogramImageFilter<TInputImage>
::ThreadedHistogramImageFilter()
{
  this->SetNumberOfRequiredInputs(3);
  this->SetNumberOfRequiredOutputs(2);

  // Allocate the output histogram
  m_OutputHistogram = ScalarImageHistogram::New();
  this->SetNthOutput(1, m_OutputHistogram);

  m_Bins = 0;
  m_TransformScale = 1.0;
  m_TransformShift = 0.0;
}

template <class TInputImage>
void
ThreadedHistogramImageFilter<TInputImage>
::SetRangeInputs(PixelObjectType *inMin, PixelObjectType *inMax)
{
  m_InputMin = inMin;
  m_InputMax = inMax;
  this->SetNthInput(1, inMin);
  this->SetNthInput(2, inMax);
}

template <class TInputImage>
void
ThreadedHistogramImageFilter<TInputImage>
::SetNumberOfBins(int nBins)
{
  if(m_Bins != nBins)
    {
    m_Bins = nBins;
    this->Modified();
    }
}

template <class TInputImage>
void
ThreadedHistogramImageFilter<TInputImage>
::SetIntensityTransform(double scale, double shift)
{
  if(m_TransformScale != scale || m_TransformShift != shift)
    {
    m_TransformScale = scale;
    m_TransformShift = shift;
    this->Modified();
    }
}

template <class TInputImage>
void
ThreadedHistogramImageFilter<TInputImage>
::GenerateInputRequestedRegion()
{
  Superclass::GenerateInputRequestedRegion();
  if ( this->GetInput() )
    {
    InputImagePointer image =
      const_cast< typename Superclass::InputImageType * >( this->GetInput() );
    image->SetRequestedRegionToLargestPossibleRegion();
    }
}

template <class TInputImage>
void
ThreadedHistogramImageFilter<TInputImage>
::EnlargeOutputRequestedRegion(itk::DataObject *data)
{
  Superclass::EnlargeOutputRequestedRegion(data);
  data->SetRequestedRegionToLargestPossibleRegion();
}

template <class TInputImage>
void
ThreadedHistogramImageFilter<TInputImage>
::AllocateOutputs()
{
  // Pass the input through as the output
  InputImagePointer image =
    const_cast< TInputImage * >( this->GetInput() );

  this->GraftOutput(image);

  // Nothing to be done for the histogram output
}

template <class TInputImage>
void
ThreadedHistogramImageFilter<TInputImage>
::BeforeThreadedGenerateData()
{
  itk::ThreadIdType numberOfThreads = this->GetNumberOfThreads();

  // Get the range of the histogram
  PixelType pxmin = m_InputMin->Get();
  PixelType pxmax = m_InputMax->Get();

  // Initialize the per-thread histograms
  m_ThreadHistogram.resize(numberOfThreads);
  for(unsigned int i = 0; i < numberOfThreads; i++)
    {
    m_ThreadHistogram[i] = HistogramType::New();
    m_ThreadHistogram[i]->Initialize(pxmin, pxmax, m_Bins);
    }

  // Initialize the output
  m_OutputHistogram->Initialize(pxmin, pxmax, m_Bins);
}

template< class TInputImage >
void
ThreadedHistogramImageFilter<TInputImage>
::ThreadedGenerateData(const RegionType &outputRegionForThread,
                       itk::ThreadIdType threadId)
{
  if ( outputRegionForThread.GetNumberOfPixels() == 0 )
    return;

  // Get the histogram for this thread
  HistogramType *hist = m_ThreadHistogram[threadId];

  itk::ImageRegionConstIterator< TInputImage > it(this->GetInput(), outputRegionForThread);
  while(!it.IsAtEnd())
    {
    hist->AddSample(it.Get());
    ++it;
    }
}

template< class TInputImage >
void
ThreadedHistogramImageFilter<TInputImage>
::AfterThreadedGenerateData()
{
  // Add up the partial histograms
  for(unsigned int i = 0; i < m_ThreadHistogram.size(); i++)
    {
    m_OutputHistogram->AddCompatibleHistogram(*m_ThreadHistogram[i]);
    }

  // Apply the transform to the histogram
  m_OutputHistogram->ApplyIntensityTransform(m_TransformScale, m_TransformShift);
}

template< class TInputImage >
void
ThreadedHistogramImageFilter<TInputImage>
::PrintSelf(std::ostream &os, itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}





#endif // THREADEDHISTOGRAMIMAGEFILTER_HXX
