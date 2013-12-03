#ifndef DEFAULTBEHAVIORSETTINGS_H
#define DEFAULTBEHAVIORSETTINGS_H

#include "AbstractPropertyContainerModel.h"

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

  // Constructor
  DefaultBehaviorSettings();
};

#endif // DEFAULTBEHAVIORSETTINGS_H
