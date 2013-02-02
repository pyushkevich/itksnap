#include "MultiComponentImageToScalarLookupTableImageFilter.h"
#include "itkVectorImage.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "IntensityCurveInterface.h"

template<class TInputImage, class TOutputLUT>
MultiComponentImageToScalarLookupTableImageFilter<TInputImage, TOutputLUT>
::MultiComponentImageToScalarLookupTableImageFilter()
{

}

template<class TInputImage, class TOutputLUT>
MultiComponentImageToScalarLookupTableImageFilter<TInputImage, TOutputLUT>
::~MultiComponentImageToScalarLookupTableImageFilter()
{

}

template<class TInputImage, class TOutputLUT>
void
MultiComponentImageToScalarLookupTableImageFilter<TInputImage, TOutputLUT>
::SetMultiComponentImage(InputImageType *image)
{
  // Figure out how many components we have
  this->SetNumberOfIndexedInputs(4);

  // Set the first input
  this->SetNthInput(0, image);
}

template<class TInputImage, class TOutputLUT>
void
MultiComponentImageToScalarLookupTableImageFilter<TInputImage, TOutputLUT>
::SetIntensityCurve(IntensityCurveInterface *curve)
{
  m_IntensityCurve = curve;
  this->SetNthInput(1, curve);
}

template<class TInputImage, class TOutputLUT>
void
MultiComponentImageToScalarLookupTableImageFilter<TInputImage, TOutputLUT>
::SetImageMinInput(InputPixelObject *input)
{
  m_InputMin = input;
  this->SetNthInput(2, input);
}

template<class TInputImage, class TOutputLUT>
void
MultiComponentImageToScalarLookupTableImageFilter<TInputImage, TOutputLUT>
::SetImageMaxInput(InputPixelObject *input)
{
  m_InputMax = input;
  this->SetNthInput(3, input);
}

template<class TInputImage, class TOutputLUT>
void
MultiComponentImageToScalarLookupTableImageFilter<TInputImage, TOutputLUT>
::GenerateOutputInformation()
{
  // We need to compute a global max/min
  InputComponentType gmin = m_InputMin.Get();
  InputComponentType gmax = m_InputMax.Get();

  LookupTableType *output = this->GetOutput();
  typename LookupTableType::IndexType idx = {{gmin}};
  typename LookupTableType::SizeType sz = {{1 + gmax - gmin}};
  typename LookupTableType::RegionType region(idx, sz);
  output->SetLargestPossibleRegion(region);
}

template<class TInputImage, class TOutputLUT>
void
MultiComponentImageToScalarLookupTableImageFilter<TInputImage, TOutputLUT>
::EnlargeOutputRequestedRegion(itk::DataObject *)
{
  LookupTableType *output = this->GetOutput();
  output->SetRequestedRegion(output->GetLargestPossibleRegion());
}

template<class TInputImage, class TOutputLUT>
void
MultiComponentImageToScalarLookupTableImageFilter<TInputImage, TOutputLUT>
::GenerateInputRequestedRegion()
{
  // The input region is the whole image
  InputImageType *input = const_cast<InputImageType *>(this->GetInput());
  input->SetRequestedRegionToLargestPossibleRegion();
}


template<class TInputImage, class TOutputLUT>
void
MultiComponentImageToScalarLookupTableImageFilter<TInputImage, TOutputLUT>
::ThreadedGenerateData(const OutputImageRegionType &region,
                       itk::ThreadIdType threadId)
{
  if(threadId == 0)
    std::cout << "Computing LUT " << region << std::endl;

  InputComponentType gmin = m_InputMin.Get();
  InputComponentType gmax = m_InputMax.Get();

  // Set the intensity mapping for the functor
  float scale, shift;
  shift = gmin;
  scale = 1.0f / (gmax - gmin);

  // Do the actual computation of the cache
  LookupTableType *output = this->GetOutput();
  for(itk::ImageRegionIteratorWithIndex<LookupTableType> it(output, region);
      !it.IsAtEnd(); ++it)
    {
    // Get the lookup value we are seeking
    long pos = it.GetIndex()[0];

    // Map the input value to range of 0 to 1
    float inZeroOne = (pos - shift) * scale;

    // Compute the intensity mapping
    float outZeroOne = m_IntensityCurve->Evaluate(inZeroOne);

    // Map the output to a RGBA pixel
    it.Set(static_cast<OutputPixelType>(255.0 * outZeroOne));
    }
}

// Template instantiation
typedef itk::VectorImage<short, 3> AnatomicImageType;
typedef itk::Image<unsigned char, 1> LUTType;
template class MultiComponentImageToScalarLookupTableImageFilter<AnatomicImageType, LUTType>;


