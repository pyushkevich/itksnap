#include "RGBALookupTableIntensityMappingFilter.h"
#include "RLEImageRegionIterator.h"
#include "ColorLookupTable.h"

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
::DynamicThreadedGenerateData(const OutputImageRegionType &region)
{
  // Get all the inputs
  std::vector<const InputImageType *> inputs(3);
  for(int d = 0; d < 3; d++)
    inputs[d] = this->GetInput(d);

  // Get the output
  OutputImageType *output = this->GetOutput(0);

  // Get the range of intensities mapped that the LUT handles
  const LookupTableType *lut = this->GetLookupTable();

  // Does zero map out of the LUT's range? We may get inputs of zero from
  // the non-orthogonal slicer (data outside of image range) that would fall
  // outside of the colormap. This is really a poor way to handle this but
  // there is not a good alternative solution right now.
  // TODO: fix this.
  bool zero_out_of_range = !lut->CheckRange(0);

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
    if(zero_out_of_range && xin0 == 0 && xin1 == 0 && xin2 == 0)
      {
      xout.Fill(0);
      }
    else
      {
      xout[0] = lut->MapIntensityToDisplay(xin0);
      xout[1] = lut->MapIntensityToDisplay(xin1);
      xout[2] = lut->MapIntensityToDisplay(xin2);
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
  LookupTableType *lut = const_cast<LookupTableType *>(this->GetLookupTable());
  lut->Update();

  OutputPixelType xout;

  // TODO: we need to handle out of bounds voxels in non-orthogonal slicing
  // better than this, i.e., via a special value reserved for such voxels.
  // Right now, defaulting to zero is a DISASTER!
  if(xin0 == 0 && xin1 == 0 && xin2 == 0 && !lut->CheckRange(0))
    {
    xout.Fill(0);
    }
  else
    {
    // TODO: this will probably crash for floating point images.
    xout[0] = lut->MapIntensityToDisplay(xin0);
    xout[1] = lut->MapIntensityToDisplay(xin1);
    xout[2] = lut->MapIntensityToDisplay(xin2);
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

