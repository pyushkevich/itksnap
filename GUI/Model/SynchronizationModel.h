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
  irisGetMacro(SyncEnabledModel, AbstractSimpleBooleanProperty *)
  irisGetMacro(SyncChannelModel, ConcreteRangedIntProperty *)
  irisGetMacro(SyncCursorModel, AbstractSimpleBooleanProperty *)
  irisGetMacro(SyncZoomModel, AbstractSimpleBooleanProperty *)
  irisGetMacro(SyncPanModel, AbstractSimpleBooleanProperty *)
  irisGetMacro(SyncCameraModel, AbstractSimpleBooleanProperty *)

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
