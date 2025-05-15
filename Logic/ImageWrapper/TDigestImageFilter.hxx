#ifndef TDIGESTIMAGEFILTER_HXX
#define TDIGESTIMAGEFILTER_HXX

#include <type_traits>
#include <cmath>
#include "TDigestImageFilter.h"
#include <itkImageRegionConstIterator.h>
#include <itkVectorImage.h>
#include <random>
#include <chrono>

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

// This is the function applied to each component
template <class TValue>
constexpr void skip_value(const TValue &value, TValue &skip_min, TValue &skip_max, unsigned long &nan_count)
{
  if constexpr (std::is_floating_point<TValue>::value)
    {
    // Finite values are added to the TDigest
    if(std::isfinite(value))
      {
      skip_min = std::min(value, skip_min);
      skip_max = std::max(value, skip_max);
      }
    else if(std::isnan(value))
      nan_count++;
    }
  else
    {
    skip_min = std::min(value, skip_min);
    skip_max = std::max(value, skip_max);
    }
};

template <class TImage, class TDigest>
class Helper
{
public:
  using RegionType = typename TImage::RegionType;
  using PixelType = typename TImage::PixelType;
  using Iterator = itk::ImageRegionConstIterator<TImage>;
  static constexpr unsigned int Dim = TImage::ImageDimension;

  static void to_buffer(Iterator &it, PixelType *buffer, int buffer_size, int &n_read)
    {
    for(n_read = 0; n_read < buffer_size && !it.IsAtEnd(); ++it, ++n_read)
      buffer[n_read] = it.Get();
    }

  /*
  static void fill_digest(
      const TImage *image, const typename TImage::RegionType &region,
      int sampling_rate,
      TDigest &tdigest, unsigned long &nan_count)
  {
    // Keep track of NaNs
    nan_count = 0;

    // Random generator for the sampling
    std::random_device r;
    std::seed_seq seed2{r(), r(), r(), r(), r(), r(), r(), r()};
    std::ranlux48_base rand_src(seed2);
    std::uniform_int_distribution<int> uniform_dist(1, sampling_rate);

    typedef itk::ImageRegionConstIterator<TImage> Iterator;
    int samples_to_go = uniform_dist(rand_src);
    for(Iterator it(image, region); !it.IsAtEnd(); ++it, --samples_to_go)
      if(samples_to_go == 0)
        {
        add_value(it.Get(), tdigest, nan_count);

        }

    tdigest.merge();
  }
  */
};

template <class TPixel, unsigned int VDim, class TDigest>
class Helper<itk::VectorImage<TPixel, VDim>, TDigest>
{
public:
  using TImage = itk::VectorImage<TPixel, VDim>;
  using RegionType = typename TImage::RegionType;
  using PixelType = typename TImage::PixelType;
  using ComponentType = typename TImage::InternalPixelType;
  static constexpr unsigned int Dim = TImage::ImageDimension;
  using Iterator = itk::ImageRegionConstIterator<TImage>;

  static void to_buffer(Iterator &it, ComponentType *buffer, int buffer_size, int &n_read)
    {
    unsigned int ncomp = it.GetImage()->GetNumberOfComponentsPerPixel();
    unsigned int buffer_size_adj = ncomp * (buffer_size / ncomp);
    for(n_read = 0; n_read < buffer_size_adj && !it.IsAtEnd(); ++it)
      {
      const auto &p = it.Get();
      for(int k = 0; k < ncomp; k++, n_read++)
        buffer[n_read] = p[k];
      }
    }

  /*
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
  */
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
  m_Log2SamplingRate = 0;
}

template <class TInputImage>
void
TDigestImageFilter<TInputImage>
::SetIntensityTransform(double scale, double shift)
{
  this->m_TDigestDataObject->m_TransformScale = scale;
  this->m_TDigestDataObject->m_TransformShift = shift;
  this->Modified();
}

template <class TInputImage>
void
TDigestImageFilter<TInputImage>
::SetLog2SamplingRate(int log_2_sampling_rate)
{
  this->m_Log2SamplingRate = log_2_sampling_rate;
  this->Modified();
}

/*
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
*/

template< class TInputImage >
void
TDigestImageFilter<TInputImage>
::StreamedGenerateData(unsigned int inputRequestedRegionNumber)
{
  auto t_start = std::chrono::steady_clock::now();
  Superclass::StreamedGenerateData(inputRequestedRegionNumber);
  auto t_stop = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t_stop - t_start);

  auto n_pixels = this->GetInput()->GetBufferedRegion().GetNumberOfPixels();
  auto n_comp = this->GetInput()->GetNumberOfComponentsPerPixel();
  auto n_digest = this->m_TDigestDataObject->GetTotalWeight();

  /*
  std::cout << "TDigest of size " << n_digest
            << " for image " << this->GetInput() << " with " << n_pixels << " pixels and " << n_comp << " components"
            << " computed in " << duration.count() << "ms."
            << std::endl;
  */
}


template< class TInputImage >
void
TDigestImageFilter<TInputImage>
::BeforeStreamedGenerateData()
{
  m_TDigestDataObject->m_Digest.reset();
  m_TDigestDataObject->m_NaNCount = 0;
}

template< class TInputImage >
void
TDigestImageFilter<TInputImage>
::ThreadedStreamedGenerateData(const RegionType &region)
{
  // Get the input image
  const TInputImage *img = this->GetInput();

  // Fill the digest for this thread
  typename TDigestDataObject::TDigest thread_digest(TDigestDataObject::DIGEST_SIZE);
  unsigned long thread_nan_count = 0;

  // An iterator used to parse the image
  typedef itk::ImageRegionConstIterator<TInputImage> Iterator;
  Iterator it(img, region);

  // A helper class used to access pixels depending on iterator type
  using HelperType = Helper<TInputImage, typename TDigestDataObject::TDigest>;

  // A buffer used to hold data extracted by the image iterator, assuming the overhead
  // of copying the data to this buffer will be negligible. The buffer size should be
  // at least as large as the number of components for multicomponent images
  int buffer_size = std::max(1024u, img->GetNumberOfComponentsPerPixel());
  int buffer_read = 0;

  // Determine the sampling rate. Samples will be taken pseudorandomly from the
  // buffer, so the buffer should be sized proportional to the sampling rate.
  int sampling_rate = 1 << this->m_Log2SamplingRate;
  buffer_size = std::max(buffer_size, 128 * sampling_rate);

  // Allocate the buffer
  ComponentType *buffer = new ComponentType[buffer_size];

  // Split depending on whether we are randomly sampling or not
  if(sampling_rate == 1)
    {
    // If not sampling, the procedure is basic
    while(!it.IsAtEnd())
      {
      // Copy a chunk of the image to the buffer
      HelperType::to_buffer(it, buffer, buffer_size, buffer_read);

      // Digest the buffer
      for(int i = 0; i < buffer_read; i++)
        add_value(buffer[i], thread_digest, thread_nan_count);
      }
    }
  else
    {
    // Random generator for the sampling
    std::random_device r;
    std::seed_seq seed2{r(), r(), r(), r(), r(), r(), r(), r()};
    std::ranlux48_base rand_src(seed2);

    // Keep track of the min/max of the skipped pixels, they need to be added to the digest
    unsigned long dummy_nan_count;
    ComponentType skip_min = std::numeric_limits<ComponentType>::max();
    ComponentType skip_max = std::numeric_limits<ComponentType>::min();

    // If not sampling, the procedure is basic
    while(!it.IsAtEnd())
      {
      // Copy a chunk of the image to the buffer
      HelperType::to_buffer(it, buffer, buffer_size, buffer_read);

      // Use the entire buffer to determine min/max and number of nans
      for(int i = 0; i < buffer_read; i++)
        skip_value(buffer[i], skip_min, skip_max, thread_nan_count);

      // Sample from the buffer with replacement
      std::uniform_int_distribution<int> uniform_dist(0, buffer_read - 1);
      int n_samples = buffer_read / sampling_rate;
      for(int j = 0; j < n_samples; j++)
        {
        int i = uniform_dist(rand_src);
        add_value(buffer[i], thread_digest, dummy_nan_count);
        }
      }

    // Incorporate the min/max into the digest.
    if(skip_max > thread_digest.max())
      thread_digest.insert(skip_max);
    if(skip_min > thread_digest.min())
      thread_digest.insert(skip_min);
    }

  // Get rid of the buffer
  delete[] buffer;

  // Complete the digest
  thread_digest.merge();

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
::AfterStreamedGenerateData()
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
