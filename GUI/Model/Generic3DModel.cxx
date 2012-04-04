#include "Generic3DModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ImageWrapperBase.h"
#include "MeshObject.h"


// All the VTK stuff
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkProperty.h"

Generic3DModel::Generic3DModel()
{
  // Create the mesh object
  m_Mesh = MeshObject::New();

  // Initialize the matrix to nil
  m_WorldMatrix.set_identity();

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

  // Listen to the layer change events
  Rebroadcast(m_Driver, LayerChangeEvent(), ModelUpdateEvent());

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

void Generic3DModel::OnUpdate()
{
  // If we experienced a change in main image, we have to respond!
  if(m_EventBucket->HasEvent(LayerChangeEvent()))
    {
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
  if(m_Driver->GetCurrentImageData()->IsMainLoaded())
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
  catch(vtkstd::bad_alloc &exc)
  {
    throw IRISException("Out of memory during mesh computation");
  }
}

