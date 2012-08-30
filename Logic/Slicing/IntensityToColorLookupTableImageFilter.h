#ifndef INTENSITYTOCOLORLOOKUPTABLEIMAGEFILTER_H
#define INTENSITYTOCOLORLOOKUPTABLEIMAGEFILTER_H

#include "SNAPCommon.h"
#include <itkImageToImageFilter.h>
#include <itkSimpleDataObjectDecorator.h>

class ColorMap;
class IntensityCurveInterface;

/**
  This filter is used to generate a color lookup table from an intensity curve,
  color map, and information regarding the range of the input image. The actual
  input image is really used in the computation, but it should still be passed
  in. The color lookup table is implemented as a 1-D image of RGBAPixel.
  */
template <class TInputImage, class TOutputLUT>
class IntensityToColorLookupTableImageFilter
    : public itk::ImageToImageFilter<TInputImage, TOutputLUT>
{
public:

  typedef IntensityToColorLookupTableImageFilter<TInputImage, TOutputLUT> Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputLUT>          Superclass;
  typedef SmartPtr<Self>                                               Pointer;
  typedef SmartPtr<const Self>                                    ConstPointer;

  typedef TInputImage                                          InputImageType;
  typedef typename InputImageType::PixelType                   InputPixelType;
  typedef TOutputLUT                                          LookupTableType;
  typedef typename LookupTableType::PixelType                 OutputPixelType;

  typedef itk::SimpleDataObjectDecorator<InputPixelType>     InputPixelObject;
  typedef typename Superclass::OutputImageRegionType    OutputImageRegionType;

  itkTypeMacro(IntensityToColorLookupTableImageFilter, ImageToImageFilter)
  itkNewMacro(Self)

  /** Set the intensity remapping curve - for contrast adjustment */
  void SetIntensityCurve(IntensityCurveInterface *curve);

  /** Set the color map - for mapping scalars to RGB space */
  void SetColorMap(ColorMap *map);

  /**
    One of the inputs to the filter is an object representing the minimum
    value of the input image intensity range. This can be the output of an
    itk::MinimumMaximumImageFilter, or it can just we a DataObject
    encapsulating a constant value. The reason we use an object and not a
    simple constant is to allow pipeline execution - for example recomputing
    the minimum and maximum of an image, if necessary
    */
  void SetImageMinInput(InputPixelObject *input);

  /** See notes for SetImageMinInput */
  void SetImageMaxInput(InputPixelObject *input);

  /** Set intensity range to a pair of constants. This is useful when the
    range of the lookup table will never change. This is an alternative to
    calling SetImageMinInput() and SetImageMaxInput() */
  void SetFixedLookupTableRange(InputPixelType imin, InputPixelType imax);

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

  void ThreadedGenerateData(const OutputImageRegionType &region, int threadId);


protected:

  IntensityToColorLookupTableImageFilter();
  virtual ~IntensityToColorLookupTableImageFilter() {}

  // Things that affect the LUT computation
  SmartPtr<IntensityCurveInterface> m_IntensityCurve;
  SmartPtr<ColorMap> m_ColorMap;
  SmartPtr<InputPixelObject> m_InputMin, m_InputMax;

  // Reference intensity range
  InputPixelType m_ReferenceMin, m_ReferenceMax;

  // Whether the reference intensity range is being used
  bool m_UseReferenceRange;



};

#endif // INTENSITYTOCOLORLOOKUPTABLEIMAGEFILTER_H
