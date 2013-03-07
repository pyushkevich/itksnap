#include "QtIPCManager.h"
#include "SNAPEvents.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "SystemInterface.h"
#include "GenericImageData.h"
#include "ImageWrapperBase.h"

QtIPCManager::QtIPCManager(QWidget *parent) :
  SNAPComponent(parent)
{
  // Start the IPC timer at 30ms intervals
  startTimer(30);
}

void QtIPCManager::SetModel(GlobalUIModel *model)
{
  m_Model = model;

  // Listen to cursor events from the model
  connectITK(m_Model->GetDriver(), CursorUpdateEvent());
}

void QtIPCManager::onModelUpdate(const EventBucket &bucket)
{
  IRISApplication *app = m_Model->GetDriver();
  SystemInterface *si = app->GetSystemInterface();

  if(bucket.HasEvent(CursorUpdateEvent()))
    {
    // TODO: should this be moved to a model?

    if(m_Model->GetSynchronizeCursorModel()->GetValue()
       && app->IsMainImageLoaded())
      {
      // Map the cursor to NIFTI coordinates
      ImageWrapperBase *iw = app->GetCurrentImageData()->GetMain();
      Vector3d cursor = iw->TransformVoxelIndexToNIFTICoordinates(
            to_double(app->GetCursorPosition()));

      // Write the NIFTI cursor to shared memory
      si->IPCBroadcastCursor(cursor);
      }
    }
}

void QtIPCManager::timerEvent(QTimerEvent *)
{
  if(!m_Model) return;

  IRISApplication *app = m_Model->GetDriver();
  if(!app->IsMainImageLoaded())
    return;

  SystemInterface *si = app->GetSystemInterface();

  // Read the IPC message
  SystemInterface::IPCMessage ipcm;
  if(si->IPCReadIfNew(ipcm))
    {
    // TODO: synchronization flag
    if(m_Model->GetSynchronizeCursorModel()->GetValue())
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
    }
}


