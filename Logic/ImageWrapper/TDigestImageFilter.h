#ifndef TDIGESTIMAGEFILTER_H
#define TDIGESTIMAGEFILTER_H

#include <itkDataObject.h>
#include <itkNumericTraits.h>
#include "SNAPCommon.h"
#include <itkSimpleDataObjectDecorator.h>
#include <digestible/digestible.h>
#include <itkVectorImage.h>
#include <itkImageToImageFilter.h>
#include <itkImageSink.h>

/**
 * A wrapper around the t-digest data structure that can be used in ITK
 * pipelines and can provide basic statistics about an image
 */
class TDigestDataObject : public itk::DataObject
{
public:
  irisITKObjectMacro(TDigestDataObject, itk::DataObject)

  float GetImageMaximum() const { return m_Digest.max(); }
  float GetImageMinimum() const { return m_Digest.min(); }
  float GetImageQuantile(double q) const { return m_Digest.quantile(100.0 * q); }
  float GetCDF(float value) const { return m_Digest.cumulative_distribution(value); }
  unsigned GetTotalWeight() const { return m_Digest.size(); }

  template <class TInputImage> friend class TDigestImageFilter;

  static constexpr int DIGEST_SIZE = 1000;

protected:
  TDigestDataObject() : m_Digest(DIGEST_SIZE) {}
  virtual ~TDigestDataObject() {}

  // The t-digest - the compression parameter determines accuracy and memory use
  typedef digestible::tdigest<float, unsigned> TDigest;
  TDigest m_Digest;

  // The number of NaN pixels
  unsigned long m_NaNCount = 0;

  // Intensity transform
  double m_TransformScale, m_TransformShift;
};

/**
 * This ITK-style filter approximates the quantiles of an image. It uses the
 * t-digest algorithm by T. Dunning to approximate the CDF of an image with
 * good properties. The computation is very fast, taking about 50ms for a
 * large 3D image.
 *
 * The image is just passed through as is. Quantiles can be obtained using the
 * GetQuantile() method after the filter has run.
 *
 * code: https://github.com/SpirentOrion/digestible
 * paper: https://www.sciencedirect.com/science/article/pii/S2665963820300403
 *
 */
template <class TInputImage>
class TDigestImageFilter : public itk::ImageSink<TInputImage>
{
public:

  /** Extract dimension from input image. */
  itkStaticConstMacro(InputImageDimension, unsigned int,
                      TInputImage::ImageDimension);

  /** Standard class typedefs. */
  typedef TDigestImageFilter                                  Self;
  typedef itk::ImageSink<TInputImage>                         Superclass;
  typedef itk::SmartPointer< Self >                           Pointer;
  typedef itk::SmartPointer< const Self >                     ConstPointer;

  /** Image related typedefs. */
  typedef typename TInputImage::Pointer InputImagePointer;

  typedef typename TInputImage::RegionType RegionType;
  typedef typename TInputImage::SizeType   SizeType;
  typedef typename TInputImage::IndexType  IndexType;
  typedef typename TInputImage::PixelType  PixelType;
  typedef typename TInputImage::InternalPixelType  InternalPixelType;

  /**
   * The component type is PixelType for scalar images and InternalPixelType
   * for VectorImages.
   */
  using IsVector = std::is_base_of<itk::VectorImage<InternalPixelType, InputImageDimension>, TInputImage>;
  using ComponentType = typename std::conditional<IsVector::value, InternalPixelType, PixelType>::type;

  /**
   *  For compatibility with older code, the filter also outputs image
   *  minimum and maximum as itk::DataObjects
   */
  typedef itk::SimpleDataObjectDecorator<ComponentType> MinMaxObjectType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Run-time type information (and related methods). */
  itkTypeMacro(TDigestImageFilter, ImageSink)

  /** Image typedef support. */
  typedef TInputImage InputImageType;

  /**
   * Set an optional transform for the histogram. The range of the output
   * histogram will be transformed as (scale * x + shift). By default, the
   * transform is (1, 0).
   */
  void SetIntensityTransform(double scale, double shift);

  /**
   * Only insert a fraction of pixels into the TDigest. The fraction inserted will
   * be approximately 2^log_2_sampling_rate i.e., if log_2_sampling_rate is 4, then
   * every 16th pixel will be sampled, on average. Sampling uses a pseudo-random
   * number generator to skip pixels. Min, max and the number of NaN values are still
   * computed from the entire image. Sampling is recommended for very large images
   * for performance reasons, since TDigest insertion is around 60ns per pixel.
   */
  void SetLog2SamplingRate(int log_2_sampling_rate);

  /**
   * Get the t-digest output, wrapped as an itk::DataObject. Before using this object
   * call Update() on it.
   */
  TDigestDataObject *GetTDigest() { return m_TDigestDataObject; }

  /** Get the image min as an itk::DataObject for use in pipelines */
  const MinMaxObjectType *GetImageMin() const { return m_ImageMinDataObject; }

  /** Get the image min as an itk::DataObject for use in pipelines */
  const MinMaxObjectType *GetImageMax() const { return m_ImageMaxDataObject; }

protected:

  TDigestImageFilter();
  virtual ~TDigestImageFilter() {}
  void PrintSelf(std::ostream & os, itk::Indent indent) const ITK_OVERRIDE;

  virtual void BeforeStreamedGenerateData() override;
  virtual void AfterStreamedGenerateData() override;
  virtual void ThreadedStreamedGenerateData(const RegionType &) override;
  virtual void StreamedGenerateData(unsigned int inputRequestedRegionNumber) override;

private:

  TDigestImageFilter(const Self &); //purposely not implemented
  void operator=(const Self &);               //purposely not implemented

  // The computed digest
  typename TDigestDataObject::Pointer m_TDigestDataObject;

  // The min/max objects
  typename MinMaxObjectType::Pointer m_ImageMinDataObject, m_ImageMaxDataObject;

  // Intensity transform
  double m_TransformScale, m_TransformShift;

  // Sampling rate
  int m_Log2SamplingRate;

  // Mutex for combining digests
  std::mutex m_Mutex;

};

#ifndef ITK_MANUAL_INSTANTIATION
#include "TDigestImageFilter.hxx"
#endif


#endif // TDIGESTIMAGEFILTER_H
