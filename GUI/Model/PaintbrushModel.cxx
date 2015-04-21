#include "PaintbrushModel.h"
#include "GenericSliceModel.h"
#include "GlobalState.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"

#include "itkRegionOfInterestImageFilter.h"
#include "itkGradientAnisotropicDiffusionImageFilter.h"
#include "itkGradientMagnitudeImageFilter.h"
#include "itkWatershedImageFilter.h"


// TODO: move this into a separate file!!!!
class BrushWatershedPipeline
{
public:
  typedef itk::Image<GreyType, 3> GreyImageType;
  typedef itk::Image<LabelType, 3> LabelImageType;
  typedef itk::Image<float, 3> FloatImageType;
  typedef itk::Image<unsigned long, 3> WatershedImageType;
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
    GreyImageType *grey,
    LabelImageType *label,
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

  bool UpdateLabelImage(
    LabelImageType *ltrg,
    CoverageModeType mode,
    LabelType drawing_color,
    LabelType overwrt_color)
    {
    // Get the watershed ID at the center voxel
    unsigned long wid = wf->GetOutput()->GetPixel(vcenter);

    // Keep track of changed voxels
    bool flagChanged = false;

    // Do the update
    typedef itk::ImageRegionConstIterator<WatershedImageType> WIter;
    typedef itk::ImageRegionIterator<LabelImageType> LIter;
    WIter wit(wf->GetOutput(), wf->GetOutput()->GetBufferedRegion());
    LIter sit(lsrc, lsrc->GetBufferedRegion());
    LIter tit(ltrg, region);
    for(; !wit.IsAtEnd(); ++sit,++tit,++wit)
      {
      LabelType pxLabel = sit.Get();
      if(wit.Get() == wid)
        {
        // Standard paint mode
        if (mode == PAINT_OVER_ALL ||
          (mode == PAINT_OVER_ONE && pxLabel == overwrt_color) ||
          (mode == PAINT_OVER_VISIBLE && pxLabel != 0))
          {
          pxLabel = drawing_color;
          }
        }
      if(pxLabel != tit.Get())
        {
        tit.Set(pxLabel);
        flagChanged = true;
        }
      }

    if(flagChanged)
      ltrg->Modified();
    return flagChanged;
    }

private:
  typedef itk::RegionOfInterestImageFilter<GreyImageType, FloatImageType> ROIType;
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






PaintbrushModel::PaintbrushModel()
{
  m_ReverseMode = false;
  m_Watershed = new BrushWatershedPipeline();
  m_ContextLayerId = (unsigned long) -1;
  m_IsEngaged = false;
}

PaintbrushModel::~PaintbrushModel()
{
  delete m_Watershed;
}

Vector3f PaintbrushModel::ComputeOffset()
{
  // Get the paintbrush properties
  PaintbrushSettings pbs =
      m_Parent->GetDriver()->GetGlobalState()->GetPaintbrushSettings();

  Vector3f offset(0.0);
  if(fmod(pbs.radius,1.0)==0)
    {
    offset.fill(0.5);
    offset(m_Parent->GetSliceDirectionInImageSpace()) = 0.0;
    }

  return offset;
}

void PaintbrushModel::ComputeMousePosition(const Vector3f &xSlice)
{
  // Only when an image is loaded
  if(!m_Parent->GetDriver()->IsMainImageLoaded())
    return;

  // Get the paintbrush properties
  PaintbrushSettings pbs =
      m_Parent->GetDriver()->GetGlobalState()->GetPaintbrushSettings();

  // Compute the new cross-hairs position in image space
  Vector3f xCross = m_Parent->MapSliceToImage(xSlice);

  // Round the cross-hairs position down to integer
  Vector3i xCrossInteger = to_int(xCross + ComputeOffset());

  // Make sure that the cross-hairs position is within bounds by clamping
  // it to image dimensions
  Vector3i xSize =
      to_int(m_Parent->GetDriver()->GetCurrentImageData()->GetVolumeExtents());

  Vector3ui newpos = to_unsigned_int(
    xCrossInteger.clamp(Vector3i(0),xSize - Vector3i(1)));

  if(newpos != m_MousePosition || m_MouseInside == false)
    {
    m_MousePosition = newpos;
    m_MouseInside = true;
    InvokeEvent(PaintbrushMovedEvent());
    }
}

bool PaintbrushModel::TestInside(const Vector2d &x, const PaintbrushSettings &ps)
{
  return this->TestInside(Vector3d(x(0), x(1), 0.0), ps);
}


// TODO: make this faster by precomputing all the repeated quantities. The inside
// check should take a lot less time!
bool PaintbrushModel::TestInside(const Vector3d &x, const PaintbrushSettings &ps)
{
  // Determine how to scale the voxels
  Vector3d xTest = x;
  if(ps.isotropic)
    {
    const Vector3f &spacing = m_Parent->GetSliceSpacing();
    double xMinVoxelDim = spacing.min_value();
    xTest(0) *= spacing(0) / xMinVoxelDim;
    xTest(1) *= spacing(1) / xMinVoxelDim;
    xTest(2) *= spacing(2) / xMinVoxelDim;
    }

  // Test inside/outside
  if(ps.mode == PAINTBRUSH_ROUND)
    {
    return xTest.squared_magnitude() <= (ps.radius-0.25) * (ps.radius-0.25);
    }
  else
    {
    return xTest.inf_norm() <= ps.radius - 0.25;
    }
}

bool
PaintbrushModel
::ProcessPushEvent(const Vector3f &xSlice, const Vector2ui &gridCell, bool reverse_mode)
{
  // Get the paintbrush properties (TODO: should we own them?)
  PaintbrushSettings pbs =
      m_Parent->GetDriver()->GetGlobalState()->GetPaintbrushSettings();

  // Store the unique ID of the layer in context
  ImageWrapperBase *layer = m_Parent->GetLayerForNthTile(gridCell[0], gridCell[1]);
  if(layer)
    {
    // Set the layer
    m_ContextLayerId = layer->GetUniqueId();
    m_IsEngaged = true;

    // Compute the mouse position
    ComputeMousePosition(xSlice);

    // Check if the right button was pressed
    ApplyBrush(reverse_mode, false);

    // Store the reverse mode
    m_ReverseMode = reverse_mode;

    // Store this as the last apply position
    m_LastApplyX = xSlice;

    // Eat the event unless cursor chasing is enabled
    return pbs.chase ? 0 : 1;
    }
  else
    {
    m_ContextLayerId = (unsigned long) -1;
    m_IsEngaged = false;
    return 0;
    }
}

bool
PaintbrushModel
::ProcessDragEvent(const Vector3f &xSlice, const Vector3f &xSliceLast,
                   double pixelsMoved, bool release)
{
  IRISApplication *driver = m_Parent->GetDriver();
  PaintbrushSettings pbs = driver->GetGlobalState()->GetPaintbrushSettings();

  if(m_IsEngaged)
    {
    // The behavior is different for 'fast' regular brushes and adaptive brush. For the
    // adaptive brush, dragging is disabled.
    if(pbs.mode != PAINTBRUSH_WATERSHED || m_ReverseMode)
      {
      // See how much we have moved since the last event. If we moved more than
      // the value of the radius, we interpolate the path and place brush strokes
      // along the path
      if(pixelsMoved > pbs.radius)
        {
        // Break up the path into steps
        size_t nSteps = (int) ceil(pixelsMoved / pbs.radius);
        for(size_t i = 0; i < nSteps; i++)
          {
          float t = (1.0 + i) / nSteps;
          Vector3f X = t * m_LastApplyX + (1.0f - t) * xSlice;
          ComputeMousePosition(X);
          ApplyBrush(m_ReverseMode, true);
          }
        }
      else
        {
        // Find the pixel under the mouse
        ComputeMousePosition(xSlice);

        // Scan convert the points into the slice
        ApplyBrush(m_ReverseMode, true);
        }

      // Store this as the last apply position
      m_LastApplyX = xSlice;
      }

    // If the mouse is being released, we need to commit the drawing
    if(release)
      {
      driver->StoreUndoPoint("Drawing with paintbrush");

      // TODO: this is ugly. The code for applying a brush should really be
      // placed in the IRISApplication.
      driver->InvokeEvent(SegmentationChangeEvent());

      m_IsEngaged = false;
      m_ContextLayerId = (unsigned long) -1;
      }

    // Eat the event unless cursor chasing is enabled
    return pbs.chase ? 0 : 1;
    }

  else
    return 0;
}

bool PaintbrushModel::ProcessMouseMoveEvent(const Vector3f &xSlice)
{
  ComputeMousePosition(xSlice);
  return true;
}


bool PaintbrushModel::ProcessMouseLeaveEvent()
{
  m_MouseInside = false;
  InvokeEvent(PaintbrushMovedEvent());
  return true;
}

void PaintbrushModel::AcceptAtCursor()
{
  IRISApplication *driver = m_Parent->GetDriver();

  m_MousePosition = m_Parent->GetDriver()->GetCursorPosition();
  m_MouseInside = true;
  ApplyBrush(false, false);

  // We need to commit the drawing
  driver->StoreUndoPoint("Drawing with paintbrush");

  // TODO: this is ugly. The code for applying a brush should really be
  // placed in the IRISApplication.
  driver->InvokeEvent(SegmentationChangeEvent());
}

bool
PaintbrushModel::ApplyBrush(bool reverse_mode, bool dragging)
{
  // Get the global objects
  IRISApplication *driver = m_Parent->GetDriver();
  GlobalState *gs = driver->GetGlobalState();

  // Get the segmentation image
  LabelImageWrapper *imgLabel = driver->GetCurrentImageData()->GetSegmentation();

  // Get the paint properties
  LabelType drawing_color = gs->GetDrawingColorLabel();
  DrawOverFilter drawover = gs->GetDrawOverFilter();

  // Get the paintbrush properties
  PaintbrushSettings pbs = gs->GetPaintbrushSettings();

  // Whether watershed filter is used (adaptive brush)
  bool flagWatershed = (
        pbs.mode == PAINTBRUSH_WATERSHED
        && (!reverse_mode) && (!dragging));

  // Define a region of interest
  LabelImageWrapper::ImageType::RegionType xTestRegion;
  for(size_t i = 0; i < 3; i++)
    {
    if(i != imgLabel->GetDisplaySliceImageAxis(m_Parent->GetId())
       || pbs.volumetric)
      {
      // For watersheds, the radius must be > 2
      double rad = (flagWatershed && pbs.radius < 1.5) ? 1.5 : pbs.radius;
      xTestRegion.SetIndex(i, (long) (m_MousePosition(i) - rad)); // + 1);
      xTestRegion.SetSize(i, (long) (2 * rad + 1)); // - 1);
      }
    else
      {
      xTestRegion.SetIndex(i, m_MousePosition(i));
      xTestRegion.SetSize(i, 1);
      }
    }

  // Crop the region by the buffered region
  xTestRegion.Crop(imgLabel->GetImage()->GetBufferedRegion());

  // Flag to see if anything was changed
  bool flagUpdate = false;

  // Special code for Watershed brush
  if(flagWatershed)
    {
    GenericImageData *gid = driver->GetCurrentImageData();

    // Get the currently engaged layer
    ImageWrapperBase *context_layer = gid->FindLayer(m_ContextLayerId, false);
    if(!context_layer)
      context_layer = gid->GetMain();

    // Precompute the watersheds
    m_Watershed->PrecomputeWatersheds(
          context_layer->GetDefaultScalarRepresentation()->GetCommonFormatImage(),
          driver->GetCurrentImageData()->GetSegmentation()->GetImage(),
          xTestRegion, to_itkIndex(m_MousePosition), pbs.watershed.smooth_iterations);

    m_Watershed->RecomputeWatersheds(pbs.watershed.level);
    }

  // Shift vector (different depending on whether the brush has odd/even diameter
  Vector3f offset = ComputeOffset();

  // Iterate over the region
  LabelImageWrapper::Iterator it(imgLabel->GetImage(), xTestRegion);
  for(; !it.IsAtEnd(); ++it)
    {
    // Check if we are inside the sphere
    LabelImageWrapper::ImageType::IndexType idx = it.GetIndex();
    Vector3f xDelta =
        offset
        + to_float(Vector3l(idx.GetIndex()))
        - to_float(m_MousePosition);

    Vector3d xDeltaSliceSpace = to_double(
          m_Parent->GetImageToDisplayTransform().TransformVector(xDelta));

    // Check if the pixel is inside
    if(!TestInside(xDeltaSliceSpace, pbs))
      continue;

    // Check if the pixel is in the watershed
    if(flagWatershed)
      {
      LabelImageWrapper::ImageType::IndexType idxoff = to_itkIndex(
        Vector3l(idx.GetIndex()) - Vector3l(xTestRegion.GetIndex().GetIndex()));
      if(!m_Watershed->IsPixelInSegmentation(idxoff))
        continue;
      }

    // Paint the pixel
    LabelType pxLabel = it.Get();

    // Standard paint mode
    if(!reverse_mode)
      {
      if(drawover.CoverageMode == PAINT_OVER_ALL ||
        (drawover.CoverageMode == PAINT_OVER_ONE && pxLabel == drawover.DrawOverLabel) ||
        (drawover.CoverageMode == PAINT_OVER_VISIBLE && pxLabel != 0))
        {
        it.Set(drawing_color);
        if(pxLabel != drawing_color) flagUpdate = true;
        }
      }
    // Background paint mode (clear label over current label)
    else
      {
      if(drawing_color != 0 && pxLabel == drawing_color)
        {
        it.Set(0);
        if(pxLabel != 0) flagUpdate = true;
        }
      else if(drawing_color == 0 && drawover.CoverageMode == PAINT_OVER_ONE)
        {
        it.Set(drawover.DrawOverLabel);
        if(pxLabel != drawover.DrawOverLabel) flagUpdate = true;
        }
      }
    }

  // Image has been updated
  if(flagUpdate)
    {
    imgLabel->GetImage()->Modified();
    }

  return flagUpdate;
}


Vector3f PaintbrushModel::GetCenterOfPaintbrushInSliceSpace()
{
  PaintbrushSettings pbs =
      m_Parent->GetDriver()->GetGlobalState()->GetPaintbrushSettings();

  if(fmod(pbs.radius, 1.0) == 0)
    return m_Parent->MapImageToSlice(to_float(m_MousePosition));
  else
    return m_Parent->MapImageToSlice(to_float(m_MousePosition) + Vector3f(0.5f));
}
