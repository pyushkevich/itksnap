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

  // The mask, if any, marks the pixels outside of the image
  const MaskImageType *mask = this->GetMaskInput();
  if(mask)
    {
    itk::ImageRegionConstIterator<MaskImageType> maskIt(mask, region);
    for( ; !inputIt.IsAtEnd(); ++inputIt, ++outputIt, ++maskIt)
      {
      OutputPixelType xout;
      if(maskIt.Get())
        xout = lut->MapIntensityToDisplay(inputIt.Get());
      else
        xout.Fill(0);
      outputIt.Set(xout);
      }
    }
  else
    {
    for( ; !inputIt.IsAtEnd(); ++inputIt, ++outputIt)
      outputIt.Set(lut->MapIntensityToDisplay(inputIt.Get()));
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

  // Map the intensity. This is used for single values (e.g., under the
  // cursor), which have no mask
  return lut->MapIntensityToDisplay(xin);
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

