#include "Generic3DModel.h"
#include "Generic3DRenderer.h"
#include "GlobalUIModel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ImageWrapperBase.h"
#include "MeshManager.h"
#include "Window3DPicker.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPointData.h"
#include "itkMutexLockHolder.h"
#include "MeshOptions.h"

// All the VTK stuff
#include "vtkPolyData.h"

#include <vnl/vnl_inverse.h>

Generic3DModel::Generic3DModel()
{
  // Initialize the matrix to nil
  m_WorldMatrix.set_identity();

  // Create the spray points
  vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
  m_SprayPoints = vtkSmartPointer<vtkPolyData>::New();
  m_SprayPoints->SetPoints(pts);

  // Create the renderer
  m_Renderer = Generic3DRenderer::New();

  // Continuous update model
  m_ContinuousUpdateModel = NewSimpleConcreteProperty(false);

  // Scalpel
  m_ScalpelStatus = SCALPEL_LINE_NULL;

  // Reset clear time
  m_ClearTime = 0;
}

#include "itkImage.h"

void Generic3DModel::Initialize(GlobalUIModel *parent)
{
  // Store the parent
  m_ParentUI = parent;
  m_Driver = parent->GetDriver();

  // Update our geometry model
  OnImageGeometryUpdate();

  // Initialize the renderer
  m_Renderer->SetModel(this);

  // Listen to the layer change events
  Rebroadcast(m_Driver, MainImageDimensionsChangeEvent(), ModelUpdateEvent());

  // Listen to segmentation change events
  Rebroadcast(m_Driver, SegmentationChangeEvent(), StateMachineChangeEvent());
  Rebroadcast(m_Driver, LevelSetImageChangeEvent(), StateMachineChangeEvent());

  // Rebroadcast model change events as state changes
  Rebroadcast(this, ModelUpdateEvent(), StateMachineChangeEvent());
  Rebroadcast(this, SprayPaintEvent(), StateMachineChangeEvent());
  Rebroadcast(this, ScalpelEvent(), StateMachineChangeEvent());
  Rebroadcast(m_ParentUI->GetGlobalState()->GetToolbarMode3DModel(),
              ValueChangedEvent(), StateMachineChangeEvent());
  Rebroadcast(m_ParentUI->GetGlobalState()->GetMeshOptions(),
              ChildPropertyChangedEvent(), StateMachineChangeEvent());
}

bool Generic3DModel::CheckState(Generic3DModel::UIState state)
{
  if(!m_ParentUI->GetDriver()->IsMainImageLoaded())
    return false;

  ToolbarMode3DType mode = m_ParentUI->GetGlobalState()->GetToolbarMode3D();

  switch(state)
    {
    case UIF_MESH_DIRTY:
      {
      if(m_Driver->GetMeshManager()->IsMeshDirty())
        return true;

      if(m_Driver->GetMeshManager()->GetBuildTime() <= this->m_ClearTime)
        return true;

      return false;
      }

    case UIF_MESH_ACTION_PENDING:
      {
      if(mode == SPRAYPAINT_MODE)
        return m_SprayPoints->GetNumberOfPoints() > 0;

      else if (mode == SCALPEL_MODE)
        return m_ScalpelStatus == SCALPEL_LINE_COMPLETED;

      else return false;
      }

    case UIF_CAMERA_STATE_SAVED:
      {
      return m_Renderer->IsSavedCameraStateAvailable();
      }

    case UIF_FLIP_ENABLED:
      {
      return mode == SCALPEL_MODE && m_ScalpelStatus == SCALPEL_LINE_COMPLETED;
      }
    }

  return false;
}

Generic3DModel::Mat4d &Generic3DModel::GetWorldMatrix()
{
  return m_WorldMatrix;
}


Vector3d Generic3DModel::GetCenterOfRotation()
{
  return affine_transform_point(m_WorldMatrix, m_Driver->GetCursorPosition());
}

void Generic3DModel::ResetView()
{
  m_Renderer->ResetView();
}

void Generic3DModel::SaveCameraState()
{
  m_Renderer->SaveCameraState();
  InvokeEvent(StateMachineChangeEvent());
}

void Generic3DModel::RestoreCameraState()
{
  m_Renderer->RestoreSavedCameraState();
}

#include "MeshExportSettings.h"
#include "vtkVRMLExporter.h"
void Generic3DModel::ExportMesh(const MeshExportSettings &settings)
{
  // Update the mesh
  this->UpdateSegmentationMesh(m_ParentUI->GetProgressCommand());

  // Prevent concurrent access to this method and mesh update
  itk::MutexLockHolder<itk::SimpleFastMutexLock> mholder(m_MutexLock);

  // Certain formats require a VTK exporter and use a render window. They
  // are handled directly in this code, rather than in the Guided code.
  // TODO: it would make sense to unify this functionality in GuidedMeshIO
  GuidedMeshIO mesh_io;
  Registry reg_format = settings.GetMeshFormat();
  if(mesh_io.GetFileFormat(reg_format) == GuidedMeshIO::FORMAT_VRML)
    {
    // Create the exporter
    vtkSmartPointer<vtkVRMLExporter> exporter = vtkSmartPointer<vtkVRMLExporter>::New();
    exporter->SetFileName(settings.GetMeshFileName().c_str());
    exporter->SetInput(m_Renderer->GetRenderWindow());
    exporter->Update();
    return;
    }

  // Export the mesh
  m_ParentUI->GetDriver()->ExportSegmentationMesh(
        settings, m_ParentUI->GetProgressCommand());
}

vtkPolyData *Generic3DModel::GetSprayPoints() const
{
  return m_SprayPoints.GetPointer();
}

void Generic3DModel::OnUpdate()
{
  // If we experienced a change in main image, we have to respond!
  if(m_EventBucket->HasEvent(MainImageDimensionsChangeEvent()))
    {
    // There is no more mesh to render - until the user does something!
    // m_Mesh->DiscardVTKMeshes();

    // Clear the spray points
    m_SprayPoints->GetPoints()->Reset();
    m_SprayPoints->Modified();

    // The geometry has changed
    this->OnImageGeometryUpdate();
    }
}

void Generic3DModel::OnImageGeometryUpdate()
{
  // Update the world matrix and other stored variables
  if(m_Driver->IsMainImageLoaded())
    {
    ImageWrapperBase *main = m_Driver->GetCurrentImageData()->GetMain();
    m_WorldMatrix = main->GetNiftiSform();
    m_WorldMatrixInverse = main->GetNiftiInvSform();
    }
  else
    {
    m_WorldMatrix.set_identity();
    m_WorldMatrixInverse.set_identity();
    }
}

#include "itkMutexLockHolder.h"

void Generic3DModel::UpdateSegmentationMesh(itk::Command *callback)
{
  // Prevent concurrent access to this method
  itk::MutexLockHolder<itk::SimpleFastMutexLock> mholder(m_MutexLock);

  try
  {
    // Generate all the mesh objects
    m_MeshUpdating = true;
    m_Driver->GetMeshManager()->UpdateVTKMeshes(callback);
    m_MeshUpdating = false;

    InvokeEvent(ModelUpdateEvent());
  }
  catch(vtkstd::bad_alloc &)
  {
    throw IRISException("Out of memory during mesh computation");
  }
  catch(IRISException & IRISexc)
  {
    throw IRISexc;
  }
}

bool Generic3DModel::IsMeshUpdating()
{
  return m_MeshUpdating;
}

bool Generic3DModel::AcceptAction()
{
  ToolbarMode3DType mode = m_ParentUI->GetGlobalState()->GetToolbarMode3D();
  IRISApplication *app = m_ParentUI->GetDriver();

  // Accept the current action
  if(mode == SPRAYPAINT_MODE)
    {
    // Merge all the spray points into the segmentation
    app->BeginSegmentationUpdate("3D spray paint");
    for(int i = 0; i < m_SprayPoints->GetNumberOfPoints(); i++)
      {
      double *x = m_SprayPoints->GetPoint(i);
      Vector3ui pos(
            static_cast<unsigned int>(x[0]),
            static_cast<unsigned int>(x[1]),
            static_cast<unsigned int>(x[2]));
      app->UpdateSegmentationVoxel(pos);
      }

    // Clear the spray points
    m_SprayPoints->GetPoints()->Reset();
    m_SprayPoints->Modified();
    InvokeEvent(SprayPaintEvent());

    // Return true if anything changed
    return app->EndSegmentationUpdate() > 0;
    }
  else if(mode == SCALPEL_MODE && m_ScalpelStatus == SCALPEL_LINE_COMPLETED)
    {
    // Get the plane origin and normal in world coordinates
    Vector3d xw = m_Renderer->GetScalpelPlaneOrigin();
    Vector3d nw = m_Renderer->GetScalpelPlaneNormal();

    // Map these properties into the image coordinates
    Vector3d xi = affine_transform_point(m_WorldMatrixInverse, xw);
    Vector3d ni = affine_transform_vector(m_WorldMatrixInverse, nw);

    // Use the driver to relabel the plane
    app->BeginSegmentationUpdate("3D scalpel");
    app->RelabelSegmentationWithCutPlane(ni, dot_product(xi, ni));
    int nMod = app->EndSegmentationUpdate();

    // Reset the scalpel state, but only if the operation was successful
    if(nMod > 0)
      {
      m_ScalpelStatus = SCALPEL_LINE_NULL;
      InvokeEvent(ScalpelEvent());
      return true;
      }
    else return false;
    }
  return true;
}

void Generic3DModel::CancelAction()
{
  ToolbarMode3DType mode = m_ParentUI->GetGlobalState()->GetToolbarMode3D();
  if(mode == SPRAYPAINT_MODE)
    {
    // Clear the spray points
    m_SprayPoints->GetPoints()->Reset();
    m_SprayPoints->Modified();
    InvokeEvent(SprayPaintEvent());
    }
  else if(mode == SCALPEL_MODE && m_ScalpelStatus == SCALPEL_LINE_COMPLETED)
    {
    // Reset the scalpel state
    m_ScalpelStatus = SCALPEL_LINE_NULL;
    InvokeEvent(ScalpelEvent());
    }
}

void Generic3DModel::FlipAction()
{
  ToolbarMode3DType mode = m_ParentUI->GetGlobalState()->GetToolbarMode3D();
  if(mode == SCALPEL_MODE && m_ScalpelStatus == SCALPEL_LINE_COMPLETED)
    {
    m_Renderer->FlipScalpelPlaneNormal();
    }
}

void Generic3DModel::ClearRenderingAction()
{
  m_Renderer->ClearRendering();
  m_ClearTime = m_Driver->GetMeshManager()->GetBuildTime();
  InvokeEvent(ModelUpdateEvent());
}

#include "ImageRayIntersectionFinder.h"
#include "SNAPImageData.h"

/** These classes are used internally for m_Ray intersection testing */
class LabelImageHitTester
{
public:
  LabelImageHitTester(const ColorLabelTable *table = NULL)
  {
    m_LabelTable = table;
  }

  int operator()(LabelType label) const
  {
    const ColorLabel &cl = m_LabelTable->GetColorLabel(label);
    return (cl.IsVisible() && cl.IsVisibleIn3D()) ? 1 : 0;
  }

private:
  const ColorLabelTable *m_LabelTable;
};

class SnakeImageHitTester
{
public:
  int operator()(float levelSetValue) const
    { return levelSetValue <= 0 ? 1 : 0; }
};

bool Generic3DModel::IntersectSegmentation(int vx, int vy, Vector3i &hit)
{
  // World coordinate of the click position and direction
  Vector3d x_world, d_world;
  m_Renderer->ComputeRayFromClick(vx, vy, x_world, d_world);

  // Convert these to image coordinates
  Vector3d x_image = affine_transform_point(m_WorldMatrixInverse, x_world);
  Vector3d d_image = affine_transform_vector(m_WorldMatrixInverse, d_world);

  int result = 0;
  if(m_Driver->IsSnakeModeLevelSetActive())
    {
    typedef ImageRayIntersectionFinder<float, SnakeImageHitTester> RayCasterType;
    RayCasterType caster;
    result = caster.FindIntersection(
          m_ParentUI->GetDriver()->GetSNAPImageData()->GetSnake()->GetImage(),
          x_image, d_image, hit);
    }
  else
    {
    typedef ImageRayIntersectionFinder<LabelType, LabelImageHitTester> RayCasterType;
    RayCasterType caster;
    LabelImageHitTester tester(m_ParentUI->GetDriver()->GetColorLabelTable());
    caster.SetHitTester(tester);
    result = caster.FindIntersection(
          m_ParentUI->GetDriver()->GetCurrentImageData()->GetSegmentation()->GetImage(),
          x_image, d_image, hit);
    }

  return (result == 1);
}

bool Generic3DModel::PickSegmentationVoxelUnderMouse(int px, int py)
{
  // Find the voxel under the cursor
  Vector3i hit;
  if(this->IntersectSegmentation(px, py, hit))
    {
    Vector3ui cursor = to_unsigned_int(hit);

    itk::ImageRegion<3> region = m_Driver->GetCurrentImageData()->GetImageRegion();
    if(region.IsInside(to_itkIndex(cursor)))
      {
      m_Driver->SetCursorPosition(cursor);
      return true;
      }
    }

  return false;
}

bool Generic3DModel::SpraySegmentationVoxelUnderMouse(int px, int py)
{
  // Find the voxel under the cursor
  Vector3i hit;
  if(this->IntersectSegmentation(px, py, hit))
    {
    itk::ImageRegion<3> region = m_Driver->GetCurrentImageData()->GetImageRegion();
    if(region.IsInside(to_itkIndex(hit)))
      {
      m_SprayPoints->GetPoints()->InsertNextPoint(hit[0], hit[1], hit[2]);
      m_SprayPoints->Modified();
      this->InvokeEvent(SprayPaintEvent());
      return true;
      }
    }

  return false;
}

void Generic3DModel::SetScalpelStartPoint(int px, int py)
{
  m_ScalpelEnd[0] = m_ScalpelStart[0] = px;
  m_ScalpelEnd[1] = m_ScalpelStart[1] = py;
  m_ScalpelStatus = SCALPEL_LINE_STARTED;
  this->InvokeEvent(ScalpelEvent());
}

void Generic3DModel::SetScalpelEndPoint(int px, int py, bool complete)
{
  m_ScalpelEnd[0] = px;
  m_ScalpelEnd[1] = py;
  if(complete)
    m_ScalpelStatus = SCALPEL_LINE_COMPLETED;
  this->InvokeEvent(ScalpelEvent());
}


