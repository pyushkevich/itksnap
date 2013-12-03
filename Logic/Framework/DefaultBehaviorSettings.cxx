#include "DefaultBehaviorSettings.h"

DefaultBehaviorSettings::DefaultBehaviorSettings()
{
  // Behaviors
  m_LinkedZoomModel = NewSimpleProperty("LinkedZoom", true);
  m_ContinuousMeshUpdateModel = NewSimpleProperty("ContinuousMeshUpdate", false);
  m_SynchronizationModel = NewSimpleProperty("Synchronization", true);
  m_SyncCursorModel = NewSimpleProperty("SyncCursor", true);
  m_SyncZoomModel = NewSimpleProperty("SyncZoom", true);
  m_SyncPanModel = NewSimpleProperty("SyncPan", true);

  // Permissions
  m_CheckForUpdatesModel = NewSimpleProperty("CheckForUpdates", true);
}
