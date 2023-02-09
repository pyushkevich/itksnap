#include "LookupTableIntensityMappingFilter.h"
#include "RLEImageRegionIterator.h"
#include <itkRGBAPixel.h>
#include "LookupTableTraits.h"
#include "IntensityToColorLookupTableImageFilter.h"

template<class TInputImage, class TOutputImage>
LookupTableIntensityMappingFilter<TInputImage, TOutputImage>
::LookupTableIntensityMappingFilter()
{
  // The image and the LUT are inputs
  this->SetNumberOfIndexedInputs(2);
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
::DynamicThreadedGenerateData(const OutputRegionType &region)
{
  typedef LookupTableTraits<InputComponentType> LUTTraits;

  // Get the input and output images
  const InputImageType *input = this->GetInput();
  OutputImageType *output = this->GetOutput(0);

  // Get the range of intensities mapped that the LUT handles
  InputComponentType lut_i0 = this->m_LookupTable->m_StartValue;
  InputComponentType lut_i1 = this->m_LookupTable->m_EndValue;

  // How do we map from the intensity value to the LUT location?
  // - for small integral types, the LUT starts at the minimim
  //   intensity value, stored in lut_i0 and runs through the
  //   maximum intensity value. One LUT entry corresponds to one
  //   unit of intensity.
  //     pos_lut = intensity - lut_i0
  // - for floating point types, the LUT maps to the range
  //   lut_i0 ... lut_i1. There are 10001 LUT entries corresponding
  //   to this range. So
  //     pos_lut = (intensity - lut_i0) * N / (lut_i1 - lut_i0)
  // The function below computes the scaling factor needed to do this
  // mapping.

  // Compute the shift and scale factors that map the input values into
  // the LUT values. These are ignored for integral pixel types, but used
  // for floating point types
  double lut_scale = LUTTraits::ComputeIntensityToLUTIndexScaleFactor(lut_i0, lut_i1);
  const auto &lut = this->m_LookupTable->m_LUT;

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
    if(xin == 0 && (lut_i0 > 0 || lut_i1 < 0))
      {
      // Special case: intensity is actually outside of the min/max range
      xout.Fill(0);
      }
    else
      {
      // TODO: handle NAN
      // Find the corresponding LUT offset
      int lut_offset = LUTTraits::ComputeLUTOffset(lut_scale, lut_i0, xin);
      xout = lut[lut_offset];
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
  typedef LookupTableTraits<InputComponentType> LUTTraits;

  // Make sure all the inputs are up to date
  m_LookupTable->Update();

  // Get the range of intensities mapped that the LUT handles
  InputComponentType lut_i0 = this->m_LookupTable->m_StartValue;
  InputComponentType lut_i1 = this->m_LookupTable->m_EndValue;

  // Compute the shift and scale factors that map the input values into
  // the LUT values. These are ignored for integral pixel types, but used
  // for floating point types
  double lut_scale = LUTTraits::ComputeIntensityToLUTIndexScaleFactor(lut_i0, lut_i1);
  const auto &lut = this->m_LookupTable->m_LUT;

  // Get the input intensity
  OutputPixelType xout;

  // TODO: we need to handle out of bounds voxels in non-orthogonal slicing
  // better than this, i.e., via a special value reserved for such voxels.
  // Right now, defaulting to zero is a DISASTER!
  if(xin == 0 && (lut_i0 > 0 || lut_i1 < 0))
    {
    // Special case: intensity is actually outside of the min/max range
    xout.Fill(0);
    }
  else
    {
    // TODO: handle NAN
    // Find the corresponding LUT offset
    int lut_offset = LookupTableTraits<InputPixelType>::ComputeLUTOffset(lut_scale, lut_i0, xin);
    xout = lut[lut_offset];
    }

  return xout;
}

// Template instantiation
#define LookupTableIntensityMappingFilterInstantiateMacro(type) \
  template class LookupTableIntensityMappingFilter<itk::Image<type, 2>, itk::Image< itk::RGBAPixel<unsigned char> > >;

LookupTableIntensityMappingFilterInstantiateMacro(unsigned char)
LookupTableIntensityMappingFilterInstantiateMacro(char)
LookupTableIntensityMappingFilterInstantiateMacro(unsigned short)
LookupTableIntensityMappingFilterInstantiateMacro(short)
LookupTableIntensityMappingFilterInstantiateMacro(float)


