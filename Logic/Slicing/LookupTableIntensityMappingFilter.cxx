#include "LookupTableIntensityMappingFilter.h"
#include "RLEImageRegionIterator.h"
#include <itkRGBAPixel.h>
#include "LookupTableTraits.h"

template<class TInputImage, class TOutputImage>
LookupTableIntensityMappingFilter<TInputImage, TOutputImage>
::LookupTableIntensityMappingFilter()
{
  // The image and the LUT are inputs
  this->SetNumberOfIndexedInputs(4);
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
::SetImageMinInput(InputPixelObject *input)
{
  m_InputMin = input;
  this->SetNthInput(2, input);
}

template<class TInputImage, class TOutputImage>
void
LookupTableIntensityMappingFilter<TInputImage, TOutputImage>
::SetImageMaxInput(InputPixelObject *input)
{
  m_InputMax = input;
  this->SetNthInput(3, input);
}

template<class TInputImage, class TOutputImage>
void
LookupTableIntensityMappingFilter<TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType &region,
                       itk::ThreadIdType threadId)
{
  // Get the input, output and the LUT
  const InputImageType *input = this->GetInput();
  OutputImageType *output = this->GetOutput(0);

  // Get the pointer to the zero value in the LUT
  OutputPixelType *lutp =
      m_LookupTable->GetBufferPointer()
      - m_LookupTable->GetLargestPossibleRegion().GetIndex()[0];

  // Range of the input image
  InputPixelType input_min = m_InputMin->Get();
  InputPixelType input_max = m_InputMax->Get();

  // Compute the shift and scale factors that map the input values into
  // the LUT values. These are ignored for integral pixel types, but used
  // for floating point types
  float lutScale;
  InputPixelType lutShift;
  LookupTableTraits<InputPixelType>::ComputeLinearMappingToLUT(
        input_min, input_max, lutScale, lutShift);

  // Define the iterators
  itk::ImageRegionConstIterator<TInputImage> inputIt(input, region);
  itk::ImageRegionIterator<TOutputImage> outputIt(output, region);

  // Perform the intensity mapping using the LUT (no bounds checking!)
  while( !inputIt.IsAtEnd() )
    {
    // Get the input intensity
    InputPixelType xin = inputIt.Get();
    OutputPixelType xout;

    // TODO: we need to handle out of bounds voxels in non-orthogonal slicing
    // better than this, i.e., via a special value reserved for such voxels.
    // Right now, defaulting to zero is a DISASTER!
    if(xin == 0 && (input_min > 0 || input_max < 0))
      {
      // Special case: intensity is actually outside of the min/max range
      xout.Fill(0);
      }
    else
      {
      // Find the corresponding LUT offset
      int lut_offset = LookupTableTraits<InputPixelType>::ComputeLUTOffset(
            lutScale, lutShift, xin);

      xout = *(lutp + lut_offset);
      }

    outputIt.Set(xout);

    ++inputIt;
    ++outputIt;
    }
}

template<class TInputImage, class TOutputImage>
typename LookupTableIntensityMappingFilter<TInputImage, TOutputImage>::OutputPixelType
LookupTableIntensityMappingFilter<TInputImage, TOutputImage>
::MapPixel(const InputPixelType &xin)
{
  // Make sure all the inputs are up to date
  m_InputMin->Update();
  m_InputMax->Update();
  m_LookupTable->Update();

  // Get the pointer to the zero value in the LUT
  OutputPixelType *lutp =
      m_LookupTable->GetBufferPointer()
      - m_LookupTable->GetLargestPossibleRegion().GetIndex()[0];

  // Range of the input image
  InputPixelType input_min = m_InputMin->Get();
  InputPixelType input_max = m_InputMax->Get();

  // Compute the shift and scale factors that map the input values into
  // the LUT values. These are ignored for integral pixel types, but used
  // for floating point types
  float lutScale;
  InputPixelType lutShift;
  LookupTableTraits<InputPixelType>::ComputeLinearMappingToLUT(
        input_min, input_max, lutScale, lutShift);

  // Get the input intensity
  OutputPixelType xout;

  // TODO: we need to handle out of bounds voxels in non-orthogonal slicing
  // better than this, i.e., via a special value reserved for such voxels.
  // Right now, defaulting to zero is a DISASTER!
  if(xin == 0 && (input_min > 0 || input_max < 0))
    {
    // Special case: intensity is actually outside of the min/max range
    xout.Fill(0);
    }
  else
    {
    // Find the corresponding LUT offset
    int lut_offset = LookupTableTraits<InputPixelType>::ComputeLUTOffset(
          lutScale, lutShift, xin);

    xout = *(lutp + lut_offset);
    }

  return xout;
}

// Declare specific instances that will exist
template class LookupTableIntensityMappingFilter<
    itk::Image<short, 2>, itk::Image< itk::RGBAPixel<unsigned char> > >;

template class LookupTableIntensityMappingFilter<
    itk::Image<float, 2>, itk::Image< itk::RGBAPixel<unsigned char> > >;


