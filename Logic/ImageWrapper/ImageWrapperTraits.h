#ifndef IMAGEWRAPPERTRAITS_H
#define IMAGEWRAPPERTRAITS_H

#include "SNAPCommon.h"
#include "ImageWrapperBase.h"

#include "CommonRepresentationPolicy.h"
#include "DisplayMappingPolicy.h"
#include "NativeIntensityMappingPolicy.h"

#include "ScalarImageWrapper.h"
#include "VectorImageWrapper.h"

namespace itk
{
  template <class TPixel, unsigned int VDim> class Image;
  template <class TPixel, unsigned int VDim> class VectorImage;
  template <class TPixel, unsigned int VDim> class VectorImageToImageAdaptor;
}

/*
class ScalarImageWrapperBase;
class VectorImageWrapperBase;
template <class TImage, class TBase> class VectorImageWrapper;
template <class TImage, class TBase> class ScalarImageWrapper;
*/

/**
 * Each of the traits classes below defines types and policies for a specific
 * type of image wrapper.
 */
class LabelImageWrapperTraits
{
public:
  typedef LabelImageWrapperTraits Self;

  typedef ScalarImageWrapper<LabelImageWrapperTraits> WrapperType;

  typedef LabelType ComponentType;
  typedef itk::Image<ComponentType, 3> ImageType;

  typedef IdentityInternalToNativeIntensityMapping NativeIntensityMapping;
  typedef ColorLabelTableDisplayMappingPolicy<Self> DisplayMapping;
  typedef NullScalarImageWrapperCommonRepresentation<GreyType, Self> CommonRepresentationPolicy;
};

class SpeedImageWrapperTraits
{
public:
  typedef SpeedImageWrapperTraits Self;

  typedef ScalarImageWrapper<SpeedImageWrapperTraits> WrapperType;

  typedef GreyType ComponentType;
  typedef itk::Image<ComponentType, 3> ImageType;

  typedef LinearInternalToNativeIntensityMapping NativeIntensityMapping;
  typedef CachingCurveAndColorMapDisplayMappingPolicy<Self> DisplayMapping;
  typedef NullScalarImageWrapperCommonRepresentation<GreyType, Self> CommonRepresentationPolicy;
};

class LevelSetImageWrapperTraits
{
public:
  typedef LevelSetImageWrapperTraits Self;

  typedef ScalarImageWrapper<LevelSetImageWrapperTraits> WrapperType;

  typedef float ComponentType;
  typedef itk::Image<ComponentType, 3> ImageType;

  typedef IdentityInternalToNativeIntensityMapping NativeIntensityMapping;
  typedef LinearColorMapDisplayMappingPolicy<Self> DisplayMapping;
  typedef NullScalarImageWrapperCommonRepresentation<GreyType, Self> CommonRepresentationPolicy;

  static void GetFixedIntensityRange(float &min, float &max)
    { min = -4.0; max = 4.0; }
};

template <class TPixel>
class ComponentImageWrapperTraits
{
public:
  typedef ComponentImageWrapperTraits<TPixel> Self;

  typedef ScalarImageWrapper<Self> WrapperType;

  typedef TPixel ComponentType;
  typedef itk::VectorImageToImageAdaptor<ComponentType, 3> ImageType;

  typedef LinearInternalToNativeIntensityMapping NativeIntensityMapping;
  typedef CachingCurveAndColorMapDisplayMappingPolicy<Self> DisplayMapping;
  typedef CastingScalarImageWrapperCommonRepresentation<GreyType, Self> CommonRepresentationPolicy;
};

template <class TFunctor>
class VectorDerivedQuantityImageWrapperTraits
{
public:
  typedef VectorDerivedQuantityImageWrapperTraits<TFunctor> Self;
  typedef ScalarImageWrapper<Self> WrapperType;

  typedef typename TFunctor::OutputPixelType ComponentType;

  typedef typename TFunctor::InputPixelType InternalComponentType;
  typedef itk::VectorImage<InternalComponentType, 3> InternalImageType;
  typedef VectorToScalarImageAccessor<TFunctor> AccessorType;
  typedef itk::ImageAdaptor<InternalImageType, AccessorType> ImageType;

  typedef IdentityInternalToNativeIntensityMapping NativeIntensityMapping;
  typedef CachingCurveAndColorMapDisplayMappingPolicy<Self> DisplayMapping;
  typedef CastingScalarImageWrapperCommonRepresentation<GreyType, Self> CommonRepresentationPolicy;
};


template <class TPixel>
class AnatomicImageWrapperTraits
{
public:
  typedef AnatomicImageWrapperTraits<TPixel> Self;

  typedef VectorImageWrapper<Self> WrapperType;

  // Component stuff
  typedef TPixel ComponentType;

  typedef ComponentImageWrapperTraits<ComponentType> ComponentWrapperTraits;
  typedef ScalarImageWrapper<ComponentWrapperTraits> ComponentWrapperType;

  typedef itk::VectorImage<ComponentType, 3> ImageType;

  typedef LinearInternalToNativeIntensityMapping NativeIntensityMapping;
  typedef MultiChannelDisplayMappingPolicy<Self> DisplayMapping;
};

// Global typedefs for traits with GreyType
typedef ComponentImageWrapperTraits<GreyType> GreyComponentImageWrapperTraits;
typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMagnitudeFunctor>
  GreyVectorMagnitudeImageWrapperTraits;
typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMaxFunctor>
  GreyVectorMaxImageWrapperTraits;
typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMeanFunctor>
  GreyVectorMeanImageWrapperTraits;

// Some global typedefs
typedef AnatomicImageWrapperTraits<GreyType>::WrapperType AnatomicImageWrapper;
typedef LabelImageWrapperTraits::WrapperType LabelImageWrapper;
typedef SpeedImageWrapperTraits::WrapperType SpeedImageWrapper;
typedef LevelSetImageWrapperTraits::WrapperType LevelSetImageWrapper;


#endif // IMAGEWRAPPERTRAITS_H
