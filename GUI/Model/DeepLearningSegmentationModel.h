#ifndef DEEPLEARNINGSEGMENTATIONCLIENT_H
#define DEEPLEARNINGSEGMENTATIONCLIENT_H

#include "AbstractPropertyContainerModel.h"
#include "PropertyModel.h"
#include <tuple>
#include "Registry.h"
#include "itkCommand.h"

class GlobalUIModel;
class ImageWrapperBase;
class LabelImageWrapper;
namespace itk {
template <unsigned int VDim> class ImageBase;
}

namespace dls_model {

/** Status of the authorization */
enum ConnectionStatusEnum {
  CONN_NO_SERVER,
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
struct DeepLearningServerPropertiesModel : public AbstractPropertyContainerModel
{
public:
  irisITKObjectMacro(DeepLearningServerPropertiesModel, AbstractPropertyContainerModel)

  irisSimplePropertyAccessMacro(Hostname, std::string)
  irisSimplePropertyAccessMacro(Nickname, std::string)
  irisSimplePropertyAccessMacro(Port, int)
  irisSimplePropertyAccessMacro(UseSSHTunnel, bool)

  irisSimplePropertyAccessMacro(FullURL, std::string)

protected:
  SmartPtr<ConcreteSimpleStringProperty> m_HostnameModel;
  SmartPtr<ConcreteSimpleStringProperty> m_NicknameModel;
  SmartPtr<ConcreteSimpleIntProperty> m_PortModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_UseSSHTunnelModel;
  SmartPtr<AbstractSimpleStringProperty> m_FullURLModel;

  bool GetFullURLValue(std::string &value);

  DeepLearningServerPropertiesModel();
};


class DeepLearningSegmentationModel : public AbstractModel
{
public:
  irisITKObjectMacro(DeepLearningSegmentationModel, AbstractModel)

  // A custom event fired when the server configuration changes
  itkEventMacro(ServerChangeEvent, IRISEvent)
  FIRES(ServerChangeEvent)

  void SetParentModel(GlobalUIModel *parent);

  /** Server URL property model */
  // typedef STLVectorWrapperItemSetDomain<int, std::string> ServerURLDomain;
  using ServerURLDomain = SimpleItemSetDomain<std::string, std::string>;
  irisGenericPropertyAccessMacro(ServerURL, std::string, ServerURLDomain)

  /** Is there a current server */
  irisSimplePropertyAccessMacro(ServerConfigured, bool)

  /** Server status */
  irisSimplePropertyAccessMacro(ServerStatus, dls_model::ConnectionStatus)

  /** Get the list of servers */
  // std::vector<std::string> GetUserServerList() const;

  /** Set the list of servers */
  // void SetUserServerList(const std::vector<std::string> &servers);

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

  /** Get the full URL or empty string if there is not a current server */
  std::string GetURL(const std::string &path);

  using StatusCheck = std::pair<std::string, dls_model::ConnectionStatus>;

  /** Static function that runs asynchronously to perform server authentication */
  static StatusCheck AsyncCheckStatus(std::string url);

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
  bool PerformScribbleInteraction(ImageWrapperBase *layer, LabelImageWrapper *seg, bool reverse);

protected:
  DeepLearningSegmentationModel();
  virtual ~DeepLearningSegmentationModel() {};

  std::string m_ActiveSession;


  GlobalUIModel *m_ParentModel;  

  // Property model for server selection
  typedef AbstractPropertyModel<std::string, ServerURLDomain> ServerURLModelType;
  SmartPtr<ServerURLModelType> m_ServerURLModel;

  SmartPtr<AbstractSimpleBooleanProperty> m_ServerConfiguredModel;
  bool GetServerConfiguredValue(bool &value);

  // Currently selected server, empty string indicates no server selected
  std::string m_Server;

  // Server status
  typedef ConcretePropertyModel<dls_model::ConnectionStatus, TrivialDomain> ServerStatusModelType;
  SmartPtr<ServerStatusModelType> m_ServerStatusModel;

  // Current progress
  SmartPtr<ConcreteRangedDoubleProperty> m_ServerProgressModel;

  // Command used to update progress
  void ProgressCallback(itk::Object *source, const itk::EventObject &event);
  using CommandType = itk::MemberCommand<Self>;
  SmartPtr<CommandType> m_ProgressCommand;

  // Available servers and their properties. The key is the displayed name of the
  // server (either nickname or URL)
  std::map<std::string, SmartPtr<DeepLearningServerPropertiesModel>> m_ServerProperties;

  // Properties for all servers, stored in serialized format

  // Get and set the server url index
  bool GetServerURLValueAndRange(std::string &value, ServerURLDomain *domain);
  void SetServerURLValue(std::string value);
  bool ResetInteractionsIfNeeded();
  bool UpdateSegmentation(const char *json, const char *commit_name);

  using LayerSelection = std::tuple<unsigned int, unsigned int>;
  LayerSelection m_ActiveLayer = std::make_tuple(0u,0u);

  using LabelSelection = int;
  LabelSelection m_LabelState = -1;
};

#endif // DEEPLEARNINGSEGMENTATIONCLIENT_H
