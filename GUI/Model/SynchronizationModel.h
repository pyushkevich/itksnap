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

  typedef SimpleItemSetDomain<unsigned long, std::string> LayerSelectionDomain;
  typedef AbstractPropertyModel<unsigned long, LayerSelectionDomain> AbstractLayerSelectionModel;

  /** Model controlling whether sync goes through a warp */
  irisGetMacro(WarpLayerModel, AbstractLayerSelectionModel *)

  /**
   * Whether the model should be broadcasting. The GUI should toggle this
   * flag depending on whether the window is active or not */
  irisGetSetMacro(CanBroadcast, bool)

  /** This method should be called by UI at regular intervals to read IPC state */
  void ReadIPCState();

protected:

  SynchronizationModel();
  virtual ~SynchronizationModel();

  virtual void OnUpdate() ITK_OVERRIDE;

  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncEnabledModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncCursorModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncZoomModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncPanModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncCameraModel;

  SmartPtr<ConcreteRangedIntProperty> m_SyncChannelModel;

  SmartPtr<AbstractLayerSelectionModel> m_WarpLayerModel;
  bool GetWarpLayerValueAndRange(unsigned long &value, LayerSelectionDomain *range);
  void SetWarpLayerValue(unsigned long value);

  GlobalUIModel *m_Parent;
  SystemInterface *m_SystemInterface;
  unsigned long m_WarpLayerId;
  IPCHandler *m_IPCHandler;

  bool m_CanBroadcast;
};

#endif // SYNCHRONIZATIONMODEL_H
