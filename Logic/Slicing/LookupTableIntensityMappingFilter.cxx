#include "LookupTableIntensityMappingFilter.h"
#include <itkImageRegionIterator.h>
#include <itkRGBAPixel.h>

template<class TInputImage, class TOutputImage>
LookupTableIntensityMappingFilter<TInputImage, TOutputImage>
::LookupTableIntensityMappingFilter()
{
  // The image and the LUT are inputs
  this->SetNumberOfInputs(2);
}

template<class TInputImage, class TOutputImage>
void
LookupTableIntensityMappingFilter<TInputImage, TOutputImage>
::SetLookupTable(LookupTableType *lut)
{
  m_LookupTable = lut;
  this->SetNthInput(1, lut);
}

template<class TInputImage, class TOutputImage>
void
LookupTableIntensityMappingFilter<TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType &region, int threadId)
{
  // Get the input, output and the LUT
  const InputImageType *input = this->GetInput();
  OutputImageType *output = this->GetOutput(0);

  // Get the pointer to the zero value in the LUT
  OutputPixelType *lutp =
      m_LookupTable->GetBufferPointer()
      - m_LookupTable->GetLargestPossibleRegion().GetIndex()[0];

  // Define the iterators
  itk::ImageRegionConstIterator<TInputImage> inputIt(input, region);
  itk::ImageRegionIterator<TOutputImage> outputIt(output, region);

  // Perform the intensity mapping using the LUT (no bounds checking!)
  while( !inputIt.IsAtEnd() )
    {
    InputPixelType xin = inputIt.Get();
    OutputPixelType xout = *(lutp + xin);
    outputIt.Set(xout);

    ++inputIt;
    ++outputIt;
    }
}

// Declare specific instances that will exist
template class LookupTableIntensityMappingFilter<
    itk::Image<short, 2>, itk::Image< itk::RGBAPixel<unsigned char> > >;
