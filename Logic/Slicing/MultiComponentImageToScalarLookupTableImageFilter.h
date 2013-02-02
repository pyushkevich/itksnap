#ifndef MULTICOMPONENTIMAGETOSCALARLOOKUPTABLEIMAGEFILTER_H
#define MULTICOMPONENTIMAGETOSCALARLOOKUPTABLEIMAGEFILTER_H

#include "SNAPCommon.h"
#include <itkImageToImageFilter.h>
#include <itkSimpleDataObjectDecorator.h>

class IntensityCurveInterface;

/**
 * This filter creates a common lookup table shared by a set of input images.
 * It is mainly used to map intensities in a multi-channel image to the
 * unsigned char, before combining them into a RGBAPixel. The class admits
 * any number of components.
 * @see IntensityToColorLookupTableImageFilter
 */
template <class TInputImage, class TOutputLUT>
class MultiComponentImageToScalarLookupTableImageFilter
    : public itk::ImageToImageFilter<TInputImage, TOutputLUT>
{
public:

  typedef MultiComponentImageToScalarLookupTableImageFilter<TInputImage, TOutputLUT>
                                                                          Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputLUT>          Superclass;
  typedef SmartPtr<Self>                                               Pointer;
  typedef SmartPtr<const Self>                                    ConstPointer;

  typedef TInputImage                                          InputImageType;
  typedef typename InputImageType::InternalPixelType       InputComponentType;
  typedef TOutputLUT                                          LookupTableType;
  typedef typename LookupTableType::PixelType                 OutputPixelType;

  typedef itk::SimpleDataObjectDecorator<InputComponentType>
                                                         InputComponentObject;
  typedef typename Superclass::OutputImageRegionType    OutputImageRegionType;

  itkTypeMacro(IntensityToColorLookupTableImageFilter, ImageToImageFilter)
  itkNewMacro(Self)

  /** Set the intensity remapping curve - for contrast adjustment */
  void SetIntensityCurve(IntensityCurveInterface *curve);

  /** Get the intensity curve */
  irisGetMacro(IntensityCurve, IntensityCurveInterface *)

  /** Set the main input - this should be a multi-component image. This
   * should be set before all of the other inputs */
  void SetMultiComponentImage(InputImageType *image);

  /**
    One of the inputs to the filter is an object representing the minimum
    value of the input image intensity range over all the components.
    */
  void SetImageMinInput(InputPixelObject *input);

  /** See notes for SetImageMinInput */
  void SetImageMaxInput(InputPixelObject *input);

  void GenerateOutputInformation();

  void EnlargeOutputRequestedRegion(itk::DataObject *output);

  void GenerateInputRequestedRegion();

  void ThreadedGenerateData(const OutputImageRegionType &region,
                            itk::ThreadIdType threadId);

protected:

  MultiComponentImageToScalarLookupTableImageFilter();
  ~MultiComponentImageToScalarLookupTableImageFilter();

  // Things that affect the LUT computation
  SmartPtr<IntensityCurveInterface> m_IntensityCurve;

  // Overall min/max over all the components
  SmartPtr<InputPixelObject> m_InputMin, m_InputMax;
};

#endif // MULTICOMPONENTIMAGETOSCALARLOOKUPTABLEIMAGEFILTER_H
