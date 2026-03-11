#ifndef DEEPLEARNINGSEGMENTATIONCLIENT_H
#define DEEPLEARNINGSEGMENTATIONCLIENT_H

#include "AbstractPropertyContainerModel.h"
#include "MultiChannelDisplayMode.h"
#include "PropertyModel.h"
#include <tuple>
#include "Registry.h"
#include "itkCommand.h"
#include <mutex>

class GlobalUIModel;
class ImageWrapperBase;
class LabelImageWrapper;
namespace itk {
template <unsigned int VDim> class ImageBase;
template <unsigned int VDim> class ImageRegion;
}

template <class ServerTraits> class RESTClient;
template <class ServerTraits> class RESTSharedData;
class DLSServerTraits;
class CURLSHCookieJar;

namespace dls_model {

enum NetworkStatusEnum
{
  NETWORK_NOT_CONNECTED = 0,
  NETWORK_CHECKING,
  NETWORK_CONNECTED_COMPATIBLE,
  NETWORK_CONNECTED_INCOMPATIBLE
};

enum TunnelStatusEnum
{
  TUNNEL_ESTABLISHING = 0,
  TUNNEL_FAILED,
  TUNNEL_CREATED
};

enum LocalServerStatusEnum
{
  LOCAL_SERVER_STARTING = 0,
  LOCAL_SERVER_FAILED,
  LOCAL_SERVER_ESTABLISHED
};

/** Status of the authorization */
enum ConnectionStatusEnum {
  CONN_NO_SERVER = 0,
  CONN_TUNNEL_ESTABLISHING,
  CONN_TUNNEL_FAILED,
  CONN_LOCAL_SERVER_STARTING,
  CONN_LOCAL_SERVER_FAILED,
  CONN_CHECKING,
  CONN_NOT_CONNECTED,
  CONN_INCOMPATIBLE,
  CONN_CONNECTED
};

/** Structure combining status with error/success states */
template <class TEnum>
struct Status
{
  TEnum       status;
  std::string message;

  bool operator!=(const Status<TEnum> &o) const
  {
    return status != o.status || message != o.message;
  }

  Status(TEnum       s = static_cast<TEnum>(0),
         std::string m = std::string())
    : status(s)
    , message(m)
  {}
};

using ConnectionStatus = Status<ConnectionStatusEnum>;
using NetworkStatus = Status<NetworkStatusEnum>;
using TunnelStatus = Status<TunnelStatusEnum>;
using LocalServerStatus = Status<LocalServerStatusEnum>;

/** Structure describing a remote model */
struct RemoteModelMetadata
{
  int dimensions = 0;
  std::set<unsigned int> channels;
  bool supports_point = false, supports_box = false, supports_lasso = false, supports_scribble = false;

  // Check if the given number of channels is supported by the model
  bool supports_channels(unsigned int k) const
  {
    return channels.empty() || channels.find(k) != channels.end();
  }
};

/** Structure representing image data that has been sent to the remote server */
/*
struct RemoteImageView
{
  // ITK-SNAP unique layer ID
  unsigned long layer_id = 0u;

  // Timepoint
  unsigned int tp = 0u;

  // Slice axis and position (for 2D images)



  bool operator == (const RemoteImageView &o)
  {
    return std::tie(layer_id, tp, region) == std::tie(o.layer_id, o.tp, o.region);
  }

  bool operator != (const RemoteImageView &o)
  {
    return !(*this == o);
  }
};
*/

} // Namespace dls_model

/**
 * Properties of a deep learning extension server
 */
class DeepLearningServerPropertiesModel : public AbstractPropertyContainerModel
{
public:
  irisITKObjectMacro(DeepLearningServerPropertiesModel, AbstractPropertyContainerModel)

  irisSimplePropertyAccessMacro(Nickname, std::string)
  irisSimplePropertyAccessMacro(RemoteConnection, bool)
  irisSimplePropertyAccessMacro(Hostname, std::string)
  irisSimplePropertyAccessMacro(Port, int)
  irisSimplePropertyAccessMacro(UseSSHTunnel, bool)
  irisSimplePropertyAccessMacro(SSHUsername, std::string)
  irisSimplePropertyAccessMacro(SSHPrivateKeyFile, std::string)
  irisSimplePropertyAccessMacro(LocalPythonVEnvPath, std::string)
  irisSimplePropertyAccessMacro(NoSSLVerify, bool)

  irisSimplePropertyAccessMacro(FullURL, std::string)
  irisSimplePropertyAccessMacro(DisplayName, std::string)

  using PythonExeDomain = SimpleItemSetDomain<std::string, std::string>;
  using PythonExeModelType = AbstractPropertyModel<std::string, PythonExeDomain>;

  irisGenericPropertyAccessMacro(LocalPythonExePath, std::string, PythonExeDomain)

  virtual void AddKnownLocalPythonExePath(const std::string exepath);

  std::string GetHash() const;

protected:
  SmartPtr<ConcreteSimpleStringProperty> m_NicknameModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_RemoteConnectionModel;
  SmartPtr<ConcreteSimpleStringProperty> m_HostnameModel;
  SmartPtr<ConcreteSimpleIntProperty> m_PortModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_UseSSHTunnelModel;
  SmartPtr<AbstractSimpleStringProperty> m_FullURLModel;
  SmartPtr<AbstractSimpleStringProperty> m_DisplayNameModel;
  SmartPtr<ConcreteSimpleStringProperty> m_SSHUsernameModel;
  SmartPtr<ConcreteSimpleStringProperty> m_SSHPrivateKeyFileModel;
  SmartPtr<ConcreteSimpleStringProperty> m_LocalPythonExePathInternalModel;
  SmartPtr<ConcreteSimpleStringProperty> m_LocalPythonVEnvPathModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_NoSSLVerifyModel;

  // World-facing python exe model, with a domain.
  SmartPtr<PythonExeModelType> m_LocalPythonExePathModel;

  bool GetFullURLValue(std::string &value);
  bool GetDisplayNameValue(std::string &value);

  bool GetLocalPythonExePathValueAndRange(std::string &value, PythonExeDomain *domain);
  void SetLocalPythonExePathValue(std::string value);

  // The domain keeps track of known Python locations
  PythonExeDomain m_LocalPythonExeDomain;

  DeepLearningServerPropertiesModel();
};


/**
 * Abstract class defining what we require from a local DLS server running as a subprocess
 */
class AbstractLocalDeepLearningServerDelegate
{
public:
  /**
   * Start the local server, returning the port address on which the server will be running.
   * Returns port number, 0 or -1:
   *    0: server already running for this set of properties
   *   -1: failed to start server
   */
  virtual int StartServerIfNeeded(DeepLearningServerPropertiesModel *properties) = 0;
};


class DeepLearningSegmentationProgressDelegate
{
public:
  virtual std::string StartTask(const char *title, bool trackProgress) = 0;
  virtual void UpdateProgress(const std::string &task_id, double percent) = 0;
  virtual void CompleteTask(const std::string &task_id) = 0;
};

class ProgressTaskGuard
{
public:
  ProgressTaskGuard(DeepLearningSegmentationProgressDelegate *delegate, const char *title, bool trackProgress=false)
  {
    if(delegate)
      task_id = delegate->StartTask(title, trackProgress);
    source = delegate;
  }

  ~ProgressTaskGuard()
  {
    Complete();
  }

  static void ProgressCallback(void *source, double progress)
  {
    ProgressTaskGuard *task = static_cast<ProgressTaskGuard *>(source);
    if(task && task->source)
    {
      // Print exact time
      auto timenow = std::chrono::system_clock::now();
      auto time_t_now = std::chrono::system_clock::to_time_t(timenow);
      auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(timenow.time_since_epoch()) % 1000;

      /*
      std::cout << std::put_time(std::localtime(&time_t_now), "%F %T.") << ms.count()
                << " -- progress from task " << task->task_id << std::endl;
      */
      task->source->UpdateProgress(task->task_id, progress);
    }
  }

  void UpdateProgress(double percent)
  {
    if(source)
      source->UpdateProgress(task_id, percent);
  }

  void Complete()
  {
    if(source)
      source->CompleteTask(task_id);
    source = nullptr;
  }

  // non-copyable
  ProgressTaskGuard(const ProgressTaskGuard&) = delete;
  ProgressTaskGuard& operator=(const ProgressTaskGuard&) = delete;

private:
  std::string task_id;
  DeepLearningSegmentationProgressDelegate *source = nullptr;
};



class DeepLearningSegmentationModel : public AbstractModel
{
public:
  static const char *MINIMUM_SERVER_VERSION;

  using ServerDomain = STLVectorWrapperItemSetDomain<int, SmartPtr<DeepLearningServerPropertiesModel>>;
  using RegionType = itk::ImageRegion<3>;

  irisITKObjectMacro(DeepLearningSegmentationModel, AbstractModel)

  // A custom event fired when the server configuration changes
  itkEventMacro(ServerChangeEvent, IRISEvent)
  FIRES(ServerChangeEvent)

  /** Set the delegate that can launch/control local server */
  void SetLocalServerDelegate(AbstractLocalDeepLearningServerDelegate *delegate);

  void SetParentModel(GlobalUIModel *parent);

  /** Property model referring to the currently selected server */
  irisGenericPropertyAccessMacro(Server, int, ServerDomain)

  /**
   * Is the model active? This is a simple flag, when not active, we don't
   * nag the user.
   */
  irisSimplePropertyAccessMacro(IsActive, bool)

  /** Is there a current server */
  irisSimplePropertyAccessMacro(IsServerConfigured, bool)

  /** Status of the network connection */
  irisSimplePropertyAccessMacro(NetworkStatus, dls_model::NetworkStatus)

  /** Status of the network connection */
  irisSimplePropertyAccessMacro(TunnelStatus, dls_model::TunnelStatus)

  /** Status of the network connection */
  irisSimplePropertyAccessMacro(LocalServerStatus, dls_model::LocalServerStatus)

  /** Overall status of the server connection (derived from specific statuses) */
  irisSimplePropertyAccessMacro(ServerStatus, dls_model::ConnectionStatus)

  /** The actual hostname and port of the server - may be localhost if tunneling */
  irisSimplePropertyAccessMacro(ProxyURL, std::string)

  /** Display URL */
  irisSimplePropertyAccessMacro(ServerDisplayURL, std::string)

  /** Load preferences from registry */
  void LoadPreferences(Registry &folder);

  /** Write preferences to registry */
  void SavePreferences(Registry &folder);

  /** Get the properties for the current server */
  DeepLearningServerPropertiesModel *GetServerProperties();

  /** Update a server, either as new, or replacing the current server */
  void UpdateServerProperties(DeepLearningServerPropertiesModel *model, bool add_as_new);

  /** Delete the selected server */
  void DeleteCurrentServer();

  /** Status check: includes a hash of the server for which the check was issued and status */
  using StatusCheck = std::pair<std::string, dls_model::NetworkStatus>;

  /**
   * Start local server, if applicable. If the selected server is local, this will start
   * the process to run that server
   */
  void StartLocalServerIfNeeded();

  /** Static function that runs asynchronously to perform server authentication */
  StatusCheck AsyncCheckStatus();

  /** Apply the results of async server authentication to the model */
  void ApplyStatusCheckResponse(const StatusCheck &result);

  /** Progress for server interactions */
  irisRangedPropertyAccessMacro(ServerProgress, double)

  /** Get available models on the server */
  std::map<std::string, dls_model::RemoteModelMetadata> GetRemotePipelines();

  /** Select the segmentation layer for AI operations, specifying which model to use */
  void SetSourceImage(const std::string &model_id, ImageWrapperBase *layer, unsigned int axis);

  /** Reset interactions on the current layer */
  void ResetInteractions();

  /** Perform a point interaction */
  bool PerformPointInteraction(std::string model_id, ImageWrapperBase *layer, int axis, Vector3ui pos, bool reverse);

  /** Perform a scribble interaction */
  bool PerformScribbleInteraction(std::string model_id, ImageWrapperBase *layer, int axis, LabelImageWrapper *seg, bool reverse)
  {
    return this->PerformScribbleOrLassoInteraction("process_scribble_interaction", model_id, layer, axis, seg, reverse);
  }

  /** Perform a lasso interaction */
  bool PerformLassoInteraction(std::string model_id, ImageWrapperBase *layer, int axis, LabelImageWrapper *seg, bool reverse)
  {
    return this->PerformScribbleOrLassoInteraction("process_lasso_interaction", model_id, layer, axis, seg, reverse);
  }

  /** Reset all statuses and proxy URL to inital state */
  void ResetConnection();

  /** Set a delegate for dumping progress */
  irisGetSetMacro(ProgressDelegate, DeepLearningSegmentationProgressDelegate *)

protected:
  DeepLearningSegmentationModel();
  virtual ~DeepLearningSegmentationModel();

  std::string m_ActiveSession, m_ActiveSessionModel;


  GlobalUIModel *m_ParentModel;  

  // Property model for server selection
  typedef AbstractPropertyModel<int, ServerDomain> ServerModelType;
  SmartPtr<ServerModelType> m_ServerModel;

  SmartPtr<AbstractSimpleBooleanProperty> m_IsServerConfiguredModel;

  SmartPtr<AbstractSimpleStringProperty> m_ServerDisplayURLModel;

  // Index of the currently selected server or -1 if no server is selected
  int m_ServerIndex = -1;

  // Local port number - local servers run on an ephemeral port
  int m_LocalPortNumber = -1;

  // Whether or not the communication with the server is active.
  SmartPtr<ConcreteSimpleBooleanProperty> m_IsActiveModel;

  // Proxy URL - used when tunneling is enabled
  SmartPtr<ConcreteSimpleStringProperty> m_ProxyURLModel;

  // Status is broken into several parts and used to generate the overall status


  // Server status
  SmartPtr<ConcretePropertyModel<dls_model::NetworkStatus, TrivialDomain>> m_NetworkStatusModel;
  SmartPtr<ConcretePropertyModel<dls_model::TunnelStatus, TrivialDomain>> m_TunnelStatusModel;
  SmartPtr<ConcretePropertyModel<dls_model::LocalServerStatus, TrivialDomain>> m_LocalServerStatusModel;

  typedef AbstractPropertyModel<dls_model::ConnectionStatus, TrivialDomain> ServerStatusModelType;
  SmartPtr<ServerStatusModelType> m_ServerStatusModel;

  bool GetServerStatusValue(dls_model::ConnectionStatus &value);

  // List of models supported by the current server
  std::map<std::string, dls_model::RemoteModelMetadata> m_RemoteModelMetadata;

  // Current progress
  SmartPtr<ConcreteRangedDoubleProperty> m_ServerProgressModel;

  // Command used to update progress
  void ProgressCallback(itk::Object *source, const itk::EventObject &event);
  using CommandType = itk::MemberCommand<Self>;
  SmartPtr<CommandType> m_ProgressCommand;

  // List of available servers
  std::vector<SmartPtr<DeepLearningServerPropertiesModel>> m_ServerProperties;

  // REST client definition
  using RESTClientType = RESTClient<DLSServerTraits>;
  using RESTSharedDataType = RESTSharedData<DLSServerTraits>;

  // Data shared between REST sessions, includes cookies, etc.
  RESTSharedDataType *m_RESTSharedData;

  // The current view of the image that is available to the DLS server
  size_t m_RemoteImageViewHash = 0;

  using LabelSelection = int;
  LabelSelection m_LabelState = -1;

  bool GetServerValueAndRange(int &value, ServerDomain *domain);
  void SetServerValue(int value);
  bool GetServerIsConfiguredValue(bool &value);
  bool GetServerDisplayURLValue(std::string &value);

  bool ResetInteractionsIfNeeded();
  bool UpdateSegmentation(const std::string &model_id, int axis, const char *json, const char *commit_name);

  bool PerformScribbleOrLassoInteraction(const char        *target_url,
                                         std::string        model_id,
                                         ImageWrapperBase  *layer,
                                         int                axis,
                                         LabelImageWrapper *seg,
                                         bool               reverse);

  // Get the server metadata for a model with a given ID or throw exception if model
  // is not in the metadata list
  const dls_model::RemoteModelMetadata &GetRemoteModelMetadata(const std::string &id);

  std::string GetActualServerURL();

  // A mutex used to prevent multiple overlapping REST calls.
  // TODO: make tunneling more flexible so we don't have to do this
  std::mutex m_Mutex;

  // Local server delegate
  AbstractLocalDeepLearningServerDelegate *m_LocalServerDelegate = nullptr;

  // Delegate for reporting progress
  DeepLearningSegmentationProgressDelegate *m_ProgressDelegate = nullptr;

  dls_model::ConnectionStatus MergeStatuses();

};


#endif // DEEPLEARNINGSEGMENTATIONCLIENT_H
