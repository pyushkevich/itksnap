#ifndef TDIGESTIMAGEFILTER_HXX
#define TDIGESTIMAGEFILTER_HXX

#include <type_traits>
#include <cmath>
#include "TDigestImageFilter.h"
#include <itkImageRegionConstIterator.h>
#include <itkVectorImage.h>

// Type-specific functions are placed in their own namespace
namespace TDigestImageFilter_impl {

// This is the function applied to each component
template <class TValue, class TDigest>
constexpr void add_value(const TValue &value, TDigest &tdigest, unsigned long &nan_count)
{
  if constexpr (std::is_floating_point<TValue>::value)
    {
    // Finite values are added to the TDigest
    if(std::isfinite(value))
      tdigest.insert(value);
    else if(std::isnan(value))
      nan_count++;

    // TODO: what should we do with +Inf and -Inf? They complicate things
    }
  else
    {
    tdigest.insert(value);
    }
};

template <class TImage, class TDigest>
class Helper
{
public:
  using RegionType = typename TImage::RegionType;
  using PixelType = typename TImage::PixelType;
  static constexpr unsigned int Dim = TImage::ImageDimension;

  static void fill_digest(
      const TImage *image, const typename TImage::RegionType &region,
      TDigest &tdigest, unsigned long &nan_count)
  {
    // Keep track of NaNs
    nan_count = 0;

    typedef itk::ImageRegionConstIterator<TImage> Iterator;
    for(Iterator it(image, region); !it.IsAtEnd(); ++it)
      add_value(it.Get(), tdigest, nan_count);

    tdigest.merge();
  }
};

template <class TPixel, unsigned int VDim, class TDigest>
class Helper<itk::VectorImage<TPixel, VDim>, TDigest>
{
public:
  using TImage = itk::VectorImage<TPixel, VDim>;
  using RegionType = typename TImage::RegionType;
  using PixelType = typename TImage::PixelType;
  static constexpr unsigned int Dim = TImage::ImageDimension;

  static void fill_digest(
      const TImage *image, const typename TImage::RegionType &region,
      TDigest &tdigest, unsigned long &nan_count)
  {
    // Keep track of NaNs
    nan_count = 0;
    unsigned int ncomp = image->GetNumberOfComponentsPerPixel();

    typedef itk::ImageRegionConstIterator<TImage> Iterator;
    for(Iterator it(image, region); !it.IsAtEnd(); ++it)
      {
      const auto &p = it.Get();
      for(unsigned int i = 0; i < ncomp; i++)
        add_value(p[i], tdigest, nan_count);
      }

    tdigest.merge();
  }
};

}; // namespace

using namespace TDigestImageFilter_impl;

template <class TInputImage>
TDigestImageFilter<TInputImage>
::TDigestImageFilter()
{
  this->SetNumberOfRequiredInputs(1);
  this->SetNumberOfRequiredOutputs(4);

  // Allocate the output digest
  m_TDigestDataObject = TDigestDataObject::New();
  this->SetNthOutput(1, m_TDigestDataObject);

  // Allocate the min/max outputs
  m_ImageMinDataObject = MinMaxObjectType::New();
  this->SetNthOutput(2, m_ImageMinDataObject);
  m_ImageMaxDataObject = MinMaxObjectType::New();
  this->SetNthOutput(3, m_ImageMaxDataObject);

  m_TransformScale = 1.0;
  m_TransformShift = 0.0;
}

template <class TInputImage>
void
TDigestImageFilter<TInputImage>
::SetIntensityTransform(double scale, double shift)
{
  this->m_TDigestDataObject->m_TransformScale = scale;
  this->m_TDigestDataObject->m_TransformShift = shift;
}


template< class TInputImage >
void
TDigestImageFilter<TInputImage>
::AllocateOutputs()
{
  // Just pass the container to the output image
  InputImageType *inputPtr = const_cast<InputImageType *>(this->GetInput());
  InputImageType *outputPtr = this->GetOutput();
  outputPtr->CopyInformation(inputPtr);
  outputPtr->SetBufferedRegion(inputPtr->GetBufferedRegion());
  outputPtr->SetPixelContainer(inputPtr->GetPixelContainer());
}


template< class TInputImage >
void
TDigestImageFilter<TInputImage>
::BeforeThreadedGenerateData()
{
  m_TDigestDataObject->m_Digest.reset();
  m_TDigestDataObject->m_NaNCount = 0;
}

template< class TInputImage >
void
TDigestImageFilter<TInputImage>
::DynamicThreadedGenerateData(const RegionType &region)
{
  // Get the input image
  const TInputImage *img = this->GetInput();

  // Fill the digest for this thread
  typename TDigestDataObject::TDigest thread_digest(TDigestDataObject::DIGEST_SIZE);
  unsigned long thread_nan_count = 0;
  using HelperType = Helper<TInputImage, typename TDigestDataObject::TDigest>;
  HelperType::fill_digest(img, region, thread_digest, thread_nan_count);

  // Use mutex to update the global heaps
  std::lock_guard<std::mutex> guard(m_Mutex);

  // Add current digest so the main digest
  m_TDigestDataObject->m_Digest.insert(thread_digest);

  // Update global nan count
  m_TDigestDataObject->m_NaNCount += thread_nan_count;
}

template< class TInputImage >
void
TDigestImageFilter<TInputImage>
::AfterThreadedGenerateData()
{
  // Mark the output as modified (do we need to?)
  m_TDigestDataObject->Modified();

  // Get the image min and max. Here we have to cast to the original data
  // type and there is a small possibility of rounding errors.
  if constexpr (std::is_floating_point<ComponentType>::value)
    {
    m_ImageMinDataObject->Set(m_TDigestDataObject->GetImageMinimum());
    m_ImageMaxDataObject->Set(m_TDigestDataObject->GetImageMaximum());
    }
  else
    {
    m_ImageMinDataObject->Set((ComponentType) std::floor(m_TDigestDataObject->GetImageMinimum()));
    m_ImageMaxDataObject->Set((ComponentType) std::ceil(m_TDigestDataObject->GetImageMaximum()));
  }

  /*
  printf("TDigest: range: %f to %f, Percentiles: 1: %f, 5: %f, 50: %f, 95: %f, 99: %f\n",
         m_TDigestDataObject->GetImageMinimum(),
         m_TDigestDataObject->GetImageMaximum(),
         m_TDigestDataObject->GetImageQuantile(0.01),
         m_TDigestDataObject->GetImageQuantile(0.05),
         m_TDigestDataObject->GetImageQuantile(0.5),
         m_TDigestDataObject->GetImageQuantile(0.95),
         m_TDigestDataObject->GetImageQuantile(0.99));
  */

}

template< class TInputImage >
void
TDigestImageFilter<TInputImage>
::PrintSelf(std::ostream &os, itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}





#endif // TDIGESTIMAGEFILTER_HXX
