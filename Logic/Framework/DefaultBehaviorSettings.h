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
  irisSimplePropertyAccessMacro(AutoContrast, bool)

  // Permissions
  enum UpdateCheckingPermission {
    UPDATE_YES, UPDATE_NO, UPDATE_UNKNOWN
  };

  /** Whether we are allowed to check for updates automatically */
  irisSimplePropertyAccessMacro(CheckForUpdates, UpdateCheckingPermission)

  // Default colormap for overlays. Since this can be a user preset, the
  // value is specified as a string. This value may point to a preset that
  // has been deleted, so we have to be careful to check and reset to default
  // if that is the case
  irisSimplePropertyAccessMacro(OverlayColorMapPreset, std::string)

  // Default layout
  irisSimplePropertyAccessMacro(OverlayLayout, LayerLayout)

protected:

  // Default behaviors
  SmartPtr<ConcreteSimpleBooleanProperty> m_LinkedZoomModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_ContinuousMeshUpdateModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SynchronizationModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncCursorModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncZoomModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncPanModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_AutoContrastModel;

  // Permissions
  SmartPtr<ConcretePropertyModel<UpdateCheckingPermission> > m_CheckForUpdatesModel;

  // Overlay behaviors
  SmartPtr<ConcreteSimpleStringProperty> m_OverlayColorMapPresetModel;

  typedef ConcretePropertyModel<LayerLayout, TrivialDomain> ConcreteLayerLayoutModel;
  SmartPtr<ConcreteLayerLayoutModel> m_OverlayLayoutModel;

  // Constructor
  DefaultBehaviorSettings();
};

#endif // DEFAULTBEHAVIORSETTINGS_H
