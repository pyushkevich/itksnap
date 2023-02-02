#ifndef ITKREPRESENTIMAGEASVECTORIMAGEFILTER_H
#define ITKREPRESENTIMAGEASVECTORIMAGEFILTER_H

#include "itkImageToImageFilter.h"
#include "itkVectorImage.h"

namespace itk
{
/**
 *\class RepresentImageAsVectorImageFilter
 * \brief Takes an image and outputs a vector image with the same buffer
 */

template <typename TInputImage,
          typename TOutputImage = VectorImage<typename TInputImage::PixelType, TInputImage::ImageDimension>>
class ITK_TEMPLATE_EXPORT RepresentImageAsVectorImageFilter : public ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(RepresentImageAsVectorImageFilter);

  using Self = RepresentImageAsVectorImageFilter;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;
  using Superclass = ImageToImageFilter<TInputImage, TOutputImage>;
  itkNewMacro(Self);
  itkTypeMacro(RepresentImageAsVectorImageFilter, ImageToImageFilter);

  static constexpr unsigned int Dimension = TInputImage::ImageDimension;

  using InputImageType = TInputImage;
  using OutputImageType = TOutputImage;
  using InputPixelType = typename InputImageType::PixelType;
  using OutputPixelType = typename OutputImageType::PixelType;
  using RegionType = typename InputImageType::RegionType;

#ifdef ITK_USE_CONCEPT_CHECKING
  // Begin concept checking
  itkConceptMacro(InputCovertibleToOutputCheck,
                  (Concept::Convertible<InputPixelType, typename NumericTraits<OutputPixelType>::ValueType>));
  // End concept checking
#endif

protected:
  RepresentImageAsVectorImageFilter() {}

  void GenerateOutputInformation() override
    {
    this->Superclass::GenerateOutputInformation();
    this->GetOutput()->SetNumberOfComponentsPerPixel(1);
    }

  // Generate data just updates the output header and copies the input data pointer
  void GenerateData() override
    {
    InputImageType *input = const_cast<InputImageType *>(this->GetInput());
    this->GetOutput()->CopyInformation(input);
    this->GetOutput()->SetBufferedRegion(input->GetBufferedRegion());
    this->GetOutput()->SetRequestedRegion(input->GetRequestedRegion());
    this->GetOutput()->SetLargestPossibleRegion(input->GetLargestPossibleRegion());
    this->GetOutput()->SetNumberOfComponentsPerPixel(1);
    this->GetOutput()->SetPixelContainer(input->GetPixelContainer());
    }
};
} // namespace itk




#endif // ITKREPRESENTIMAGEASVECTORIMAGEFILTER_H
