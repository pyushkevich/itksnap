#ifndef RLEImage_h
#define RLEImage_h

#include <utility> //std::pair
#include <vector>
#include <itkImageBase.h>
#include <itkImage.h>

/**
* It is best if pixel type and counter type have the same byte size
* (for memory alignment purposes).
* 
* Copied and adapted from itk::Image.
*/
template< typename TPixel, typename RunLengthCounterType = unsigned short >
class RLEImage : public itk::ImageBase < 3 >
{
public:
    /** Standard class typedefs */
    typedef RLEImage                        Self;
    typedef itk::ImageBase < 3 >            Superclass;
    typedef itk::SmartPointer< Self >       Pointer;
    typedef itk::SmartPointer< const Self > ConstPointer;
    typedef itk::WeakPointer< const Self >  ConstWeakPointer;

    /** Method for creation through the object factory. */
    itkNewMacro(Self);

    /** Run-time type information (and related methods). */
    itkTypeMacro(RLEImage, ImageBase);

    /** Pixel typedef support. Used to declare pixel type in filters
    * or other operations. */
    typedef TPixel PixelType;

    /** Typedef alias for PixelType */
    typedef TPixel ValueType;

    /** First element is count of repetitions,
    * second element is the pixel value. */
    typedef std::pair<RunLengthCounterType, PixelType> RLSegment;

    /** A Run-Length encoded line of pixels. */
    typedef std::vector<RLSegment> RLLine;

    /** Internal Pixel representation. Used to maintain a uniform API
    * with Image Adaptors and allow to keep a particular internal
    * representation of data while showing a different external
    * representation. */
    typedef RLLine InternalPixelType;

    //typedef PixelType IOPixelType;

    /** Dimension of the image.  This constant is used by functions that are
    * templated over image type (as opposed to being templated over pixel type
    * and dimension) when they need compile time access to the dimension of
    * the image. */
    itkStaticConstMacro(ImageDimension, unsigned int, 3);

    /** Index typedef support. An index is used to access pixel values. */
    typedef typename Superclass::IndexType      IndexType;
    typedef typename Superclass::IndexValueType IndexValueType;

    /** Offset typedef support. An offset is used to access pixel values. */
    typedef typename Superclass::OffsetType OffsetType;

    /** Size typedef support. A size is used to define region bounds. */
    typedef typename Superclass::SizeType      SizeType;
    typedef typename Superclass::SizeValueType SizeValueType;

    /** Direction typedef support. A matrix of direction cosines. */
    typedef typename Superclass::DirectionType DirectionType;

    /** Region typedef support. A region is used to specify a subset of an image.
    */
    typedef typename Superclass::RegionType RegionType;

    /** Spacing typedef support.  Spacing holds the size of a pixel.  The
    * spacing is the geometric distance between image samples. */
    typedef typename Superclass::SpacingType      SpacingType;
    typedef typename Superclass::SpacingValueType SpacingValueType;

    /** Origin typedef support.  The origin is the geometric coordinates
    * of the index (0,0). */
    typedef typename Superclass::PointType PointType;

    /** Offset typedef (relative position between indices) */
    typedef typename Superclass::OffsetValueType OffsetValueType;

    /**
    * example usage:
    * typedef typename ImageType::template Rebind< float >::Type OutputImageType;
    *
    */
    template <typename UPixelType, unsigned int UImageDimension = VImageDimension>
    struct Rebind
    {
        typedef itk::Image<UPixelType, UImageDimension>  Type;
    };


    /** Allocate the image memory. The size of the image must
    * already be set, e.g. by calling SetRegions().
    * Pixel values are initialized to zero. */
    virtual void Allocate();

    /** Restore the data object to its initial state. This means releasing
    * memory. */
    virtual void Initialize();

    /** Fill the image buffer with a value.  Be sure to call Allocate()
    * first. */
    void FillBuffer(const TPixel & value);

    /** \brief Set a pixel value.
    *
    * Allocate() needs to have been called first -- for efficiency,
    * this function does not check that the image has actually been
    * allocated yet. HORRIBLY SLOW! */
    void SetPixel(const IndexType & index, const TPixel & value);

    /** \brief Get a pixel (read only version). SLOW! */
    const TPixel & GetPixel(const IndexType & index) const;

    /** Get a reference to a pixel (e.g. for editing). SLOW! */
    TPixel & GetPixel(const IndexType & index);

    /** \brief Access a pixel. This version can be an lvalue. SLOW! */
    TPixel & operator[](const IndexType & index)
    {
        return this->GetPixel(index);
    }

    /** \brief Access a pixel. This version can only be an rvalue. SLOW! */
    const TPixel & operator[](const IndexType & index) const
    {
        return this->GetPixel(index);
    }

    virtual unsigned int GetNumberOfComponentsPerPixel() const;

    ///** Return a pointer to the beginning of the buffer.  This is used by
    //* the image iterator class. */
    //virtual TPixel * GetBufferPointer()
    //{
    //    return m_Buffer ? m_Buffer->GetBufferPointer() : ITK_NULLPTR;
    //}
    //virtual const TPixel * GetBufferPointer() const
    //{
    //    return m_Buffer ? m_Buffer->GetBufferPointer() : ITK_NULLPTR;
    //}

    /** Graft the data and information from one image to another. This
    * is a convenience method to setup a second image with all the meta
    * information of another image and use the same pixel
    * container. Note that this method is different than just using two
    * SmartPointers to the same image since separate DataObjects are
    * still maintained. This method is similar to
    * ImageSource::GraftOutput(). The implementation in ImageBase
    * simply calls CopyInformation() and copies the region ivars.
    * The implementation here refers to the superclass' implementation
    * and then copies over the pixel container. */
    //virtual void Graft(const DataObject *data);

    ///** Return the Pixel Accessor object */
    //AccessorType GetPixelAccessor(void)
    //{
    //    return AccessorType();
    //}

    ///** Return the Pixel Accesor object */
    //const AccessorType GetPixelAccessor(void) const
    //{
    //    return AccessorType();
    //}

    /** Return the NeighborhoodAccessor functor */
    //NeighborhoodAccessorFunctorType GetNeighborhoodAccessor()
    //{
    //    return NeighborhoodAccessorFunctorType();
    //}

    /** Return the NeighborhoodAccessor functor */
    //const NeighborhoodAccessorFunctorType GetNeighborhoodAccessor() const
    //{
    //    return NeighborhoodAccessorFunctorType();
    //}


    ///** Container used to store pixels in the image. */
    //typedef itk::ImportImageContainer<itk::SizeValueType, RLLine> PixelContainer;

    ///** A pointer to the pixel container. */
    //typedef typename PixelContainer::Pointer      PixelContainerPointer;
    //typedef typename PixelContainer::ConstPointer PixelContainerConstPointer;

    ///** Set the container to use. Note that this does not cause the
    //* DataObject to be modified. */
    //void SetPixelContainer(PixelContainer *container);

    ///** Return a pointer to the container. */
    //PixelContainer * GetPixelContainer()
    //{
    //    return m_Buffer.GetPointer();
    //}
    //const PixelContainer * GetPixelContainer() const
    //{
    //    return m_Buffer.GetPointer();
    //}

    /** Construct this RLEImage from a regular itk::Image. */
    void fromITKImage(typename itk::Image<TPixel, 3>::Pointer image);
    
    /** Convert this RLEImage to a regular itk::Image. */
    typename itk::Image<TPixel, 3>::Pointer toITKImage();

protected:
    RLEImage();
    void PrintSelf(std::ostream & os, itk::Indent indent) const;

    virtual ~RLEImage() {}

    /** Compute helper matrices used to transform Index coordinates to
    * PhysicalPoint coordinates and back. This method is virtual and will be
    * overloaded in derived classes in order to provide backward compatibility
    * behavior in classes that did not used to take image orientation into
    * account.  */
    virtual void ComputeIndexToPhysicalPointMatrices();


private:
    RLEImage(const Self &);          //purposely not implemented
    void operator=(const Self &); //purposely not implemented

    /** Memory for the current buffer. */
    std::vector<std::vector<RLLine> > myBuffer;
};


#ifndef ITK_MANUAL_INSTANTIATION
#include "RLEImage.txx"
#endif

#endif //RLEImage_h