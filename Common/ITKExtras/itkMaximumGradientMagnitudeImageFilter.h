#ifndef ITKMAXIMUMGRADIENTMAGNITUDEIMAGEFILTER_H
#define ITKMAXIMUMGRADIENTMAGNITUDEIMAGEFILTER_H

#include "itkImageToImageFilter.h"
#include "itkSimpleDataObjectDecorator.h"

#include <vector>

#include "itkNumericTraits.h"

namespace itk
{
/** \class MaximumGradientMagnitudeImageFilter
 * \brief Computes the minimum and the maximum gradient magnitude of
 * an image.
 *
 * It is templated over input image type only.
 * This filter just copies the input image through this output to
 * be included within the pipeline. The implementation uses the
 * StatisticsImageFilter.
 *
 * \ingroup Operators
 * \sa StatisticsImageFilter
 * \ingroup ITKImageStatistics
 */
template< typename TInputImage >
class MaximumGradientMagnitudeImageFilter:
  public ImageToImageFilter< TInputImage, TInputImage >
{
public:
  /** Extract dimension from input image. */
  itkStaticConstMacro(InputImageDimension, unsigned int,
                      TInputImage::ImageDimension);
  itkStaticConstMacro(OutputImageDimension, unsigned int,
                      TInputImage::ImageDimension);

  /** Standard class typedefs. */
  typedef MaximumGradientMagnitudeImageFilter            Self;
  typedef ImageToImageFilter< TInputImage, TInputImage > Superclass;
  typedef SmartPointer< Self >                           Pointer;
  typedef SmartPointer< const Self >                     ConstPointer;

  /** Image related typedefs. */
  typedef typename TInputImage::Pointer InputImagePointer;

  typedef typename TInputImage::RegionType RegionType;
  typedef typename TInputImage::SizeType   SizeType;
  typedef typename TInputImage::IndexType  IndexType;
  typedef typename TInputImage::PixelType  PixelType;

  /** Smart Pointer type to a DataObject. */
  typedef typename DataObject::Pointer DataObjectPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(MaximumGradientMagnitudeImageFilter, ImageToImageFilter);

  /** Extract some information from the image types.  Dimensionality
   * of the two images is assumed to be the same. */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TInputImage::ImageDimension);

  /** Image typedef support. */
  typedef TInputImage InputImageType;

  /** Type of DataObjects used for scalar outputs */
  typedef SimpleDataObjectDecorator< double > DoubleObjectType;

  /** Return the computed Maximum. */
  double GetMaximum() const
  { return this->GetMaximumOutput()->Get(); }
  DoubleObjectType * GetMaximumOutput();

  const DoubleObjectType * GetMaximumOutput() const;

  /** Make a DataObject of the correct type to be used as the specified
   * output. */
  typedef ProcessObject::DataObjectPointerArraySizeType DataObjectPointerArraySizeType;
  using Superclass::MakeOutput;
  virtual DataObjectPointer MakeOutput(DataObjectPointerArraySizeType idx);

  /** Use the image spacing information in calculations. Use this option if you
   *  want derivatives in physical space. Default is UseImageSpacingOn. */
  void SetUseImageSpacingOn()
  { this->SetUseImageSpacing(true); }

  /** Ignore the image spacing. Use this option if you want derivatives in
      isotropic pixel space.  Default is UseImageSpacingOn. */
  void SetUseImageSpacingOff()
  { this->SetUseImageSpacing(false); }

  /** Set/Get whether or not the filter will use the spacing of the input
      image in its calculations */
  itkSetMacro(UseImageSpacing, bool);
  itkGetConstMacro(UseImageSpacing, bool);


#ifdef ITK_USE_CONCEPT_CHECKING
  // Begin concept checking
  itkConceptMacro( LessThanComparableCheck,
                   ( Concept::LessThanComparable< PixelType > ) );
  itkConceptMacro( GreaterThanComparableCheck,
                   ( Concept::GreaterThanComparable< PixelType > ) );
  itkConceptMacro( OStreamWritableCheck,
                   ( Concept::OStreamWritable< PixelType > ) );
  // End concept checking
#endif

protected:
  MaximumGradientMagnitudeImageFilter();
  virtual ~MaximumGradientMagnitudeImageFilter() {}
  void PrintSelf(std::ostream & os, Indent indent) const;

  /** Pass the input through unmodified. Do this by Grafting in the
    AllocateOutputs method. */
  void AllocateOutputs();

  /** Initialize some accumulators before the threads run. */
  void BeforeThreadedGenerateData();

  /** Do final mean and variance computation from data accumulated in threads.
    */
  void AfterThreadedGenerateData();

  /** Multi-thread version GenerateData. */
  void  ThreadedGenerateData(const RegionType &
                             outputRegionForThread,
                             ThreadIdType threadId);

  // Override since the filter needs all the data for the algorithm
  void GenerateInputRequestedRegion();

  // Override since the filter produces all of its output
  void EnlargeOutputRequestedRegion(DataObject *data);

private:
  MaximumGradientMagnitudeImageFilter(const Self &); //purposely not implemented
  void operator=(const Self &);            //purposely not implemented

  std::vector< double > m_ThreadMax;

  bool m_UseImageSpacing;
};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkMaximumGradientMagnitudeImageFilter.hxx"
#endif

#endif // ITKMAXIMUMGRADIENTMAGNITUDEIMAGEFILTER_H
