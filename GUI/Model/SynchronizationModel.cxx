#include "SynchronizationModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "SystemInterface.h"
#include "GenericSliceModel.h"
#include "GenericImageData.h"
#include "ImageWrapperBase.h"
#include "SliceWindowCoordinator.h"
#include "Generic3DModel.h"
#include "Generic3DRenderer.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "IPCHandler.h"

/** Structure passed on to IPC */
struct IPCMessage
{
  // The cursor position in world coordinates
  Vector3d cursor;

  // The zoom factor (screen pixels / mm)
  double zoom_level[3];

  // The position of the viewport center relative to cursor
  // in all three slice views
  Vector2f viewPositionRelative[3];

  // 3D camera state
  CameraState camera;

  // Version of the data structure
  enum VersionEnum { VERSION = 0x1005 };
};


SynchronizationModel::SynchronizationModel()
{
  // Create the models
  m_SyncEnabledModel = NewSimpleConcreteProperty(true);
  m_SyncCursorModel = NewSimpleConcreteProperty(true);
  m_SyncZoomModel = NewSimpleConcreteProperty(true);
  m_SyncPanModel = NewSimpleConcreteProperty(true);
  m_SyncCameraModel = NewSimpleConcreteProperty(true);

  m_SyncChannelModel = NewRangedConcreteProperty(1, 1, 99, 1);

  // Create an IPC handler
  m_IPCHandler = new IPCHandler();

  // Warp layer ID is zero by default (no warp)
  m_WarpLayerId = 0;

  // Broadcast state
  m_CanBroadcast = false;

  // Warp layer model
  m_WarpLayerModel = wrapGetterSetterPairAsProperty(
                       this,
                       &Self::GetWarpLayerValueAndRange,
                       &Self::SetWarpLayerValue);
}

SynchronizationModel::~SynchronizationModel()
{
  if(m_IPCHandler->IsAttached())
    m_IPCHandler->Close();
  delete m_IPCHandler;
}

void SynchronizationModel::SetParentModel(GlobalUIModel *parent)
{
  // Set the parent
  m_Parent = parent;
  m_SystemInterface = m_Parent->GetDriver()->GetSystemInterface();

  // Initialize the IPC handler
  m_IPCHandler->Attach(
        m_SystemInterface->GetUserPreferencesFileName(),
        (short) IPCMessage::VERSION, sizeof(IPCMessage));

  // TODO: the defaults should be read from global preferences

  // Listen to the events from the parent that correspond to changes that
  // need to be broadcast, and send them downstream as model update events

  // Cursor changes
  Rebroadcast(m_Parent->GetDriver(), CursorUpdateEvent(), ModelUpdateEvent());

  // Viewpoint geometry changes
  for(int i = 0; i < 3; i++)
    {
    GenericSliceModel *gsm = m_Parent->GetSliceModel(i);
    Rebroadcast(gsm, SliceModelGeometryChangeEvent(), ModelUpdateEvent());
    }

  // Changes to the 3D viewpoint (stored in a vtkCamera object)
  Rebroadcast(m_Parent->GetModel3D()->GetRenderer(),
              Generic3DRenderer::CameraUpdateEvent(), ModelUpdateEvent());

  // Changes to the loaded layers
  Rebroadcast(m_Parent->GetDriver(), LayerChangeEvent(), ModelUpdateEvent());
}


void SynchronizationModel::OnUpdate()
{
  // If there is no synchronization or no image, get out
  IRISApplication *app = m_Parent->GetDriver();
  if(!app->IsMainImageLoaded()
     || !m_SyncEnabledModel->GetValue()
     || !m_CanBroadcast)
    return;

  // Figure out what to broadcast
  bool bc_cursor =
      m_EventBucket->HasEvent(CursorUpdateEvent())
      && m_SyncCursorModel->GetValue();

  bool bc_zoom =
      m_EventBucket->HasEvent(SliceModelGeometryChangeEvent())
      && m_SyncZoomModel->GetValue();

  bool bc_pan =
      (m_EventBucket->HasEvent(SliceModelGeometryChangeEvent())
       || m_EventBucket->HasEvent(CursorUpdateEvent()))
      && m_SyncPanModel->GetValue();

  bool bc_camera =
      m_EventBucket->HasEvent(Generic3DRenderer::CameraUpdateEvent())
      && m_SyncCameraModel->GetValue();

  // Read the contents of shared memory into the local message object
  IPCMessage message;
  m_IPCHandler->Read(static_cast<void *>(&message));

  // Cursor change
  if(bc_cursor)
    {
    // Map the cursor to NIFTI coordinates
    ImageWrapperBase *iw = app->GetCurrentImageData()->GetMain();

    // Get the NIFTI coordinate of the current cursor position
    auto cp = app->GetCursorPosition();
    message.cursor = iw->TransformVoxelCIndexToNIFTICoordinates(to_double(cp));

    // Handle special case of warp fields
    ImageWrapperBase *warp = m_WarpLayerId > 0
        ? app->GetCurrentImageData()->FindLayer(m_WarpLayerId, false) : nullptr;
    if(warp && warp->GetNumberOfComponents() == 3)
      {
      // Look up the warp at this position and add to the RAS coordinate
      vnl_vector<double> voxel(3);
      warp->SampleIntensityAtReferenceIndex(
            to_itkIndex(cp), warp->GetTimePointIndex(), true, voxel);
      for(unsigned int j = 0; j < 3; j++)
        message.cursor[j] += (j < 2 ? -1 : 1) * voxel[j];
      }
    }

  // Zoom/Pan change
  for(int i = 0; i < 3; i++)
    {
    GenericSliceModel *gsm = m_Parent->GetSliceModel(i);
    AnatomicalDirection dir = app->GetAnatomicalDirectionForDisplayWindow(i);

    if(bc_zoom)
      message.zoom_level[dir] = gsm->GetViewZoom();
    if(bc_pan)
      message.viewPositionRelative[dir] = to_float(gsm->GetViewPositionRelativeToCursor());
    }

  // 3D viewpoint
  if(bc_camera)
    {
    // Get the camera state
    CameraState cs = m_Parent->GetModel3D()->GetRenderer()->GetCameraState();
    message.camera = cs;
    }

  // Broadcast the new message
  m_IPCHandler->Broadcast(static_cast<void *>(&message));
}

bool SynchronizationModel
::GetWarpLayerValueAndRange(unsigned long &value, SynchronizationModel::LayerSelectionDomain *range)
{
  // Search for all the layers that may serve as warps
  IRISApplication *app = m_Parent->GetDriver();

  // Calculate the range
  if(range)
    {
    range->clear();
    (*range)[0l] = "None";
    }

  // Keep track of the number of eligible layers
  unsigned int n_warps = 0;
  bool current_found = false;
  LayerIterator it = app->GetCurrentImageData()->GetLayers(OVERLAY_ROLE);
  for(; !it.IsAtEnd(); ++it)
    {
    if(it.GetLayer()->ImageSpaceMatchesReferenceSpace() &&
       it.GetLayer()->GetNumberOfComponents() == 3)
      {
      n_warps++;
      if(range)
        (*range)[it.GetLayer()->GetUniqueId()] = it.GetLayer()->GetNickname();

      if(it.GetLayer()->GetUniqueId() == m_WarpLayerId)
        current_found = true;
      }
    }

  // If there are no warps, there is nothing to choose from
  if(n_warps == 0 || (m_WarpLayerId > 0 && !current_found))
    {
    m_WarpLayerId = 0;
    value = 0;
    return false;
    }

  value = m_WarpLayerId;
  return true;
}


void SynchronizationModel::SetWarpLayerValue(unsigned long value)
{
  // Set the layer id
  m_WarpLayerId = value;
}


void SynchronizationModel::ReadIPCState()
{
  IRISApplication *app = m_Parent->GetDriver();
  if(!app->IsMainImageLoaded() || !m_SyncCursorModel->GetValue())
    return;

  // Read the IPC message
  IPCMessage message;
  if(m_IPCHandler->ReadIfNew(static_cast<void *>(&message)))
    {
    if(m_SyncCursorModel->GetValue())
      {
      // Map the cursor position to the image coordinates
      GenericImageData *id = app->GetCurrentImageData();
      Vector3d vox =
          id->GetMain()->TransformNIFTICoordinatesToVoxelCIndex(message.cursor);

      // Round the cursor to integer value
      itk::Index<3> pos; Vector3ui vpos;
      pos[0] = vpos[0] = (unsigned int) (vox[0] + 0.5);
      pos[1] = vpos[1] = (unsigned int) (vox[1] + 0.5);
      pos[2] = vpos[2] = (unsigned int) (vox[2] + 0.5);

      // Check if the voxel position is inside the image region
      if(vpos != app->GetCursorPosition() && id->GetImageRegion().IsInside(pos))
        {
        app->SetCursorPosition(vpos);
        }
      }

    // Set the zoom/pan levels
    for(int i = 0; i < 3; i++)
      {
      GenericSliceModel *gsm = m_Parent->GetSliceModel(i);
      AnatomicalDirection dir = app->GetAnatomicalDirectionForDisplayWindow(i);

      if(m_SyncZoomModel->GetValue()
         && gsm->IsSliceInitialized()
         && gsm->GetViewZoom() != message.zoom_level[dir]
         && static_cast<float>(message.zoom_level[dir]) > 0.0f)
        {
          gsm->SetViewZoom(message.zoom_level[dir]);
        }

      if(m_SyncPanModel->GetValue()
         && gsm->IsSliceInitialized()
         && to_float(gsm->GetViewPositionRelativeToCursor()) != message.viewPositionRelative[dir])
        {
        gsm->SetViewPositionRelativeToCursor(to_double(message.viewPositionRelative[dir]));
        }
      }

    // Set the camera state
    if(m_SyncCameraModel->GetValue())
      {
      // Get the currently used 3D camera
      CameraState cs = message.camera;
      m_Parent->GetModel3D()->GetRenderer()->SetCameraState(message.camera);
      }
    }
}




