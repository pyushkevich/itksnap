#ifndef ITKBWAANDRFINTERPOLATION_H
#define ITKBWAANDRFINTERPOLATION_H

#include "itkBWAfilter.h"
#include "itkRFLabelMap.h"
#include "itkRandomForest.h"

#include "itkAddImageFilter.h"
#include "itkMultiplyImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkImageToImageFilter.h"
#include <unordered_map>
#include "itkImage.h"

namespace itk
{

using ProbabilityType = itk::Image<double,3>;

template< class ImageScalarType, class ImageVectorType, class TLabelImage>
class CombineBWAandRFFilter : public ImageToImageFilter< TLabelImage, TLabelImage >
{
public:
    /** Standard class typedefs. */
    using Self = CombineBWAandRFFilter;
    using Superclass = ImageToImageFilter< TLabelImage, TLabelImage >;
    using Pointer = SmartPointer< Self >;

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
    using SliceSetType = std::set< typename TLabelImage::IndexValueType >;

    TLabelImage* GetInterpolation();
    ProbabilityType * GetProbabilityMap();

    /** each label gets a set of slices in which it is present */
    using LabeledSlicesType = std::unordered_map< typename TLabelImage::PixelType, SliceSetType >;
    using SliceIndicesType = std::vector< LabeledSlicesType >;
    using SliceType =  Image< typename TLabelImage::PixelType, TLabelImage::ImageDimension - 1 >;

    /** Slice indices between which interpolation is done. */
    SliceIndicesType GetLabeledSliceIndices()
    {
        return m_LabeledSlices;
    }

    /** Add a scalar input image */
    void AddScalarImage(const ImageScalarType *image);

    /** Add a vector (multi-component) input image */
    void AddVectorImage(const ImageVectorType *image);

    using  BWAFilterType = itk::BinaryWeightedAveragingFilter< TLabelImage, ProbabilityType >;
    using RFLabelFilterType = itk::RFLabelMap<TLabelImage>;
    using BinaryThresholdFilterType = itk::BinaryThresholdImageFilter<ProbabilityType, TLabelImage>;
    using BinarizerType = itk::BinaryThresholdImageFilter< TLabelImage, TLabelImage >;
    using MultiplyImageFilterType = itk::MultiplyImageFilter< ProbabilityType >;
    using AddImageFilterType = itk::AddImageFilter<ProbabilityType, ProbabilityType>;
    using RandomForestClassifierType = itk::RandomForest< ImageScalarType,ImageVectorType,TLabelImage>;

protected:
    CombineBWAandRFFilter ();
    ~CombineBWAandRFFilter () override = default;

    typename TLabelImage::PixelType         m_Label;
    SliceIndicesType                        m_LabeledSlices; // one for each axis

    using BoundingBoxesType = std::unordered_map< typename TLabelImage::PixelType, typename TLabelImage::RegionType >;
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
    void GenerateData() override;

    bool                                    m_ContourInformationOnly;

    /* Contains the functions to perform the interpolation*/
    void InterpolateLabelAlongAxis(typename TLabelImage::PixelType label, int axis);

private:

    CombineBWAandRFFilter (const Self &); //purposely not implemented
    void operator=(const Self &);  //purposely not implemented
    std::vector<int> m_SegmentationIndices;
    int         m_SlicingAxis {-1};
    int         m_UserAxis {-1};
    bool        m_intermediateslices {true};

};
} //namespace ITK

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkBWAandRFinterpolation.hxx"
#endif

#endif // itkBWAandRFinterpolation_h
