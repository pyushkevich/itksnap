//#ifndef ITKBWAANDRFINTERPOLATION_H
//#define ITKBWAANDRFINTERPOLATION_H

#include "itkBWAfilter.h"

#include "itkRFLabelMap.h"
#include "itkRandomForest.h"


#include "itkAddImageFilter.h"
#include "itkMultiplyImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkImageToImageFilter.h"
#include "itksys/hash_map.hxx"
#include "itkImage.h"

namespace itk
{


typedef itk::Image<double,3>                                  ProbabilityType;

template< class ImageScalarType, class ImageVectorType, class TLabelImage>
class CombineBWAandRFFilter : public ImageToImageFilter< TLabelImage, TLabelImage >
{
public:
    /** Standard class typedefs. */
    typedef CombineBWAandRFFilter             Self;
    typedef ImageToImageFilter< TLabelImage, TLabelImage > Superclass;
    typedef SmartPointer< Self >                 Pointer;

    /** Method for creation through the object factory. */
    itkNewMacro(Self);

    /** Run-time type information (and related methods). */
    itkTypeMacro(CombineBWAandRFFilter , ImageToImageFilter);

    void SetSegmentationImage(const TLabelImage* mask);

    void SetIntermediateSlicesOnly(bool flag)
    {
        m_intermediateslices = flag;
    }

    /** Interpolate only this label. Interpolates all labels if set to 0 (default). */
    itkSetMacro( Label, typename TLabelImage::PixelType );

    /** Interpolate using Contour information only - no random forest*/
    itkSetMacro( ContourInformationOnly, bool);

    /** Allows interpolation to overwrite existig segmentation labels (not including the interpolated label)*/
    itkSetMacro( OverwriteSegmentation, bool);

    /** Which label is interpolated. 0 means all labels (default). */
    itkGetMacro( Label, typename TLabelImage::PixelType );

    /** Which label is interpolated. 0 means all labels (default). */
    itkGetConstMacro( Label, typename TLabelImage::PixelType );

    /** User specified axis of interpolation */
    itkSetMacro( UserAxis, int );

    /** User specified axis of interpolation */
    itkGetMacro( UserAxis, int );

    /** If there is a pixel whose all 4-way neighbors belong the the same label
    except along one axis, and along that axis its neighbors are 0 (background),
    then that axis should be interpolated along. Interpolation is possible
    along more than one axis. Updates LabeledSliceIndices.*/
    void DetermineSliceOrientations();

    /** An std::set of slice indices which need to be interpolated. */
    typedef std::set< typename TLabelImage::IndexValueType > SliceSetType;

    TLabelImage* GetInterpolation();
    ProbabilityType * GetProbabilityMap();

    /** each label gets a set of slices in which it is present */
    typedef itksys::hash_map< typename TLabelImage::PixelType, SliceSetType > LabeledSlicesType;
    typedef std::vector< LabeledSlicesType >                             SliceIndicesType;
    typedef Image< typename TLabelImage::PixelType, TLabelImage::ImageDimension - 1 > SliceType;

    /** Slice indices between which interpolation is done. */
    SliceIndicesType GetLabeledSliceIndices()
    {
        return m_LabeledSlices;
    }

    /** Add a scalar input image */
    void AddScalarImage(ImageScalarType *image);

    /** Add a vector (multi-component) input image */
    void AddVectorImage(ImageVectorType *image);

    typedef itk::BinaryWeightedAveragingFilter< TLabelImage, ProbabilityType >  BWAFilterType;
    typedef itk::RFLabelMap<TLabelImage> RFLabelFilterType;
    typedef itk::BinaryThresholdImageFilter<ProbabilityType, TLabelImage> BinaryThresholdFilterType;
    typedef itk::BinaryThresholdImageFilter< TLabelImage, TLabelImage > BinarizerType;
    typedef itk::MultiplyImageFilter< ProbabilityType > MultiplyImageFilterType;
    typedef itk::AddImageFilter<ProbabilityType, ProbabilityType> AddImageFilterType;
    typedef itk::RandomForest< ImageScalarType,ImageVectorType,TLabelImage> RandomForestClassifierType;

protected:
    CombineBWAandRFFilter ();
    ~CombineBWAandRFFilter (){}

    typename TLabelImage::PixelType         m_Label;
    SliceIndicesType                        m_LabeledSlices; // one for each axis

    typedef itksys::hash_map< typename TLabelImage::PixelType, typename TLabelImage::RegionType > BoundingBoxesType;
    BoundingBoxesType                       m_BoundingBoxes; // bounding box for each label

    /** Calculates a bounding box of non-zero pixels. */
    typename SliceType::RegionType
    BoundingBox( itk::SmartPointer< SliceType > image );

    /** Expands a region to incorporate the provided index.
    *   Assumes both a valid region and a valid index.
    *   It can be invoked with 2D or 3D region, hence the additional template parameter. */
    template< typename T2 >
    void
    ExpandRegion( typename T2::RegionType& region, const typename T2::IndexType& index );

    typename TLabelImage::ConstPointer      GetSegmentationImage();

    DataObject::Pointer MakeOutput(unsigned int idx);

    /** Does the real work. */
    virtual void                            GenerateData();
    bool                                    m_ContourInformationOnly;

    /* Contains the functions to perform the interpolation*/
    void InterpolateLabel(typename TLabelImage::PixelType label, std::vector<int> SegIndices,typename TLabelImage::RegionType bbox );

private:

    CombineBWAandRFFilter (const Self &); //purposely not implemented
    void operator=(const Self &);  //purposely not implemented
    std::vector<int> m_SegmentationIndices;
    int         m_SlicingAxis;
    int         m_UserAxis;
    bool        m_intermediateslices;
    bool        m_OverwriteSegmentation;

};
} //namespace ITK

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkBWAandRFinterpolation.txx"
#endif

#endif // itkBWAandRFinterpolation_h
