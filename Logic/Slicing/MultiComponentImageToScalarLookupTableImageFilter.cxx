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
  int nc = image->GetNumberOfComponentsPerPixel();
  this->SetNumberOfIndexedInputs(2 * nc + 2);

  // Resize the min/max sources
  m_InputMin.resize(nc);
  m_InputMax.resize(nc);

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
::SetComponentMinMax(unsigned int comp,
                     InputComponentObject *omin,
                     InputComponentObject *omax)
{
  m_InputMin[comp] = omin;
  m_InputMax[comp] = omax;
  this->SetNthInput(2 + comp * 2, omin);
  this->SetNthInput(2 + comp * 2 + 1, omax);
}

template<class TInputImage, class TOutputLUT>
void
MultiComponentImageToScalarLookupTableImageFilter<TInputImage, TOutputLUT>
::GenerateOutputInformation()
{
  // We need to compute a global max/min
  InputComponentType gmin, gmax;
  for(int i = 0; i < m_InputMin.size(); i++)
    {
    // Since this method could be called before the upstream pipeline has
    // executed, we need to force a all to Update on the input min/max
    m_InputMin[i]->Update();
    m_GlobalMin = (i == 0 || m_GlobalMin > m_InputMin[i]->Get())
        ? m_InputMin[i]->Get()
        : m_GlobalMin;
    m_GlobalMax = (i == 0 || m_GlobalMax < m_InputMax[i]->Get())
        ? m_InputMax[i]->Get()
        : m_GlobalMax;
    }

  LookupTableType *output = this->GetOutput();
  typename LookupTableType::IndexType idx = {{m_GlobalMin}};
  typename LookupTableType::SizeType sz = {{1 + m_GlobalMax - m_GlobalMin}};
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

  // Set the intensity mapping for the functor
  float scale, shift;
  shift = m_GlobalMin;
  scale = 1.0f / (m_GlobalMax - m_GlobalMin);

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


