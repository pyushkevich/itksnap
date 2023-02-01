#ifndef IMAGEWRAPPERTRAITS_H
#define IMAGEWRAPPERTRAITS_H

#include "SNAPCommon.h"
#include "ImageWrapperBase.h"
#include "RLEImageRegionIterator.h"

#include "DisplayMappingPolicy.h"
#include "NativeIntensityMappingPolicy.h"

#include "ScalarImageWrapper.h"
#include "VectorImageWrapper.h"
#include "ColorMap.h"

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

  typedef ScalarImageWrapperBase WrapperBaseType;
  typedef ScalarImageWrapper<LabelImageWrapperTraits> WrapperType;

  typedef LabelType ComponentType;
  typedef RLEImage<ComponentType> ImageType;
  typedef itk::Image<ComponentType, 2> SliceType;
  typedef RLEImage<ComponentType, 4> Image4DType;

  typedef IdentityInternalToNativeIntensityMapping NativeIntensityMapping;
  typedef ColorLabelTableDisplayMappingPolicy<Self> DisplayMapping;

  // Whether this image is shown on top of all other layers by default
  itkStaticConstMacro(StickyByDefault, bool, true);

  // Whether this image is produced from another by a pipeline (e.g., speed image)
  itkStaticConstMacro(PipelineOutput, bool, false);
};

class SpeedImageWrapperTraits
{
public:
  typedef SpeedImageWrapperTraits Self;

  typedef ScalarImageWrapperBase WrapperBaseType;
  typedef ScalarImageWrapper<SpeedImageWrapperTraits> WrapperType;

  typedef short ComponentType;
  typedef itk::Image<ComponentType, 3> ImageType;
  typedef itk::Image<ComponentType, 2> SliceType;
  typedef itk::Image<ComponentType, 4> Image4DType;

  typedef SpeedImageInternalToNativeIntensityMapping NativeIntensityMapping;
  typedef LinearColorMapDisplayMappingPolicy<Self> DisplayMapping;

  static void GetFixedIntensityRange(float &min, float &max)
    { min = -0x7fff; max = 0x7fff; }

  itkStaticConstMacro(DefaultColorMap, ColorMap::SystemPreset, ColorMap::COLORMAP_SPEED);

  // Whether this image is shown on top of all other layers by default
  itkStaticConstMacro(StickyByDefault, bool, false);

  // Whether this image is produced from another by a pipeline (e.g., speed image)
  itkStaticConstMacro(PipelineOutput, bool, true);
};

class LevelSetImageWrapperTraits
{
public:
  typedef LevelSetImageWrapperTraits Self;

  typedef ScalarImageWrapperBase WrapperBaseType;
  typedef ScalarImageWrapper<LevelSetImageWrapperTraits> WrapperType;

  typedef float ComponentType;
  typedef itk::Image<ComponentType, 3> ImageType;
  typedef itk::Image<ComponentType, 2> SliceType;
  typedef itk::Image<ComponentType, 4> Image4DType;

  typedef IdentityInternalToNativeIntensityMapping NativeIntensityMapping;
  typedef LinearColorMapDisplayMappingPolicy<Self> DisplayMapping;

  static void GetFixedIntensityRange(float &min, float &max)
    { min = -4.0; max = 4.0; }

  itkStaticConstMacro(DefaultColorMap, ColorMap::SystemPreset, ColorMap::COLORMAP_LEVELSET);

  // Whether this image is shown on top of all other layers by default
  itkStaticConstMacro(StickyByDefault, bool, true);

  // Whether this image is produced from another by a pipeline (e.g., speed image)
  itkStaticConstMacro(PipelineOutput, bool, true);
};

template <class TPixel, bool TLinearMapping>
class ComponentImageWrapperTraits
{
public:
  typedef ComponentImageWrapperTraits<TPixel, TLinearMapping> Self;

  typedef ScalarImageWrapperBase WrapperBaseType;
  typedef ScalarImageWrapper<Self> WrapperType;

  typedef TPixel ComponentType;
  typedef itk::VectorImageToImageAdaptor<ComponentType, 3> ImageType;
  typedef itk::Image<ComponentType, 2> SliceType;
  typedef itk::VectorImageToImageAdaptor<ComponentType, 4> Image4DType;

  // The type of internal to native intensity mapping used in the traits is controlled
  // by the boolean template parameter
  typedef typename std::conditional<TLinearMapping,
    LinearInternalToNativeIntensityMapping,
    IdentityInternalToNativeIntensityMapping>::type NativeIntensityMapping;

  typedef CachingCurveAndColorMapDisplayMappingPolicy<Self> DisplayMapping;

  itkStaticConstMacro(DefaultColorMap, ColorMap::SystemPreset, ColorMap::COLORMAP_GREY);

  // Whether this image is shown on top of all other layers by default
  itkStaticConstMacro(StickyByDefault, bool, false);

  // Whether this image is produced from another by a pipeline (e.g., speed image)
  itkStaticConstMacro(PipelineOutput, bool, false);
};

template <class TFunctor>
class VectorDerivedQuantityImageWrapperTraits
{
public:
  typedef VectorDerivedQuantityImageWrapperTraits<TFunctor> Self;
  typedef ScalarImageWrapperBase WrapperBaseType;
  typedef ScalarImageWrapper<Self> WrapperType;

  typedef typename TFunctor::OutputPixelType ComponentType;

  typedef typename TFunctor::InputPixelType InternalComponentType;
  typedef itk::VectorImage<InternalComponentType, 3> InternalImageType;
  typedef VectorToScalarImageAccessor<TFunctor> AccessorType;
  typedef itk::ImageAdaptor<InternalImageType, AccessorType> ImageType;
  typedef itk::Image<ComponentType, 2> SliceType;

  typedef itk::VectorImage<InternalComponentType, 4> InternalImage4DType;
  typedef itk::ImageAdaptor<InternalImage4DType, AccessorType> Image4DType;

  typedef IdentityInternalToNativeIntensityMapping NativeIntensityMapping;
  typedef CachingCurveAndColorMapDisplayMappingPolicy<Self> DisplayMapping;

  itkStaticConstMacro(DefaultColorMap, ColorMap::SystemPreset, ColorMap::COLORMAP_GREY);

  // Whether this image is shown on top of all other layers by default
  itkStaticConstMacro(StickyByDefault, bool, false);

  // Whether this image is produced from another by a pipeline (e.g., speed image)
  itkStaticConstMacro(PipelineOutput, bool, false);
};


template <class TPixel, bool TLinearMapping = false>
class AnatomicImageWrapperTraits
{
public:
  typedef AnatomicImageWrapperTraits<TPixel> Self;

  typedef VectorImageWrapperBase WrapperBaseType;
  typedef VectorImageWrapper<Self> WrapperType;

  // Component stuff
  typedef TPixel ComponentType;

  typedef ComponentImageWrapperTraits<ComponentType, TLinearMapping> ComponentWrapperTraits;
  typedef ScalarImageWrapper<ComponentWrapperTraits> ComponentWrapperType;

  typedef itk::VectorImage<ComponentType, 3> ImageType;
  typedef itk::VectorImage<ComponentType, 2> SliceType;
  typedef itk::VectorImage<ComponentType, 4> Image4DType;

  // The type of internal to native intensity mapping used in the traits is controlled
  // by the boolean template parameter
  typedef typename std::conditional<TLinearMapping,
    LinearInternalToNativeIntensityMapping,
    IdentityInternalToNativeIntensityMapping>::type NativeIntensityMapping;

  typedef MultiChannelDisplayMappingPolicy<Self> DisplayMapping;

  // Whether this image is shown on top of all other layers by default
  itkStaticConstMacro(StickyByDefault, bool, false);

  // Whether this image is produced from another by a pipeline (e.g., speed image)
  itkStaticConstMacro(PipelineOutput, bool, false);
};


template <class TPixel, bool TLinearMapping = false>
class AnatomicScalarImageWrapperTraits
{
public:
  typedef AnatomicScalarImageWrapperTraits<TPixel> Self;

  typedef ScalarImageWrapperBase WrapperBaseType;
  typedef ScalarImageWrapper<Self> WrapperType;

  typedef TPixel ComponentType;
  typedef itk::Image<ComponentType, 3> ImageType;
  typedef itk::Image<ComponentType, 2> SliceType;
  typedef itk::Image<ComponentType, 4> Image4DType;

  typedef CachingCurveAndColorMapDisplayMappingPolicy<Self> DisplayMapping;

  // The type of internal to native intensity mapping used in the traits is controlled
  // by the boolean template parameter
  typedef typename std::conditional<TLinearMapping,
    LinearInternalToNativeIntensityMapping,
    IdentityInternalToNativeIntensityMapping>::type NativeIntensityMapping;

  // Default color map
  itkStaticConstMacro(DefaultColorMap, ColorMap::SystemPreset, ColorMap::COLORMAP_GREY);

  // Whether this image is shown on top of all other layers by default
  itkStaticConstMacro(StickyByDefault, bool, false);

  // Whether this image is produced from another by a pipeline (e.g., speed image)
  itkStaticConstMacro(PipelineOutput, bool, false);
};

/**
 * This class defines all the traits available for a pixel type
 * to help with template instantiation
 */
template<class TPixel>
class ImageWrapperTraits
{
public:
  typedef AnatomicImageWrapperTraits<TPixel, false> VectorTraits;
  typedef AnatomicScalarImageWrapperTraits<TPixel, false> ScalarTraits;
  typedef ComponentImageWrapperTraits<TPixel, false> ComponentTraits;

  typedef VectorToScalarMagnitudeFunctor<TPixel, float> MagnitudeFunctor;
  typedef VectorDerivedQuantityImageWrapperTraits<MagnitudeFunctor> MagnitudeTraits;
  typedef VectorToScalarMaxFunctor<TPixel, float> MaxFunctor;
  typedef VectorDerivedQuantityImageWrapperTraits<MaxFunctor> MaxTraits;
  typedef VectorToScalarMeanFunctor<TPixel, float> MeanFunctor;
  typedef VectorDerivedQuantityImageWrapperTraits<MeanFunctor> MeanTraits;
};

// Some global typedefs
typedef SpeedImageWrapperTraits::WrapperType SpeedImageWrapper;
typedef LevelSetImageWrapperTraits::WrapperType LevelSetImageWrapper;


#endif // IMAGEWRAPPERTRAITS_H
