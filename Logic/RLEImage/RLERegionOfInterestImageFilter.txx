#ifndef RLERegionOfInterestImageFilter_txx
#define RLERegionOfInterestImageFilter_txx

#include "itkRegionOfInterestImageFilter.h"
#include "itkImageAlgorithm.h"
#include "itkObjectFactory.h"
#include "itkProgressReporter.h"
#include "itkImage.h"

namespace itk
{
template< typename TPixel, unsigned int VImageDimension, typename CounterType >
void RegionOfInterestImageFilter<RLEImage<TPixel, VImageDimension, CounterType>,
    RLEImage<TPixel, VImageDimension, CounterType> >
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "RegionOfInterest: " << m_RegionOfInterest << std::endl;
}

template< typename TPixel, unsigned int VImageDimension, typename CounterType >
void RegionOfInterestImageFilter<RLEImage<TPixel, VImageDimension, CounterType>,
    RLEImage<TPixel, VImageDimension, CounterType> >
::GenerateInputRequestedRegion()
{
  // call the superclass' implementation of this method
  Superclass::GenerateInputRequestedRegion();

  // get pointer to the input
  typename Superclass::InputImagePointer inputPtr =
    const_cast< RLEImageType * >( this->GetInput() );

  if ( inputPtr )
    {
    // request the region of interest
    inputPtr->SetRequestedRegion(m_RegionOfInterest);
    }
}

template< typename TPixel, unsigned int VImageDimension, typename CounterType >
void RegionOfInterestImageFilter<RLEImage<TPixel, VImageDimension, CounterType>,
    RLEImage<TPixel, VImageDimension, CounterType> >
::EnlargeOutputRequestedRegion(DataObject *output)
{
  // call the superclass' implementation of this method
  Superclass::EnlargeOutputRequestedRegion(output);

  // generate everything in the region of interest
  output->SetRequestedRegionToLargestPossibleRegion();
}

/**
 * RegionOfInterestImageFilter can produce an image which is a different size
 * than its input image.  As such, RegionOfInterestImageFilter needs to provide an
 * implementation for GenerateOutputInformation() in order to inform
 * the pipeline execution model.  The original documentation of this
 * method is below.
 *
 * \sa ProcessObject::GenerateOutputInformaton()
 */
template< typename TPixel, unsigned int VImageDimension, typename CounterType >
void RegionOfInterestImageFilter<RLEImage<TPixel, VImageDimension, CounterType>,
    RLEImage<TPixel, VImageDimension, CounterType> >
::GenerateOutputInformation()
{
  // do not call the superclass' implementation of this method since
  // this filter allows the input the output to be of different dimensions

  // get pointers to the input and output
  typename Superclass::OutputImagePointer outputPtr = this->GetOutput();
  typename Superclass::InputImageConstPointer inputPtr  = this->GetInput();

  if ( !outputPtr || !inputPtr )
    {
    return;
    }

  // Set the output image size to the same value as the region of interest.
  RegionType region;
  IndexType  start;
  start.Fill(0);

  region.SetSize( m_RegionOfInterest.GetSize() );
  region.SetIndex(start);

  // Copy Information without modification.
  outputPtr->CopyInformation(inputPtr);

  // Adjust output region
  outputPtr->SetLargestPossibleRegion(region);

  // Correct origin of the extracted region.
  IndexType roiStart( m_RegionOfInterest.GetIndex() );
  typename Superclass::OutputImageType::PointType outputOrigin;
  inputPtr->TransformIndexToPhysicalPoint(roiStart, outputOrigin);
  outputPtr->SetOrigin(outputOrigin);
}

/**
   * RegionOfInterestImageFilter can be implemented as a multithreaded filter.
   * Therefore, this implementation provides a ThreadedGenerateData()
   * routine which is called for each processing thread. The output
   * image data is allocated automatically by the superclass prior to
   * calling ThreadedGenerateData().  ThreadedGenerateData can only
   * write to the portion of the output image specified by the
   * parameter "outputRegionForThread"
   *
   * \sa ImageToImageFilter::ThreadedGenerateData(),
   *     ImageToImageFilter::GenerateData()
   */
template< typename TPixel, unsigned int VImageDimension, typename CounterType >
void RegionOfInterestImageFilter<RLEImage<TPixel, VImageDimension, CounterType>,
    RLEImage<TPixel, VImageDimension, CounterType> >
::ThreadedGenerateData(const RegionType & outputRegionForThread,
                       ThreadIdType threadId)
{
  // Get the input and output pointers
  const RLEImageType *in = this->GetInput();
  RLEImageType *out = this->GetOutput();

  // Define the portion of the input to walk for this thread
  InputImageRegionType inputRegionForThread;
  inputRegionForThread.SetSize(outputRegionForThread.GetSize());

  IndexType start, end;
  IndexType roiStart( m_RegionOfInterest.GetIndex() );
  IndexType threadStart( outputRegionForThread.GetIndex() );
  for (unsigned int i = 0; i < ImageDimension; i++)
  {
      start[i] = roiStart[i] + threadStart[i];
      end[i] = roiStart[i] + threadStart[i] + outputRegionForThread.GetSize(i);
  }
  inputRegionForThread.SetIndex(start);
  bool copyLines = (in->GetLargestPossibleRegion().GetSize(0) == outputRegionForThread.GetSize(0));
  
  for (SizeValueType z = 0; z < outputRegionForThread.GetSize(2); z++)
  {
      for (SizeValueType y = 0; y < outputRegionForThread.GetSize(1); y++)
      {
          if (copyLines)
              out->myBuffer[z][y] = in->myBuffer[z + start[2]][y + start[1]];
          else //determine begin and end iterator and copy range
          {
              typename RLEImageType::RLLine &oLine = out->myBuffer[z + threadStart[2]][y + threadStart[1]];
              oLine.clear();
              const typename RLEImageType::RLLine &iLine = in->myBuffer[z + start[2]][y + start[1]];
              CounterType t = 0;
              SizeValueType x = 0;
              //find start
              for (; x < iLine.size(); x++)
              {
                  t += iLine[x].first;
                  if (t > start[0])
                      break;
              }
              assert(x < iLine.size());

              SizeValueType begin = x;
              if (t >= end[0]) //both begin and end are in this segment
              {
                  oLine.push_back(
                      typename RLEImageType::RLSegment(end[0] - start[0], iLine[x].second));
                  continue; //next line
              }
              else if (t - start[0] < iLine[x].first) //not the first pixel in segment
              {
                  oLine.push_back(typename RLEImageType::RLSegment(t - start[0], iLine[x].second));
                  begin++; //start copying from next segment
              }

              if (t < end[0])
                  for (x++; x < iLine.size(); x++)
                  {
                      t += iLine[x].first;
                      if (t >= end[0])
                          break;
                  }
              if (t == end[0])
                  oLine.insert(oLine.end(), iLine.begin() + begin, iLine.begin() + x + 1);
              else //we need to take special care of the last segment
              {
                  oLine.insert(oLine.end(), iLine.begin() + begin, iLine.begin() + x);
                  oLine.push_back(
                      typename RLEImageType::RLSegment(end[0] + iLine[x].first - t, iLine[x].second));
              }
          }
      }
  }
}
} // end namespace itk

#endif //RLERegionOfInterestImageFilter_txx
