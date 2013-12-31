#include "RGBALookupTableIntensityMappingFilter.h"
#include "itkImageRegionIterator.h"

template<class TInputImage>
RGBALookupTableIntensityMappingFilter<TInputImage>
::RGBALookupTableIntensityMappingFilter()
{
  // Multiple images and the LUT are inputs
  this->SetNumberOfIndexedInputs(4);
}

template<class TInputImage>
void
RGBALookupTableIntensityMappingFilter<TInputImage>
::SetLookupTable(LookupTableType *lut)
{
  m_LookupTable = lut;
  this->SetNthInput(3, lut);
}

template<class TInputImage>
void
RGBALookupTableIntensityMappingFilter<TInputImage>
::ThreadedGenerateData(const OutputImageRegionType &region,
                       itk::ThreadIdType threadId)
{
  // Get all the inputs
  std::vector<const InputImageType *> inputs(3);
  for(int d = 0; d < 3; d++)
    inputs[d] = this->GetInput(d);

  // Get the output
  OutputImageType *output = this->GetOutput(0);

  // Get the pointer to the zero value in the LUT
  OutputComponentType *lutp =
      m_LookupTable->GetBufferPointer()
      - m_LookupTable->GetLargestPossibleRegion().GetIndex()[0];

  // Define the iterators
  typedef itk::ImageRegionConstIterator<InputImageType> InputIteratorType;
  std::vector<InputIteratorType> inputIt;
  for(int d = 0; d < 3; d++)
    inputIt.push_back(InputIteratorType(inputs[d], region));
  itk::ImageRegionIterator<OutputImageType> outputIt(output, region);

  // Perform the intensity mapping using the LUT (no bounds checking!)
  while( !outputIt.IsAtEnd() )
    {
    OutputPixelType xout;
    for(int d = 0; d < 3; d++)
      {
      InputPixelType xin = inputIt[d].Get();
      xout[d] = *(lutp + xin);
      ++inputIt[d];
      }

    xout[3] = 255; // alpha = 1
    outputIt.Set(xout);
    ++outputIt;
    }
}

// Declare specific instances that will exist
template class RGBALookupTableIntensityMappingFilter< itk::Image<short, 2> >;

