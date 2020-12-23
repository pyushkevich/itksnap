#ifndef INCREASEDIMENSIONIMAGEFILTER_H
#define INCREASEDIMENSIONIMAGEFILTER_H

#include "itkImageToImageFilter.h"

/**
 * This filter selects one of its inputs and presents it as an output. It is
 * used when there are several alternative pipelines that can be used to
 * produce an output. Updating the output will Update() the correct pipeline.
 *
 * The class is templated over the type TTag, which is an arbitrary type
 * that is used to identify input pipelines. This allows pipelines to be associated
 * with non-integer values (structs, enums, etc.)
 */
template <class TInputImage, class TOutputImage>
class IncreaseDimensionImageFilter
    : public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:

  typedef IncreaseDimensionImageFilter<TInputImage, TOutputImage>  Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputImage> Superclass;
  typedef itk::SmartPointer<Self>                               Pointer;
  typedef itk::SmartPointer<const Self>                    ConstPointer;
  typedef TInputImage                                    InputImageType;
  typedef TOutputImage                                  OutputImageType;

  itkNewMacro(Self)
  itkStaticConstMacro(ImageDimension, unsigned int, InputImageType::ImageDimension);

  /**
   * Generate the data - by selecting an input and presenting it as output
   */
  void GenerateData() ITK_OVERRIDE;

protected:

  IncreaseDimensionImageFilter();
  virtual ~IncreaseDimensionImageFilter() {};
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "IncreaseDimensionImageFilter.txx"
#endif


#endif // INCREASEDIMENSIONIMAGEFILTER_H
