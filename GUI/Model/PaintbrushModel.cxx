#include "PaintbrushModel.h"
#include "GenericSliceModel.h"
#include "GlobalState.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "SegmentationUpdateIterator.h"
#include "BrushWatershedPipeline.hxx"
#include "DeepLearningSegmentationModel.h"


PaintbrushModel::PaintbrushModel()
{
  m_ReverseMode = false;
  m_Watershed = new BrushWatershedPipeline();
  m_ContextLayerId = (unsigned long)-1;
  m_IsEngaged = false;
}

PaintbrushModel::~PaintbrushModel() { delete m_Watershed; }

Vector3d
PaintbrushModel::ComputeOffset()
{
  // Get the paintbrush properties
  PaintbrushSettings pbs = m_Parent->GetDriver()->GetGlobalState()->GetPaintbrushSettings();

  Vector3d offset(0.0);
  if (fmod(pbs.radius, 1.0) == 0)
  {
    offset.fill(0.5);
    offset(m_Parent->GetSliceDirectionInImageSpace()) = 0.0;
  }

  return offset;
}

void
PaintbrushModel::ComputeMousePosition(const Vector3d &xSlice)
{
  // Only when an image is loaded
  if (!m_Parent->GetDriver()->IsMainImageLoaded())
    return;

  // Get the paintbrush properties
  PaintbrushSettings pbs = m_Parent->GetDriver()->GetGlobalState()->GetPaintbrushSettings();

  // Compute the new cross-hairs position in image space
  Vector3d xCross = m_Parent->MapSliceToImage(xSlice);

  // Round the cross-hairs position down to integer
  Vector3i xCrossInteger = to_int(xCross + ComputeOffset());

  // Make sure that the cross-hairs position is within bounds by clamping
  // it to image dimensions
  Vector3i xSize = to_int(m_Parent->GetDriver()->GetCurrentImageData()->GetVolumeExtents());

  Vector3ui newpos = to_unsigned_int(xCrossInteger.clamp(Vector3i(0), xSize - Vector3i(1)));

  if (newpos != m_MousePosition || m_MouseInside == false)
  {
    m_MousePosition = newpos;
    m_MouseInside = true;
    InvokeEvent(PaintbrushMovedEvent());
  }
}

bool
PaintbrushModel::HasMainImageTransformed()
{
  return !m_Parent->GetDriver()->GetMainImage()->ImageSpaceMatchesReferenceSpace();
}

bool
PaintbrushModel::TestInside(const Vector2d &x, const PaintbrushSettings &ps)
{
  return this->TestInside(Vector3d(x(0), x(1), 0.0), ps);
}


// TODO: make this faster by precomputing all the repeated quantities. The inside
// check should take a lot less time!
bool
PaintbrushModel::TestInside(const Vector3d &x, const PaintbrushSettings &ps)
{
  // Determine how to scale the voxels
  Vector3d xTest = x;
  if (ps.isotropic)
  {
    const Vector3d &spacing = m_Parent->GetSliceSpacing();
    double          xMinVoxelDim = spacing.min_value();
    xTest(0) *= spacing(0) / xMinVoxelDim;
    xTest(1) *= spacing(1) / xMinVoxelDim;
    xTest(2) *= spacing(2) / xMinVoxelDim;
  }

  // Test inside/outside
  if (ps.shape == PAINTBRUSH_ROUND)
  {
    return xTest.squared_magnitude() <= (ps.radius - 0.25) * (ps.radius - 0.25);
  }
  else
  {
    return xTest.inf_norm() <= ps.radius - 0.25;
  }
}

bool
PaintbrushModel ::ProcessPushEvent(const Vector3d &xSlice, const Vector2ui &gridCell, bool reverse_mode)
{
  // Get the paintbrush properties (TODO: should we own them?)
  PaintbrushSettings pbs = m_Parent->GetDriver()->GetGlobalState()->GetPaintbrushSettings();

  // Store the unique ID of the layer in context
  ImageWrapperBase *layer = m_Parent->GetLayerForNthTile(gridCell[0], gridCell[1]);
  if (layer)
  {
    // Set the layer
    m_ContextLayerId = layer->GetUniqueId();
    m_IsEngaged = true;

    // Compute the mouse position
    ComputeMousePosition(xSlice);

    // Check if the right button was pressed
    ApplyBrush(reverse_mode, false, false);

    // Store the reverse mode
    m_ReverseMode = reverse_mode;

    // Store this as the last apply position
    m_LastApplyX = xSlice;

    // Eat the event unless cursor chasing is enabled
    return pbs.chase ? 0 : 1;
  }
  else
  {
    m_ContextLayerId = (unsigned long)-1;
    m_IsEngaged = false;
    return 0;
  }
}

bool
PaintbrushModel ::ProcessDragEvent(const Vector3d &xSlice,
                                   const Vector3d &xSliceLast,
                                   double          pixelsMoved,
                                   bool            release)
{
  IRISApplication   *driver = m_Parent->GetDriver();
  PaintbrushSettings pbs = driver->GetGlobalState()->GetPaintbrushSettings();

  if (m_IsEngaged)
  {
    // The behavior is different for 'fast' regular brushes and adaptive brush. For the
    // adaptive brush, dragging is disabled.
    if (pbs.smart_mode != PAINTBRUSH_WATERSHED || m_ReverseMode)
    {
      // See how much we have moved since the last event. If we moved more than
      // the value of the radius, we interpolate the path and place brush strokes
      // along the path
      if (pixelsMoved > pbs.radius)
      {
        // Break up the path into steps
        size_t nSteps = (int)ceil(pixelsMoved / pbs.radius);
        for (size_t i = 0; i < nSteps; i++)
        {
          double   t = (1.0 + i) / nSteps;
          Vector3d X = t * m_LastApplyX + (1.0 - t) * xSlice;
          ComputeMousePosition(X);
          ApplyBrush(m_ReverseMode, true, i == nSteps-1 ? release : false);
        }
      }
      else
      {
        // Find the pixel under the mouse
        ComputeMousePosition(xSlice);

        // Scan convert the points into the slice
        ApplyBrush(m_ReverseMode, true, release);
      }

      // Store this as the last apply position
      m_LastApplyX = xSlice;
    }

    // If the mouse is being released, we need to commit the drawing
    if (release)
    {
      // Commit the drawing
      CommitDrawing();

      m_IsEngaged = false;
      m_ContextLayerId = (unsigned long)-1;
    }

    // Eat the event unless cursor chasing is enabled
    return pbs.chase ? 0 : 1;
  }

  else
    return 0;
}

bool
PaintbrushModel::ProcessMouseMoveEvent(const Vector3d &xSlice)
{
  ComputeMousePosition(xSlice);
  return true;
}


bool
PaintbrushModel::ProcessMouseLeaveEvent()
{
  m_MouseInside = false;
  InvokeEvent(PaintbrushMovedEvent());
  return true;
}

#include "UndoDataManager.h"

void
PaintbrushModel::CommitDrawing()
{
  IRISApplication   *driver = m_Parent->GetDriver();
  GlobalState       *gs = driver->GetGlobalState();
  PaintbrushSettings pbs = gs->GetPaintbrushSettings();
  LabelImageWrapper *seg = driver->GetSelectedSegmentationLayer();

  // Handle the deep learning case
  if (pbs.smart_mode == PAINTBRUSH_DLS)
  {
    // Get the model
    auto *model = m_Parent->GetParentUI()->GetDeepLearningSegmentationModel();
    auto *img =
      m_Parent->GetDriver()->GetCurrentImageData()->GetMain()->GetDefaultScalarRepresentation();

    // Undo the drawing we just did
    seg->StoreUndoPoint("Temporary undo point");
    seg->Undo();
    auto &commit = seg->GetUndoManager()->PeekCommit(seg->GetUndoManager()->GetNumberOfCommits() - 1);

    // If only one delta, then treat it as a point interaction
    bool rc = false;
    if(commit.GetDeltas().size() == 1 && m_MouseInside)
    {
      rc = model->PerformPointInteraction(img, m_MousePosition, m_ReverseMode);
    }
    else if(commit.GetDeltas().size() > 1)
    {
      // Compute the image corresponding to the last drawing
      SmartPtr<LabelImageWrapper> w_delta = LabelImageWrapper::New();
      w_delta->InitializeToWrapper(seg, (LabelType) 0);
      auto *img_delta = const_cast<LabelImageWrapper::ImageType *>(w_delta->GetImage());
      auto counts = seg->GenerateImageForRedo(commit, img_delta, gs->GetDrawingColorLabel());
      w_delta->PixelsModified();
      model->PerformScribbleInteraction(img, w_delta, counts.n_background > counts.n_foreground);
    }

  }

  // We need to commit the drawing
  driver->GetSelectedSegmentationLayer()->StoreUndoPoint("Drawing with paintbrush");
  driver->RecordCurrentLabelUse();

  // TODO: this is ugly. The code for applying a brush should really be
  // placed in the IRISApplication.
  driver->InvokeEvent(SegmentationChangeEvent());
}

void
PaintbrushModel::AcceptAtCursor()
{
  IRISApplication *driver = m_Parent->GetDriver();

  m_MousePosition = m_Parent->GetDriver()->GetCursorPosition();
  m_MouseInside = true;
  ApplyBrush(false, true, true);
  CommitDrawing();
}

bool
PaintbrushModel::ApplyBrush(bool reverse_mode, bool dragging, bool release)
{
  if (HasMainImageTransformed())
    return ApplyBrushByPolygonRasterization(reverse_mode, dragging);

  // Get the global objects
  IRISApplication  *driver = m_Parent->GetDriver();
  GlobalState      *gs = driver->GetGlobalState();
  GenericImageData *gid = driver->GetCurrentImageData();

  // Get the segmentation image
  LabelImageWrapper *imgLabel = driver->GetSelectedSegmentationLayer();

  // Get the paint properties
  LabelType      drawing_color = gs->GetDrawingColorLabel();
  DrawOverFilter drawover = gs->GetDrawOverFilter();

  // Get the paintbrush properties
  PaintbrushSettings pbs = gs->GetPaintbrushSettings();

  // Whether watershed filter is used (adaptive brush)
  bool flagWatershed = (pbs.smart_mode == PAINTBRUSH_WATERSHED && (!reverse_mode) && (!dragging));

  // Define a region of interest
  LabelImageWrapper::ImageType::RegionType xTestRegion;
  for (size_t i = 0; i < 3; i++)
  {
    if (i != imgLabel->GetDisplaySliceImageAxis(m_Parent->GetId()) || pbs.volumetric)
    {
      // For watersheds, the radius must be > 2
      double rad = (flagWatershed && pbs.radius < 1.5) ? 1.5 : pbs.radius;
      xTestRegion.SetIndex(i, (long)(m_MousePosition(i) - rad)); // + 1);
      xTestRegion.SetSize(i, (long)(2 * rad + 1));               // - 1);
    }
    else
    {
      xTestRegion.SetIndex(i, m_MousePosition(i));
      xTestRegion.SetSize(i, 1);
    }
  }

  // Crop the region by the buffered region
  xTestRegion.Crop(imgLabel->GetImage()->GetBufferedRegion());

  // Special code for Watershed brush
  if (flagWatershed)
  {
    // Get the currently engaged layer
    ImageWrapperBase *context_layer = gid->FindLayer(m_ContextLayerId, false);
    if (!context_layer)
      context_layer = gid->GetMain();

    // Obtain a cast to float pipeline from the layer
    // TODO: this causes repeated memory allocation - should probably create when entering mode
    auto *img_source =
      context_layer->CreateCastToFloatPipeline("WatershedBrush", this->m_Parent->GetId());

    // Precompute the watersheds
    m_Watershed->PrecomputeWatersheds(img_source,
                                      driver->GetSelectedSegmentationLayer()->GetImage(),
                                      xTestRegion,
                                      to_itkIndex(m_MousePosition),
                                      pbs.watershed.smooth_iterations);

    m_Watershed->RecomputeWatersheds(pbs.watershed.level);

    // Release the casting pipeline
    context_layer->ReleaseInternalPipeline("WatershedBrush", this->m_Parent->GetId());
  }

  // Shift vector (different depending on whether the brush has odd/even diameter
  Vector3d offset = ComputeOffset();

  // Iterate over the region using
  SegmentationUpdateIterator it_update(imgLabel, xTestRegion, drawing_color, drawover);

  for (; !it_update.IsAtEnd(); ++it_update)
  {
    SegmentationUpdateIterator::IndexType idx = it_update.GetIndex();

    Vector3d xDelta = offset + to_double(idx) - to_double(m_MousePosition);
    Vector3d xDeltaSliceSpace =
      to_double(m_Parent->GetImageToDisplayTransform()->TransformVector(xDelta));

    // Check if the pixel is inside
    if (!TestInside(xDeltaSliceSpace, pbs))
      continue;

    // Check if the pixel is in the watershed
    if (flagWatershed)
    {
      LabelImageWrapper::ImageType::IndexType idxoff;
      for (unsigned int i = 0; i < 3; i++)
        idxoff[i] = idx.GetIndex()[i] - xTestRegion.GetIndex().GetIndex()[i];

      if (!m_Watershed->IsPixelInSegmentation(idxoff))
        continue;
    }

    // Paint the pixel
    if (reverse_mode)
      it_update.PaintAsBackground();
    else
      it_update.PaintAsForeground();
  }

  // Finalize the iteration
  if (!it_update.Finalize())
    return false;

  // Send the delta for undo
  imgLabel->StoreIntermediateUndoDelta(it_update.RelinquishDelta());

  // Changes were made
  return true;
}

bool
PaintbrushModel::ApplyBrushDeepLearning(bool reverse_mode)
{
  // Get the model
  auto *model = m_Parent->GetParentUI()->GetDeepLearningSegmentationModel();
  auto *img =
    m_Parent->GetDriver()->GetCurrentImageData()->GetMain()->GetDefaultScalarRepresentation();

  // Handle the point interaction
  if (m_MouseInside)
  {
    return model->PerformPointInteraction(img, m_MousePosition, reverse_mode);
  }

  return false;
}


Vector3d
PaintbrushModel::GetCenterOfPaintbrushInSliceSpace()
{
  PaintbrushSettings pbs = m_Parent->GetDriver()->GetGlobalState()->GetPaintbrushSettings();

  if (fmod(pbs.radius, 1.0) == 0)
    return m_Parent->MapImageToSlice(to_double(m_MousePosition));
  else
    return m_Parent->MapImageToSlice(to_double(m_MousePosition) + Vector3d(0.5));
}

bool
PaintbrushModel ::ApplyBrushByPolygonRasterization(bool reverse_mode, bool dragging)
{
  // build 2d vertices
  std::vector<Vector2d> vts2d = m_BrushPoints;
  Vector3d              ctr = GetCenterOfPaintbrushInSliceSpace();

  for (auto &v : vts2d)
  {
    v[0] += ctr[0];
    v[1] += ctr[1];
  }

  // run voxelization code
  m_Parent->Voxelize2DPolygonToSegmentationSlice(
    vts2d, "Oblique Paintbrush Update", false, reverse_mode);
  return true;
}
