#ifndef RLEImageConstIterator_h
#define RLEImageConstIterator_h

#include "itkImage.h"
#include "itkIndex.h"
#include "itkNumericTraits.h"
#include "RLEImage.h"
#include "itkImageConstIterator.h"
#include "itkImageConstIteratorWithIndex.h"

namespace itk
{
/** \class ImageConstIterator
 * \brief A multi-dimensional image iterator templated over image type.
 */

template< typename TPixel, typename RunLengthCounterType>
class ImageConstIterator<RLEImage<TPixel, RunLengthCounterType> >
{
public:
  /** Standard class typedefs. */
  typedef ImageConstIterator Self;

  /** Dimension of the image the iterator walks.  This constant is needed so
   * functions that are templated over image iterator type (as opposed to
   * being templated over pixel type and dimension) can have compile time
   * access to the dimension of the image that the iterator walks. */
  itkStaticConstMacro(ImageIteratorDimension, unsigned int, 3);

  /** Run-time type information (and related methods). */
  itkTypeMacroNoParent(ImageConstIterator);

  /** Image typedef support. */
  typedef RLEImage<TPixel, RunLengthCounterType> ImageType;

  /** Run-Length Line (we iterate along it). */
  typedef typename ImageType::RLLine RLLine;

  /** Buffer Type used. */
  typedef typename ImageType::BufferType BufferType;

  /** Index typedef support. */
  typedef typename ImageType::IndexType      IndexType;

  /** Size typedef support. */
  typedef typename ImageType::SizeType      SizeType;

  /** Offset typedef support. */
  typedef typename ImageType::OffsetType      OffsetType;

  /** Region typedef support. */
  typedef typename ImageType::RegionType RegionType;

  /** Internal Pixel Type */
  typedef typename ImageType::InternalPixelType InternalPixelType;

  /** External Pixel Type */
  typedef typename ImageType::PixelType PixelType;

  /** Default Constructor. Need to provide a default constructor since we
   * provide a copy constructor. */
  ImageConstIterator():
    m_Region()
  {
    m_Image = ITK_NULLPTR;
    m_Index.fill(0);
    m_BeginIndex.fill(0);
    m_EndIndex.fill(0);
    m_LineBegin = 0;
    m_LineEnd = 0;
    realIndex = 0;
    segmentRemainder = 0;
  }

  /** Default Destructor. */
  virtual ~ImageConstIterator() {}

  /** Copy Constructor. The copy constructor is provided to make sure the
   * handle to the image is properly reference counted. */
  ImageConstIterator(const Self & it)
      :myBuffer(it.GetImage()->GetBuffer())
  {
    rlLine = it.rlLine;
    m_Image = it.m_Image;     // copy the smart pointer
    m_Region = it.m_Region;
    m_Index = it.m_Index;
    
    m_LineBegin = it.m_LineBegin;
    m_LineEnd = it.m_LineEnd;

    realIndex = it.realIndex;
    segmentRemainder = it.segmentRemainder;
    m_BeginIndex = it.m_BeginIndex;
    m_EndIndex = it.m_EndIndex;
  }

  /** Constructor establishes an iterator to walk a particular image and a
   * particular region of that image. */
  ImageConstIterator(const ImageType *ptr, const RegionType & region)
      :myBuffer(ptr->GetBuffer())
  {
    m_Image = ptr;
    SetRegion(region);
  }

  /** operator= is provided to make sure the handle to the image is properly
   * reference counted. */
  Self & operator=(const Self & it)
  {
    if(this != &it)
      {
          myBuffer = it.myBuffer;
          rlLine = it.rlLine;
          m_Image = it.m_Image;     // copy the smart pointer
          m_Region = it.m_Region;
          m_Index = it.m_Index;

          m_LineBegin = it.m_LineBegin;
          m_LineEnd = it.m_LineEnd;

          realIndex = it.realIndex;
          segmentRemainder = it.segmentRemainder;
          m_BeginIndex = it.m_BeginIndex;
          m_EndIndex = it.m_EndIndex;
      }
    return *this;
  }

  /** Set the region of the image to iterate over. */
  virtual void SetRegion(const RegionType & region)
  {
    m_Region = region;

    if ( region.GetNumberOfPixels() > 0 ) // If region is non-empty
      {
      const RegionType & bufferedRegion = m_Image->GetBufferedRegion();
      itkAssertOrThrowMacro( ( bufferedRegion.IsInside(m_Region) ),
                             "Region " << m_Region << " is outside of buffered region " << bufferedRegion );
      }

    m_Index = m_Region.GetIndex();
    m_BeginIndex = m_Index;

    m_EndIndex[0] = m_BeginIndex[0] + m_Region.GetSize(0);
    m_EndIndex[1] = m_BeginIndex[1] + m_Region.GetSize(1);
    m_EndIndex[2] = m_BeginIndex[2] + m_Region.GetSize(2);
    SetIndex(m_Index); //sets realIndex, segmentRemainder, m_LineBegin and m_LineEnd
  }

  /** Get the dimension (size) of the index. */
  static unsigned int GetImageIteratorDimension()
  { return 3; }

  /** Comparison operator. Two iterators are the same if they "point to" the
   * same memory location */
  bool
  operator!=(const Self & it) const
  {
      // two iterators are the same if they "point to" the same memory location
      return &myBuffer[m_Index[2]][m_Index[1]][realIndex].second !=
          &it.myBuffer[it.m_Index[2]][it.m_Index[1]][it.realIndex].second;
  }

  /** Comparison operator. Two iterators are the same if they "point to" the
   * same memory location */
  bool
  operator==(const Self & it) const
  {
      // two iterators are the same if they "point to" the same memory location
      return &myBuffer[m_Index[2]][m_Index[1]][realIndex].second ==
          &it.myBuffer[it.m_Index[2]][it.m_Index[1]][it.realIndex].second;
  }

  /** Get the index. This provides a read only reference to the index. */
  const IndexType GetIndex() const
  { return m_Index; }

  /** Set the realIndex and segmentRemainder
   * No bounds checking is performed.
   * \sa GetIndex */
  virtual void SetIndex(const IndexType & ind)
  { 
      m_Index = ind;
      rlLine = &myBuffer[m_Index[2]][m_Index[1]];

      RunLengthCounterType t = 0;
      SizeValueType x = 0;

      for (; x < (*rlLine).size(); x++)
      {
          t += (*rlLine)[x].first;
          if (t > m_Index[0])
              break;
      }
      realIndex = x;
      segmentRemainder = t - m_Index[0];
  }

  /** Get the region that this iterator walks. ImageConstIterators know the
   * beginning and the end of the region of the image to iterate over. */
  const RegionType & GetRegion() const
  { return m_Region; }

  /** Get the image that this iterator walks. */
  const ImageType * GetImage() const
  { return m_Image.GetPointer(); }

  /** Get the pixel value */
  PixelType Get(void) const
  {
      return Value();
  }

  /** Return a const reference to the pixel
   * This method will provide the fastest access to pixel
   * data, but it will NOT support ImageAdaptors. */
  const PixelType & Value(void) const
  {
      return myBuffer[m_Index[2]][m_Index[1]][realIndex].second;
  }

  /** Move an iterator to the beginning of the region. "Begin" is
   * defined as the first pixel in the region. */
  void GoToBegin()
  {
      SetIndex(m_BeginIndex);
  }

  /** Move an iterator to the end of the region. "End" is defined as
   * one pixel past the last pixel of the region. */
  void GoToEnd()
  {
      m_Index = m_EndIndex;
      m_Index[0]--;
      m_Index[1]--;
      m_Index[2]--;
      SetIndex(m_Index); //first set to the last pixel so we have valid member variables
      m_Index.Fill(0);
      m_Index[2] = m_EndIndex[2];
  }

  /** Is the iterator at the beginning of the region? "Begin" is defined
   * as the first pixel in the region. */
  bool IsAtBegin(void) const
  {
      return (m_Index == m_BeginIndex);
  }

  /** Is the iterator at the end of the region? "End" is defined as one
   * pixel past the last pixel of the region. */
  bool IsAtEnd(void) const
  {
      IndexType ind;
      ind[0] = m_Region.GetIndex(0);
      ind[1] = m_Region.GetIndex(1);
      ind[2] = m_EndIndex[2];
      return (m_Index == ind);
  }

protected: //made protected so other iterators can access
  typename ImageType::ConstWeakPointer m_Image;

  RegionType m_Region; // region to iterate over
  IndexType m_Index;   // current index

  const RLLine * rlLine;

  mutable IndexValueType realIndex; // index into line's segment
  mutable IndexValueType segmentRemainder; // how many pixels remain in current segment

  IndexType m_BeginIndex; // index to first pixel in region
  IndexType m_EndIndex;   // index to one pixel past last pixel in region

  const BufferType & myBuffer;
};

template< typename TPixel, typename RunLengthCounterType>
class ImageConstIteratorWithIndex<RLEImage<TPixel, RunLengthCounterType> >
    :public ImageConstIterator < RLEImage<TPixel, RunLengthCounterType> >
{
    //just inherit constructors
public:
    /** Default Constructor. Need to provide a default constructor since we
    * provide a copy constructor. */
    ImageConstIteratorWithIndex() :ImageConstIterator< ImageType >(){ }


    /** Copy Constructor. The copy constructor is provided to make sure the
    * handle to the image is properly reference counted. */
    ImageConstIteratorWithIndex(const Self & it)
    {
        this->ImageConstIterator< ImageType >::operator=(it);
    }

    /** Constructor establishes an iterator to walk a particular image and a
    * particular region of that image. */
    ImageConstIteratorWithIndex(const ImageType *ptr, const RegionType & region)
        :ImageConstIterator< ImageType >(ptr, region) { }
}; //no additional implementation required

} // end namespace itk

#endif //RLEImageConstIterator_h
