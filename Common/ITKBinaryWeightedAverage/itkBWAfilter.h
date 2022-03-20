#ifndef ITKBWAFILTER_H
#define ITKBWAFILTER_H

#include "itkImageToImageFilter.h"
#include "itkAndImageFilter.h"
#include "itkSubtractImageFilter.h"
#include "itkConnectedComponentImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkOrImageFilter.h"
#include "itkAddImageFilter.h"
#include "itkMultiplyImageFilter.h"
#include "itkTileImageFilter.h"
#include "itkComputeInterpolation.h"
#include "itkMaximumImageFilter.h"
#include "itkSignedMaurerDistanceMapImageFilter.h"
#include "itkPermuteAxesImageFilter.h"
#include "itkFlipImageFilter.h"

namespace itk
{
template< class TLabelImage, class TMainImage>
class BinaryWeightedAveragingFilter:public ImageToImageFilter< TLabelImage, TLabelImage >
{
public:
    /** Standard class typedefs. */
    typedef BinaryWeightedAveragingFilter Self;
    typedef ImageToImageFilter< TLabelImage, TLabelImage > Superclass;
    typedef SmartPointer< Self > Pointer;

    /** Method for creation through the object factory. */
    itkNewMacro(Self);

    /** Run-time type information (and related methods). */
    itkTypeMacro(BinaryWeightedAveragingFilter, ImageToImageFilter);

    TLabelImage* GetInterpolation();
    TMainImage* GetProbabilityMap();

    /** Extract some information from the image types.  Dimensionality
   * of the two images is assumed to be the same. */
    typedef typename TLabelImage::PixelType          LabelPixelType;
    typedef typename TLabelImage::InternalPixelType  LabelInternalPixelType;

    typedef typename TMainImage::PixelType          MainPixelType;
    typedef typename TMainImage::InternalPixelType  MainInternalPixelType;

    /** Image type information. */
    typedef itk::Image< LabelPixelType, TLabelImage::ImageDimension - 1 > TLabelImageSliceType;
    typedef itk::Image< MainPixelType, TMainImage::ImageDimension - 1 > TMainImageSliceType;

    /** Initialize ITK filters required */
    typedef itk::AndImageFilter<TLabelImageSliceType,TLabelImageSliceType,TLabelImageSliceType>AndImageFilterType;
    typedef itk::SubtractImageFilter<TLabelImageSliceType,TLabelImageSliceType,TLabelImageSliceType>SubtractImageFilterType;
    typedef itk::OrImageFilter<TLabelImageSliceType,TLabelImageSliceType,TLabelImageSliceType>OrImageFilterType;
    typedef itk::BinaryThresholdImageFilter<TLabelImageSliceType, TLabelImageSliceType>BinaryThresholdImageFilterType;
    typedef itk::ConnectedComponentImageFilter <TLabelImageSliceType,TLabelImageSliceType >ConnectedComponentImageFilterType;
    typedef itk::SignedMaurerDistanceMapImageFilter<TLabelImageSliceType, TMainImageSliceType> SignedDistanceMapFilterType;
    typedef itk::TileImageFilter< TLabelImageSliceType, TLabelImage > TilerType;
    typedef itk::TileImageFilter< TMainImageSliceType, TMainImage > DoubleTilerType;
    typedef itk::OrImageFilter<TLabelImage,TLabelImage,TLabelImage>ThreeDOrImageFilterType;
    typedef itk::ComputeInterpolation< TMainImageSliceType, TLabelImageSliceType>  InterpolationFilterType;
    typedef itk::MaximumImageFilter< TMainImage>  MaximumImageFilterType;
    typedef itk::MaximumImageFilter< TMainImageSliceType>  T2DMaximumImageFilterType;

    typedef itk::PermuteAxesImageFilter < TLabelImage> PermuteAxesImageFilterType;
    typedef itk::PermuteAxesImageFilter < TMainImage> PermuteAxesDoubleImageFilterType;
    typedef itk::FlipImageFilter < TLabelImage> FlipImageFilterType;
    typedef itk::FlipImageFilter < TMainImage> FlipDoubleImageFilterType;
    typedef itk::FlipImageFilter < TLabelImageSliceType > FlipSliceImageFilterType;
    typedef itk::FlipImageFilter < TMainImageSliceType > FlipDoubleSliceImageFilterType;

    void SetIntermediateSlicesOnly(bool flag)
    {
        m_intermediateslices = flag;
    }

    void SetSegmentationIndices(std::vector<int> seg_index){
        m_SegmentationIndices = seg_index;
    }

    void SetBoundingBox(typename TLabelImage::RegionType bbox){
        m_boundingbox = bbox;
    }

    void SetSlicingAxis(int SlicingAxis){
        m_slicingaxis = SlicingAxis;
    }

protected:
    BinaryWeightedAveragingFilter();
    ~BinaryWeightedAveragingFilter(){}

    /** Does the real work. */
    void GenerateData() override;

    /** Create the Output */
    DataObject::Pointer MakeOutput(unsigned int idx);

private:
    BinaryWeightedAveragingFilter(const Self &); //purposely not implemented
    void operator=(const Self &);  //purposely not implemented
    bool m_intermediateslices;
    std::vector<int> m_SegmentationIndices;
    int m_slicingaxis;
    int m_firstdirection;
    int m_seconddirection;
    typename TLabelImage::RegionType m_boundingbox;

    template <typename TImage>
    void CreateImage(TImage* const image, int numRows, int numCols)
    {
      // Create an image with 2 connected components
      typename TImage::IndexType corner = {{0,0}};

      typename TImage::SizeType size = {{numRows, numCols}};
      typename TImage::RegionType region(corner, size);

      image->SetRegions(region);
      image->Allocate();

      image->FillBuffer(0);
    }

};
} //namespace ITK

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkBWAfilter.hxx"
#endif

#endif // BINARYWEIGHTEDAVERAGEFILTER_H
