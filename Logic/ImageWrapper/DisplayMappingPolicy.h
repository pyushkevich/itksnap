#ifndef DISPLAYMAPPINGPOLICY_H
#define DISPLAYMAPPINGPOLICY_H

#include "ImageWrapperBase.h"
#include "itkDataObject.h"
#include "itkObjectFactory.h"

class ColorLabelTable;
class LabelToRGBAFilter;
class IntensityCurveVTK;
class Registry;
template <class TEnum> class RegistryEnumMap;
template <class T, class U> class IntensityToColorLookupTableImageFilter;
template <class T, class U> class LookupTableIntensityMappingFilter;
template <class T> class RGBALookupTableIntensityMappingFilter;
template <class T, class U> class MultiComponentImageToScalarLookupTableImageFilter;

/**
 * @brief An abstract class describing intensity mapping between image
 * intensities stored internally in an itk image and RGBA intensities
 * displayed to the user. This class is a parent in a hierarchy of
 * policies used to customize ImageWrapper behavior.
 */
class AbstractDisplayMappingPolicy : public itk::DataObject
{
public:

  irisITKAbstractObjectMacro(AbstractDisplayMappingPolicy, itk::DataObject)

  typedef ImageWrapperBase::DisplaySliceType DisplaySliceType;
  typedef ImageWrapperBase::DisplaySlicePointer DisplaySlicePointer;

  virtual IntensityCurveInterface *GetIntensityCurve() const = 0;
  virtual ColorMap *GetColorMap() const = 0;

  virtual DisplaySlicePointer GetDisplaySlice(unsigned int slice) = 0;

  virtual void Save(Registry &folder) = 0;
  virtual void Restore(Registry &folder) = 0;
};

class AbstractColorLabelTableDisplayMappingPolicy : public AbstractDisplayMappingPolicy
{
public:

  irisITKAbstractObjectMacro(AbstractColorLabelTableDisplayMappingPolicy,
                             AbstractDisplayMappingPolicy)

  /**
   * Set the table of color labels used to produce color slice images
   */
  virtual void SetLabelColorTable(ColorLabelTable *labels) = 0;

  /**
   * Get the color label table
   */
  virtual ColorLabelTable *GetLabelColorTable() const = 0;
};

template <class TWrapperTraits>
class ColorLabelTableDisplayMappingPolicy
    : public AbstractColorLabelTableDisplayMappingPolicy
{
public:

  irisITKObjectMacro(ColorLabelTableDisplayMappingPolicy<TWrapperTraits>,
                     AbstractColorLabelTableDisplayMappingPolicy)

  typedef typename TWrapperTraits::WrapperType WrapperType;
  typedef typename TWrapperTraits::ImageType ImageType;
  typedef itk::Image<LabelType, 2> InputSliceType;
  typedef ImageWrapperBase::DisplaySliceType DisplaySliceType;
  typedef ImageWrapperBase::DisplaySlicePointer DisplaySlicePointer;

  /**
   * Set the table of color labels used to produce color slice images
   */
  void SetLabelColorTable(ColorLabelTable *labels);

  /**
   * Get the color label table
   */
  ColorLabelTable *GetLabelColorTable() const;

  void Initialize(WrapperType *wrapper);
  void UpdateImagePointer(ImageType *image);

  void DeriveFromReferenceWrapper(WrapperType *refwrapper);

  DisplaySlicePointer GetDisplaySlice(unsigned int slice);

  virtual IntensityCurveInterface *GetIntensityCurve() const { return NULL; }
  virtual ColorMap *GetColorMap() const { return NULL; }

  virtual void Save(Registry &folder) {}
  virtual void Restore(Registry &folder) {}

protected:

  ColorLabelTableDisplayMappingPolicy();
  ~ColorLabelTableDisplayMappingPolicy();

  typedef LabelToRGBAFilter RGBAFilterType;
  typedef SmartPtr<RGBAFilterType> RGBAFilterPointer;

  RGBAFilterPointer m_RGBAFilter[3];
  WrapperType *m_Wrapper;
};

/**
 * @brief The parent class for the policies that involve curve-based mappings,
 * for both scalar and vector images.
 */
class AbstractContinuousImageDisplayMappingPolicy : public AbstractDisplayMappingPolicy
{
public:
  irisITKAbstractObjectMacro(AbstractContinuousImageDisplayMappingPolicy,
                             AbstractDisplayMappingPolicy)

  virtual IntensityCurveInterface *GetIntensityCurve() const = 0;
  virtual Vector2d GetNativeImageRangeForCurve() = 0;
  virtual void AutoFitContrast() = 0;
};

class AbstractCachingAndColorMapDisplayMappingPolicy
    : public AbstractContinuousImageDisplayMappingPolicy
{
public:
  irisITKAbstractObjectMacro(AbstractCachingAndColorMapDisplayMappingPolicy,
                             AbstractContinuousImageDisplayMappingPolicy)

  virtual void SetReferenceIntensityRange(double refMin, double refMax) = 0;
  virtual void ClearReferenceIntensityRange() = 0;

};

template<class TWrapperTraits>
class CachingCurveAndColorMapDisplayMappingPolicy
    : public AbstractCachingAndColorMapDisplayMappingPolicy
{
public:

  irisITKObjectMacro(CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>,
                     AbstractCachingAndColorMapDisplayMappingPolicy)

  typedef typename TWrapperTraits::WrapperType WrapperType;
  typedef typename TWrapperTraits::ImageType ImageType;
  typedef typename ImageType::PixelType PixelType;
  typedef itk::Image<PixelType, 2> InputSliceType;

  typedef ImageWrapperBase::DisplaySliceType DisplaySliceType;
  typedef ImageWrapperBase::DisplaySlicePointer DisplaySlicePointer;
  typedef ImageWrapperBase::DisplayPixelType DisplayPixelType;

  void CopyIntensityMap(WrapperType *srcWrapper);

  /**
   * Set the reference intensity range - a range of intensity that
   * is mapped to the domain of the intensity curve
   * @see GetIntensityMapFunction
   */
  void SetReferenceIntensityRange(double refMin, double refMax);
  void ClearReferenceIntensityRange();


  Vector2d GetNativeImageRangeForCurve();


  /**
   * Get the display slice in a given direction.  To change the
   * display slice, call parent's MoveToSlice() method
   */
  DisplaySlicePointer GetDisplaySlice(unsigned int dim);

  /**
    Get a pointer to the colormap
    */
  ColorMap *GetColorMap () const;

  void SetColorMap(ColorMap *map);

  /**
   * Get a pointer to the intensity curve
   */
  virtual IntensityCurveInterface *GetIntensityCurve() const;

  /**
   * Set a new intensity curve
   */
  void SetIntensityCurve(IntensityCurveInterface *curve);

  /**
    Automatically rescale the intensity range based on image histogram
    quantiles.
    */
  void AutoFitContrast();

  void Initialize(WrapperType *wrapper);
  void UpdateImagePointer(ImageType *image);

  void DeriveFromReferenceWrapper(WrapperType *refwrapper);

  virtual void Save(Registry &folder);
  virtual void Restore(Registry &folder);


protected:

  CachingCurveAndColorMapDisplayMappingPolicy();
  ~CachingCurveAndColorMapDisplayMappingPolicy();


  // Lookup table
  typedef itk::Image<DisplayPixelType, 1>                     LookupTableType;

  // Filter that generates the lookup table
  typedef IntensityToColorLookupTableImageFilter<
              ImageType, LookupTableType>               LookupTableFilterType;

  // Filter that applies the lookup table to slices
  typedef LookupTableIntensityMappingFilter<
                InputSliceType, DisplaySliceType>         IntensityFilterType;


  // LUT generator
  SmartPtr<LookupTableFilterType> m_LookupTableFilter;

  // Filters for the three slice directions
  SmartPtr<IntensityFilterType> m_IntensityFilter[3];

  /**
   * Implementation of the intensity curve funcitonality. The intensity map
   * transforms input intensity values into the range [0 1], which serves as
   * the input into the color map transform.
   */
  SmartPtr<IntensityCurveVTK> m_IntensityCurveVTK;

  /**
    * Color map, which maps intensity values normalized to the range [0 1]
    * to output RGB values
    */
  SmartPtr<ColorMap> m_ColorMap;

  WrapperType *m_Wrapper;

};

/**
 * This little struct describes the behavior of the multichannel display
 * mapping policy
 */
struct MultiChannelDisplayMode
{
  /**
   * Whether or not components are mapped to RGB channels. This is only
   * allowed if the image has 3 components
   */
  bool UseRGB;

  /**
   * When not in RGB mode, which scalar representation is selected for
   * display. Only used if UseRGB is false.
   */
  VectorImageWrapperBase::ScalarRepresentation SelectedScalarRep;

  /**
   * When the scalar representation is 'component', which component is
   * selected for display.
   */
  int SelectedComponent;

  /**
   * Animation speed - relevant only when scalar representation is component.
   * The value of 0 means there is no animation. TODO: Implement this!
   */
  int AnimationSpeed;

  /** Default constructor - select first component */
  MultiChannelDisplayMode();

  /** Initialize for RGB mode */
  static MultiChannelDisplayMode DefaultForRGB();

  /** Save to registry */
  void Save(Registry &reg);

  /** Restore from registry */
  static MultiChannelDisplayMode Load(Registry &reg);

  typedef VectorImageWrapperBase::ScalarRepresentation ScalarRepresentation;
  static RegistryEnumMap<ScalarRepresentation> &GetScalarRepNames();
};



/**
 * Display mapping policy for speed and level set images, i.e., images that
 * have a defined range of intensity and a colormap that takes them from
 * intensity values to display RGB values. This policy can work with both
 * integral and floating point types.
 */
class AbstractLinearColorMapDisplayMappingPolicy : public AbstractDisplayMappingPolicy
{
public:
  irisITKAbstractObjectMacro(AbstractLinearColorMapDisplayMappingPolicy,
                             AbstractContinuousImageDisplayMappingPolicy)

};

namespace itk {
  template <class TInput,class TOutput,class TFunctor>
    class UnaryFunctorImageFilter;
}

template <class TWrapperTraits>
class LinearColorMapDisplayMappingPolicy
    : public AbstractLinearColorMapDisplayMappingPolicy
{
public:

  irisITKObjectMacro(LinearColorMapDisplayMappingPolicy<TWrapperTraits>,
                     AbstractLinearColorMapDisplayMappingPolicy)

  typedef typename TWrapperTraits::WrapperType WrapperType;
  typedef typename TWrapperTraits::ImageType ImageType;
  typedef typename ImageType::PixelType PixelType;

  typedef itk::Image<PixelType, 2> InputSliceType;
  typedef ImageWrapperBase::DisplaySliceType DisplaySliceType;
  typedef ImageWrapperBase::DisplaySlicePointer DisplaySlicePointer;
  typedef ImageWrapperBase::DisplayPixelType DisplayPixelType;

  void Initialize(WrapperType *wrapper);
  void UpdateImagePointer(ImageType *image);

  virtual DisplaySlicePointer GetDisplaySlice(unsigned int slice);

  virtual IntensityCurveInterface *GetIntensityCurve() const { return NULL; }

  virtual void Save(Registry &folder);
  virtual void Restore(Registry &folder);

  irisGetMacro(ColorMap, ColorMap *)

  protected:

  LinearColorMapDisplayMappingPolicy();
  virtual ~LinearColorMapDisplayMappingPolicy();

  class MappingFunctor
  {
  public:
    DisplayPixelType operator()(PixelType in);
    double m_Scale, m_Shift;
    ColorMap *m_ColorMap;
    bool operator != (const MappingFunctor &f);
  };

  typedef itk::UnaryFunctorImageFilter
    <InputSliceType, DisplaySliceType, MappingFunctor> IntensityFilterType;
  typedef SmartPtr<IntensityFilterType> IntensityFilterPointer;

  IntensityFilterPointer m_Filter[3];
  MappingFunctor m_Functor;

  SmartPtr<ColorMap> m_ColorMap;
  WrapperType *m_Wrapper;
};


class AbstractMultiChannelDisplayMappingPolicy
    : public AbstractContinuousImageDisplayMappingPolicy
{
public:
  irisITKAbstractObjectMacro(AbstractMultiChannelDisplayMappingPolicy,
                             AbstractContinuousImageDisplayMappingPolicy)

  irisVirtualGetMacro(DisplayMode, MultiChannelDisplayMode)
  irisVirtualSetMacro(DisplayMode, MultiChannelDisplayMode)

};

template <class TWrapperTraits>
class MultiChannelDisplayMappingPolicy
    : public AbstractMultiChannelDisplayMappingPolicy
{
public:

  irisITKObjectMacro(MultiChannelDisplayMappingPolicy<TWrapperTraits>,
                     AbstractMultiChannelDisplayMappingPolicy)


  typedef typename TWrapperTraits::WrapperType WrapperType;
  typedef typename TWrapperTraits::ImageType ImageType;
  typedef typename ImageType::PixelType PixelType;
  typedef typename ImageType::InternalPixelType InternalPixelType;

  typedef itk::Image<InternalPixelType, 2> InputSliceType;

  typedef ImageWrapperBase::DisplaySliceType DisplaySliceType;
  typedef ImageWrapperBase::DisplaySlicePointer DisplaySlicePointer;
  typedef ImageWrapperBase::DisplayPixelType DisplayPixelType;

  typedef typename VectorImageWrapperBase::ScalarRepresentation ScalarRep;

  void Initialize(WrapperType *wrapper);
  void UpdateImagePointer(ImageType *image);

  void DeriveFromReferenceWrapper(WrapperType *refwrapper);


  /** Set the display mode */
  virtual void SetDisplayMode(MultiChannelDisplayMode mode);

  /** Get the display mode */
  irisGetMacro(DisplayMode, MultiChannelDisplayMode)

  DisplaySlicePointer GetDisplaySlice(unsigned int slice);

  Vector2d GetNativeImageRangeForCurve();

  /**
   * @brief Returns true when the display mode is such that the image min, max
   * and histogram used for contrast adjustment purposes use intensities pooled
   * from all of the components. This is the case when the components are being
   * animated or when display is in RGB mode.
   */
  bool IsContrastMultiComponent() const;

  virtual IntensityCurveInterface *GetIntensityCurve() const;
  virtual ColorMap *GetColorMap() const;

  virtual void Save(Registry &folder);
  virtual void Restore(Registry &folder);

  virtual void AutoFitContrast();

protected:

  MultiChannelDisplayMappingPolicy();
  ~MultiChannelDisplayMappingPolicy();

  typedef RGBALookupTableIntensityMappingFilter<InputSliceType> ApplyLUTFilter;
  typedef itk::Image<unsigned char, 1>                         LookupTableType;

  typedef MultiComponentImageToScalarLookupTableImageFilter<ImageType, LookupTableType>
                                                             GenerateLUTFilter;

  MultiChannelDisplayMode m_DisplayMode;

  ScalarImageWrapperBase *m_ScalarRepresentation;
  WrapperType *m_Wrapper;

  SmartPtr<GenerateLUTFilter> m_LUTGenerator;
  SmartPtr<ApplyLUTFilter> m_RGBMapper[3];

  void ModifiedEventCallback();
};



#endif // DISPLAYMAPPINGPOLICY_H
