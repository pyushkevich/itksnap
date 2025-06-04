#ifndef DEEPLEARNINGSEGMENTATIONCLIENT_H
#define DEEPLEARNINGSEGMENTATIONCLIENT_H

#include "AbstractPropertyContainerModel.h"
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
}

template <class ServerTraits> class RESTClient;
template <class ServerTraits> class RESTSharedData;
class DLSServerTraits;
class CURLSHCookieJar;

namespace dls_model {

/** Status of the authorization */
enum ConnectionStatusEnum {
  CONN_NO_SERVER,
  CONN_TUNNEL_ESTABLISHING,
  CONN_TUNNEL_FAILED,
  CONN_LOCAL_SERVER_STARTING,
  CONN_LOCAL_SERVER_FAILED,
  CONN_CHECKING,
  CONN_NOT_CONNECTED,
  CONN_CONNECTED
};

/** Structure describing the connection status */
struct ConnectionStatus
{
  ConnectionStatusEnum status = CONN_NOT_CONNECTED;
  std::string server_version, error_message;

  bool operator != (const ConnectionStatus &o) const
   { return status != o.status
            || error_message != o.error_message
            || server_version != o.server_version; }

   ConnectionStatus(ConnectionStatusEnum s = CONN_NOT_CONNECTED) : status(s) {}
};

}

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




class DeepLearningSegmentationModel : public AbstractModel
{
public:
  using ServerDomain = STLVectorWrapperItemSetDomain<int, SmartPtr<DeepLearningServerPropertiesModel>>;

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

  /** Server status */
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
  using StatusCheck = std::pair<std::string, dls_model::ConnectionStatus>;

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

  /** Select the segmentation layer for AI operations */
  void SetSourceImage(ImageWrapperBase *layer);

  /** Reset interactions on the current layer */
  void ResetInteractions();

  /** Perform a point interaction */
  bool PerformPointInteraction(ImageWrapperBase *layer, Vector3ui pos, bool reverse);

  /** Perform a scribble interaction */
  bool PerformScribbleInteraction(ImageWrapperBase *layer, LabelImageWrapper *seg, bool reverse)
  {
    return this->PerformScribbleOrLassoInteraction("process_scribble_interaction", layer, seg, reverse);
  }

  /** Perform a lasso interaction */
  bool PerformLassoInteraction(ImageWrapperBase *layer, LabelImageWrapper *seg, bool reverse)
  {
    return this->PerformScribbleOrLassoInteraction("process_lasso_interaction", layer, seg, reverse);
  }

protected:
  DeepLearningSegmentationModel();
  virtual ~DeepLearningSegmentationModel();

  std::string m_ActiveSession;


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

  // Server status
  typedef AbstractPropertyModel<dls_model::ConnectionStatus, TrivialDomain> ServerStatusModelType;
  SmartPtr<ServerStatusModelType> m_ServerStatusModel;

  bool GetServerStatusValue(dls_model::ConnectionStatus &value);
  void SetServerStatusValue(dls_model::ConnectionStatus value);
  dls_model::ConnectionStatus m_ServerStatus;

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

  using LayerSelection = std::tuple<unsigned int, unsigned int>;
  LayerSelection m_ActiveLayer = std::make_tuple(0u,0u);

  using LabelSelection = int;
  LabelSelection m_LabelState = -1;

  bool GetServerValueAndRange(int &value, ServerDomain *domain);
  void SetServerValue(int value);
  bool GetServerIsConfiguredValue(bool &value);
  bool GetServerDisplayURLValue(std::string &value);

  bool ResetInteractionsIfNeeded();
  bool UpdateSegmentation(const char *json, const char *commit_name);

  bool PerformScribbleOrLassoInteraction(const char        *target_url,
                                         ImageWrapperBase  *layer,
                                         LabelImageWrapper *seg,
                                         bool               reverse);

  std::string GetActualServerURL();

  // A mutex used to prevent multiple overlapping REST calls.
  // TODO: make tunneling more flexible so we don't have to do this
  std::mutex m_Mutex;

  // Local server delegate
  AbstractLocalDeepLearningServerDelegate *m_LocalServerDelegate = nullptr;
};


#endif // DEEPLEARNINGSEGMENTATIONCLIENT_H
