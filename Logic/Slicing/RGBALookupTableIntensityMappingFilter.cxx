#include "RGBALookupTableIntensityMappingFilter.h"
#include "RLEImageRegionIterator.h"
#include "LookupTableTraits.h"
#include "IntensityToColorLookupTableImageFilter.h"

template<class TInputImage>
RGBALookupTableIntensityMappingFilter<TInputImage>
::RGBALookupTableIntensityMappingFilter()
{
  // Multiple images and the LUT are inputs
  this->SetNumberOfIndexedInputs(2);
}

template<class TInputImage>
void
RGBALookupTableIntensityMappingFilter<TInputImage>
::SetLookupTable(LookupTableType *lut)
{
  m_LookupTable = lut;
  this->SetNthInput(1, lut);
}

template<class TInputImage>
void
RGBALookupTableIntensityMappingFilter<TInputImage>
::DynamicThreadedGenerateData(const OutputImageRegionType &region)
{
  typedef LookupTableTraits<InputPixelType> LUTTraits;

  // Get all the inputs
  std::vector<const InputImageType *> inputs(3);
  for(int d = 0; d < 3; d++)
    inputs[d] = this->GetInput(d);

  // Get the output
  OutputImageType *output = this->GetOutput(0);

  // Get the range of intensities mapped that the LUT handles
  auto lut_i0 = this->m_LookupTable->m_StartValue;
  auto lut_i1 = this->m_LookupTable->m_EndValue;

  // Compute the shift and scale factors that map the input values into
  // the LUT values. These are ignored for integral pixel types, but used
  // for floating point types
  double lut_scale = LUTTraits::ComputeIntensityToLUTIndexScaleFactor(lut_i0, lut_i1);
  const auto &lut = this->m_LookupTable->m_LUT;

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
    if(xin0 == 0 && xin1 == 0 && xin2 == 0 && (lut_i0 > 0 || lut_i1 < 0))
      {
      xout.Fill(0);
      }
    else
      {
      xout[0] = lut[LUTTraits::ComputeLUTOffset(lut_scale, lut_i0, xin0)];
      xout[1] = lut[LUTTraits::ComputeLUTOffset(lut_scale, lut_i0, xin1)];
      xout[2] = lut[LUTTraits::ComputeLUTOffset(lut_scale, lut_i0, xin2)];
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
  typedef LookupTableTraits<InputPixelType> LUTTraits;

  // Update the lookup table
  m_LookupTable->Update();

  // Get the range of intensities mapped that the LUT handles
  auto lut_i0 = this->m_LookupTable->m_StartValue;
  auto lut_i1 = this->m_LookupTable->m_EndValue;

  // Compute the shift and scale factors that map the input values into
  // the LUT values. These are ignored for integral pixel types, but used
  // for floating point types
  double lut_scale = LUTTraits::ComputeIntensityToLUTIndexScaleFactor(lut_i0, lut_i1);
  const auto &lut = this->m_LookupTable->m_LUT;

  OutputPixelType xout;

  // TODO: we need to handle out of bounds voxels in non-orthogonal slicing
  // better than this, i.e., via a special value reserved for such voxels.
  // Right now, defaulting to zero is a DISASTER!
  if(xin0 == 0 && xin1 == 0 && xin2 == 0 && (lut_i0 > 0 || lut_i1 < 0))
    {
    xout.Fill(0);
    }
  else
    {
    // TODO: this will probably crash for floating point images.
    xout[0] = lut[LUTTraits::ComputeLUTOffset(lut_scale, lut_i0, xin0)];
    xout[1] = lut[LUTTraits::ComputeLUTOffset(lut_scale, lut_i0, xin1)];
    xout[2] = lut[LUTTraits::ComputeLUTOffset(lut_scale, lut_i0, xin2)];
    xout[3] = 255; // alpha = 1
    }

  return xout;
}


// Template instantiation
#define RGBALookupTableIntensityMappingFilterInstantiateMacro(type) \
  template class RGBALookupTableIntensityMappingFilter<itk::Image<type, 2> >;

RGBALookupTableIntensityMappingFilterInstantiateMacro(unsigned char)
RGBALookupTableIntensityMappingFilterInstantiateMacro(char)
RGBALookupTableIntensityMappingFilterInstantiateMacro(unsigned short)
RGBALookupTableIntensityMappingFilterInstantiateMacro(short)
RGBALookupTableIntensityMappingFilterInstantiateMacro(float)

