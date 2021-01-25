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
::SetRangeInputs(const PixelObjectType *inMin, const PixelObjectType *inMax)
{
  m_InputMin = const_cast<PixelObjectType*>(inMin);
  m_InputMax = const_cast<PixelObjectType*>(inMax);
  this->SetNthInput(1, m_InputMin);
  this->SetNthInput(2, m_InputMax);
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

template< class TInputImage >
void
ThreadedHistogramImageFilter<TInputImage>
::GenerateData()
{
  this->AllocateOutputs();

  // Get the range of the histogram
  PixelType pxmin = m_InputMin->Get();
  PixelType pxmax = m_InputMax->Get();

  // Initialize the output
  m_OutputHistogram->Initialize(pxmin, pxmax, m_Bins);

  // A mutex to control updating the output histogram
  std::mutex histo_mutex;

  // Parrallel block
  itk::MultiThreaderBase::Pointer mt = itk::MultiThreaderBase::New();
  mt->ParallelizeImageRegion<Self::OutputImageDimension>(
        this->GetOutput()->GetBufferedRegion(),
        [this, pxmin, pxmax, &histo_mutex](const RegionType &region)
    {
    // Create a thread-local histogram
    HistogramType::Pointer local_hist = HistogramType::New();
    local_hist->Initialize(pxmin, pxmax, m_Bins);

    // Compute the histogram
    for(itk::ImageRegionConstIterator< TInputImage > it(this->GetInput(), region);
        !it.IsAtEnd(); ++it)
      {
      local_hist->AddSample(it.Get());
      }

    // In a reentrant block, update the main histogram
    std::lock_guard<std::mutex> guard(histo_mutex);
    m_OutputHistogram->AddCompatibleHistogram(local_hist);
    }, nullptr);

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
