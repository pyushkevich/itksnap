#ifndef MULTICHANNELLUTINTENSITYMAPPINGFILTER_H
#define MULTICHANNELLUTINTENSITYMAPPINGFILTER_H

#include "SNAPCommon.h"
#include "itkRGBAPixel.h"
#include <itkImageToImageFilter.h>

template <class TInputPixel, class TDisplayPixel> class ColorLookupTable;


/**
 * This filter takes multiple input channels and a common lookup table, and
 * generates an image of pixels of a certain vector type, e.g., RGBAPixel.
 */
template<class TInputImage>
class RGBALookupTableIntensityMappingFilter :
    public itk::ImageToImageFilter<TInputImage,
                                   itk::Image<itk::RGBAPixel<unsigned char>, 2> >
{
public:

  typedef typename itk::RGBAPixel<unsigned char>              OutputPixelType;
  typedef itk::Image<OutputPixelType, 2>                      OutputImageType;

  typedef RGBALookupTableIntensityMappingFilter<TInputImage>             Self;
  typedef itk::ImageToImageFilter<TInputImage, OutputImageType>    Superclass;
  typedef itk::SmartPointer<Self>                                     Pointer;
  typedef itk::SmartPointer<const Self>                          ConstPointer;

  typedef TInputImage                                          InputImageType;
  typedef typename InputImageType::PixelType                   InputPixelType;

  typedef typename OutputPixelType::ComponentType         OutputComponentType;

  typedef ColorLookupTable<InputPixelType, OutputComponentType> LookupTableType;
  typedef typename Superclass::OutputImageRegionType    OutputImageRegionType;

  // Optional mask of the input slices: 255 inside the image, 0 outside
  typedef itk::Image<unsigned char, InputImageType::ImageDimension> MaskImageType;

  // This is necessary to use itkGet/SetInputMacros to avoid gcc compiling error
  using ProcessObject = itk::ProcessObject;

  itkTypeMacro(RGBALookupTableIntensityMappingFilter, ImageToImageFilter)
  itkNewMacro(Self)

  /** Set the intensity remapping curve - for contrast adjustment */
  itkSetInputMacro(LookupTable, LookupTableType)

  /** Get the intensity remapping curve - for contrast adjustment */
  itkGetInputMacro(LookupTable, LookupTableType)

  /**
   * Set the mask of the input slices (optional). Pixels where the mask is 0
   * are outside of the image (see NonOrthogonalSlicer) and are made fully
   * transparent, whatever their intensities (#193).
   */
  itkSetInputMacro(MaskInput, MaskImageType)
  itkGetInputMacro(MaskInput, MaskImageType)

  /** Set the mapping mode to two-channel HSV */
  void SetMappingModeToTwoChannelHueValue();

  /** Set the mapping mode to two-channel HSV */
  void SetMappingModeToThreeChannelRGB();

  /** The actual work */
  void DynamicThreadedGenerateData(const OutputImageRegionType &region) override;

  /** Process a single pixel */
  OutputPixelType MapPixel(const InputPixelType &xin0, const InputPixelType &xin1, const InputPixelType &xin2);


protected:
  bool m_TwoChannelHueValueMode = false;
  RGBALookupTableIntensityMappingFilter();
  virtual ~RGBALookupTableIntensityMappingFilter() {}
  void MapPixelXYZtoRGB(
      InputPixelType xin0, InputPixelType xin1, InputPixelType xin2,
      const LookupTableType *lut, OutputPixelType &xout);
  void MapPixelXYtoHSV(
      InputPixelType xin0, InputPixelType xin1,
      const LookupTableType *lut, OutputPixelType &xout);
};


#endif // MULTICHANNELLUTINTENSITYMAPPINGFILTER_H
