#ifndef SYNCHRONIZATIONMODEL_H
#define SYNCHRONIZATIONMODEL_H

#include "PropertyModel.h"
#include <itkCommand.h>
#include <string>
#include <vector>
#include <utility>

class GlobalUIModel;
class SystemInterface;
class IPCHandler;
class AbstractSharedMemorySystemInterface;

/**
 * @brief Model that interfaces with the GUI controls that handle
 * synchronization
 */
class SynchronizationModel : public AbstractModel
{
public:
  irisITKObjectMacro(SynchronizationModel, AbstractModel)

  void SetParentModel(GlobalUIModel *parent);

  /** Pass a system object that is used to make IPC calls */
  void SetSystemInterface(AbstractSharedMemorySystemInterface *si);

  /** Size and version of the IPC message payload, for use before the model is created. */
  static size_t GetIPCMessageSize();
  static short  GetIPCMessageVersion();

  /** Force detach - do this in a crash */
  void ForceDetach();

  /** Update the window title stored in the IPC instance directory slot. */
  void UpdateWindowTitle(const std::string &title);

  /**
   * Return {pid, title} for all live ITK-SNAP instances visible in the
   * instance directory (including this process).
   */
  std::vector<std::pair<long, std::string>> GetRunningInstances();

  /** Send a file/URL drop request to another running ITK-SNAP instance. */
  void SendDropToInstance(long pid, const std::string &filename);

  /** Filename/URL from the most recently received IPC drop request. */
  irisGetMacro(PendingDropFilename, std::string)

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

  /** Enable sync debugging */
  irisGetSetMacro(DebugSync, bool)

  /** This method should be called by UI at regular intervals to read IPC state */
  void ReadIPCState(bool only_read_new=true);

protected:

  SynchronizationModel();
  virtual ~SynchronizationModel();

  virtual void OnUpdate() override;

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
  IPCHandler *m_IPCHandler = nullptr;

  bool m_CanBroadcast;
  bool m_DebugSync = false;

  std::string m_PendingDropFilename;
};

#endif // SYNCHRONIZATIONMODEL_H
