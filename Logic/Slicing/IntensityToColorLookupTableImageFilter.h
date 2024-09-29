#ifndef INTENSITYTOCOLORLOOKUPTABLEIMAGEFILTER_H
#define INTENSITYTOCOLORLOOKUPTABLEIMAGEFILTER_H

#include "SNAPCommon.h"
#include <itkImageToImageFilter.h>
#include <itkVectorImage.h>
#include <itkSimpleDataObjectDecorator.h>
#include "ColorMap.h"

class ColorMap;
class IntensityCurveInterface;
template <class TInputPixel, class TDisplayPixel> class ColorLookupTable;


/**
 * Traits that determine color map behavior (to use one or not, basically)
 */
class DefaultColorMapTraits
{
public:
  using ColorMapType = ColorMap;
  using DisplayPixelType = itk::RGBAPixel<unsigned char>;

  static constexpr bool IsRequired = true;

  static DisplayPixelType apply(const ColorMapType *cm, double x, bool ignore_alpha)
  {
    DisplayPixelType p = cm->MapIndexToRGBA(x);
    if(ignore_alpha)
      p[3] = 255;
    return p;
  }

  static void get_outside_and_nan_values(
      const ColorMapType *cm, bool ignore_alpha,
      DisplayPixelType &lower, DisplayPixelType &upper, DisplayPixelType &nan)
    {
    lower = cm->MapIndexToRGBA(-1.0);
    upper = cm->MapIndexToRGBA(2.0);
    nan = cm->GetNANColor();
    if(ignore_alpha)
      {
      lower[3] = 255;
      upper[3] = 255;
      nan[3] = 255;
      }
    }
};

/**
 * Traits that determine color map behavior (to use one or not, basically)
 */
class VectorToRGBColorMapTraits
{
public:
  using ColorMapType = ColorMap;
  using DisplayPixelType = unsigned char;
  static constexpr bool IsRequired = false;
  static DisplayPixelType apply(const ColorMapType *, double x, bool itkNotUsed(ignore_alpha))
  {
    return (DisplayPixelType) 255.0 * x;
  }

  static void get_outside_and_nan_values(
      const ColorMapType *cm,  bool ignore_alpha,
      DisplayPixelType &lower, DisplayPixelType &upper, DisplayPixelType &nan)
    {
    lower = 0;
    upper = 255;
    nan = 0;
    }
};

/**
 * This is a parent class for filters that generate a lookup table based on the
 * intensity range of an input image. There are two subclasses in SNAP, one for
 * LUTs that map input values to RGBA colors directly, and another that maps
 * input values to RGB components. The class requires three inputs: the image,
 * and objects representing the image min/max intensities. The image may be a
 * vector image, a regular image, or an ImageAdaptor.
 *
 * TODO: the current approach is to map the entire range between the image
 * minimum and maximum to the color map. However, since the user only sees
 * the intensities between the intensity curve min and max, this means that
 * a lot of the color map gets wasted.
 */
template <class TInputImage, class TColorMapTraits>
class IntensityToColorLookupTableImageFilter
    : public itk::ImageToImageFilter<TInputImage, TInputImage>
{
public:

  using Self = IntensityToColorLookupTableImageFilter<TInputImage, TColorMapTraits>;
  using Superclass = itk::ImageToImageFilter<TInputImage,TInputImage>;
  using Pointer = SmartPtr<Self>;
  using ConstPointer = SmartPtr<const Self>;

  // This is necessary to use itkGet/SetInputMacros to avoid gcc compiling error
  using ProcessObject = itk::ProcessObject;

  using DataObjectPointer = ProcessObject::DataObjectPointer;
  using DataObjectIdentifierType = ProcessObject::DataObjectIdentifierType;

  // All the stuff from the input image
  using ImageType = TInputImage;
  using PixelType = typename ImageType::PixelType;
  using InternalPixelType = typename ImageType::InternalPixelType;
  static constexpr unsigned int ImageDimension = ImageType::ImageDimension;
  using IsVector = std::is_base_of<itk::VectorImage<InternalPixelType, ImageDimension>, ImageType>;
  using ComponentType = typename std::conditional<IsVector::value, InternalPixelType, PixelType>::type;
  using RegionType = typename ImageType::RegionType;

  // Output LUT
  using DisplayPixelType = typename TColorMapTraits::DisplayPixelType;
  using LookupTableType = ColorLookupTable<ComponentType, DisplayPixelType>;

  // The type of the min/max inputs. The are usually InputPixelType, but may
  // be different, i.e., for VectorImage it's InternalPixelType.
  using MinMaxObjectType = itk::SimpleDataObjectDecorator<ComponentType>;

  itkTypeMacro(IntensityToColorLookupTableImageFilter, ImageToImageFilter)
  itkNewMacro(Self)

  /** Set the intensity remapping curve - for contrast adjustment */
  itkSetInputMacro(IntensityCurve, IntensityCurveInterface)

  /** Get the intensity remapping curve - for contrast adjustment */
  itkGetInputMacro(IntensityCurve, IntensityCurveInterface)

  /** Set the color map - whether it is used depends on color map traits */
  itkSetInputMacro(ColorMap, ColorMap)

  /** Get the intensity remapping curve - for contrast adjustment */
  itkGetInputMacro(ColorMap, ColorMap)

  /**
    One of the inputs to the filter is an object representing the minimum
    value of the input image intensity range. This can be the output of an
    itk::MinimumMaximumImageFilter, or it can just we a DataObject
    encapsulating a constant value. The reason we use an object and not a
    simple constant is to allow pipeline execution - for example recomputing
    the minimum and maximum of an image, if necessary
    */
  itkSetInputMacro(ImageMinInput, MinMaxObjectType)
  itkGetInputMacro(ImageMinInput, MinMaxObjectType)

  /** See notes for SetImageMinInput */
  itkSetInputMacro(ImageMaxInput, MinMaxObjectType)
  itkGetInputMacro(ImageMaxInput, MinMaxObjectType)

  /** Set intensity range to a pair of constants. This is useful when the
    range of the lookup table will never change. This is an alternative to
    calling SetImageMinInput() and SetImageMaxInput() */
  void SetFixedLookupTableRange(ComponentType imin, ComponentType imax);

  /** Get the lookup table, which is the main output of this filter */
  LookupTableType *GetLookupTable();

  /**
   * Specify whether the alpha in the color map should be ignored, i.e.,
   * when rendering base layers instead of overlays
   */
  itkSetMacro(IgnoreAlpha, bool)
  itkGetMacro(IgnoreAlpha, bool)

  /**
    It is possible to use a separate reference intensity range when mapping
    raw intensities into the domain of the intensity curve. This is relevant
    when working with images derived from another image.

    TODO: this may not be necessary in the future
    */
  void SetReferenceIntensityRange(double min, double max);
  void RemoveReferenceIntensityRange();

  virtual void AllocateOutputs() ITK_OVERRIDE;

  virtual void GenerateInputRequestedRegion() ITK_OVERRIDE;

  virtual void GenerateData() override;

  virtual DataObjectPointer MakeOutput(const DataObjectIdentifierType &name) override;


protected:

  IntensityToColorLookupTableImageFilter();
  virtual ~IntensityToColorLookupTableImageFilter() {}

  // Reference intensity range
  ComponentType m_ReferenceMin, m_ReferenceMax;

  // Whether the reference intensity range is being used
  bool m_UseReferenceRange;

  // Whether transparency is used or ignored
  bool m_IgnoreAlpha = false;
};

#endif // INTENSITYTOCOLORLOOKUPTABLEIMAGEFILTER_H
