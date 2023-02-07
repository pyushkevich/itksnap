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


#if(false)

/**
 * A special pixel type that is internally a short but is presented to the
 * outside world as a floating point number. Used to represent speed image
 * values, where precision offered by short is enough. The first parameter
 * is floating point type (float or double), and the second is the short
 * value corresponding to the floating point value 1.0, the default is 10000
 */

template <class TFloat, short VRange = 10000>
class SmallClippedFloat
{
public:
  typedef SmallClippedFloat<TFloat, VRange> Self;

  TFloat operator()() const { return m_Value * m_Scale; }
  Self &operator = (const TFloat &f) { m_Value = (short) VRange * f; return *this; }

  /** Construct from floating point */
  SmallClippedFloat(float value) { *this = value; }

private:
  static constexpr TFloat m_Scale = 1.0 / VRange;
  short m_Value;
};

template<class TFloat, short VRange>
class vnl_numeric_traits< SmallClippedFloat<TFloat, VRange> >
{
public:
  typedef SmallClippedFloat<TFloat, VRange> T;
  //: Additive identity
  static const T zero = T(0.0);
  //: Multiplicative identity
  static const T one = T(1.0);
  //: Maximum value which this type can assume
  static const T maxval = T(1.0);
  //: Return value of abs()
  typedef T abs_t;
  //: Name of a type twice as long as this one for accumulators and products.
  typedef typename vnl_numeric_traits<short>::double_t double_t;
  //: Name of type which results from multiplying this type with a double
  typedef double real_t;
};

/**
 * To save memory, we define the pixels in the speed image as shorts in the range
 * between -10000 and 10000. This should provide enough precision.
 */
typedef SmallClippedFloat<float> SpeedImagePixel;

#endif


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
