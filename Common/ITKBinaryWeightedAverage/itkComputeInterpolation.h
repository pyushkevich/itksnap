#ifndef ITKCOMPUTEINTERPOLATION_H
#define ITKCOMPUTEINTERPOLATION_H

#include "itkImageToImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkApproximateSignedDistanceMapImageFilter.h"
#include "itkAddImageFilter.h"
#include "itkImageFileWriter.h"
#include "itkMultiplyImageFilter.h"

namespace itk
{
template< class TInputImage, class TOutputImage>
class ComputeInterpolation : public ImageToImageFilter< TInputImage, TOutputImage >
{
public:
    /** Standard class typedefs. */
    typedef ComputeInterpolation            Self;
    typedef ImageToImageFilter< TInputImage, TOutputImage > Superclass;
    typedef SmartPointer< Self >                 Pointer;

    /** Method for creation through the object factory. */
    itkNewMacro(Self);

    /** Run-time type information (and related methods). */
    itkTypeMacro(ComputeInterpolation, ImageToImageFilter);

    /** The image to be inpainted in regions where the mask is white.*/
    void SetInputImage1(const TInputImage* image);

    /** The mask to be inpainted. White pixels will be inpainted, black pixels will be passed through to the output.*/
    void SetInputImage2(const TInputImage* mask);

    TOutputImage* GetInterpolation();
    TInputImage* GetProbabilityMap();

    typedef itk::BinaryThresholdImageFilter<TInputImage, TOutputImage>BinaryThresholdImageFilterType;
    typedef itk::MultiplyImageFilter<TInputImage, TInputImage, TInputImage> MultiplyImageFilterType;
    typedef itk::AddImageFilter<TInputImage, TInputImage,TInputImage> AddImageFilterType;
    typedef itk::ImageFileWriter< TInputImage > TwoDWriterType;

    void SetConstant(double t)
    {
        t1 = t;
    }

protected:
    ComputeInterpolation();
    ~ComputeInterpolation(){}

    typename TInputImage::ConstPointer GetInputImage1();
    typename TInputImage::ConstPointer GetInputImage2();

    DataObject::Pointer MakeOutput(unsigned int idx);

    /** Does the real work. */
    void GenerateData() override;

private:

    ComputeInterpolation(const Self &); //purposely not implemented
    void operator=(const Self &);  //purposely not implemented
    double t1;
};
} //namespace ITK


#ifndef ITK_MANUAL_INSTANTIATION
#include "itkComputeInterpolation.txx"
#endif


#endif // itkComputeInterpolation_h
