#ifndef INCREASEDIMENSIONIMAGEFILTER_TXX
#define INCREASEDIMENSIONIMAGEFILTER_TXX

#include "IncreaseDimensionImageFilter.h"

template <class TInputImage, class TOutputImage>
IncreaseDimensionImageFilter<TInputImage, TOutputImage>
::IncreaseDimensionImageFilter()
{
}

template <class TInputImage, class TOutputImage>
void
IncreaseDimensionImageFilter<TInputImage, TOutputImage>
::GenerateData()
{
  // Just pass the container to the output image
  InputImageType *inputPtr = const_cast<InputImageType *>(this->GetInput());
  OutputImageType *outputPtr = this->GetOutput();
  outputPtr->SetPixelContainer(inputPtr->GetPixelContainer());
}

template <class TInputImage, class TOutputImage>
void
IncreaseDimensionImageFilter<TInputImage, TOutputImage>
::GenerateOutputInformation()
{
  // From itk::TiledImageFilter, which does similar thing as this filter
  // "Do not call the superclass's GenerateOutptuInformation method.
  //  The input images are likely a different dimension than the input,
  //  so the superclass's implementation is not compatible."

  unsigned int n_in = InputImageType::ImageDimension;
  unsigned int n_out = OutputImageType::ImageDimension;

  itkAssertOrThrowMacro(
        n_in <= n_out,
        "Dimensions of input image must not exceed those of "
        "the output image in IncreaseDimensionImageFilter")

  // Just pass the container to the output image
  InputImageType *inputPtr = const_cast<InputImageType *>(this->GetInput());
  OutputImageType *outputPtr = this->GetOutput();

  // Data for the output image
  typename OutputImageType::PointType origin;
  typename OutputImageType::SpacingType spacing;
  typename OutputImageType::DirectionType direction;
  typename OutputImageType::SizeType size;
  typename OutputImageType::IndexType index;

  // Initialize
  origin.Fill(0.0);
  spacing.Fill(1.0);
  direction.SetIdentity();
  index.Fill(0);
  size.Fill(1);

  // Copy the lower dimensions
  for(unsigned int i = 0; i < n_in; i++)
    {
    origin[i] = inputPtr->GetOrigin()[i];
    spacing[i] = inputPtr->GetSpacing()[i];
    index[i] = inputPtr->GetLargestPossibleRegion().GetIndex()[i];
    size[i] = inputPtr->GetLargestPossibleRegion().GetSize()[i];
    for(unsigned int j = 0; j < n_in; j++)
      direction(i,j) = inputPtr->GetDirection()(i,j);
    }

  typename OutputImageType::RegionType region(index, size);

  outputPtr->SetOrigin(origin);
  outputPtr->SetSpacing(spacing);
  outputPtr->SetDirection(direction);
  outputPtr->SetLargestPossibleRegion(region);
  outputPtr->SetRequestedRegionToLargestPossibleRegion();
  outputPtr->SetNumberOfComponentsPerPixel(inputPtr->GetNumberOfComponentsPerPixel());
  outputPtr->SetBufferedRegion(region);
}


#endif // INCREASEDIMENSIONIMAGEFILTER_TXX
