#include "DefaultBehaviorSettings.h"
#include "ColorMap.h"

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

  // Overlay behavior
  m_OverlayColorMapPresetModel =
      NewSimpleProperty("OverlayColorMapPreset",
                        std::string(ColorMap::GetPresetName(ColorMap::COLORMAP_GREY)));

  // Layer layout (uses enum)
  RegistryEnumMap<LayerLayout> enummap;
  enummap.AddPair(DisplayLayoutModel::LAYOUT_STACKED, "Stacked");
  enummap.AddPair(DisplayLayoutModel::LAYOUT_STACKED, "Tiled");
  m_OverlayLayoutModel =
      NewSimpleEnumProperty("OverlayLayout",
                            DisplayLayoutModel::LAYOUT_STACKED,
                            enummap);
}
