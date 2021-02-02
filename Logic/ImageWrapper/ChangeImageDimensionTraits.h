#ifndef CHANGEIMAGEDIMENSIONTRAITS_H
#define CHANGEIMAGEDIMENSIONTRAITS_H

namespace itk
{
template <typename TPixel, unsigned int VDim> class Image;
template <typename TPixel, unsigned int VDim> class VectorImage;
template <typename TImage, typename TAccessor> class ImageAdaptor;
template <typename TImage, typename TAccessor> class ImageAdaptor;
template <typename TPixel, unsigned int VDim> class VectorImageToImageAdaptor;
}

template<typename TPixel, unsigned int VDim, typename TCounter> class RLEImage;

/**
 This traits class is used to obtain a type that matches the input
 type (itk::Image, itk::VectorImage, etc) but has different number
 of dimensions.
 */
template <class TImage, unsigned int OutputDimension>
struct DefaultChangeImageDimensionTraits
{
  // Default implementation should generate an error
  using OutputImageType = int;
};

template <class TPixel, unsigned int VDim, unsigned int OutputDimension>
struct DefaultChangeImageDimensionTraits<
    itk::Image<TPixel, VDim>,
    OutputDimension>
{
  // Default implementation should generate an error
  using OutputImageType = itk::Image<TPixel, OutputDimension>;
};

template <class TImage, typename TAccessor, unsigned int OutputDimension>
struct DefaultChangeImageDimensionTraits<
    itk::ImageAdaptor<TImage, TAccessor>,
    OutputDimension>
{
  // Default implementation should generate an error
  using InternalImageTraits = DefaultChangeImageDimensionTraits<TImage, OutputDimension>;
  using OutputInternalImageType = typename InternalImageTraits::OutputImageType;
  using OutputImageType = itk::ImageAdaptor<OutputInternalImageType, TAccessor>;
};

template <class TPixel, unsigned int VDim, unsigned int OutputDimension>
struct DefaultChangeImageDimensionTraits<
    itk::VectorImageToImageAdaptor<TPixel, VDim>,
    OutputDimension>
{
  // Default implementation should generate an error
  using OutputImageType = itk::VectorImageToImageAdaptor<TPixel, OutputDimension>;
};

template <typename TPixel, unsigned int VDim, typename TCounter,
          unsigned int OutputDimension>
struct DefaultChangeImageDimensionTraits<
    RLEImage<TPixel, VDim, TCounter>,
    OutputDimension>
{
  // Default implementation should generate an error
  using OutputImageType = RLEImage<TPixel, OutputDimension, TCounter>;
};

#endif // CHANGEIMAGEDIMENSIONTRAITS_H
