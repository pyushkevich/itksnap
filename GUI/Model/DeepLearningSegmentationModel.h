#ifndef DEEPLEARNINGSEGMENTATIONCLIENT_H
#define DEEPLEARNINGSEGMENTATIONCLIENT_H

#include "PropertyModel.h"
#include <tuple>
#include "Registry.h"

class GlobalUIModel;
class ImageWrapperBase;
class LabelImageWrapper;
namespace itk {
template <unsigned int VDim> class ImageBase;
template <class T> class MemberCommand;
}

namespace dls_model {

/** Status of the authorization */
enum ConnectionStatusEnum {
  CONN_NO_SERVER,
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
};

}

class DeepLearningSegmentationModel : public AbstractModel
{
public:
  irisITKObjectMacro(DeepLearningSegmentationModel, AbstractModel)

  // A custom event fired when the server configuration changes
  itkEventMacro(ServerChangeEvent, IRISEvent)
  FIRES(ServerChangeEvent)

  void SetParentModel(GlobalUIModel *parent);

  /** Server URL property model */
  typedef STLVectorWrapperItemSetDomain<int, std::string> ServerURLDomain;
  irisGenericPropertyAccessMacro(ServerURL, int, ServerURLDomain)

  /** Server status */
  irisSimplePropertyAccessMacro(ServerStatus, dls_model::ConnectionStatus)

  /** Get the list of servers */
  std::vector<std::string> GetUserServerList() const;

  /** Set the list of servers */
  void SetUserServerList(const std::vector<std::string> &servers);

  /** Load preferences from registry */
  void LoadPreferences(Registry &folder);

  /** Write preferences to registry */
  void SavePreferences(Registry &folder);

  /** Get the full URL or empty string if there is not a current server */
  std::string GetURL(const std::string &path);

  /** Static function that runs asynchronously to perform server authentication */
  static dls_model::ConnectionStatus AsyncCheckStatus(std::string url);

  /** Apply the results of async server authentication to the model */
  void ApplyStatusCheckResponse(const dls_model::ConnectionStatus &result);

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

  // List of known server URLs
  std::vector<std::string> m_ServerURLList;

  // Property model for server selection
  typedef AbstractPropertyModel<int, ServerURLDomain> ServerURLModelType;
  SmartPtr<ServerURLModelType> m_ServerURLModel;

  // Current server URL index in the list of URLs
  int m_ServerURLIndex;

  // Server status
  typedef ConcretePropertyModel<dls_model::ConnectionStatus, TrivialDomain> ServerStatusModelType;
  SmartPtr<ServerStatusModelType> m_ServerStatusModel;

  // Current progress
  SmartPtr<ConcreteRangedDoubleProperty> m_ServerProgressModel;

  // Command used to update progress
  void ProgressCallback(itk::Object *source, const itk::EventObject &event);
  using CommandType = itk::MemberCommand<Self>;
  SmartPtr<CommandType> m_ProgressCommand;

  // Get and set the server url index
  bool GetServerURLValueAndRange(int &value, ServerURLDomain *domain);
  void SetServerURLValue(int value);
  bool ResetInteractionsIfNeeded();
  bool UpdateSegmentation(const char *json, const char *commit_name);

  using LayerSelection = std::tuple<unsigned int, unsigned int>;
  LayerSelection m_ActiveLayer = std::make_tuple(0u,0u);

  using LabelSelection = int;
  LabelSelection m_LabelState = -1;
};

#endif // DEEPLEARNINGSEGMENTATIONCLIENT_H
