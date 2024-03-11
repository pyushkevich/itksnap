#include "LookupTableIntensityMappingFilter.h"
#include "RLEImageRegionIterator.h"
#include <itkRGBAPixel.h>
#include "ColorLookupTable.h"

template<class TInputImage, class TOutputImage>
LookupTableIntensityMappingFilter<TInputImage, TOutputImage>
::LookupTableIntensityMappingFilter()
{
  // The image and the LUT are inputs
  this->SetNumberOfIndexedInputs(1);
  this->AddRequiredInputName("LookupTable");
}

template<class TInputImage, class TOutputImage>
void
LookupTableIntensityMappingFilter<TInputImage, TOutputImage>
::DynamicThreadedGenerateData(const OutputRegionType &region)
{
  // Get the input and output images
  const InputImageType *input = this->GetInput();
  OutputImageType *output = this->GetOutput(0);

  // Get the range of intensities mapped that the LUT handles
  const LookupTableType *lut = this->GetLookupTable();

  // Define the iterators
  itk::ImageRegionConstIterator<TInputImage> inputIt(input, region);
  itk::ImageRegionIterator<TOutputImage> outputIt(output, region);

  // Does zero map out of the LUT's range? We may get inputs of zero from
  // the non-orthogonal slicer (data outside of image range) that would fall
  // outside of the colormap. This is really a poor way to handle this but
  // there is not a good alternative solution right now.
  // TODO: fix this.
  bool zero_out_of_range = !lut->CheckRange(0);

  // Perform the intensity mapping using the LUT (no bounds checking!)
  while( !inputIt.IsAtEnd() )
    {
    // Get the input intensity
    InputPixelType xin = inputIt.Get();
    OutputPixelType xout;

    if(zero_out_of_range && xin == 0)
      {
      // Special case: intensity is actually outside of the min/max range
      xout.Fill(0);
      }
    else
      {
      xout = lut->MapIntensityToDisplay(xin);
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
  LookupTableType *lut = const_cast<LookupTableType *>(this->GetLookupTable());
  lut->Update();

  // Get the input intensity
  OutputPixelType xout;

  // TODO: we need to handle out of bounds voxels in non-orthogonal slicing
  // better than this, i.e., via a special value reserved for such voxels.
  // Right now, defaulting to zero is a DISASTER!
  if(xin == 0 && !lut->CheckRange(0))
    {
    // Special case: intensity is actually outside of the min/max range
    xout.Fill(0);
    }
  else
    {
    xout = lut->MapIntensityToDisplay(xin);
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
LookupTableIntensityMappingFilterInstantiateMacro(double)

