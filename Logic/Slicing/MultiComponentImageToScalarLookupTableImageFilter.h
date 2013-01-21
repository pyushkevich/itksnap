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

  /** Set the minumum and maximum inputs for each component */
  void SetComponentMinMax(unsigned int comp,
                          InputComponentObject *omin,
                          InputComponentObject *omax);

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
  std::vector< SmartPtr<InputComponentObject> > m_InputMin, m_InputMax;

  // Overall min/max over all the components
  InputComponentType m_GlobalMin, m_GlobalMax;

};

#endif // MULTICOMPONENTIMAGETOSCALARLOOKUPTABLEIMAGEFILTER_H
