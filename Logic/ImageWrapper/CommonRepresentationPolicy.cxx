#include "CommonRepresentationPolicy.h"
#include "ImageWrapperTraits.h"
#include "itkCastImageFilter.h"
#include "itkVectorImageToImageAdaptor.h"

template<class TOutputPixel, class TWrapperTraits>
InPlaceScalarImageWrapperCommonRepresentation<TOutputPixel, TWrapperTraits>
::InPlaceScalarImageWrapperCommonRepresentation()
{
  m_Image = NULL;
}

template<class TOutputPixel, class TWrapperTraits>
InPlaceScalarImageWrapperCommonRepresentation<TOutputPixel, TWrapperTraits>
::~InPlaceScalarImageWrapperCommonRepresentation()
{
  m_Image = NULL;
}

template<class TOutputPixel, class TWrapperTraits>
typename InPlaceScalarImageWrapperCommonRepresentation<TOutputPixel, TWrapperTraits>::OutputImageType *
InPlaceScalarImageWrapperCommonRepresentation<TOutputPixel, TWrapperTraits>
::GetOutput(ScalarImageWrapperBase::ExportChannel channel)
{
  return m_Image;
}

template<class TOutputPixel, class TWrapperTraits>
void
InPlaceScalarImageWrapperCommonRepresentation<TOutputPixel, TWrapperTraits>
::UpdateInputImage(InputImageType *image)
{
  m_Image = image;
}


template<class TOutputPixel, class TWrapperTraits>
CastingScalarImageWrapperCommonRepresentation<TOutputPixel, TWrapperTraits>
::CastingScalarImageWrapperCommonRepresentation()
{
  // Allocate the filters
  for(int i = 0; i < ScalarImageWrapperBase::CHANNEL_COUNT; i++)
    {
    CastFilterPointer caster = CastFilterType::New();
    m_CastFilter.push_back(caster);
    }
}

template<class TOutputPixel, class TWrapperTraits>
CastingScalarImageWrapperCommonRepresentation<TOutputPixel, TWrapperTraits>
::~CastingScalarImageWrapperCommonRepresentation()
{
}

template<class TOutputPixel, class TWrapperTraits>
void
CastingScalarImageWrapperCommonRepresentation<TOutputPixel, TWrapperTraits>
::UpdateInputImage(InputImageType *image)
{
  // Update the inputs
  for(int i = 0; i < ScalarImageWrapperBase::CHANNEL_COUNT; i++)
    m_CastFilter[i]->SetInput(image);
}

template<class TOutputPixel, class TWrapperTraits>
typename CastingScalarImageWrapperCommonRepresentation<TOutputPixel, TWrapperTraits>::OutputImageType *
CastingScalarImageWrapperCommonRepresentation<TOutputPixel, TWrapperTraits>
::GetOutput(ScalarImageWrapperBase::ExportChannel channel)
{
  return m_CastFilter[channel]->GetOutput();
}

template class CastingScalarImageWrapperCommonRepresentation<
    GreyType, GreyComponentImageWrapperTraits >;

template class CastingScalarImageWrapperCommonRepresentation<
    GreyType, GreyVectorMagnitudeImageWrapperTraits >;

template class CastingScalarImageWrapperCommonRepresentation<
    GreyType, GreyVectorMaxImageWrapperTraits >;

template class CastingScalarImageWrapperCommonRepresentation<
    GreyType, GreyVectorMeanImageWrapperTraits >;

template class InPlaceScalarImageWrapperCommonRepresentation<
    GreyType, SpeedImageWrapperTraits >;

template class InPlaceScalarImageWrapperCommonRepresentation<
    GreyType, GreyAnatomicScalarImageWrapperTraits >;

template class CastingScalarImageWrapperCommonRepresentation<
    GreyType, LevelSetImageWrapperTraits >;

template class CastingScalarImageWrapperCommonRepresentation<
    GreyType, GreyAnatomicScalarImageWrapperTraits >;






