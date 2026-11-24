#ifndef LOOKUPTABLEINTENSITYMAPPINGFILTER_H
#define LOOKUPTABLEINTENSITYMAPPINGFILTER_H

#include "SNAPCommon.h"
#include <itkImageToImageFilter.h>
#include <itkSimpleDataObjectDecorator.h>
#include <itkVectorImage.h>

template <class TInputPixel, class TDisplayPixel> class ColorLookupTable;


/**
  This ITK filter uses a lookup table to map image intensities. The input
  image should be of an integral type.
  */
template<class TInputImage, class TOutputImage>
class LookupTableIntensityMappingFilter :
    public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:

  typedef LookupTableIntensityMappingFilter<TInputImage, TOutputImage>   Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputImage>       Superclass;
  typedef itk::SmartPointer<Self>                                     Pointer;
  typedef itk::SmartPointer<const Self>                          ConstPointer;

  typedef TInputImage                                          InputImageType;
  typedef typename InputImageType::PixelType                   InputPixelType;
  typedef TOutputImage                                        OutputImageType;
  typedef typename OutputImageType::PixelType                 OutputPixelType;
  typedef typename OutputImageType::RegionType               OutputRegionType;

  using InputInternalPixelType = typename InputImageType::InternalPixelType;
  static constexpr unsigned int InputImageDimension = InputImageType::ImageDimension;
  using InputIsVector = std::is_base_of<itk::VectorImage<InputInternalPixelType, InputImageDimension>, InputImageType>;
  using InputComponentType = typename std::conditional<InputIsVector::value, InputInternalPixelType, InputPixelType>::type;

  // Output LUT
  using LookupTableType = ColorLookupTable<InputComponentType, OutputPixelType>;

  // Optional mask of the input slice: 255 inside the image, 0 outside
  using MaskImageType = itk::Image<unsigned char, InputImageDimension>;

  // This is necessary to use itkGet/SetInputMacros to avoid gcc compiling error
  using ProcessObject = itk::ProcessObject;

  itkTypeMacro(LookupTableIntensityMappingFilter, ImageToImageFilter)
  itkNewMacro(Self)

  /** Set the intensity remapping curve - for contrast adjustment */
  itkSetInputMacro(LookupTable, LookupTableType)

  /** Get the intensity remapping curve - for contrast adjustment */
  itkGetInputMacro(LookupTable, LookupTableType)

  /**
   * Set the mask of the input slice (optional). Pixels where the mask is 0 are
   * outside of the image (see NonOrthogonalSlicer) and are made fully
   * transparent, whatever their intensity (#193).
   */
  itkSetInputMacro(MaskInput, MaskImageType)
  itkGetInputMacro(MaskInput, MaskImageType)

  /** The actual work */
  void DynamicThreadedGenerateData(const OutputRegionType &region) override;

  /** Process a single pixel */
  OutputPixelType MapPixel(const InputPixelType &pixel);

protected:

  LookupTableIntensityMappingFilter();
  virtual ~LookupTableIntensityMappingFilter() {}
};


#endif // LOOKUPTABLEINTENSITYMAPPINGFILTER_H
