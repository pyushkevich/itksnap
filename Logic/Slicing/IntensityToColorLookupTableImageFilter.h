#ifndef INTENSITYTOCOLORLOOKUPTABLEIMAGEFILTER_H
#define INTENSITYTOCOLORLOOKUPTABLEIMAGEFILTER_H

#include "SNAPCommon.h"
#include <itkImageToImageFilter.h>
#include <itkSimpleDataObjectDecorator.h>

class ColorMap;
class IntensityCurveInterface;

/**
 * This is a parent class for filters that generate a lookup table based on the
 * intensity range of an input image. There are two subclasses in SNAP, one for
 * LUTs that map input values to RGBA colors directly, and another that maps
 * input values to RGB components. The class requires three inputs: the image,
 * and objects representing the image min/max intensities. The image may be a
 * vector image, a regular image, or an ImageAdaptor.
 */
template <class TInputImage, class TOutputLUT, class TComponent>
class AbstractLookupTableImageFilter
    : public itk::ImageToImageFilter<TInputImage, TOutputLUT>
{
public:

  typedef AbstractLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
                                                                          Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputLUT>          Superclass;
  typedef SmartPtr<Self>                                               Pointer;
  typedef SmartPtr<const Self>                                    ConstPointer;

  typedef TInputImage                                          InputImageType;
  typedef typename InputImageType::PixelType                   InputPixelType;
  typedef TOutputLUT                                          LookupTableType;
  typedef typename LookupTableType::PixelType                 OutputPixelType;

  typedef typename Superclass::OutputImageRegionType    OutputImageRegionType;

  // The type of the min/max inputs. The are usually InputPixelType, but may
  // be different, i.e., for VectorImage it's InternalPixelType.
  typedef TComponent                                       InputComponentType;
  typedef itk::SimpleDataObjectDecorator<InputComponentType> MinMaxObjectType;

  itkTypeMacro(AbstractLookupTableImageFilter, ImageToImageFilter)

  /** Set the intensity remapping curve - for contrast adjustment */
  virtual void SetIntensityCurve(IntensityCurveInterface *curve) = 0;

  /**
    One of the inputs to the filter is an object representing the minimum
    value of the input image intensity range. This can be the output of an
    itk::MinimumMaximumImageFilter, or it can just we a DataObject
    encapsulating a constant value. The reason we use an object and not a
    simple constant is to allow pipeline execution - for example recomputing
    the minimum and maximum of an image, if necessary
    */
  void SetImageMinInput(MinMaxObjectType *input);

  /** See notes for SetImageMinInput */
  void SetImageMaxInput(MinMaxObjectType *input);

  itkGetMacro(ImageMinInput, MinMaxObjectType *)
  itkGetMacro(ImageMaxInput, MinMaxObjectType *)


  /** Set intensity range to a pair of constants. This is useful when the
    range of the lookup table will never change. This is an alternative to
    calling SetImageMinInput() and SetImageMaxInput() */
  void SetFixedLookupTableRange(InputComponentType imin, InputComponentType imax);

  /**
    It is possible to use a separate reference intensity range when mapping
    raw intensities into the domain of the intensity curve. This is relevant
    when working with images derived from another image.

    TODO: this may not be necessary in the future
    */
  void SetReferenceIntensityRange(double min, double max);
  void RemoveReferenceIntensityRange();


  void GenerateOutputInformation();

  void EnlargeOutputRequestedRegion(itk::DataObject *output);

  void GenerateInputRequestedRegion();

  void ThreadedGenerateData(const OutputImageRegionType &region,
                            itk::ThreadIdType threadId);


protected:

  AbstractLookupTableImageFilter();
  virtual ~AbstractLookupTableImageFilter() {}

  // Things that affect the LUT computation
  SmartPtr<MinMaxObjectType> m_ImageMinInput, m_ImageMaxInput;

  // This method does the actual computation
  virtual OutputPixelType ComputeLUTValue(float inZeroOne) = 0;

  // Reference intensity range
  InputComponentType m_ReferenceMin, m_ReferenceMax;

  // Whether the reference intensity range is being used
  bool m_UseReferenceRange;
};

/**
  This filter is used to generate a color lookup table from an intensity curve,
  color map, and information regarding the range of the input image. The actual
  input image is not really used in the computation, but it should still be passed
  in. The color lookup table is implemented as a 1-D image of RGBAPixel.
  */
template<class TInputImage, class TOutputLUT,
         class TComponent = typename TInputImage::PixelType>
class IntensityToColorLookupTableImageFilter
    : public AbstractLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
{
public:
  typedef IntensityToColorLookupTableImageFilter<
            TInputImage, TOutputLUT, TComponent>                          Self;
  typedef AbstractLookupTableImageFilter<
            TInputImage, TOutputLUT, TComponent>                    Superclass;
  typedef SmartPtr<Self>                                               Pointer;
  typedef SmartPtr<const Self>                                    ConstPointer;

  typedef typename Superclass::OutputPixelType                 OutputPixelType;

  itkTypeMacro(IntensityToColorLookupTableImageFilter,
               AbstractLookupTableImageFilter)
  itkNewMacro(Self)

  /** Set the intensity remapping curve - for contrast adjustment */
  void SetIntensityCurve(IntensityCurveInterface *curve);

  irisGetMacro(IntensityCurve, IntensityCurveInterface *)

  /** Set the color map - for mapping scalars to RGB space */
  void SetColorMap(ColorMap *map);

protected:

  SmartPtr<IntensityCurveInterface> m_IntensityCurve;
  SmartPtr<ColorMap> m_ColorMap;

  virtual OutputPixelType ComputeLUTValue(float inZeroOne);
};

/**
 * This filter creates a common lookup table shared by a set of input images.
 * It is mainly used to map intensities in a multi-channel image to the
 * unsigned char, before combining them into a RGBAPixel. The class admits
 * any number of components.
 * @see IntensityToColorLookupTableImageFilter
 */
template <class TInputImage, class TOutputLUT,
          class TComponent = typename TInputImage::InternalPixelType>
class MultiComponentImageToScalarLookupTableImageFilter
    : public AbstractLookupTableImageFilter<TInputImage, TOutputLUT, TComponent>
{
public:
  typedef MultiComponentImageToScalarLookupTableImageFilter<
            TInputImage, TOutputLUT, TComponent>                          Self;
  typedef AbstractLookupTableImageFilter<
            TInputImage, TOutputLUT, TComponent>                    Superclass;
  typedef SmartPtr<Self>                                               Pointer;
  typedef SmartPtr<const Self>                                    ConstPointer;

  typedef typename Superclass::OutputPixelType                 OutputPixelType;

  itkTypeMacro(MultiComponentImageToScalarLookupTableImageFilter,
               AbstractLookupTableImageFilter)
  itkNewMacro(Self)

  /** Set the intensity remapping curve - for contrast adjustment */
  void SetIntensityCurve(IntensityCurveInterface *curve);

  irisGetMacro(IntensityCurve, IntensityCurveInterface *)
	
protected:

  SmartPtr<IntensityCurveInterface> m_IntensityCurve;

  virtual OutputPixelType ComputeLUTValue(float inZeroOne);
};



#endif // INTENSITYTOCOLORLOOKUPTABLEIMAGEFILTER_H
