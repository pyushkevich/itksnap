
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
  // By default not using a reference range
  m_UseReferenceRange = false;

  // Set required and optional inputs
  this->AddRequiredInputName("IntensityCurve");
  this->AddOptionalInputName("ColorMap");
  this->AddOptionalInputName("ImageMinInput");
  this->AddOptionalInputName("ImageMaxInput");

  // Allocate the output LUT and assign as output
  this->SetOutput("LookupTable", this->MakeOutput("LookupTable"));
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
  ComponentType imin = m_UseReferenceRange ? m_ReferenceMin : this->GetImageMinInput()->Get();
  ComponentType imax = m_UseReferenceRange ? m_ReferenceMax : this->GetImageMaxInput()->Get();
  const IntensityCurveInterface *curve = this->GetIntensityCurve();
  const ColorMap *colormap = this->GetColorMap();

  // Range of the curve (a pair)
  auto [tmin, tmax] = curve->GetRange();

  // Get the region representing the LUT, for multithreading
  unsigned int lut_size = LUTTraits::GetLUTSize(imin, imax);

  // Get the mapping from index into lut_region to the curve coordinate value. For
  // short/char imin->0 and imax->1. For float, 0->trange[0] and 10000->trange[1]
  double scale, shift;
  LUTTraits::ComputeLinearMappingToUnitInterval(imin, imax, tmin, tmax, scale, shift);

  // Allocate the LUT
  LookupTableType *lut = this->GetLookupTable();
  lut->m_LUT.resize(lut_size);

  // Set the starting and ending indices of the LUT. For short/char this is the
  // minimum intensity, and for float, this is tmin
  LUTTraits::GetLUTIntensityRange(imin, imax, tmin, tmax,
                                  lut->m_StartValue, lut->m_EndValue);

  // Multi-threaded computation
  itk::MultiThreaderBase::Pointer mt = itk::MultiThreaderBase::New();
  itk::ImageRegion<1> lut_region;
  lut_region.SetSize(0, lut_size);
  mt->ParallelizeImageRegion<1>(lut_region,
        [curve, colormap, scale, shift, lut, lut_region](const auto &thread_region)
    {
    // Iterate over the range of LUT entries we are computing
    int i0 = (int) thread_region.GetIndex()[0];
    int i1 = i0 + (int) thread_region.GetSize()[0];
    for(int i = i0; i < i1; i++)
      {
      // This is the t coordinate of the intensity curve to loop up
      double t = i * scale + shift;

      // Get the corresponding color map index
      double x = curve->Evaluate(t);

      // Finally, we use the color map to send this to RGBA or if there
      // is no color map, just scale it to the 0-255 range.
      DisplayPixelType rgb = TColorMapTraits::apply(colormap, x);

      // Assign to colormap
      lut->m_LUT[i] = rgb;
      }
    }, nullptr);
  }

template<class TInputImage, class TColorMapTraits>
typename IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>::DataObjectPointer
IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>
::MakeOutput(const DataObjectIdentifierType &name)
{
  if(name == "LookupTable")
    return LookupTableType::New().GetPointer();
  else
    return Superclass::MakeOutput(name);
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

template<class TInputImage, class TColorMapTraits>
typename IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>::LookupTableType *
IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>::GetLookupTable()
{
  return dynamic_cast<LookupTableType *>(itk::ProcessObject::GetOutput("LookupTable"));
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

