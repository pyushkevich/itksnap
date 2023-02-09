
#include "IntensityToColorLookupTableImageFilter.h"

#include "LookupTableTraits.h"
#include "IntensityCurveInterface.h"
#include "itkImage.h"
#include "itkVectorImage.h"
#include "VectorToScalarImageAccessor.h"
#include "itkMultiThreaderBase.h"

/* ===============================================================
    AbstractLookupTableImageFilter implementation
   =============================================================== */


template <class TInputImage, class TColorMapTraits>
IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>
::IntensityToColorLookupTableImageFilter()
{
  this->SetNumberOfRequiredInputs(1);
  this->SetNumberOfRequiredOutputs(2);

  // By default not using a reference range
  m_UseReferenceRange = false;
  m_ImageMinInput = NULL;
  m_ImageMaxInput = NULL;

  // Allocate the output LUT and assign as output
  m_LookupTable = LookupTableType::New();
  this->SetNthOutput(1, m_LookupTable);
}

template <class TInputImage, class TColorMapTraits>
void
IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>
::SetImageMinInput(const MinMaxObjectType *input)
{
  m_ImageMinInput = const_cast<MinMaxObjectType *>(input);
  this->SetInput("image_min", m_ImageMinInput);
}

template <class TInputImage, class TColorMapTraits>
void
IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>
::SetImageMaxInput(const MinMaxObjectType *input)
{
  m_ImageMaxInput = const_cast<MinMaxObjectType *>(input);
  this->SetInput("image_max", m_ImageMaxInput);
}

template <class TInputImage, class TColorMapTraits>
void
IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>
::SetReferenceIntensityRange(double min, double max)
{
  m_UseReferenceRange = true;
  m_ReferenceMin = (ComponentType) min;
  m_ReferenceMax = (ComponentType) max;
  this->Modified();
}

template <class TInputImage, class TColorMapTraits>
void
IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>
::RemoveReferenceIntensityRange()
{
  m_UseReferenceRange = false;
  this->Modified();
}

template <class TInputImage, class TColorMapTraits>
void
IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>
::AllocateOutputs()
{
  // Just pass the container to the output image
  ImageType *inputPtr = const_cast<ImageType *>(this->GetInput());
  ImageType *outputPtr = this->GetOutput();
  outputPtr->CopyInformation(inputPtr);
  outputPtr->SetBufferedRegion(inputPtr->GetBufferedRegion());
  outputPtr->SetPixelContainer(inputPtr->GetPixelContainer());
}

/*
template <class TInputImage, class TColorMapTraits>
void
IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>
::GenerateOutputInformation()
{
  // Since this method could be called before the upstream pipeline has
  // executed, we need to force a all to Update on the input min/max
  m_ImageMinInput->Update();

  LookupTableType *output = this->GetOutput();

  // Use the type-specific traits to compute the range of the lookup table
  typename LookupTableType::RegionType region =
      LookupTableTraits<InputComponentType>::ComputeLUTRange(
        m_ImageMinInput->Get(), m_ImageMaxInput->Get());

  // TODO: remove
  output->SetLargestPossibleRegion(region);
}

template <class TInputImage, class TColorMapTraits>
void
IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>
::EnlargeOutputRequestedRegion(itk::DataObject *)
{
  LookupTableType *output = this->GetOutput();
  output->SetRequestedRegion(output->GetLargestPossibleRegion());
}
*/

template <class TInputImage, class TColorMapTraits>
void
IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>
::GenerateInputRequestedRegion()
{
  // The input region is the whole image
  ImageType *input = const_cast<ImageType *>(this->GetInput());
  input->SetRequestedRegionToLargestPossibleRegion();
}

template <class TInputImage, class TColorMapTraits>
void
IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>
::GenerateData()
{
  typedef LookupTableTraits<ComponentType> LUTTraits;

  // Allocate the image output
  this->AllocateOutputs();

  // What we do next really depends on TComponent. For char/short, we just want to
  // cache the RGB value for every possible intensity between image min and image
  // max, so we can map intensity to color by trivial lookup. For floating point
  // images, we want the LUT to be 10000 values between the min and max of the
  // intensity curve. We compute everything here and let the traits decide.
  ComponentType imin = m_UseReferenceRange ? m_ReferenceMin : m_ImageMinInput->Get();
  ComponentType imax = m_UseReferenceRange ? m_ReferenceMax : m_ImageMaxInput->Get();

  // Range of the curve (a pair)
  auto [tmin, tmax] = m_IntensityCurve->GetRange();

  // Get the region representing the LUT, for multithreading
  unsigned int lut_size = LUTTraits::GetLUTSize(imin, imax);

  // Get the mapping from index into lut_region to the curve coordinate value. For
  // short/char imin->0 and imax->1. For float, 0->trange[0] and 10000->trange[1]
  double scale, shift;
  LUTTraits::ComputeLinearMappingToUnitInterval(imin, imax, tmin, tmax, scale, shift);

  // Allocate the LUT
  m_LookupTable->m_LUT.resize(lut_size);

  // Set the starting and ending indices of the LUT. For short/char this is the
  // minimum intensity, and for float, this is tmin
  LUTTraits::GetLUTIntensityRange(imin, imax, tmin, tmax,
                                  m_LookupTable->m_StartValue, m_LookupTable->m_EndValue);

  // Multi-threaded computation
  itk::MultiThreaderBase::Pointer mt = itk::MultiThreaderBase::New();
  itk::ImageRegion<1> lut_region;
  lut_region.SetSize(0, lut_size);
  mt->ParallelizeImageRegion<1>(lut_region,
        [this, scale, shift, lut_region](const auto &thread_region)
    {
    // Iterate over the range of LUT entries we are computing
    int i0 = (int) thread_region.GetIndex()[0];
    int i1 = i0 + (int) thread_region.GetSize()[0];
    for(int i = i0; i < i1; i++)
      {
      // This is the t coordinate of the intensity curve to loop up
      double t = (i - shift) * scale;

      // Get the corresponding color map index
      double x = m_IntensityCurve->Evaluate(t);

      // Finally, we use the color map to send this to RGBA or if there
      // is no color map, just scale it to the 0-255 range.
      DisplayPixelType rgb = TColorMapTraits::apply(m_ColorMap, x);

      // Assign to colormap
      m_LookupTable->m_LUT[i] = rgb;
      }
    }, nullptr);
}

template <class TInputImage, class TColorMapTraits>
void
IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>
::SetFixedLookupTableRange(ComponentType imin, ComponentType imax)
{
  SmartPtr<MinMaxObjectType> omin = MinMaxObjectType::New();
  omin->Set(imin);
  this->SetImageMinInput(omin);

  SmartPtr<MinMaxObjectType> omax = MinMaxObjectType::New();
  omax->Set(imax);
  this->SetImageMaxInput(omax);
}

template <class TInputImage, class TColorMapTraits>
void
IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>
::SetIntensityCurve(IntensityCurveInterface *curve)
{
  m_IntensityCurve = curve;
  this->SetInput("curve", curve);
}

template <class TInputImage, class TColorMapTraits>
void
IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>
::SetColorMap(ColorMap *map)
{
  m_ColorMap = map;
  this->SetInput("colormap", map);
}



#define LookupTableImageFilterInstantiateMacro(type) \
  template class IntensityToColorLookupTableImageFilter<itk::Image<type, 3>, DefaultColorMapTraits>; \
  template class IntensityToColorLookupTableImageFilter<itk::VectorImage<type, 3>, VectorToRGBColorMapTraits>; \
  template class IntensityToColorLookupTableImageFilter<typename VectorToScalarImageAccessorTypes<type>::ComponentImageAdaptor, DefaultColorMapTraits>; \
  template class IntensityToColorLookupTableImageFilter<typename VectorToScalarImageAccessorTypes<type>::MagnitudeImageAdaptor, DefaultColorMapTraits>; \
  template class IntensityToColorLookupTableImageFilter<typename VectorToScalarImageAccessorTypes<type>::MaxImageAdaptor, DefaultColorMapTraits>; \
  template class IntensityToColorLookupTableImageFilter<typename VectorToScalarImageAccessorTypes<type>::MeanImageAdaptor, DefaultColorMapTraits>;

LookupTableImageFilterInstantiateMacro(unsigned char)
LookupTableImageFilterInstantiateMacro(char)
LookupTableImageFilterInstantiateMacro(unsigned short)
LookupTableImageFilterInstantiateMacro(short)
LookupTableImageFilterInstantiateMacro(float)

