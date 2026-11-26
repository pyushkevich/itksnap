#include "RGBALookupTableIntensityMappingFilter.h"
#include "RLEImageRegionIterator.h"
#include "ColorLookupTable.h"
#include "vtkMath.h"
#include <cmath>

template<class TInputImage>
RGBALookupTableIntensityMappingFilter<TInputImage>
::RGBALookupTableIntensityMappingFilter()
{
  // Multiple images are indexed inputs
  this->SetNumberOfRequiredInputs(3);
  this->AddRequiredInputName("LookupTable");
}

template<class TInputImage>
void
RGBALookupTableIntensityMappingFilter<TInputImage>
::SetMappingModeToTwoChannelHueValue()
{
  this->m_TwoChannelHueValueMode = true;
  this->SetNumberOfRequiredInputs(2);
}

template<class TInputImage>
void
RGBALookupTableIntensityMappingFilter<TInputImage>
::SetMappingModeToThreeChannelRGB()
{
  this->m_TwoChannelHueValueMode = false;
  this->SetNumberOfRequiredInputs(3);
}

template<class TInputImage>
void
RGBALookupTableIntensityMappingFilter<TInputImage>
::MapPixelXYZtoRGB(
    InputPixelType xin0, InputPixelType xin1, InputPixelType xin2,
    const LookupTableType *lut, OutputPixelType &xout)
{
    xout[0] = lut->MapIntensityToDisplay(xin0);
    xout[1] = lut->MapIntensityToDisplay(xin1);
    xout[2] = lut->MapIntensityToDisplay(xin2);
    xout[3] = 255; // alpha = 1
}

template<class TInputImage>
void
RGBALookupTableIntensityMappingFilter<TInputImage>
::MapPixelXYtoHSV(
    InputPixelType xin0, InputPixelType xin1,
    const LookupTableType *lut, OutputPixelType &xout)
{
    // Get normalized vectors
    float x_norm = lut->MapIntensityToDisplay(xin0) / 255. - 0.5;
    float y_norm = lut->MapIntensityToDisplay(xin1) / 255. - 0.5;

    // Compute the phase
    float phase = ::atan2(y_norm, x_norm), mag = ::sqrt(x_norm*x_norm + y_norm*y_norm);
    float r,g,b;
    vtkMath::HSVToRGB(0.5 + phase / 6.28318530718, 1.0, ::fminf(1.0f, mag), &r, &g, &b);

    xout[0] = (unsigned char) (255 * r);
    xout[1] = (unsigned char) (255 * g);
    xout[2] = (unsigned char) (255 * b);
    xout[3] = 255; // alpha = 1
}

template<class TInputImage>
void
RGBALookupTableIntensityMappingFilter<TInputImage>
::DynamicThreadedGenerateData(const OutputImageRegionType &region)
{
  // Get the output
  OutputImageType *output = this->GetOutput(0);

  // Get the range of intensities mapped that the LUT handles
  const LookupTableType *lut = this->GetLookupTable();

  itk::ImageRegionIterator<OutputImageType> outputIt(output, region);

  // The mask, if any, marks the pixels outside of the image, which are made
  // fully transparent
  const MaskImageType *mask = this->GetMaskInput();
  itk::ImageRegionConstIterator<MaskImageType> maskIt;
  if(mask)
    maskIt = itk::ImageRegionConstIterator<MaskImageType>(mask, region);

  if(this->m_TwoChannelHueValueMode)
  {
      // Standard XY to Hue/Value mapping mode

      // Get all the inputs
      std::vector<const InputImageType *> inputs(2);
      for(int d = 0; d < 2; d++)
          inputs[d] = this->GetInput(d);

      // Define the iterators
      typedef itk::ImageRegionConstIterator<InputImageType> InputIteratorType;
      InputIteratorType it0(inputs[0], region);
      InputIteratorType it1(inputs[1], region);

      // Perform the intensity mapping using the LUT (no bounds checking!)
      while( !outputIt.IsAtEnd() )
      {
          OutputPixelType xout;

          if(mask && !maskIt.Get())
            xout.Fill(0);
          else
            this->MapPixelXYtoHSV(it0.Get(), it1.Get(), lut, xout);
          outputIt.Set(xout);
          ++it0; ++it1;
          ++outputIt;
          if(mask)
            ++maskIt;
      }

  }
  else
  {
      // Standard XYZ to RGB mapping mode

      // Get all the inputs
      std::vector<const InputImageType *> inputs(3);
      for(int d = 0; d < 3; d++)
          inputs[d] = this->GetInput(d);

      // Define the iterators
      typedef itk::ImageRegionConstIterator<InputImageType> InputIteratorType;
      InputIteratorType it0(inputs[0], region);
      InputIteratorType it1(inputs[1], region);
      InputIteratorType it2(inputs[2], region);

      // Perform the intensity mapping using the LUT (no bounds checking!)
      while( !outputIt.IsAtEnd() )
      {
          OutputPixelType xout;
          if(mask && !maskIt.Get())
            xout.Fill(0);
          else
            this->MapPixelXYZtoRGB(it0.Get(), it1.Get(), it2.Get(), lut, xout);
          outputIt.Set(xout);
          ++it0; ++it1; ++it2;
          ++outputIt;
          if(mask)
            ++maskIt;
      }
  }
}

template<class TInputImage>
typename RGBALookupTableIntensityMappingFilter<TInputImage>::OutputPixelType
RGBALookupTableIntensityMappingFilter<TInputImage>
::MapPixel(const InputPixelType &xin0, const InputPixelType &xin1, const InputPixelType &xin2)
{
  // Update the lookup table
  LookupTableType *lut = const_cast<LookupTableType *>(this->GetLookupTable());
  lut->Update();

  // This is used for single values (e.g., under the cursor), which have no mask
  OutputPixelType xout;
  if(this->m_TwoChannelHueValueMode)
      this->MapPixelXYtoHSV(xin0, xin1, lut, xout);
  else
      this->MapPixelXYZtoRGB(xin0, xin1, xin2, lut, xout);
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
RGBALookupTableIntensityMappingFilterInstantiateMacro(double)
