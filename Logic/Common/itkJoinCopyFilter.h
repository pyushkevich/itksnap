#ifndef __itkJoinCopyFilter_h
#define __itkJoinCopyFilter_h
 
#include <itkInPlaceImageFilter.h>

namespace itk{
    /** \class JoinCopyFilter
     * \brief: sets DrawingColor in output image if input pixel equals SeedValue
     *
     * \ingroup ImageFilters
     */
    template<typename TInputImage1, typename TInputImage2, typename TOutputImage>
	class JoinCopyFilter:
    public InPlaceImageFilter<TInputImage2, TOutputImage>{
    public:
	/** Standard class typedefs. */
	typedef JoinCopyFilter                                Self;
	typedef InPlaceImageFilter<TInputImage2, TOutputImage>     Superclass;
	typedef SmartPointer<Self>                                 Pointer;
	typedef SmartPointer< const Self >                         ConstPointer;

	/** Method for creation through the object factory. */
	itkNewMacro(Self);
 
	/** Run-time type information (and related methods). */
	itkTypeMacro(JoinCopyFilter, InPlaceImageFilter);
  
	void SetJsrc(const TInputImage1 *image1);
	void SetJdst(const TInputImage2 *image2);

	/** Set/Get SeedIndex value */
	itkSetMacro(SeedIndex, typename TInputImage1::IndexType);
	itkGetConstMacro(SeedIndex, typename TInputImage1::IndexType);

	/** Set/Get DrawingColor value */
	itkSetMacro(DrawingColor, typename TOutputImage::ValueType);
	itkGetConstMacro(DrawingColor, typename TOutputImage::ValueType);

	/** Get UpdateFlag value */
	itkGetConstMacro(UpdateFlag, bool);

	/** Get/Set SeedActive value */
	itkSetMacro(SeedActive, bool);
	itkGetConstMacro(SeedActive, bool);

    protected:
	JoinCopyFilter();
	~JoinCopyFilter(){}
  
	bool m_SeedActive;
	typename TInputImage1::IndexType m_SeedIndex;
	typename TInputImage1::ValueType m_SeedValue;
	typename TOutputImage::ValueType m_DrawingColor;
	bool m_UpdateFlag;


	virtual void BeforeThreadedGenerateData();
	virtual void ThreadedGenerateData(const typename Superclass::OutputImageRegionType& outputRegionForThread, ThreadIdType threadId);
 
    private:
	JoinCopyFilter(const Self &); //purposely not implemented
	void operator=(const Self &);  //purposely not implemented
 
	};
    } //namespace ITK
 
 
#ifndef ITK_MANUAL_INSTANTIATION
#include "itkJoinCopyFilter.cxx"
#endif
 
#endif // __itkJoinCopyFilter_h

