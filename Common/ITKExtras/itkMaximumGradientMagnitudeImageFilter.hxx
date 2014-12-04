#ifndef ITKMAXIMUMGRADIENTMAGNITUDEIMAGEFILTER_HXX
#define ITKMAXIMUMGRADIENTMAGNITUDEIMAGEFILTER_HXX

#include "itkMaximumGradientMagnitudeImageFilter.h"

#include "itkConstNeighborhoodIterator.h"
#include "itkNeighborhoodInnerProduct.h"
#include "itkImageRegionIterator.h"
#include "itkDerivativeOperator.h"
#include "itkNeighborhoodAlgorithm.h"
#include "itkProgressReporter.h"


#include <vector>

namespace itk
{
template< typename TInputImage >
MaximumGradientMagnitudeImageFilter< TInputImage >
::MaximumGradientMagnitudeImageFilter()
{
  this->SetNumberOfRequiredOutputs(2);
  // first output is a copy of the image, DataObject created by
  // superclass
  //
  // allocate the data objects for the remaining outputs which are
  // just decorators around floating point types

  typename DoubleObjectType::Pointer output =
      static_cast<DoubleObjectType *>(this->MakeOutput(1).GetPointer());
  this->ProcessObject::SetNthOutput(1, output.GetPointer());

  this->GetMaximumOutput()->Set(0.0);

  m_UseImageSpacing = true;
}

template< typename TInputImage >
DataObject::Pointer
MaximumGradientMagnitudeImageFilter< TInputImage >
::MakeOutput(DataObjectPointerArraySizeType output)
{
  switch ( output )
    {
    case 0:
      return TInputImage::New().GetPointer();
      break;
    case 1:
      return DoubleObjectType::New().GetPointer();
      break;
    default:
      // might as well make an image
      return TInputImage::New().GetPointer();
      break;
    }
}


template< typename TInputImage >
typename MaximumGradientMagnitudeImageFilter< TInputImage >::DoubleObjectType *
MaximumGradientMagnitudeImageFilter< TInputImage >
::GetMaximumOutput()
{
  return static_cast< DoubleObjectType * >( this->ProcessObject::GetOutput(1) );
}

template< typename TInputImage >
const typename MaximumGradientMagnitudeImageFilter< TInputImage >::DoubleObjectType *
MaximumGradientMagnitudeImageFilter< TInputImage >
::GetMaximumOutput() const
{
  return static_cast< const DoubleObjectType * >( this->ProcessObject::GetOutput(1) );
}

template< typename TInputImage >
void
MaximumGradientMagnitudeImageFilter< TInputImage >
::GenerateInputRequestedRegion()
{
  Superclass::GenerateInputRequestedRegion();
  if ( this->GetInput() )
    {
    InputImagePointer image =
      const_cast< typename Superclass::InputImageType * >( this->GetInput() );
    image->SetRequestedRegionToLargestPossibleRegion();
    }
}

template< typename TInputImage >
void
MaximumGradientMagnitudeImageFilter< TInputImage >
::EnlargeOutputRequestedRegion(DataObject *data)
{
  Superclass::EnlargeOutputRequestedRegion(data);
  data->SetRequestedRegionToLargestPossibleRegion();
}

template< typename TInputImage >
void
MaximumGradientMagnitudeImageFilter< TInputImage >
::AllocateOutputs()
{
  // Pass the input through as the output
  InputImagePointer image =
    const_cast< TInputImage * >( this->GetInput() );

  this->GraftOutput(image);

  // Nothing that needs to be allocated for the remaining outputs
}

template< typename TInputImage >
void
MaximumGradientMagnitudeImageFilter< TInputImage >
::BeforeThreadedGenerateData()
{
  ThreadIdType numberOfThreads = this->GetNumberOfThreads();

  // Create the thread temporaries
  m_ThreadMax = std::vector< double >(numberOfThreads, 0.0);
}

template< typename TInputImage >
void
MaximumGradientMagnitudeImageFilter< TInputImage >
::AfterThreadedGenerateData()
{
  ThreadIdType i;
  ThreadIdType numberOfThreads = this->GetNumberOfThreads();


  // Find the min/max over all threads
  double maximum = 0.0;
  for ( i = 0; i < numberOfThreads; i++ )
    {
    if ( m_ThreadMax[i] > maximum )
      {
      maximum = m_ThreadMax[i];
      }
    }

  // Set the outputs
  this->GetMaximumOutput()->Set(maximum);
}

template< typename TInputImage >
void
MaximumGradientMagnitudeImageFilter< TInputImage >
::ThreadedGenerateData(const RegionType & outputRegionForThread,
                       ThreadIdType threadId)
{
  if ( outputRegionForThread.GetNumberOfPixels() == 0 )
    return;

  typedef float RealType;

  // Local maximum
  double localMax = 0.0;

  unsigned int i;

  ZeroFluxNeumannBoundaryCondition< TInputImage > nbc;

  ConstNeighborhoodIterator< TInputImage > nit;
  ConstNeighborhoodIterator< TInputImage > bit;

  NeighborhoodInnerProduct< TInputImage, RealType > SIP;

  // Allocate output
  typename  InputImageType::ConstPointer input  = this->GetInput();

  // Set up operators
  DerivativeOperator< RealType, ImageDimension > op[ImageDimension];

  for ( i = 0; i < ImageDimension; i++ )
    {
    op[i].SetDirection(0);
    op[i].SetOrder(1);
    op[i].CreateDirectional();

    if ( m_UseImageSpacing == true )
      {
      if ( this->GetInput()->GetSpacing()[i] == 0.0 )
        {
        itkExceptionMacro(<< "Image spacing cannot be zero.");
        }
      else
        {
        op[i].ScaleCoefficients(1.0 / this->GetInput()->GetSpacing()[i]);
        }
      }
    }

  // Calculate iterator radius
  Size< ImageDimension > radius;
  for ( i = 0; i < ImageDimension; ++i )
    {
    radius[i]  = op[0].GetRadius()[0];
    }

  // Find the data-set boundary "faces"
  typename NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< TInputImage >::
  FaceListType faceList;
  NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< TInputImage > bC;
  faceList = bC(input, outputRegionForThread, radius);

  typename NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< TInputImage >::
  FaceListType::iterator fit;
  fit = faceList.begin();

  // support progress methods/callbacks
  ProgressReporter progress( this, threadId, outputRegionForThread.GetNumberOfPixels() );

  // Process non-boundary face
  nit = ConstNeighborhoodIterator< TInputImage >(radius, input, *fit);

  std::slice          x_slice[ImageDimension];
  const SizeValueType center = nit.Size() / 2;
  for ( i = 0; i < ImageDimension; ++i )
    {
    x_slice[i] = std::slice( center - nit.GetStride(i) * radius[i],
                             op[i].GetSize()[0], nit.GetStride(i) );
    }

  // Process each of the boundary faces.  These are N-d regions which border
  // the edge of the buffer.
  for ( fit = faceList.begin(); fit != faceList.end(); ++fit )
    {
    bit = ConstNeighborhoodIterator< InputImageType >(radius,
                                                      input, *fit);
    bit.OverrideBoundaryCondition(&nbc);
    bit.GoToBegin();

    while ( !bit.IsAtEnd() )
      {
      RealType a = NumericTraits< RealType >::Zero;
      for ( i = 0; i < ImageDimension; ++i )
        {
        const RealType g = SIP(x_slice[i], bit, op[i]);
        a += g * g;
        }

      if(localMax < a)
        localMax = a;

      ++bit;
      progress.CompletedPixel();
      }
    }

  m_ThreadMax[threadId] = vcl_sqrt(localMax);
}

template< typename TImage >
void
MaximumGradientMagnitudeImageFilter< TImage >
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Maximum: "
     << static_cast< typename NumericTraits< PixelType >::PrintType >( this->GetMaximum() )
     << std::endl;
}
} // end namespace itk
#endif
