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

  // Warp layer ID is zero by default (no warp)
  m_WarpLayerId = 0;

  // Broadcast state
  m_CanBroadcast = false;

  // Warp layer model
  m_WarpLayerModel = wrapGetterSetterPairAsProperty(
                       this,
                       &Self::GetWarpLayerValueAndRange,
                       &Self::SetWarpLayerValue);

  // Rebroadcast changes from the sync enabled model
  Rebroadcast(m_SyncEnabledModel, ValueChangedEvent(), ModelUpdateEvent());
}

SynchronizationModel::~SynchronizationModel()
{
  if (m_DebugSync)
  {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    cout << "PID " << std::setw(6) << m_IPCHandler->GetProcessID() << "    "
         << std::put_time(std::localtime(&now_time_t), "%X")
         << "    SynchronizationModel::~SynchronizationModel" << endl;
  }

  if(m_IPCHandler && m_IPCHandler->IsAttached())
    {
    if (m_DebugSync)
      cout << "  *Detaching* from IPC" << endl;
    m_IPCHandler->Detach();
    }

  delete m_IPCHandler;
}

void SynchronizationModel::SetParentModel(GlobalUIModel *parent)
{
  // Set the parent
  m_Parent = parent;
  m_SystemInterface = m_Parent->GetDriver()->GetSystemInterface();

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

  // Changes to main image dimensions have to be rebroadcast
  Rebroadcast(m_Parent->GetDriver(), MainImageDimensionsChangeEvent(), ModelUpdateEvent());
}

void
SynchronizationModel::SetSystemInterface(AbstractSharedMemorySystemInterface *si)
{
  m_IPCHandler = new IPCHandler(si);
}

void
SynchronizationModel::ForceDetach()
{
  if (m_DebugSync)
  {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    cout << "PID " << std::setw(6) << m_IPCHandler->GetProcessID() << "    "
         << std::put_time(std::localtime(&now_time_t), "%X")
         << "    SynchronizationModel::ForceDetach" << endl;
  }

  if(m_IPCHandler && m_IPCHandler->IsAttached())
  {
    if (m_DebugSync)
      cout << "  *Detaching* from IPC" << endl;
    m_IPCHandler->Detach();
  }

}

void
SynchronizationModel::OnUpdate()
{
  IRISApplication *app = m_Parent->GetDriver();
  bool             bc_main_update = m_EventBucket->HasEvent(MainImageDimensionsChangeEvent());
  bool bc_sync_state_changed = m_EventBucket->HasEvent(ValueChangedEvent(), m_SyncEnabledModel);
  bool do_sync = m_SyncEnabledModel->GetValue();
  bool have_main = app->IsMainImageLoaded();

  if (m_DebugSync)
  {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    cout << "PID " << std::setw(6) << m_IPCHandler->GetProcessID() << "    "
         << std::put_time(std::localtime(&now_time_t), "%X")
         << "    SynchronizationModel::OnUpdate" << endl;

    cout << "  Event bucket: " << *m_EventBucket << endl;
    cout << "  Flags: "
         << "    bc_main_update=" << (bc_main_update ? 1 : 0)
         << "    have_main=" << (have_main ? 1 : 0) << "    do_sync=" << (do_sync ? 1 : 0)
         << "    bc_sync_state_changed=" << (bc_sync_state_changed ? 1 : 0)
         << "    can_broadcast=" << (m_CanBroadcast ? 1 : 0) << endl;
  }

  // If we are not participating in sync, or there is no main image, we should make sure that
  // we are detached from the shared memory and not proceed further.
  if (!do_sync || !have_main)
  {
    if (m_IPCHandler->IsAttached())
    {
      if (m_DebugSync)
        cout << "  *Detaching from IPC*" << endl;
      m_IPCHandler->Detach();
    }

    return;
  }

  // Behavior depends greatly on whether the main image has been loaded or not
  if (bc_main_update || bc_sync_state_changed)
  {
    if (!m_IPCHandler->IsAttached())
    {
      // Main image has just been loaded. We should attach to the shared memory and
      // read the shared memory state if it exists.
      auto status = m_IPCHandler->Attach(m_SystemInterface->GetUserPreferencesFileName(),
                                         (short)IPCMessage::VERSION,
                                         sizeof(IPCMessage));

      if (m_DebugSync)
      {
        cout << "  *Attaching to IPC*, return status: "
             << (status == IPCHandler::IPC_ATTACHED
                   ? "IPC_ATTACHED"
                   : (status == IPCHandler::IPC_CREATED ? "IPC_CREATED" : "IPC_ERROR"))
             << endl;
      }

      // If we attached to an existing session, then update from that session.
      if (status == IPCHandler::IPC_ATTACHED)
      {
        ReadIPCState(false);
      }
    }
  }

  // If we reached this point, either there has not been a change in main image and
  // not a change in sync state; or a main image has been loaded, but shared memory
  // was created. In both of these situations, we want to broadcast our current state
  // to other ITK-SNAP sessions.
  if (!m_CanBroadcast)
    return;

  // Figure out what to broadcast
  bool bc_cursor = m_EventBucket->HasEvent(CursorUpdateEvent()) && m_SyncCursorModel->GetValue();

  bool bc_zoom =
    m_EventBucket->HasEvent(SliceModelGeometryChangeEvent()) && m_SyncZoomModel->GetValue();

  bool bc_pan = (m_EventBucket->HasEvent(SliceModelGeometryChangeEvent()) ||
                 m_EventBucket->HasEvent(CursorUpdateEvent())) &&
                m_SyncPanModel->GetValue();

  bool bc_camera = m_EventBucket->HasEvent(Generic3DRenderer::CameraUpdateEvent()) &&
                   m_SyncCameraModel->GetValue();

  // Read the contents of shared memory into the local message object
  IPCMessage message;
  m_IPCHandler->Read(static_cast<void *>(&message));

  // Cursor change
  if (bc_cursor)
  {
    // Map the cursor to NIFTI coordinates
    ImageWrapperBase *iw = app->GetCurrentImageData()->GetMain();

    // Get the NIFTI coordinate of the current cursor position
    auto cp = app->GetCursorPosition();
    message.cursor = iw->TransformVoxelCIndexToNIFTICoordinates(to_double(cp));

    // Handle special case of warp fields
    ImageWrapperBase *warp =
      m_WarpLayerId > 0 ? app->GetCurrentImageData()->FindLayer(m_WarpLayerId, false) : nullptr;
    if (warp && warp->GetNumberOfComponents() == 3)
    {
      // Look up the warp at this position and add to the RAS coordinate
      vnl_vector<double> voxel(3);
      warp->SampleIntensityAtReferenceIndex(to_itkIndex(cp), warp->GetTimePointIndex(), true, voxel);
      for (unsigned int j = 0; j < 3; j++)
        message.cursor[j] += (j < 2 ? -1 : 1) * voxel[j];
    }
  }

  // Zoom/Pan change
  for (int i = 0; i < 3; i++)
  {
    GenericSliceModel  *gsm = m_Parent->GetSliceModel(i);
    AnatomicalDirection dir = app->GetAnatomicalDirectionForDisplayWindow(i);

    if (bc_zoom)
      message.zoom_level[dir] = gsm->GetViewZoom();
    if (bc_pan)
      message.viewPositionRelative[dir] = to_float(gsm->GetViewPositionRelativeToCursor());
  }

  // 3D viewpoint
  if (bc_camera)
  {
    // Get the camera state
    CameraState cs = m_Parent->GetModel3D()->GetRenderer()->GetCameraState();
    message.camera = cs;
  }

  // Broadcast the new message
  if (m_DebugSync)
    cout << "  *Broadcasting* to IPC"
         << "    bc_cursor=" << (bc_cursor ? 1 : 0)
         << "    bc_zoom=" << (bc_zoom ? 1 : 0)
         << "    bc_pan=" << (bc_pan ? 1 : 0)
         << "    bc_camera=" << (bc_camera ? 1 : 0)
         << endl << endl;

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


void
SynchronizationModel::ReadIPCState(bool only_read_new)
{
  IRISApplication *app = m_Parent->GetDriver();
  if (!app->IsMainImageLoaded() || !m_SyncEnabledModel->GetValue())
    return;

  // Read the IPC message
  IPCMessage message;
  bool       rc = only_read_new ? m_IPCHandler->ReadIfNew(static_cast<void *>(&message))
                                : m_IPCHandler->Read(static_cast<void *>(&message));

  if (m_DebugSync && rc)
  {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    cout << "PID " << std::setw(6) << m_IPCHandler->GetProcessID() << "    "
         << std::put_time(std::localtime(&now_time_t), "%X")
         << "    SynchronizationModel::ReadIPCState **read message**"
         << "  from  " << m_IPCHandler->GetLastMessageSenderProcessID() << endl;
    cout << "  Flags: "
         << "    only_read_new=" << (only_read_new ? 1 : 0)
         << endl;
  }


  if (rc)
  {
    if (m_SyncCursorModel->GetValue())
    {
      // Map the cursor position to the image coordinates
      GenericImageData *id = app->GetCurrentImageData();
      Vector3d vox = id->GetMain()->TransformNIFTICoordinatesToVoxelCIndex(message.cursor);

      // Round the cursor to integer value
      itk::Index<3> pos;
      Vector3ui     vpos;
      pos[0] = vpos[0] = (unsigned int)(vox[0] + 0.5);
      pos[1] = vpos[1] = (unsigned int)(vox[1] + 0.5);
      pos[2] = vpos[2] = (unsigned int)(vox[2] + 0.5);

      // Check if the voxel position is inside the image region
      if (vpos != app->GetCursorPosition() && id->GetImageRegion().IsInside(pos))
      {
        app->SetCursorPosition(vpos);
        if (m_DebugSync)
          cout << "  Setting cursor position to " << vpos << " from IPC" << endl;
      }
    }

    // Set the zoom/pan levels
    for (int i = 0; i < 3; i++)
    {
      GenericSliceModel  *gsm = m_Parent->GetSliceModel(i);
      AnatomicalDirection dir = app->GetAnatomicalDirectionForDisplayWindow(i);

      if (m_SyncZoomModel->GetValue() && gsm->IsSliceInitialized() &&
          gsm->GetViewZoom() != message.zoom_level[dir] &&
          static_cast<float>(message.zoom_level[dir]) > 0.0f)
      {
        gsm->SetViewZoom(message.zoom_level[dir]);
        if (m_DebugSync)
          cout << "Setting view zoom " << dir << " to " << message.zoom_level[dir] << " from IPC"
               << std::endl;
      }

      if (m_SyncPanModel->GetValue() && gsm->IsSliceInitialized() &&
          to_float(gsm->GetViewPositionRelativeToCursor()) != message.viewPositionRelative[dir])
      {
        gsm->SetViewPositionRelativeToCursor(to_double(message.viewPositionRelative[dir]));
        if (m_DebugSync)
          cout << "Setting view position " << dir << " to " << message.viewPositionRelative[dir]
               << " from IPC" << endl;
      }
    }

    // Set the camera state
    if (m_SyncCameraModel->GetValue())
    {
      // Get the currently used 3D camera
      CameraState cs = message.camera;
      m_Parent->GetModel3D()->GetRenderer()->SetCameraState(message.camera);
      if (m_DebugSync)
        cout << "Setting camera state from IPC" << endl;
    }

    if(m_DebugSync)
      cout << endl;
  }
}
