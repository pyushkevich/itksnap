#ifndef GENERIC3DMODEL_H
#define GENERIC3DMODEL_H

#include "AbstractModel.h"
#include "vtkSmartPointer.h"
#include "SNAPEvents.h"

class GlobalUIModel;
class IRISApplication;
class MeshObject;
class Generic3DRenderer;
class vtkPolyData;

namespace itk
{
class Command;
}

class Generic3DModel : public AbstractModel
{
public:

  irisITKObjectMacro(Generic3DModel, AbstractModel)

  // Special events
  itkEventMacro(SprayPaintEvent, IRISEvent)
  FIRES(SprayPaintEvent)

  // Matrix for various transforms
  typedef vnl_matrix_fixed<double, 4, 4> Mat4d;

  Generic3DModel();

  // Set the parent model
  void Initialize(GlobalUIModel *parent);

  // Tell the model to update the segmentation mesh
  void UpdateSegmentationMesh(itk::Command *callback);

  // Accept the current drawing operation
  bool AcceptAction();

  // Position cursor at the screen position under the cursor
  bool PickSegmentationVoxelUnderMouse(int px, int py);

  // Add a spraypaint bubble at the screen position under the cursor
  bool SpraySegmentationVoxelUnderMouse(int px, int py);

  // Get the parent model
  irisGetMacro(ParentUI, GlobalUIModel *)

  // Get the renderer
  irisGetMacro(Renderer, Generic3DRenderer *)

  // Get the mesh object
  irisGetMacro(Mesh, MeshObject *)

  // Get the transform from image space to world coordinates
  Mat4d &GetWorldMatrix();

  // Get the center of rotation for the 3D window
  Vector3d GetCenterOfRotation();

  // Reset the viewpoint
  void ResetView();

  // Get the spray points
  vtkPolyData *GetSprayPoints() const;

protected:

  // Respond to updates
  void OnUpdate();

  // Do this when main image geometry has changed
  void OnImageGeometryUpdate();

  // Parent (where the global UI state is stored)
  GlobalUIModel *m_ParentUI;

  // Renderer
  SmartPtr<Generic3DRenderer> m_Renderer;

  // Helps to have a pointer to the iris application
  IRISApplication *m_Driver;

  // Mesh object - used to handle all mesh operations
  SmartPtr<MeshObject> m_Mesh;

  // World matrix - a copy of the NIFTI transform in the main image,
  // updated on the event main image changes
  Mat4d m_WorldMatrix;

  // Set of spraypainted points in image coordinates
  vtkSmartPointer<vtkPolyData> m_SprayPoints;

};

#endif // GENERIC3DMODEL_H
