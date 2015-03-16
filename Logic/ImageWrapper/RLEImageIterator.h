#ifndef RLEImageIterator_h
#define RLEImageIterator_h

#include "RLEImageConstIterator.h"
#include "itkImageIteratorWithIndex.h"

namespace itk
{
/**
 * \class ImageIterator
 * \brief A multi-dimensional iterator templated over image type.
 *
 * This is a base class of ImageConstIterator that adds write-access
 * functionality.  Please see ImageConstIterator for more information.
 *
 */

template< typename TPixel, typename RunLengthCounterType>
class ImageIterator<RLEImage<TPixel, RunLengthCounterType> >
    :public ImageConstIterator<RLEImage<TPixel, RunLengthCounterType> >
{
public:
  /** Standard class typedefs. */
  typedef ImageIterator Self;

  /** Dimension of the image the iterator walks.  This constant is needed so
   * functions that are templated over image iterator type (as opposed to
   * being templated over pixel type and dimension) can have compile time
   * access to the dimension of the image that the iterator walks. */
  itkStaticConstMacro(ImageIteratorDimension, unsigned int, 3);

  /** Define the superclass */
  typedef ImageConstIterator< RLEImage<TPixel, RunLengthCounterType> > Superclass;

  /** Inherit types from the superclass */
  typedef typename Superclass::IndexType             IndexType;
  typedef typename Superclass::SizeType              SizeType;
  typedef typename Superclass::OffsetType            OffsetType;
  typedef typename Superclass::RegionType            RegionType;
  typedef typename Superclass::ImageType             ImageType;
  typedef typename Superclass::InternalPixelType     InternalPixelType;
  typedef typename Superclass::PixelType             PixelType;

  /** Default Constructor. Need to provide a default constructor since we
   * provide a copy constructor. */
  ImageIterator(){}

  /** Default Destructor */
  ~ImageIterator() {}

  /** Copy Constructor. The copy constructor is provided to make sure the
   * handle to the image is properly reference counted. */
  ImageIterator(const Self & it) :
      ImageConstIterator<ImageType>(it) {}

  /** Constructor establishes an iterator to walk a particular image and a
   * particular region of that image. */
  ImageIterator(ImageType *ptr, const RegionType & region):
      ImageConstIterator<ImageType>(ptr, region){}

  /** operator= is provided to make sure the handle to the image is properly
   * reference counted. */
  Self & operator=(const Self & it)
  {
      this->ImageConstIterator<ImageType>::operator=(it);
      return *this;
  }

  /** Set the pixel value */
  void Set(const PixelType & value) const
  {
      const_cast<ImageType *>(m_Image.GetPointer())->
          SetPixel(*const_cast<ImageType::RLLine *>(rlLine), segmentRemainder, realIndex, value);
  }

  ///** Return a reference to the pixel
  // * Setting this value would change value of the whole run-length segment.
  // * If we wanted to safely enable it,
  // * we would isolate this pixel into its own segment. */
  //PixelType & Value(void)
  //{
  //    return myBuffer[m_Index[2]][m_Index[1]][realIndex].second;
  //}

  /** Get the image that this iterator walks. */
  ImageType * GetImage() const
  {
    // const_cast is needed here because m_Image is declared as a const pointer
    // in the base class which is the ConstIterator.
    return const_cast< ImageType * >( this->m_Image.GetPointer() );
  }

protected:

  /** This constructor is declared protected in order to enforce
    const-correctness */
  ImageIterator(const ImageConstIterator< ImageType > & it) :
      ImageConstIterator<ImageType>(it) {}
  Self & operator=(const ImageConstIterator< ImageType > & it)
  {
      this->ImageConstIterator< ImageType>::operator=(it);
      return *this;
  }
};

template< typename TPixel, typename RunLengthCounterType>
class ImageIteratorWithIndex<RLEImage<TPixel, RunLengthCounterType> >
    :public ImageIterator < RLEImage<TPixel, RunLengthCounterType> >
{
    //just inherit constructors
public:
    /** Default Constructor. Need to provide a default constructor since we
    * provide a copy constructor. */
    ImageIteratorWithIndex() :ImageIterator< ImageType >(){ }


    /** Copy Constructor. The copy constructor is provided to make sure the
    * handle to the image is properly reference counted. */
    ImageIteratorWithIndex(const Self & it)
    {
        this->ImageIterator< ImageType >::operator=(it);
    }

    /** Constructor establishes an iterator to walk a particular image and a
    * particular region of that image. */
    ImageIteratorWithIndex(const ImageType *ptr, const RegionType & region)
        :ImageIterator< ImageType >(ptr, region) { }
}; //no additional implementation required
} // end namespace itk

#endif //RLEImageIterator_h
