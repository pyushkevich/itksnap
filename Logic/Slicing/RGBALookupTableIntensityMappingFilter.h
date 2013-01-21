#ifndef MULTICHANNELLUTINTENSITYMAPPINGFILTER_H
#define MULTICHANNELLUTINTENSITYMAPPINGFILTER_H

#include "SNAPCommon.h"
#include "itkRGBAPixel.h"
#include <itkImageToImageFilter.h>


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

  typedef itk::Image<OutputComponentType, 1>                  LookupTableType;
  typedef typename Superclass::OutputImageRegionType    OutputImageRegionType;

  itkTypeMacro(RGBALookupTableIntensityMappingFilter, ImageToImageFilter)
  itkNewMacro(Self)

  /** Set the intensity remapping curve - for contrast adjustment */
  void SetLookupTable(LookupTableType *lut);

  /** The actual work */
  void ThreadedGenerateData(const OutputImageRegionType &region,
                            itk::ThreadIdType threadId);

protected:

  RGBALookupTableIntensityMappingFilter();
  virtual ~RGBALookupTableIntensityMappingFilter() {}

  SmartPtr<LookupTableType> m_LookupTable;
};


#endif // MULTICHANNELLUTINTENSITYMAPPINGFILTER_H
