#include "Generic3DRenderer.h"
#include "Generic3DModel.h"
#include "GlobalUIModel.h"
#include "SNAPAppearanceSettings.h"
#include "GenericImageData.h"
#include "Rebroadcaster.h"
#include "ColorLabel.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "SNAPImageData.h"
#include "ImageMeshLayers.h"
#include "StandaloneMeshWrapper.h"
#include "SegmentationMeshWrapper.h"
#include "MeshWrapperBase.h"
#include "MeshManager.h"
#include "Window3DPicker.h"

#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkCamera.h"
#include "vtkTransform.h"
#include "vtkLineSource.h"
#include "vtkPropAssembly.h"
#include "vtkMath.h"
#include "vtkUnstructuredGridVolumeRayCastMapper.h"
#include "vtkDiscreteMarchingCubes.h"
#include "vtkWindowedSincPolyDataFilter.h"
#include "vtkThreshold.h"
#include "vtkDataSetMapper.h"
#include "vtkTransformFilter.h"
#include "vtkTransform.h"
#include "vtkLookupTable.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkGeometryFilter.h"
#include "vtkGlyph3D.h"
#include "vtkCubeSource.h"
#include "vtkSphereSource.h"
#include "vtkImplicitPlaneWidget.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkCubeSource.h"
#include "vtkCoordinate.h"
#include "vtkQuadricLODActor.h"
#include "vtkScalarBarActor.h"
#include "vtkTextProperty.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkCommand.h"
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkImageImport.h>
#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <IntensityCurveInterface.h>
#include <ColorMap.h>
#include <AffineTransformHelper.h>
#include <itkTransform.h>

#include <vnl/vnl_cross.h>
#include <chrono>
#include <thread>


bool operator == (const CameraState &c1, const CameraState &c2)
{
  return
      c1.position == c2.position &&
      c1.focal_point == c2.focal_point &&
      c1.view_up == c2.view_up &&
      c1.clipping_range == c2.clipping_range &&
      c1.view_angle == c2.view_angle &&
      c1.parallel_scale == c2.parallel_scale &&
      c1.parallel_projection == c2.parallel_projection;
}

bool operator != (const CameraState &c1, const CameraState &c2)
{
  return !(c1==c2);
}

Generic3DRenderer::Generic3DRenderer()
{
  // Create a picker
  m_Picker = vtkSmartPointer<Window3DPicker>::New();

  // Coordinate mapper
  m_CoordinateMapper = vtkSmartPointer<vtkCoordinate>::New();
  m_CoordinateMapper->SetCoordinateSystemToViewport();

  // Actor Pool
  m_ActorPool = ActorPool::New();

  // ------------------ AXES ----------------------------

  // Initialize the line sources for the axes
  for(int i = 0; i < 3; i++)
    {
    // Create the line source (no point coordinates yet)
    m_AxisLineSource[i] = vtkSmartPointer<vtkLineSource>::New();
    m_AxisLineSource[i]->SetResolution(10);

    // Create mapper, actor, etc
    vtkSmartPointer<vtkPolyDataMapper> mapper =
        vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(m_AxisLineSource[i]->GetOutputPort());

    m_AxisActor[i] = vtkSmartPointer<vtkActor>::New();
    m_AxisActor[i]->SetMapper(mapper);

    this->m_Renderer->AddActor(m_AxisActor[i]);
    }


  // ------------------ SPRAYPAINT ----------------------------

  // Create a glyph filter which handles spray paint
  vtkSmartPointer<vtkSphereSource> cube = vtkSmartPointer<vtkSphereSource>::New();
  m_SprayGlyphFilter = vtkSmartPointer<vtkGlyph3D>::New();
  m_SprayGlyphFilter->SetSourceConnection(cube->GetOutputPort());
  m_SprayGlyphFilter->SetScaleModeToDataScalingOff();
  m_SprayGlyphFilter->SetScaleFactor(1.4);

  // Create a transform filter for the glyph filter
  m_SprayTransform = vtkSmartPointer<vtkTransform>::New();
  vtkSmartPointer<vtkTransformFilter> tf_glyph = vtkSmartPointer<vtkTransformFilter>::New();
  tf_glyph->SetTransform(m_SprayTransform.GetPointer());
  tf_glyph->SetInputConnection(m_SprayGlyphFilter->GetOutputPort());

  // Create the spray paint property
  m_SprayProperty = vtkSmartPointer<vtkProperty>::New();
  m_SprayProperty->SetShading(VTK_FLAT);

  // Create a mapper, etc for the glyph filter and add to the renderer
  vtkSmartPointer<vtkPolyDataMapper> mapper_spray = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper_spray->SetInputConnection(tf_glyph->GetOutputPort());

  // Create and add an actor for the spray
  m_SprayActor = vtkSmartPointer<vtkActor>::New();
  m_SprayActor->SetMapper(mapper_spray);
  m_SprayActor->SetProperty(m_SprayProperty);

  // ------------------ SCALPEL ----------------------------

  // Create the line actor for scalpel positioning
  m_ScalpelLineSource = vtkSmartPointer<vtkLineSource>::New();

  // Create mapper, actor, etc
  vtkSmartPointer<vtkPolyDataMapper2D> mapper_scalpel =
      vtkSmartPointer<vtkPolyDataMapper2D>::New();
  mapper_scalpel->SetInputConnection(m_ScalpelLineSource->GetOutputPort());

  m_ScalpelLineActor = vtkSmartPointer<vtkActor2D>::New();
  m_ScalpelLineActor->SetMapper(mapper_scalpel);

  this->m_Renderer->AddActor(m_ScalpelLineActor);

  m_ScalarBarActor = vtkScalarBarActor::New();
  m_ScalarBarActor->SetBarRatio(m_ScalarBarActor->GetBarRatio() * 0.5);
  m_ScalarBarActor->UnconstrainedFontSizeOn();
  m_ScalarBarActor->GetLabelTextProperty()->SetFontSize(10);
  auto textProp = m_ScalarBarActor->GetTitleTextProperty();
  textProp->SetFontSize(9);
  textProp->SetJustificationToLeft();
  this->m_Renderer->AddActor2D(m_ScalarBarActor);

  m_ImageCubeSource = vtkSmartPointer<vtkCubeSource>::New();
  m_ImageCubeTransform = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  m_ImageCubeTransform->SetInputConnection(m_ImageCubeSource->GetOutputPort());
  m_ScalpelPlaneWidget = vtkSmartPointer<vtkImplicitPlaneWidget>::New();
  m_ScalpelPlaneWidget->SetInputConnection(m_ImageCubeTransform->GetOutputPort());

  m_ScalpelPlaneWidget->SetPlaceFactor(1.0);

  m_ScalpelPlaneWidget->OutlineTranslationOff();
  m_ScalpelPlaneWidget->OutsideBoundsOff();
  m_ScalpelPlaneWidget->ScaleEnabledOff();

  m_ScalpelPlaneWidget->GetPlaneProperty()->SetOpacity(0.5);
  m_ScalpelPlaneWidget->GetOutlineProperty()->SetOpacity(0.2);
  m_ScalpelPlaneWidget->SetTubing(1);

  // Rebroadcast Modified event from the camera object as a CameraUpdateEvent
  Rebroadcast(m_Renderer->GetActiveCamera(), vtkCommand::ModifiedEvent, CameraUpdateEvent());
}

void Generic3DRenderer::SetModel(Generic3DModel *model)
{
  // Save the model
  m_Model = model;

  // Get a pointer to the application class
  IRISApplication *app = m_Model->GetParentUI()->GetDriver();

  // Record and rebroadcast changes in the model
  //Rebroadcast(app->GetMeshManager(), itk::ModifiedEvent(), ModelUpdateEvent());

  // Respond to changes in image dimension - these require big updates
  Rebroadcast(app, MainImageDimensionsChangeEvent(), ModelUpdateEvent());

  // Respond to changes to the segmentation. These are ignored unless we are
  // in continous update mode, in which case the renderers are rebuilt
  // Rebroadcast(app, SegmentationChangeEvent(), ModelUpdateEvent());
  Rebroadcast(app, LevelSetImageChangeEvent(), ModelUpdateEvent());
  Rebroadcast(m_Model->GetContinuousUpdateModel(), ValueChangedEvent(), ModelUpdateEvent());

  // Respond to cursor events
  Rebroadcast(m_Model->GetParentUI(), CursorUpdateEvent(), ModelUpdateEvent());

  // Respond to label appearance change events
  Rebroadcast(app->GetColorLabelTable(),
              SegmentationLabelChangeEvent(), ModelUpdateEvent());

  // Respond to change in current label
  Rebroadcast(app->GetGlobalState()->GetDrawingColorLabelModel(),
              ValueChangedEvent(), ModelUpdateEvent());

  // Rebroadcast generic model update event from parent model
  Rebroadcast(m_Model, ModelUpdateEvent(), ModelUpdateEvent());

  // Respond to spray paint events
  Rebroadcast(m_Model, Generic3DModel::SprayPaintEvent(), ModelUpdateEvent());

  // Respond to scalpel events
  Rebroadcast(m_Model, Generic3DModel::ScalpelEvent(), ModelUpdateEvent());

  // Respond to changes in toolbar mode
  Rebroadcast(m_Model->GetParentUI()->GetGlobalState()->GetToolbarMode3DModel(),
              ValueChangedEvent(), ModelUpdateEvent());

  // Listen to changes in appearance
  Rebroadcast(m_Model->GetParentUI()->GetAppearanceSettings(),
              ChildPropertyChangedEvent(), ModelUpdateEvent());

  // Change in the selected segmentation layer affects what is rendered
  Rebroadcast(app->GetGlobalState()->GetSelectedSegmentationLayerIdModel(),
              ValueChangedEvent(), ModelUpdateEvent());

  // Appearance changes in main/overlays (for volume rendering)
  Rebroadcast(m_Model->GetParentUI()->GetDriver(), WrapperChangeEvent(), ModelUpdateEvent());
  Rebroadcast(m_Model->GetParentUI()->GetDriver(), WrapperVisibilityChangeEvent(), ModelUpdateEvent());

  // Visibility of Color Bar
  Rebroadcast(m_Model->GetDisplayColorBarModel(), ValueChangedEvent(), ModelUpdateEvent());

  // Respond to mesh layer display mapping policy change event
  Rebroadcast(app->GetIRISImageData()->GetMeshLayers(),
              WrapperDisplayMappingChangeEvent(), ModelUpdateEvent());

	// Respond to levelset mesh layer display mapping policy change event
	Rebroadcast(app->GetSNAPImageData()->GetMeshLayers(),
							WrapperDisplayMappingChangeEvent(), ModelUpdateEvent());

  // Respond to active mesh layer change event
  Rebroadcast(app->GetIRISImageData()->GetMeshLayers(),
              ActiveLayerChangeEvent(), ModelUpdateEvent());

  // Respond to active mesh layer change event
  Rebroadcast(app->GetSNAPImageData()->GetMeshLayers(),
              ActiveLayerChangeEvent(), ModelUpdateEvent());

  // Respond to timepoint change event
  Rebroadcast(app, CursorTimePointUpdateEvent(), ModelUpdateEvent());

  // Update the main components
  this->UpdateAxisRendering();
  this->UpdateCamera(true);

  // Hook up the spray pipeline
  m_SprayGlyphFilter->SetInputData(m_Model->GetSprayPoints());

  // Set the model in the picker
  m_Picker->SetModel(m_Model);

  UpdateColorLegendAppearance();
}

void Generic3DRenderer::UpdateMeshAssembly()
{
  if(m_Model->IsMeshUpdating() )
    {
    return;
    }

  if(!m_Model->GetDriver()->IsMainImageLoaded())
    return;

  ImageMeshLayers *layers = m_Model->GetMeshLayers();

  // Get active layer id and current time point
  unsigned long active_layer_id = layers->GetActiveLayerId();
  unsigned int tp = m_Model->GetDriver()->GetCursorTimePoint();

  // Remove all mesh actors from the renderer before the update
  ResetMeshAssembly();

  if (layers->size() == 0 || active_layer_id == 0)
    return;

  MeshWrapperBase* active_layer = layers->GetLayer(active_layer_id);
  assert(active_layer);

  // Get display mapping policy for color configuration
  MeshDisplayMappingPolicy* dmp = active_layer->GetMeshDisplayMappingPolicy();

  // Layer or timepoint change requires rebuilding the map entirely
  if (tp != m_CrntActorMapTimePoint || active_layer_id != m_CrntActorMapLayerId)
    {
      m_CrntActorMapTimePoint = tp;
      m_CrntActorMapLayerId = active_layer_id;
    }

  // Process all addition and update
  dmp->UpdateActorMap(m_ActorPool, tp);

	// Add meshes in the actor map to the renderer
	auto actorMap = m_ActorPool->GetActorMap();

  for (auto it = actorMap->begin(); it != actorMap->end(); ++it)
    m_Renderer->AddActor(it->second);

	ApplyDisplayMappingPolicyChange();

  m_Renderer->Modified();
}

void Generic3DRenderer::ApplyDisplayMappingPolicyChange()
{
  if (m_CrntActorMapLayerId == 0) // no layer activated yet
    return;

  // Get current layer
  auto active_layer = m_Model->GetMeshLayers()->GetLayer(m_CrntActorMapLayerId);

  // Use display mapping policy to update appearance
  MeshDisplayMappingPolicy* dmp = active_layer->GetMeshDisplayMappingPolicy();
	dmp->UpdateAppearance(m_ActorPool, m_CrntActorMapTimePoint);

  // Update legend
  dmp->ConfigureLegend(m_ScalarBarActor);
}

void Generic3DRenderer::ResetMeshAssembly()
{
  if(m_Model->IsMeshUpdating())
    return;

  auto actorMap = m_ActorPool->GetActorMap();

  for(auto it_actor = actorMap->begin(); it_actor != actorMap->end(); it_actor++)
    this->m_Renderer->RemoveActor(it_actor->second);

  m_ActorPool->RecycleAll();

  InvokeEvent(ModelUpdateEvent());
}

void Generic3DRenderer::UpdateAxisRendering()
{
  // Update the coordinates of the line source
  IRISApplication *app = m_Model->GetParentUI()->GetDriver();
  SNAPAppearanceSettings *as = m_Model->GetParentUI()->GetAppearanceSettings();

  if(app->IsMainImageLoaded())
    {
    Vector3ui cursor = app->GetCursorPosition();
    Vector3ui dims = app->GetCurrentImageData()->GetImageRegion().GetSize();

    // Get the axis appearance properties
    OpenGLAppearanceElement *axisapp =
        as->GetUIElement(SNAPAppearanceSettings::CROSSHAIRS_3D);

    // Voxel to world transform
    vtkSmartPointer<vtkTransform> tran = vtkSmartPointer<vtkTransform>::New();
    tran->SetMatrix(m_Model->GetWorldMatrix().data_block());

    for(int i = 0; i < 3; i++)
      {
      // Update visibility
      m_AxisActor[i]->SetVisibility(axisapp->GetVisibilityFlag());

      // Update the cursor position
      Vector3d p1 = to_double(cursor), p2 = to_double(cursor);
      p1[i] = 0; p2[i] = dims[i];
      m_AxisLineSource[i]->SetPoint1(p1.data_block());
      m_AxisLineSource[i]->SetPoint2(p2.data_block());
      m_AxisLineSource[i]->Update();

      // Update the visual appearance
      vtkProperty *prop = m_AxisActor[i]->GetProperty();
      prop->SetColor(axisapp->GetColor().data_block());
      prop->SetOpacity(axisapp->GetAlpha());
      if(axisapp->GetLineType() > vtkPen::SOLID_LINE)
        {
        // TODO: reconcile this with 2D line style stuff
        prop->SetLineStipplePattern(0x9999);
        // prop->SetLineStippleRepeatFactor(static_cast<int>(axisapp->GetDashSpacing()));
        prop->SetLineWidth(axisapp->GetLineThickness());
        m_AxisActor[i]->SetVisibility(axisapp->GetVisible());
        }

      // Update the transform
      m_AxisActor[i]->SetUserTransform(tran);
      }

    // Also update the image cube
    m_ImageCubeSource->SetBounds(0, dims[0], 0, dims[1], 0, dims[2]);
    m_ImageCubeTransform->SetTransform(tran);
    m_ImageCubeTransform->Update();
    m_ImageCubeTransform->GetOutput()->ComputeBounds();
    }

  // Set the background of the window
  Vector3d clrBack = as->GetUIElement(SNAPAppearanceSettings::BACKGROUND_3D)->GetColor();
  m_Renderer->SetBackground(clrBack.data_block());
}

void Generic3DRenderer::UpdateColorLegendAppearance()
{
  m_ScalarBarActor->SetVisibility(m_Model->GetDisplayColorBar());
}

void Generic3DRenderer::UpdateScalpelRendering()
{
  // If not in scalpel mode, just disable everything
  if(m_Model->GetParentUI()->GetGlobalState()->GetToolbarMode3D() != SCALPEL_MODE)
    {
    m_ScalpelPlaneWidget->SetEnabled(0);
    m_ScalpelLineActor->SetVisibility(0);
    return;
    }

  if(m_Model->GetScalpelStatus() == Generic3DModel::SCALPEL_LINE_STARTED)
    {
    m_ScalpelLineActor->SetVisibility(1);
    m_ScalpelLineSource->SetPoint1(
          m_Model->GetScalpelStart()[0], m_Model->GetScalpelStart()[1], 0);
    m_ScalpelLineSource->SetPoint2(
          m_Model->GetScalpelEnd()[0], m_Model->GetScalpelEnd()[1], 0);
    m_ScalpelLineSource->Update();
    }
  else
    {
    m_ScalpelLineActor->SetVisibility(0);
    }

  if(m_Model->GetScalpelStatus() == Generic3DModel::SCALPEL_LINE_COMPLETED)
    {
    // Get the endpoints of the cut the user drew on the viewport
    Vector2i w0 = m_Model->GetScalpelStart(), w1 = m_Model->GetScalpelEnd();

    // If for some reason the endpoints are the same, we just make the cut
    // plane vertical on the screen
    if(w0 == w1)
      w1[1] = w0[1] + 1;

    // Use the coordinate mapper to map the two points on the viewport to
    // world coordinates
    m_CoordinateMapper->SetValue(w0[0], w0[1], 0.0);
    Vector3d p0(m_CoordinateMapper->GetComputedWorldValue(this->m_Renderer));

    m_CoordinateMapper->SetValue(w1[0], w1[1], 0.0);
    Vector3d p1(m_CoordinateMapper->GetComputedWorldValue(this->m_Renderer));

    // Vector from start to end in world coords
    Vector3d v01 = p1 - p0;

    // Vector pointing into the screen
    m_CoordinateMapper->SetValue(w0[0], w0[1], 1.0);
    Vector3d w = Vector3d(m_CoordinateMapper->GetComputedWorldValue(this->m_Renderer)) - p0;

    // Compute the plane normal vector as the cross-product of the vector between
    // the endpoints and the vector pointing out of the viewport
    Vector3d n = - vnl_cross_3d(v01, w);
    n.normalize();

    // Compute the intercept of the implicit plane
    double intercept = dot_product(p0, n);

    // Compute the center of the image volume
    Vector3d xctr(m_ScalpelPlaneWidget->GetInput()->GetCenter());
    Vector3d orig = xctr - n * (dot_product(xctr, n) - intercept);

    // m_ImageCubeTransform->Update();
    // m_ImageCubeTransform->GetOutput()->ComputeBounds();
    // m_ScalpelPlaneWidget->SetInteractor(this->GetRenderWindowInteractor());
    // m_ScalpelPlaneWidget->SetPlaceFactor(1.0);
    m_ScalpelPlaneWidget->PlaceWidget();
    m_ScalpelPlaneWidget->SetEnabled(1);

    // We will also need to set the origin of the plane
    m_ScalpelPlaneWidget->SetNormal(n.data_block());
    m_ScalpelPlaneWidget->SetOrigin(orig.data_block());
    }
  else
    {
    m_ScalpelPlaneWidget->SetEnabled(0);
    }
}

void Generic3DRenderer::UpdateScalpelPlaneAppearance()
{
  // The color of the normal is the same as the current label
  IRISApplication *app = m_Model->GetParentUI()->GetDriver();
  LabelType dl = app->GetGlobalState()->GetDrawingColorLabel();
  ColorLabel cdl = app->GetColorLabelTable()->GetColorLabel(dl);

  // Get a color and a brighter color
  // (I'm using http://www.poynton.com/PDFs/ColorFAQ.pdf)
  Vector3d color = cdl.GetRGBAsDoubleVector();

  // Convert the color to HSV
  Vector3d hsv, hsv_brighter;
  vtkMath::RGBToHSV(color.data_block(), hsv.data_block());

  // If the value part is below 0.5 (dark color), use default color for the
  // normal
  if(hsv[2] < 0.5)
    {
    hsv[1] = 0; hsv[2] = 0.9;
    hsv_brighter = hsv; hsv_brighter[2] = 1.0;
    }
  else
    {
    hsv_brighter = hsv; hsv_brighter[2] = 1.1 * hsv[2];
    }

  Vector3d color_main, color_bright;
  vtkMath::HSVToRGB(hsv.data_block(), color_main.data_block());
  vtkMath::HSVToRGB(hsv_brighter.data_block(), color_bright.data_block());
/*
  Vector3d color_xyz, color_brighter, color_darker;

  vtkMath::RGBToXYZ(color.data_block(), color_xyz.data_block());
  color_xyz *= 1.1;
  vtkMath::XYZToRGB(color_xyz.data_block(), color_brighter.data_block());

  vtkMath::RGBToXYZ(color.data_block(), color_xyz.data_block());
  color_xyz /= 1.1;
  vtkMath::XYZToRGB(color_xyz.data_block(), color_darker.data_block());
*/
  m_ScalpelPlaneWidget->GetNormalProperty()->SetColor(color_main.data_block());
  m_ScalpelPlaneWidget->GetSelectedNormalProperty()->SetColor(color_bright.data_block());
  m_ScalpelPlaneWidget->GetEdgesProperty()->SetColor(color_main.data_block());
  m_ScalpelPlaneWidget->GetPlaneProperty()->SetColor(color_main.data_block());
  m_ScalpelPlaneWidget->GetSelectedPlaneProperty()->SetColor(color_main.data_block());
  m_ScalpelPlaneWidget->UpdatePlacement();
}

void Generic3DRenderer::UpdateSprayGlyphAppearanceAndShape()
{
  IRISApplication *app = m_Model->GetParentUI()->GetDriver();
  if(app->IsMainImageLoaded())
    {
    ImageWrapperBase *main = app->GetCurrentImageData()->GetMain();

    // The color of the spray paint is the same as the current label
    LabelType dl = app->GetGlobalState()->GetDrawingColorLabel();
    ColorLabel cdl = app->GetColorLabelTable()->GetColorLabel(dl);
    m_SprayProperty->SetColor(cdl.GetRGBAsDoubleVector().data_block());

    // Set the spray transform
    vnl_matrix_fixed<double, 4, 4> vox2nii = main->GetNiftiSform();
    m_SprayTransform->SetMatrix(vox2nii.data_block());
    }
}

void Generic3DRenderer::UpdateCamera(bool reset)
{
  // Update the coordinates of the line source
  IRISApplication *app = m_Model->GetParentUI()->GetDriver();

  if(app->IsMainImageLoaded())
    {
    Vector3ui cursor = app->GetCursorPosition();
    Vector3d spacing = app->GetCurrentImageData()->GetImageSpacing();
    ImageWrapperBase *main = app->GetCurrentImageData()->GetMain();
    Vector3d dim = element_product(to_double(main->GetSize()), spacing);
    Vector3d ctr = main->TransformVoxelCIndexToNIFTICoordinates(to_double(cursor));


    if(reset)
      {
      Vector3d x0 = ctr - dim * 0.5, x1 = ctr + dim * 0.5;

      // Center camera on the cursor
      m_Renderer->GetActiveCamera()->SetFocalPoint(ctr[0], ctr[1], ctr[2]);

      // Place camera along the R-L axis
      m_Renderer->GetActiveCamera()->SetPosition(x0[0], ctr[1], ctr[2]);

      // Make camera point so that Superior is up
      m_Renderer->GetActiveCamera()->SetViewUp(0,0,1);

      // Make the camera point at the crosshair. We use the reset camera
      // method, with the bounding box centered on the current cursor position
      m_Renderer->ResetCamera(x0[0], x1[0], x0[1], x1[1], x0[2], x1[2]);
      }
    else
      {
      // Only center the camera, don't change the other view parameters
      m_Renderer->GetActiveCamera()->SetFocalPoint(ctr.data_block());
      }
    }
}

void Generic3DRenderer::SaveCameraState()
{
  m_SavedCameraState = vtkSmartPointer<vtkCamera>::New();
  m_SavedCameraState->DeepCopy(this->m_Renderer->GetActiveCamera());
}

void Generic3DRenderer::ClearRendering()
{
  this->ResetMeshAssembly();
}

void Generic3DRenderer::RestoreSavedCameraState()
{
  if(m_SavedCameraState)
    this->m_Renderer->GetActiveCamera()->DeepCopy(m_SavedCameraState);
  InvokeEvent(ModelUpdateEvent());
  InvokeEvent(CameraUpdateEvent());
}

void Generic3DRenderer::DeleteSavedCameraState()
{
  m_SavedCameraState = NULL;
}

bool Generic3DRenderer::IsSavedCameraStateAvailable()
{
  return m_SavedCameraState != NULL;
}

CameraState Generic3DRenderer::GetCameraState() const
{
  CameraState cs;
  vtkCamera *camera = m_Renderer->GetActiveCamera();

  cs.position.set(camera->GetPosition());
  cs.focal_point.set(camera->GetFocalPoint());
  cs.view_up.set(camera->GetViewUp());
  cs.view_angle = camera->GetViewAngle();
  cs.parallel_projection = camera->GetParallelProjection();
  cs.parallel_scale = camera->GetParallelScale();
  cs.clipping_range.set(camera->GetClippingRange());

  return cs;
}

void Generic3DRenderer::SetCameraState(const CameraState &cs)
{
  // Update the camera parameters. VTK implementation of the SetXXX methods
  // will invoke the Modified() event if necessary
  vtkCamera *camera = m_Renderer->GetActiveCamera();

  // Get the m-time of the camera before updates
  unsigned long mtime = camera->GetMTime();

  // Update the camera
  camera->SetPosition(cs.position.data_block());
  camera->SetFocalPoint(cs.focal_point.data_block());
  camera->SetViewUp(cs.view_up.data_block());
  camera->SetViewAngle(cs.view_angle);
  camera->SetParallelProjection(cs.parallel_projection);
  camera->SetParallelScale(cs.parallel_scale);
  camera->SetClippingRange(cs.clipping_range.data_block());

  // If the camera really updated, fire an event
  if(camera->GetMTime() > mtime)
    InvokeEvent(ModelUpdateEvent());
}

void Generic3DRenderer::SetRenderWindow(vtkRenderWindow *rwin)
{
  Superclass::SetRenderWindow(rwin);

  // The interactor has become available - connect it to subcomponents
  rwin->GetInteractor()->SetPicker(m_Picker);
  m_ScalpelPlaneWidget->SetInteractor(rwin->GetInteractor());

  // Why is this necessary?
  // rwin->SetMultiSamples(4);
  // rwin->SetLineSmoothing(1);
}

void Generic3DRenderer::UpdateMeshAppearance()
{
  // Get the app driver
  IRISApplication *driver = m_Model->GetParentUI()->GetDriver();

	if (!driver->IsMainImageLoaded())
		return;

  auto actorMap = m_ActorPool->GetActorMap();

  // For each actor, update the property to reflect current state
  for(auto it_actor = actorMap->begin(); it_actor != actorMap->end(); ++it_actor)
    {
    // Get the next prop
    vtkActor *actor = it_actor->second;

    // Get the label of that mesh
    const ColorLabel &cl = driver->GetColorLabelTable()->GetColorLabel(it_actor->first);

    // Get the property
    vtkProperty *prop = actor->GetProperty();

    // Assign the color and opacity
    prop->SetColor(cl.GetRGB(0) / 255.0, cl.GetRGB(1) / 255.0, cl.GetRGB(2) / 255.0);

    if(cl.IsVisibleIn3D())
      prop->SetOpacity(cl.GetAlpha() / 255.0);
    else
      prop->SetOpacity(0.0);
    }
}



class VolumeAssembly : public itk::Object
{
public:
  irisITKObjectMacro(VolumeAssembly, itk::Object)

  // Minipipeline used to import ITK data
  ScalarImageWrapperBase::VTKImporterMiniPipeline ImportPipeline;

  // VTK assembly
  vtkSmartPointer<vtkVolume> Volume;
  vtkSmartPointer<vtkSmartVolumeMapper> Mapper;
  vtkSmartPointer<vtkColorTransferFunction> ColorCurve;
  vtkSmartPointer<vtkVolumeProperty> Property;
  vtkSmartPointer<vtkPiecewiseFunction> OpacityCurve;
  vtkSmartPointer<vtkPiecewiseFunction> GradientCurve;
  vtkSmartPointer<vtkRenderer> Renderer;

  // Update time on the curve
  itk::ModifiedTimeType CurveUpdateTime = 0;
  itk::ModifiedTimeType TransformUpdateTime = 0;

protected:
  VolumeAssembly() {}
  virtual ~VolumeAssembly()
  {
    // Remove this from the renderer when owner gets unloaded to avoid seg fault
    Renderer->RemoveViewProp(this->Volume);
  }
};

void Generic3DRenderer::UpdateVolumeCurves(ImageWrapperBase *layer, VolumeAssembly *va)
{
  auto *sw = layer->GetDefaultScalarRepresentation();
  auto *curve = sw->GetIntensityCurve();
  //auto *nmap = sw->GetNativeIntensityMapping();
  auto *cmap = sw->GetColorMap();

  double imin = sw->GetImageMinAsDouble();
  double imax = sw->GetImageMaxAsDouble();
  unsigned int k = 100;

  va->ColorCurve->RemoveAllPoints();
  va->OpacityCurve->RemoveAllPoints();
  for(unsigned int j = 0; j <= k; j++)
    {
    double t = j * 1.0 / k;
    double i = imin + (imax - imin) * t;
    double x = curve->Evaluate(t);
    auto rgba = cmap->MapIndexToRGBA(x);
    va->ColorCurve->AddRGBPoint(i, rgba[0] / 255., rgba[1] / 255., rgba[2] / 255.);

    // Here we apply additional ramp to the alpha
    double x_ramp = (x < 0) ? 0.0 : (x > 1.0) ? 1.0 : x;
    va->OpacityCurve->AddPoint(i, rgba[3] * x_ramp / 255.);
    }

  va->CurveUpdateTime = std::max(cmap->GetMTime(), curve->GetMTime());
}

void Generic3DRenderer::UpdateVolumeTransform(ImageWrapperBase *layer, VolumeAssembly *va)
{
  auto *sw = layer->GetDefaultScalarRepresentation();
  auto dir = sw->GetImageBase()->GetDirection().GetVnlMatrix().as_matrix();
  auto spc = sw->GetImageBase()->GetSpacing().GetVnlVector();
  auto org = sw->GetImageBase()->GetOrigin().GetVnlVector();

  // Transform applied to the volume is the product of the registration matrix and the
  // image to physical transform
  vnl_matrix_fixed<double, 4, 4> vtk2nii =
      AffineTransformHelper::GetRASMatrix(sw->GetITKTransform()) *
      ImageWrapperBase::ConstructVTKtoNiftiTransform(dir, org, spc);

  vtkNew<vtkMatrix4x4> tran;
  tran->DeepCopy(vtk2nii.data_block());
  va->Volume->SetUserMatrix(tran);

  // Update the timestamp
  va->TransformUpdateTime = sw->GetMTime();
  if(sw->GetITKTransform())
    va->TransformUpdateTime = std::max(va->TransformUpdateTime, sw->GetITKTransform()->GetMTime());
}


void Generic3DRenderer::UpdateVolumeRendering()
{
  // Associate each layer with a volume rendering
  IRISApplication *app = m_Model->GetParentUI()->GetDriver();
  GenericImageData *id = app->GetCurrentImageData();

  // Keep track of volume renderers that are not used
  std::set<vtkVolume *> volumes_to_remove;
  auto *props = m_Renderer->GetViewProps();
  for(int k = 0; k < props->GetNumberOfItems(); k++)
    {
    vtkVolume *vol = dynamic_cast<vtkVolume *>(props->GetItemAsObject(k));
    if(vol)
      volumes_to_remove.insert(vol);
    }

  for(LayerIterator li = id->GetLayers(MAIN_ROLE | OVERLAY_ROLE); !li.IsAtEnd(); ++li)
    {
    auto *layer = li.GetLayer();

    // If the layer is not rendering right now, free up the VR related resources
    if(!layer->GetDefaultScalarRepresentation()->IsVolumeRenderingEnabled())
      {
      layer->SetUserData("volume", nullptr);
      continue;
      }

    // Associate a volume assembly with the layer
    typename VolumeAssembly::Pointer va =
        dynamic_cast<VolumeAssembly *>(layer->GetUserData("volume"));

    if(!va)
      {
      va = VolumeAssembly::New();
      va->ImportPipeline = layer->GetDefaultScalarRepresentation()->CreateVTKImporterPipeline();
      va->Mapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
      va->Mapper->SetInputConnection(va->ImportPipeline.importer->GetOutputPort());

      va->ColorCurve = vtkSmartPointer<vtkColorTransferFunction>::New();
      va->OpacityCurve = vtkSmartPointer<vtkPiecewiseFunction>::New();
      UpdateVolumeCurves(layer, va);

      va->GradientCurve = vtkSmartPointer<vtkPiecewiseFunction>::New();
      va->GradientCurve->AddPoint(0, 0.0);
      va->GradientCurve->AddPoint(90, 0.5);
      va->GradientCurve->AddPoint(100, 1.0);

      va->Property = vtkSmartPointer<vtkVolumeProperty>::New();
      va->Property->SetColor(va->ColorCurve);
      va->Property->SetScalarOpacity(va->OpacityCurve);
      // va->Property->SetGradientOpacity(va->GradientCurve);
      va->Property->SetInterpolationTypeToLinear();
      va->Property->ShadeOn();
      va->Property->SetAmbient(0.4);
      va->Property->SetDiffuse(0.6);
      va->Property->SetSpecular(0.2);

      va->Volume = vtkSmartPointer<vtkVolume>::New();
      va->Volume->SetMapper(va->Mapper);
      va->Volume->SetProperty(va->Property);

      va->Renderer = this->m_Renderer; // when image get removed, volume can remove itself

      this->m_Renderer->AddViewProp(va->Volume);
      layer->SetUserData("volume", va);

      va->ImportPipeline.importer->Update();

      // Update the volume transform
      this->UpdateVolumeTransform(layer, va);
      }
    else
      {
      // Check if the transfer function needs updating
      auto *sw = layer->GetDefaultScalarRepresentation();
      if(sw->GetIntensityCurve()->GetMTime() > va->CurveUpdateTime ||
         sw->GetColorMap()->GetMTime() > va->CurveUpdateTime)
        {
        UpdateVolumeCurves(layer, va);
        }

      // Check if the transform needs updating
      if((sw->GetImageBase()->GetMTime() > va->TransformUpdateTime) ||
         (sw->GetITKTransform() && sw->GetITKTransform()->GetMTime() > va->TransformUpdateTime))
        {
        UpdateVolumeTransform(layer, va);
        }
      }

    // Add the volume to the used volumes set
    if(va)
      volumes_to_remove.erase(va->Volume);
    }

  // Remove unused volumes
  for(auto *prop : volumes_to_remove)
    m_Renderer->RemoveViewProp(prop);
}

void Generic3DRenderer::OnUpdate()
{
  // Update the model first
  m_Model->Update();

  // Access the application
  IRISApplication *app = m_Model->GetParentUI()->GetDriver();
  GlobalState *gs = app->GetGlobalState();

  // Get the mode
  ToolbarMode3DType mode = gs->GetToolbarMode3D();

  // Define a bunch of local flags to make this code easier to read
  bool main_changed = m_EventBucket->HasEvent(MainImageDimensionsChangeEvent());
  bool labels_props_changed = m_EventBucket->HasEvent(SegmentationLabelChangeEvent());
  bool cursor_moved = m_EventBucket->HasEvent(CursorUpdateEvent());
  bool active_label_changed = m_EventBucket->HasEvent(
        ValueChangedEvent(), gs->GetDrawingColorLabelModel());
  bool spray_action = m_EventBucket->HasEvent(Generic3DModel::SprayPaintEvent());
  bool scalpel_action = m_EventBucket->HasEvent(Generic3DModel::ScalpelEvent());
  bool mode_changed = m_EventBucket->HasEvent(
        ValueChangedEvent(), gs->GetToolbarMode3DModel());

  bool appearance_changed =
      m_EventBucket->HasEvent(ChildPropertyChangedEvent(),
                              m_Model->GetParentUI()->GetAppearanceSettings());

  bool seg_layer_changed =
      m_EventBucket->HasEvent(ValueChangedEvent(),
                              gs->GetSelectedSegmentationLayerIdModel());

  bool need_render = false;

  // For volume rendering
  bool layer_mapping_changed =
      m_EventBucket->HasEvent(WrapperDisplayMappingChangeEvent());
  bool wrapper_vr_options_changed =
      m_EventBucket->HasEvent(WrapperVisibilityChangeEvent());

  // Setmentation changes event should be handled when continuous update is on
  bool continuous_update_needed =
      m_Model->GetContinuousUpdate() && (
      m_EventBucket->HasEvent(SegmentationChangeEvent()) ||
      m_EventBucket->HasEvent(LevelSetImageChangeEvent()));

  bool mesh_updated =
      m_EventBucket->HasEvent(ActiveLayerChangeEvent(),
                              app->GetIRISImageData()->GetMeshLayers()) ||
      m_EventBucket->HasEvent(ActiveLayerChangeEvent(),
                              app->GetSNAPImageData()->GetMeshLayers());

  // Need to rebuild the actor map
  bool need_rebuild_actor_map =
      m_EventBucket->HasEvent(CursorTimePointUpdateEvent(),app) ||
      continuous_update_needed ||
      mesh_updated;

  // Without rebuilding the actor map, update the actors inside the map
  bool need_update_actor_color =
      m_EventBucket->HasEvent(WrapperDisplayMappingChangeEvent(),
                              app->GetIRISImageData()->GetMeshLayers());

  // Update visibility of the color bar
  bool color_bar_visiblity_changed =
      m_EventBucket->HasEvent(ValueChangedEvent(),
                              m_Model->GetDisplayColorBarModel());

  // Deal with the updates to the mesh state
  if(main_changed || seg_layer_changed || need_rebuild_actor_map)
    {
    // This line fixes an issue with continuous rendering not working sometimes.
    // The real problem is that this method is being called multiple times with
    // the same event bucket and locking the mesh update. The correct solution
    // is to prevent this by using some kind of threading approach to collect
    // events in buckets before invoking the callback.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    UpdateMeshAssembly();
    need_render = true;
    }
	else if(labels_props_changed)
    {
    UpdateMeshAppearance();
    need_render = true;
    }

  // If the segmentation changed

  // If Existing Mesh Assembly Display Mapping Policy changed
  if (need_update_actor_color)
    {
    ApplyDisplayMappingPolicyChange();
    need_render = true;
    }

  // Deal with axes
  if(main_changed || cursor_moved)
    {
    UpdateAxisRendering();
    need_render = true;
    }

  if(appearance_changed)
    {
    UpdateAxisRendering();
    need_render = true;
    }

  // Deal with camera
  if(main_changed)
    {
    UpdateCamera(true);
    DeleteSavedCameraState();
    need_render = true;
    }
  else if(cursor_moved)
    {
    UpdateCamera(false);
    need_render = true;
    }

  // Deal with the spray paint appearance and shape
  if(main_changed || labels_props_changed || active_label_changed)
    {
    UpdateSprayGlyphAppearanceAndShape();
    UpdateScalpelPlaneAppearance();
    need_render = true;
    }

  // Deal with spray events
  if(main_changed || spray_action || mode_changed)
    {
    if(m_Model->GetSprayPoints()->GetNumberOfPoints() && mode == SPRAYPAINT_MODE)
      this->m_Renderer->AddActor(m_SprayActor);
    else
      this->m_Renderer->RemoveActor(m_SprayActor);
    need_render = true;
    }

  // Deal with scalpel events
  if(main_changed || scalpel_action || mode_changed)
    {
    UpdateScalpelRendering();
    need_render = true;
    }

  // Deal with volume rendering
  if(main_changed || layer_mapping_changed || wrapper_vr_options_changed)
    {
    UpdateVolumeRendering();
    need_render = true;
    }

  // Deal with color bar visibility
  if(color_bar_visiblity_changed)
    {
    UpdateColorLegendAppearance();
    }

  // Force rendering to occur
  if (need_render)
    this->GetRenderWindow()->Render();
}

void Generic3DRenderer::ResetView()
{
  this->UpdateCamera(true);
  InvokeEvent(ModelUpdateEvent());
}

Vector3d Generic3DRenderer::GetScalpelPlaneNormal() const
{
  return Vector3d(m_ScalpelPlaneWidget->GetNormal());
}

Vector3d Generic3DRenderer::GetScalpelPlaneOrigin() const
{
  return Vector3d(m_ScalpelPlaneWidget->GetOrigin());
}

void Generic3DRenderer::FlipScalpelPlaneNormal()
{
  Vector3d n = this->GetScalpelPlaneNormal();
  m_ScalpelPlaneWidget->SetNormal(-n[0], -n[1], -n[2]);
  InvokeEvent(ModelUpdateEvent());
}

void Generic3DRenderer::ComputeRayFromClick(int x, int y, Vector3d &point, Vector3d &ray, Vector3d &dx, Vector3d &dy)
{
  // Get the position of the click in world coordinates
  m_CoordinateMapper->SetValue(x, y, 0);
  point = Vector3d(m_CoordinateMapper->GetComputedWorldValue(this->m_Renderer));

  // Get the normal vector to the viewport
  m_CoordinateMapper->SetValue(x, y, 1);
  ray = Vector3d(m_CoordinateMapper->GetComputedWorldValue(this->m_Renderer)) - point;

  // Get the viewport in-plane vectors
  m_CoordinateMapper->SetValue(x+1, y, 0);
  dx = Vector3d(m_CoordinateMapper->GetComputedWorldValue(this->m_Renderer)) - point;

  // Get the viewport in-plane vectors
  m_CoordinateMapper->SetValue(x, y+1, 0);
  dy = Vector3d(m_CoordinateMapper->GetComputedWorldValue(this->m_Renderer)) - point;
}
