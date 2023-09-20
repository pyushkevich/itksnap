#ifndef ITKRANDOMFOREST_H
#define ITKRANDOMFOREST_H

#include "itkImageToImageFilter.h"
#include "RFTrain.h"
#include "ImageCollectionConstIteratorWithIndex.h"
#include "itkVectorImage.h"
#include "Library/classifier.h"
#include "Library/classification.h"
#include "RandomForestClassifier.h"


namespace itk
{


typedef itk::Image<double,3> ProbabilityType;

template< class ImageScalarType, class ImageVectorType, class TLabelImage>
class RandomForest : public ImageToImageFilter< TLabelImage, ProbabilityType >
{
public:
    /** Standard class typedefs. */
    typedef RandomForest            Self;
    typedef ImageToImageFilter< TLabelImage, ProbabilityType > Superclass;
    typedef SmartPointer< Self >                 Pointer;

    /** Method for creation through the object factory. */
    itkNewMacro(Self);

    /** Run-time type information (and related methods). */
    itkTypeMacro(RandomForest, ImageToImageFilter);

    /** The mask to be inpainted. White pixels will be inpainted, black pixels will be passed through to the output.*/
    void SetLabelMap(const TLabelImage* mask);

    typedef typename TLabelImage::PixelType LabelPixelType;
    typedef typename ImageScalarType::PixelType InputPixelType;
    typedef RandomForestClassifier<InputPixelType, LabelPixelType,3> RFClassifierType;
    typedef typename TLabelImage::IndexValueType LabelIndexType;

    void SetSegmentationIndices(std::set<LabelIndexType> SegmentationIndices)
    {
        m_SegmentationIndices = SegmentationIndices;
    }

    void SetBoundingBox(typename TLabelImage::RegionType bbox){
        m_boundingbox = bbox;
    }

    void SetIntermediateSlicesOnly(bool flag)
    {
        m_intermediateslices = flag;
    }

    void SetSlicingAxis(int SlicingAxis){
        m_slicingaxis = SlicingAxis;
    }

    /** Add a scalar input image */
    void AddScalarImage(ImageScalarType *image);

    /** Add a vector (multi-component) input image */
    void AddVectorImage(ImageVectorType *image);

protected:
    RandomForest();
    ~RandomForest(){}

    typename TLabelImage::Pointer GetLabelMap();

//    DataObject::Pointer GetIntensityImage();

    DataObject::Pointer MakeOutput(unsigned int idx);

    /** Does the real work. */
    void GenerateData() override;

private:

    RandomForest(const Self &); //purposely not implemented
    void operator=(const Self &);  //purposely not implemented
    std::set<LabelIndexType> m_SegmentationIndices;
    typename TLabelImage::RegionType m_boundingbox;
    bool m_intermediateslices;
    int m_slicingaxis;

};
} //namespace ITK


#ifndef ITK_MANUAL_INSTANTIATION
#include "itkRandomForest.txx"
#endif


#endif // itkRandomForest_h
