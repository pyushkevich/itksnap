#ifndef GENERIC3DMODEL_H
#define GENERIC3DMODEL_H

#include "AbstractModel.h"
#include "PropertyModel.h"
#include "vtkSmartPointer.h"
#include "SNAPEvents.h"
#include "itkMutexLock.h"

class GlobalUIModel;
class IRISApplication;
class MeshManager;
class Generic3DRenderer;
class vtkPolyData;
class MeshExportSettings;

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
  itkEventMacro(ScalpelEvent, IRISEvent)
  FIRES(SprayPaintEvent)
  FIRES(ScalpelEvent)
  FIRES(StateMachineChangeEvent)

  // Matrix for various transforms
  typedef vnl_matrix_fixed<double, 4, 4> Mat4d;

  Generic3DModel();

  // States pertaining to this model
  enum UIState {
    UIF_MESH_DIRTY = 0,
    UIF_MESH_ACTION_PENDING,
    UIF_CAMERA_STATE_SAVED,
    UIF_FLIP_ENABLED
  };

  // State of scalpel drawging
  enum ScalpelStatus {
    SCALPEL_LINE_NULL = 0,
    SCALPEL_LINE_STARTED,
    SCALPEL_LINE_COMPLETED
  };

  // Set the parent model
  void Initialize(GlobalUIModel *parent);

  // Check the state
  bool CheckState(UIState state);

  // A flag indicating that the mesh should be continually updated
  // TODO: replace this with an update in a background thread
  irisSimplePropertyAccessMacro(ContinuousUpdate, bool)

  // Tell the model to update the segmentation mesh
  void UpdateSegmentationMesh(itk::Command *callback);

  // Reentrant function to check if mesh is being constructed in another thread
  bool IsMeshUpdating();

  // Accept the current drawing operation
  bool AcceptAction();

  // Cancel the current drawing operation
  void CancelAction();

  // Flip the direction of the normal for the scalpel operation
  void FlipAction();

  // Clear the rendering
  void ClearRenderingAction();

  // Position cursor at the screen position under the cursor
  bool PickSegmentationVoxelUnderMouse(int px, int py);

  // Add a spraypaint bubble at the screen position under the cursor
  bool SpraySegmentationVoxelUnderMouse(int px, int py);

  // Set the endpoints of the scalpel line
  void SetScalpelStartPoint(int px, int py);
  irisGetMacro(ScalpelStart, Vector2i)

  // Set the endpoints of the scalpel line
  void SetScalpelEndPoint(int px, int py, bool complete);
  irisGetMacro(ScalpelEnd, Vector2i)

  irisGetSetMacro(ScalpelStatus, ScalpelStatus)

  // Get the parent model
  irisGetMacro(ParentUI, GlobalUIModel *)

  // Get the renderer
  irisGetMacro(Renderer, Generic3DRenderer *)

  // Get the transform from image space to world coordinates
  Mat4d &GetWorldMatrix();

  // Get the center of rotation for the 3D window
  Vector3d GetCenterOfRotation();

  // Reset the viewpoint
  void ResetView();

  // Save the camera state
  void SaveCameraState();

  // Restore the camera state
  void RestoreCameraState();

  // Export the 3D model
  void ExportMesh(const MeshExportSettings &settings);

  // Get the spray points
  vtkPolyData *GetSprayPoints() const;

protected:

  // Respond to updates
  void OnUpdate();

  // Do this when main image geometry has changed
  void OnImageGeometryUpdate();

  // Find the labeled voxel under the cursor
  bool IntersectSegmentation(int vx, int vy, Vector3i &hit);

  // Parent (where the global UI state is stored)
  GlobalUIModel *m_ParentUI;

  // Renderer
  SmartPtr<Generic3DRenderer> m_Renderer;

  // Helps to have a pointer to the iris application
  IRISApplication *m_Driver;

  // World matrix - a copy of the NIFTI transform in the main image,
  // updated on the event main image changes
  Mat4d m_WorldMatrix, m_WorldMatrixInverse;

  // Set of spraypainted points in image coordinates
  vtkSmartPointer<vtkPolyData> m_SprayPoints;

  // On-screen endpoints of the scalpel line
  Vector2i m_ScalpelStart, m_ScalpelEnd;

  // State of the scalpel drawing
  ScalpelStatus m_ScalpelStatus;

  // Continuous update model
  SmartPtr<ConcreteSimpleBooleanProperty> m_ContinuousUpdateModel;

  // Is the mesh updating
  bool m_MeshUpdating;

  // Time of the last mesh clear operation
  unsigned long m_ClearTime;

  // A mutex lock to allow background processing of mesh updates
  itk::SimpleFastMutexLock m_MutexLock;

};

#endif // GENERIC3DMODEL_H
