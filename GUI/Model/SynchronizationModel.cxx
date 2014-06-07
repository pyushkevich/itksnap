#include "SynchronizationModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "SystemInterface.h"
#include "GenericSliceModel.h"
#include "GenericImageData.h"
#include "ImageWrapperBase.h"
#include "SliceWindowCoordinator.h"

SynchronizationModel::SynchronizationModel()
{
  // Create the models
  m_SyncEnabledModel = NewSimpleConcreteProperty(true);
  m_SyncCursorModel = NewSimpleConcreteProperty(true);
  m_SyncZoomModel = NewSimpleConcreteProperty(true);
  m_SyncPanModel = NewSimpleConcreteProperty(true);
  m_SyncCameraModel = NewSimpleConcreteProperty(true);

  m_SyncChannelModel = NewRangedConcreteProperty(1, 1, 99, 1);
}

SynchronizationModel::~SynchronizationModel()
{

}

void SynchronizationModel::OnUpdate()
{
  // If there is no synchronization or no image, get out
  IRISApplication *app = m_Parent->GetDriver();
  if(!app->IsMainImageLoaded()
     || !m_SyncEnabledModel->GetValue())
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

  // Cursor change
  if(bc_cursor)
    {
    // Map the cursor to NIFTI coordinates
    ImageWrapperBase *iw = app->GetCurrentImageData()->GetMain();
    Vector3d cursor =
        iw->TransformVoxelIndexToNIFTICoordinates(
          to_double(app->GetCursorPosition()));

    // Write the NIFTI cursor to shared memory
    m_SystemInterface->IPCBroadcastCursor(cursor);
    }

  // Zoom/Pan change
  for(int i = 0; i < 3; i++)
    {
    GenericSliceModel *gsm = m_Parent->GetSliceModel(i);
    AnatomicalDirection dir = app->GetAnatomicalDirectionForDisplayWindow(i);

    if(bc_zoom)
      {
      m_SystemInterface->IPCBroadcastZoomLevel(dir, gsm->GetViewZoom());
      }
    if(bc_pan)
      {
      m_SystemInterface->IPCBroadcastViewPosition(
            dir, gsm->GetViewPositionRelativeToCursor());
      }
    }

  // 3D viewpoint ?

}


void SynchronizationModel::SetParentModel(GlobalUIModel *parent)
{
  // Set the parent
  m_Parent = parent;
  m_SystemInterface = m_Parent->GetDriver()->GetSystemInterface();

  // TODO: the defaults should be read from global preferences

  // TODO: establish the channel for the SNAP window by reading IPC

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

  // 3D viewpoint changes (TODO)
}

void SynchronizationModel::ReadIPCState()
{
  IRISApplication *app = m_Parent->GetDriver();
  if(!app->IsMainImageLoaded() || !m_SyncCursorModel->GetValue())
    return;

  // Read the IPC message
  SystemInterface::IPCMessage ipcm;
  if(m_SystemInterface->IPCReadIfNew(ipcm))
    {
    if(m_SyncCursorModel->GetValue())
      {
      // Map the cursor position to the image coordinates
      GenericImageData *id = app->GetCurrentImageData();
      Vector3d vox =
          id->GetMain()->TransformNIFTICoordinatesToVoxelIndex(ipcm.cursor);

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
         && gsm->GetViewZoom() != ipcm.zoom_level[dir])
        {
          gsm->SetViewZoom(ipcm.zoom_level[dir]);
        }

      if(m_SyncPanModel->GetValue()
         && gsm->IsSliceInitialized()
         && gsm->GetViewPositionRelativeToCursor() != ipcm.viewPositionRelative[dir])
        {
        gsm->SetViewPositionRelativeToCursor(ipcm.viewPositionRelative[dir]);
        }
      }
    }
}




