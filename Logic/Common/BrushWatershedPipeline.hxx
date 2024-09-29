#ifndef BRUSHWATERSHEDPIPELINE_HXX
#define BRUSHWATERSHEDPIPELINE_HXX

#include "RLERegionOfInterestImageFilter.h"
#include "itkGradientAnisotropicDiffusionImageFilter.h"
#include "itkGradientMagnitudeImageFilter.h"
#include "itkWatershedImageFilter.h"


// TODO: move this into a separate file!!!!
class BrushWatershedPipeline
{
public:
  typedef LabelImageWrapperTraits::ImageType LabelImageType;
  typedef itk::Image<float, 3> FloatImageType;
  typedef itk::Image<itk::IdentifierType, 3> WatershedImageType;
  typedef WatershedImageType::IndexType IndexType;

  BrushWatershedPipeline()
    {
    roi = ROIType::New();
    adf = ADFType::New();
    adf->SetInput(roi->GetOutput());
    adf->SetConductanceParameter(0.5);
    gmf = GMFType::New();
    gmf->SetInput(adf->GetOutput());
    wf = WFType::New();
    wf->SetInput(gmf->GetOutput());
    }

  void PrecomputeWatersheds(
    const FloatImageType *grey,
    const LabelImageType *label,
    itk::ImageRegion<3> region,
    itk::Index<3> vcenter,
    size_t smoothing_iter)
    {
    this->region = region;

    // Get the offset of vcenter in the region
    if(region.IsInside(vcenter))
      for(size_t d = 0; d < 3; d++)
        this->vcenter[d] = vcenter[d] - region.GetIndex()[d];
    else
      for(size_t d = 0; d < 3; d++)
        this->vcenter[d] = region.GetSize()[d] / 2;

    // Create a backup of the label image
    LROIType::Pointer lroi = LROIType::New();
    lroi->SetInput(label);
    lroi->SetRegionOfInterest(region);
    lroi->Update();
    lsrc = lroi->GetOutput();
    lsrc->DisconnectPipeline();

    // Initialize the watershed pipeline
    roi->SetInput(grey);
    roi->SetRegionOfInterest(region);
    adf->SetNumberOfIterations(smoothing_iter);

    // Set the initial level to lowest possible - to get all watersheds
    wf->SetLevel(1.0);
    wf->Update();
    }

  void RecomputeWatersheds(double level)
    {
    // Reupdate the filter with new level
    wf->SetLevel(level);
    wf->Update();
    }

  bool IsPixelInSegmentation(IndexType idx)
    {
    // Get the watershed ID at the center voxel
    unsigned long wctr = wf->GetOutput()->GetPixel(vcenter);
    unsigned long widx = wf->GetOutput()->GetPixel(idx);
    return wctr == widx;
    }

private:
  typedef itk::RegionOfInterestImageFilter<FloatImageType, FloatImageType> ROIType;
  typedef itk::RegionOfInterestImageFilter<LabelImageType, LabelImageType> LROIType;
  typedef itk::GradientAnisotropicDiffusionImageFilter<FloatImageType,FloatImageType> ADFType;
  typedef itk::GradientMagnitudeImageFilter<FloatImageType, FloatImageType> GMFType;
  typedef itk::WatershedImageFilter<FloatImageType> WFType;

  ROIType::Pointer roi;
  ADFType::Pointer adf;
  GMFType::Pointer gmf;
  WFType::Pointer wf;

  itk::ImageRegion<3> region;
  LabelImageType::Pointer lsrc;
  itk::Index<3> vcenter;
};


#endif // BRUSHWATERSHEDPIPELINE_HXX
