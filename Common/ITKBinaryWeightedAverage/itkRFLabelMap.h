#ifndef ITKRFLABELMAP_H
#define ITKRFLABELMAP_H

#include "itkImageToImageFilter.h"
#include "itkImage.h"
#include "itkBinaryBallStructuringElement.h"
#include "itkBinaryDilateImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkAddImageFilter.h"

namespace itk
{
template< class TInputImage>
class RFLabelMap : public ImageToImageFilter< TInputImage,TInputImage >
{
public:
    /** Standard class typedefs. */
    typedef RFLabelMap            Self;
    typedef ImageToImageFilter< TInputImage, TInputImage > Superclass;
    typedef SmartPointer< Self >                 Pointer;

    /** Method for creation through the object factory. */
    itkNewMacro(Self)

    /** Run-time type information (and related methods). */
    itkTypeMacro(RFLabelMap, ImageToImageFilter)

    void SetSlicingAxis(int SlicingAxis)
    {
        m_SlicingAxis = SlicingAxis;
    }

    typedef typename TInputImage::PixelType TPixel;
    typedef itk::BinaryBallStructuringElement< TPixel,3 > StructuringElementType;
    typedef itk::BinaryDilateImageFilter <TInputImage, TInputImage, StructuringElementType> BinaryDilateImageFilterType;
    typedef itk::AddImageFilter < TInputImage, TInputImage, TInputImage> AddImageFilterType;

protected:
    RFLabelMap(){}
    ~RFLabelMap(){}

    /** Does the real work. */
    void GenerateData() override;

private:

    RFLabelMap(const Self &); //purposely not implemented
    void operator=(const Self &);  //purposely not implemented
    int m_SlicingAxis;

};
} //namespace ITK


#ifndef ITK_MANUAL_INSTANTIATION
#include "itkRFLabelMap.txx"
#endif


#endif // itkRFLabelMap_h
