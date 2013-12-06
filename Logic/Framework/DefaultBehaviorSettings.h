#ifndef DEFAULTBEHAVIORSETTINGS_H
#define DEFAULTBEHAVIORSETTINGS_H

#include "AbstractPropertyContainerModel.h"
#include "DisplayLayoutModel.h"

/**
 * A set of default behaviors for SNAP. These are read at startup and used
 * to initialize the SNAP state.
 */
class DefaultBehaviorSettings : public AbstractPropertyContainerModel
{
public:
  irisITKObjectMacro(DefaultBehaviorSettings, AbstractPropertyContainerModel)

  // Default behaviors
  irisSimplePropertyAccessMacro(LinkedZoom, bool)
  irisSimplePropertyAccessMacro(ContinuousMeshUpdate, bool)
  irisSimplePropertyAccessMacro(Synchronization, bool)
  irisSimplePropertyAccessMacro(SyncCursor, bool)
  irisSimplePropertyAccessMacro(SyncZoom, bool)
  irisSimplePropertyAccessMacro(SyncPan, bool)

  // Permissions
  irisSimplePropertyAccessMacro(CheckForUpdates, bool)

  // Default colormap for overlays. Since this can be a user preset, the
  // value is specified as a string. This value may point to a preset that
  // has been deleted, so we have to be careful to check and reset to default
  // if that is the case
  irisSimplePropertyAccessMacro(OverlayColorMapPreset, std::string)

  // Default layout
  typedef DisplayLayoutModel::LayerLayout LayerLayout;
  irisSimplePropertyAccessMacro(OverlayLayout, DisplayLayoutModel::LayerLayout)

protected:

  // Default behaviors
  SmartPtr<ConcreteSimpleBooleanProperty> m_LinkedZoomModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_ContinuousMeshUpdateModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SynchronizationModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncCursorModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncZoomModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncPanModel;

  // Permissions
  SmartPtr<ConcreteSimpleBooleanProperty> m_CheckForUpdatesModel;

  // Overlay behaviors
  SmartPtr<ConcreteSimpleStringProperty> m_OverlayColorMapPresetModel;

  typedef ConcretePropertyModel<LayerLayout, TrivialDomain> ConcreteLayerLayoutModel;
  SmartPtr<ConcreteLayerLayoutModel> m_OverlayLayoutModel;

  // Constructor
  DefaultBehaviorSettings();
};

#endif // DEFAULTBEHAVIORSETTINGS_H
