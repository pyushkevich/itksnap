#include "IntensityToColorLookupTableImageFilter.h"
#include "itkImageRegionIteratorWithIndex.h"

#include "LookupTableTraits.h"
#include "IntensityCurveInterface.h"
#include "ColorMap.h"
#include "itkImage.h"


/* ===============================================================
    AbstractLookupTableImageFilter implementation
   =============================================================== */


template<class TInputImage, class TOutputLUT, class TComponent>
AbstractLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
::AbstractLookupTableImageFilter()
{
  // By default not using a reference range
  m_UseReferenceRange = false;
  m_ImageMinInput = NULL;
  m_ImageMaxInput = NULL;
}

template<class TInputImage, class TOutputLUT, class TComponent>
void
AbstractLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
::SetImageMinInput(MinMaxObjectType *input)
{
  m_ImageMinInput = input;
  this->SetInput("image_min", input);
}

template<class TInputImage, class TOutputLUT, class TComponent>
void
AbstractLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
::SetImageMaxInput(MinMaxObjectType *input)
{
  m_ImageMaxInput = input;
  this->SetInput("image_max", input);
}

template<class TInputImage, class TOutputLUT, class TComponent>
void
AbstractLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
::SetReferenceIntensityRange(double min, double max)
{
  m_UseReferenceRange = true;
  m_ReferenceMin = (InputComponentType) min;
  m_ReferenceMax = (InputComponentType) max;
  this->Modified();
}

template<class TInputImage, class TOutputLUT, class TComponent>
void
AbstractLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
::RemoveReferenceIntensityRange()
{
  m_UseReferenceRange = false;
  this->Modified();
}

template<class TInputImage, class TOutputLUT, class TComponent>
void
AbstractLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
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

template<class TInputImage, class TOutputLUT, class TComponent>
void
AbstractLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
::EnlargeOutputRequestedRegion(itk::DataObject *)
{
  LookupTableType *output = this->GetOutput();
  output->SetRequestedRegion(output->GetLargestPossibleRegion());
}

template<class TInputImage, class TOutputLUT, class TComponent>
void
AbstractLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
::GenerateInputRequestedRegion()
{
  // The input region is the whole image
  InputImageType *input = const_cast<InputImageType *>(this->GetInput());
  input->SetRequestedRegionToLargestPossibleRegion();
}

template<class TInputImage, class TOutputLUT, class TComponent>
void
AbstractLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
::ThreadedGenerateData(const OutputImageRegionType &region,
                       itk::ThreadIdType threadId)
{
  // Get the image max and min
  InputComponentType imin = m_ImageMinInput->Get(), imax = m_ImageMaxInput->Get();

  // Compute the mapping from LUT position to [0 1] range for the curve
  float scale, shift;  
  LookupTableTraits<InputComponentType>::ComputeLinearMappingToUnitInterval(
        m_UseReferenceRange ? m_ReferenceMin : imin,
        m_UseReferenceRange ? m_ReferenceMax : imax,
        scale, shift);

  // Do the actual computation of the cache
  LookupTableType *output = this->GetOutput();
  for(itk::ImageRegionIteratorWithIndex<LookupTableType> it(output, region);
      !it.IsAtEnd(); ++it)
    {
    // Get the lookup value we are seeking
    long pos = it.GetIndex()[0];

    // Map the input value to range of 0 to 1
    float inZeroOne = (pos - shift) * scale;

    // Compute the intensity mapping
    it.Set(this->ComputeLUTValue(inZeroOne));
    }
}

template<class TInputImage, class TOutputLUT, class TComponent>
void
AbstractLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
::SetFixedLookupTableRange(InputComponentType imin, InputComponentType imax)
{
  SmartPtr<MinMaxObjectType> omin = MinMaxObjectType::New();
  omin->Set(imin);
  this->SetImageMinInput(omin);

  SmartPtr<MinMaxObjectType> omax = MinMaxObjectType::New();
  omax->Set(imax);
  this->SetImageMaxInput(omax);
}


/* ===============================================================
    IntensityToColorLookupTableImageFilter implementation
   =============================================================== */

template<class TInputImage, class TOutputLUT, class TComponent>
typename IntensityToColorLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>::OutputPixelType
IntensityToColorLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
::ComputeLUTValue(float inZeroOne)
{
  // Apply the intensity curve
  float outZeroOne = m_IntensityCurve->Evaluate(inZeroOne);

  // Map the output to a RGBA pixel
  return m_ColorMap->MapIndexToRGBA(outZeroOne);
}

template<class TInputImage, class TOutputLUT, class TComponent>
void
IntensityToColorLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
::SetIntensityCurve(IntensityCurveInterface *curve)
{
  m_IntensityCurve = curve;
  this->SetInput("curve", curve);
}

template<class TInputImage, class TOutputLUT, class TComponent>
void
IntensityToColorLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
::SetColorMap(ColorMap *map)
{
  m_ColorMap = map;
  this->SetInput("colormap", map);
}



/* ===============================================================
    MultiComponentImageToScalarLookupTableImageFilter implementation
   =============================================================== */

template<class TInputImage, class TOutputLUT, class TComponent>
void
MultiComponentImageToScalarLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
::SetIntensityCurve(IntensityCurveInterface *curve)
{
  m_IntensityCurve = curve;
  this->SetInput("curve", curve);
}

template<class TInputImage, class TOutputLUT, class TComponent>
typename MultiComponentImageToScalarLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>::OutputPixelType
MultiComponentImageToScalarLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
::ComputeLUTValue(float inZeroOne)
{
  // Compute the intensity mapping
  float outZeroOne = m_IntensityCurve->Evaluate(inZeroOne);

  // Map the output to a RGBA pixel
  return static_cast<OutputPixelType>(255.0 * outZeroOne);
}








// Template instantiation
#include "itkVectorImageToImageAdaptor.h"
#include "VectorToScalarImageAccessor.h"

typedef itk::Image<short, 3> GreyImageType;
typedef itk::VectorImageToImageAdaptor<GreyType, 3> GreyComponentAdaptorType;
typedef itk::Image<itk::RGBAPixel<unsigned char>, 1> LUTType;
typedef itk::Image<unsigned char, 1> ScalarLUTType;
typedef itk::Image<float, 3> FloatImageType;
typedef itk::VectorImage<short, 3> AnatomicImageType;

template class AbstractLookupTableImageFilter<GreyImageType, LUTType, GreyType>;
template class AbstractLookupTableImageFilter<FloatImageType, LUTType, float>;
template class AbstractLookupTableImageFilter<GreyComponentAdaptorType, LUTType, GreyType>;
template class AbstractLookupTableImageFilter<GreyVectorMagnitudeImageAdaptor, LUTType, float>;
template class AbstractLookupTableImageFilter<GreyVectorMaxImageAdaptor, LUTType, float>;
template class AbstractLookupTableImageFilter<GreyVectorMeanImageAdaptor, LUTType, float>;
template class AbstractLookupTableImageFilter<AnatomicImageType, ScalarLUTType, GreyType>;

template class IntensityToColorLookupTableImageFilter<GreyImageType, LUTType>;
template class IntensityToColorLookupTableImageFilter<FloatImageType, LUTType>;
template class IntensityToColorLookupTableImageFilter<GreyComponentAdaptorType, LUTType>;
template class IntensityToColorLookupTableImageFilter<GreyVectorMagnitudeImageAdaptor, LUTType>;
template class IntensityToColorLookupTableImageFilter<GreyVectorMaxImageAdaptor, LUTType>;
template class IntensityToColorLookupTableImageFilter<GreyVectorMeanImageAdaptor, LUTType>;

template class MultiComponentImageToScalarLookupTableImageFilter<AnatomicImageType, ScalarLUTType>;

