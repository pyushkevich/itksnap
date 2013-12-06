#ifndef SYNCHRONIZATIONMODEL_H
#define SYNCHRONIZATIONMODEL_H

#include "PropertyModel.h"

class GlobalUIModel;
class SystemInterface;

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


};

#endif // SYNCHRONIZATIONMODEL_H
