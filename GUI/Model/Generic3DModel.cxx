#include "Generic3DModel.h"
#include "Generic3DRenderer.h"
#include "GlobalUIModel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ImageWrapperBase.h"
#include "MeshObject.h"
#include "Window3DPicker.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPointData.h"

// All the VTK stuff
#include "vtkPolyData.h"

#include <vnl/vnl_inverse.h>

Generic3DModel::Generic3DModel()
{
  // Create the mesh object
  m_Mesh = MeshObject::New();

  // Initialize the matrix to nil
  m_WorldMatrix.set_identity();

  // Create the spray points
  vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
  m_SprayPoints = vtkSmartPointer<vtkPolyData>::New();
  m_SprayPoints->SetPoints(pts);

  // Create the renderer
  m_Renderer = Generic3DRenderer::New();
}

void Generic3DModel::Initialize(GlobalUIModel *parent)
{
  // Store the parent
  m_ParentUI = parent;
  m_Driver = parent->GetDriver();

  // Initialize the mesh source
  m_Mesh->Initialize(m_Driver);

  // Update our geometry model
  OnImageGeometryUpdate();

  // Initialize the renderer
  m_Renderer->SetModel(this);

  // Listen to the layer change events
  Rebroadcast(m_Driver, MainImageDimensionsChangeEvent(), ModelUpdateEvent());

  // Listen to segmentation change events
  Rebroadcast(m_Driver, SegmentationChangeEvent(), ModelUpdateEvent());
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
    m_Mesh->DiscardVTKMeshes();

    // Clear the spray points
    m_SprayPoints->GetPoints()->Reset();
    m_SprayPoints->Modified();

    // The geometry has changed
    this->OnImageGeometryUpdate();
    }
  if(m_EventBucket->HasEvent(SegmentationChangeEvent()))
    {
    // Segmentation changed - this means that the mesh object is dirty.
    // But we don't update it automatically because that would be way too
    // slow. Instead, we need the user to ask for a re-rendering!

    }
}

void Generic3DModel::OnImageGeometryUpdate()
{
  // Update the world matrix and other stored variables
  if(m_Driver->IsMainImageLoaded())
    m_WorldMatrix = m_Driver->GetCurrentImageData()->GetMain()->GetNiftiSform();
  else
    m_WorldMatrix.set_identity();
}

void Generic3DModel::UpdateSegmentationMesh(itk::Command *callback)
{
  try
  {
    // Generate all the mesh objects
    m_Mesh->DiscardVTKMeshes();
    m_Mesh->GenerateVTKMeshes(callback);
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

bool Generic3DModel::AcceptAction()
{
  // Accept the current action
  if(m_ParentUI->GetToolbarMode3D() == SPRAYPAINT_MODE)
    {
    IRISApplication *app = m_ParentUI->GetDriver();

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
  return true;
}

bool Generic3DModel::PickSegmentationVoxelUnderMouse(int px, int py)
{
  // Get the picker
  vtkRenderWindowInteractor *rwi = this->GetRenderer()->GetRenderWindowInteractor();
  Window3DPicker *picker = Window3DPicker::SafeDownCast(rwi->GetPicker());

  // Perform the pick operation
  picker->Pick(px, py, 0, rwi->GetRenderWindow()->GetRenderers()->GetFirstRenderer());

  // If the pick is successful, set the cursor position
  if(picker->IsPickSuccessful())
    {
    itk::ImageRegion<3> region = m_Driver->GetCurrentImageData()->GetImageRegion();
    if(region.IsInside(to_itkIndex(picker->GetPickPosition())))
      {
      m_Driver->SetCursorPosition(to_unsigned_int(picker->GetPickPosition()));
      return true;
      }
    }

  return false;
}

bool Generic3DModel::SpraySegmentationVoxelUnderMouse(int px, int py)
{
  // Get the picker
  vtkRenderWindowInteractor *rwi = this->GetRenderer()->GetRenderWindowInteractor();
  Window3DPicker *picker = Window3DPicker::SafeDownCast(rwi->GetPicker());

  // Perform the pick operation
  picker->Pick(px, py, 0, rwi->GetRenderWindow()->GetRenderers()->GetFirstRenderer());

  // If the pick is successful, add a spray point
  if(picker->IsPickSuccessful())
    {
    Vector3i pos = picker->GetPickPosition();
    ImageWrapperBase *main = m_Driver->GetCurrentImageData()->GetMain();
    if(main->GetBufferedRegion().IsInside(to_itkIndex(pos)))
      {
      // Add the point to the spray points
      Vector3d xPick = to_double(pos);
      m_SprayPoints->GetPoints()->InsertNextPoint(xPick.data_block());

      m_SprayPoints->Modified();
      this->InvokeEvent(SprayPaintEvent());
      }
    }

  // Invoke a spray paint event - the renderer needs to know
  return true;
}

