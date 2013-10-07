#ifndef THREADEDHISTOGRAMIMAGEFILTER_H
#define THREADEDHISTOGRAMIMAGEFILTER_H

#include <itkImageToImageFilter.h>
#include <itkSimpleDataObjectDecorator.h>
#include <itkNumericTraits.h>
#include <ScalarImageHistogram.h>

/**
 * This ITK-style filter computes the histogram of an ITK scalar image. It
 * uses threading for faster histogram computation. It also is meant to be
 * used with the itk::MinimumMaximumImageFilter to avoid an extra pass for
 * determining the range of the histogram. The histogram in this filter is
 * constructed from equal size bins between the input min and max, and the
 * number of bins is a power of two.
 */
template <class TInputImage>
class ThreadedHistogramImageFilter :
    public itk::ImageToImageFilter<TInputImage, TInputImage>
{
public:

  /** Extract dimension from input image. */
  itkStaticConstMacro(InputImageDimension, unsigned int,
                      TInputImage::ImageDimension);
  itkStaticConstMacro(OutputImageDimension, unsigned int,
                      TInputImage::ImageDimension);

  /** Standard class typedefs. */
  typedef ThreadedHistogramImageFilter                        Self;
  typedef itk::ImageToImageFilter< TInputImage, TInputImage > Superclass;
  typedef itk::SmartPointer< Self >                           Pointer;
  typedef itk::SmartPointer< const Self >                     ConstPointer;

  /** Image related typedefs. */
  typedef typename TInputImage::Pointer InputImagePointer;

  typedef typename TInputImage::RegionType RegionType;
  typedef typename TInputImage::SizeType   SizeType;
  typedef typename TInputImage::IndexType  IndexType;
  typedef typename TInputImage::PixelType  PixelType;

  /** Histogram typedefs */
  typedef ScalarImageHistogram HistogramType;
  typedef SmartPtr<HistogramType> HistogramPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Run-time type information (and related methods). */
  itkTypeMacro(ThreadedHistogramImageFilter, ImageToImageFilter)

  /** Image typedef support. */
  typedef TInputImage InputImageType;

  /** Type of DataObjects used for scalar outputs */
  typedef itk::SimpleDataObjectDecorator< PixelType > PixelObjectType;

  /**
   * Set the range inputs. These are in the format returned by the min/max
   * image filter. These are inputs to the filter, so will be updated through
   * the pipeline mechanism if the image itself is updated. It is also possible
   * for the min/max to be sourced from another image.
   */
  void SetRangeInputs(PixelObjectType *inMin, PixelObjectType *inMax);

  /**
   * Set the desired number of bins for the histogram.
   */
  void SetNumberOfBins(int nBins);

  /**
   * Set an optional transform for the histogram. The range of the output
   * histogram will be transformed as (scale * x + shift). By default, the
   * transform is (1, 0).
   */
  void SetIntensityTransform(double scale, double shift);

  /**
   * Get the histogram output
   */
  HistogramType *GetHistogramOutput() const { return m_OutputHistogram; }

protected:

  ThreadedHistogramImageFilter();
  virtual ~ThreadedHistogramImageFilter() {}
  void PrintSelf(std::ostream & os, itk::Indent indent) const;

  /** Pass the input through unmodified. Do this by Grafting in the
    AllocateOutputs method. */
  void AllocateOutputs();

  /** Initialize some accumulators before the threads run. */
  void BeforeThreadedGenerateData();

  /** Do final mean and variance computation from data accumulated in threads.
    */
  void AfterThreadedGenerateData();

  /** Multi-thread version GenerateData. */
  void  ThreadedGenerateData(const RegionType &outputRegionForThread,
                             itk::ThreadIdType threadId);

  // Override since the filter needs all the data for the algorithm
  void GenerateInputRequestedRegion();

  // Override since the filter produces all of its output
  void EnlargeOutputRequestedRegion(itk::DataObject *data);

private:

  ThreadedHistogramImageFilter(const Self &); //purposely not implemented
  void operator=(const Self &);               //purposely not implemented

  // Inputs
  SmartPtr<PixelObjectType> m_InputMin, m_InputMax;

  // Parameter: number of bins
  unsigned int m_Bins;

  // Intensity transform
  double m_TransformScale, m_TransformShift;

  // Per-thread histograms
  std::vector< HistogramPointer > m_ThreadHistogram;

  // The output histogram
  HistogramPointer m_OutputHistogram;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "ThreadedHistogramImageFilter.hxx"
#endif


#endif // THREADEDHISTOGRAMIMAGEFILTER_H
