#include "RGBALookupTableIntensityMappingFilter.h"
#include "RLEImageRegionIterator.h"

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

  // Lookup table range
  int lut_min = m_LookupTable->GetLargestPossibleRegion().GetIndex()[0];
  int lut_max = lut_min + m_LookupTable->GetLargestPossibleRegion().GetSize()[0] - 1;

  // Get the pointer to the zero value in the LUT
  OutputComponentType *lutp = m_LookupTable->GetBufferPointer() - lut_min;

  // Define the iterators
  typedef itk::ImageRegionConstIterator<InputImageType> InputIteratorType;
  InputIteratorType it0(inputs[0], region);
  InputIteratorType it1(inputs[1], region);
  InputIteratorType it2(inputs[2], region);

  itk::ImageRegionIterator<OutputImageType> outputIt(output, region);

  // Perform the intensity mapping using the LUT (no bounds checking!)
  while( !outputIt.IsAtEnd() )
    {
    OutputPixelType xout;

    InputPixelType xin0 = it0.Get();
    InputPixelType xin1 = it1.Get();
    InputPixelType xin2 = it2.Get();

    // TODO: we need to handle out of bounds voxels in non-orthogonal slicing
    // better than this, i.e., via a special value reserved for such voxels.
    // Right now, defaulting to zero is a DISASTER!
    if(xin0 == 0 && xin1 == 0 && xin2 == 0 && (lut_min > 0 || lut_max < 0))
      {
      xout.Fill(0);
      }
    else
      {
      xout[0] = *(lutp + xin0);
      xout[1] = *(lutp + xin1);
      xout[2] = *(lutp + xin2);
      xout[3] = 255; // alpha = 1
      }

    outputIt.Set(xout);
    ++it0; ++it1; ++it2;
    ++outputIt;
    }
}

template<class TInputImage>
typename RGBALookupTableIntensityMappingFilter<TInputImage>::OutputPixelType
RGBALookupTableIntensityMappingFilter<TInputImage>
::MapPixel(const InputPixelType &xin0, const InputPixelType &xin1, const InputPixelType &xin2)
{
  // Update the lookup table
  m_LookupTable->Update();

  // Lookup table range
  int lut_min = m_LookupTable->GetLargestPossibleRegion().GetIndex()[0];
  int lut_max = lut_min + m_LookupTable->GetLargestPossibleRegion().GetSize()[0] - 1;

  // Get the pointer to the zero value in the LUT
  OutputComponentType *lutp = m_LookupTable->GetBufferPointer() - lut_min;

  OutputPixelType xout;

  // TODO: we need to handle out of bounds voxels in non-orthogonal slicing
  // better than this, i.e., via a special value reserved for such voxels.
  // Right now, defaulting to zero is a DISASTER!
  if(xin0 == 0 && xin1 == 0 && xin2 == 0 && (lut_min > 0 || lut_max < 0))
    {
    xout.Fill(0);
    }
  else
    {
    xout[0] = *(lutp + xin0);
    xout[1] = *(lutp + xin1);
    xout[2] = *(lutp + xin2);
    xout[3] = 255; // alpha = 1
    }

  return xout;
}

// Declare specific instances that will exist
template class RGBALookupTableIntensityMappingFilter< itk::Image<short, 2> >;

