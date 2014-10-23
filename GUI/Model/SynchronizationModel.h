#ifndef SYNCHRONIZATIONMODEL_H
#define SYNCHRONIZATIONMODEL_H

#include "PropertyModel.h"

class GlobalUIModel;
class SystemInterface;
class IPCHandler;

/**
 * @brief Model that interfaces with the GUI controls that handle
 * synchronization
 */
class SynchronizationModel : public AbstractModel
{
public:
  irisITKObjectMacro(SynchronizationModel, AbstractModel)

  void SetParentModel(GlobalUIModel *parent);

  /** Models controlling sync state */
  irisSimplePropertyAccessMacro(SyncEnabled, bool)
  irisRangedPropertyAccessMacro(SyncChannel, int)
  irisSimplePropertyAccessMacro(SyncCursor, bool)
  irisSimplePropertyAccessMacro(SyncZoom, bool)
  irisSimplePropertyAccessMacro(SyncPan, bool)
  irisSimplePropertyAccessMacro(SyncCamera, bool)

  /**
   * Whether the model should be broadcasting. The GUI should toggle this
   * flag depending on whether the window is active or not */
  irisGetSetMacro(CanBroadcast, bool)

  /** This method should be called by UI at regular intervals to read IPC state */
  void ReadIPCState();

protected:

  SynchronizationModel();
  virtual ~SynchronizationModel();

  virtual void OnUpdate();

  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncEnabledModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncCursorModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncZoomModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncPanModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncCameraModel;

  SmartPtr<ConcreteRangedIntProperty> m_SyncChannelModel;

  GlobalUIModel *m_Parent;
  SystemInterface *m_SystemInterface;

  IPCHandler *m_IPCHandler;

  bool m_CanBroadcast;
};

#endif // SYNCHRONIZATIONMODEL_H
