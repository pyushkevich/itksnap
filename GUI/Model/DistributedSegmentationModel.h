/*=========================================================================

  Program:   ITK-SNAP
  Module:    DistributedSegmentationModel.h
  Language:  C++
  Date:      March 2018

  Copyright (c) 2018 Paul A. Yushkevich

  This file is part of ITK-SNAP

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef DISTRIBUTEDSEGMENTATIONMODEL_H
#define DISTRIBUTEDSEGMENTATIONMODEL_H

#include "PropertyModel.h"
#include "Registry.h"

class GlobalUIModel;
class IRISApplication;
class ProgressReporterDelegate;

namespace dss_model {

/** Status of the authorization */
enum AuthStatus {
  AUTH_NOT_CONNECTED,
  AUTH_CONNECTED_NOT_AUTHENTICATED,
  AUTH_AUTHENTICATED
};

/** Structure describing the authentication status */
struct AuthResponse
{
  AuthStatus status;
  std::string user_email;
  AuthResponse() : status(AUTH_NOT_CONNECTED) {}

  bool operator != (const AuthResponse &o) const
    { return status != o.status && user_email != o.user_email; }
};

/** Structure describing a single service summary */
struct ServiceSummary
{
  std::string name;
  std::string desc;
  std::string githash;
  std::string version;
};

/** Service summary sorter */
bool service_summary_cmp(const ServiceSummary &a, const ServiceSummary &b);

/** Service Listing */
typedef std::vector<ServiceSummary> ServiceListing;

/** Response from server status check call */
struct StatusCheckResponse
{
  ServiceListing service_listing;
  AuthResponse auth_response;
};

/** Tag type enum */
enum TagType {
  TAG_LAYER_ANATOMICAL = 0, TAG_LAYER_MAIN, TAG_LAYER_OVERLAY,
  TAG_SEGMENTATION_LABEL, TAG_POINT_LANDMARK, TAG_UNKNOWN
};


/** Tag type string values */
extern std::string tag_type_strings[];

/** Tag specification */
struct TagSpec
{
  bool required;
  TagType type;
  std::string name;
  std::string hint;

  // Reference to the object (layer, label, landmark) associated with this gat
  unsigned long object_id;

  bool operator == (const TagSpec &other) const;

  // Test if this tag refers to a layer
  bool IsLayerType() const;

};

/** Tag target specification */
struct TagTargetSpec
{
  TagSpec tag_spec;
  unsigned long object_id;
  std::string desc;

  bool operator == (const TagSpec &other) const;
};

/** Service description */
struct ServiceDetailResponse
{
  // Is the response valid
  bool valid;

  // Descriptive info
  std::string longdesc, url;

  // List of tag specs
  std::vector<TagSpec> tag_specs;
};

/** Ticket status */
enum TicketStatus {
  STATUS_INIT = 0, STATUS_READY, STATUS_CLAIMED, STATUS_SUCCESS, STATUS_FAILED, STATUS_TIMEDOUT, STATUS_UNKNOWN
};

/** Ticket status string values */
extern std::string ticket_status_strings[];

/** Ticket id type */
typedef long IdType;

/** Status of a single ticket */
struct TicketStatusSummary
{
  IdType id;
  TicketStatus status;
  std::string service_name;
};

/** Respose from listing tickets */
typedef std::map<IdType, TicketStatusSummary> TicketListingResponse;

enum LogType {
  LOG_INFO = 0, LOG_WARNING, LOG_ERROR, LOG_UNKNOWN
};

struct Attachment
{
  IdType id;
  std::string url;
  std::string mimetype;
  std::string desc;
};

struct TicketLogEntry
{
  IdType id;
  std::string atime;
  LogType type;
  std::string text;
  std::vector<Attachment> attachments;
};

/** Ticket details */
struct TicketDetailResponse
{
  // We keep the id of the ticket for which this detail is generated
  IdType ticket_id;
  double progress;

  // Queue position
  int queue_position;

  // The log is ordered by log-id
  std::vector<TicketLogEntry> log;

  bool from_json(const std::string &json);
};


/** Universal ticket reference - includes ticket id and server */
struct UniversalTicketId
{
  std::string server_url;
  IdType ticket_id;

  UniversalTicketId(std::string in_url = std::string() , IdType in_id = 0L);
  bool operator < (const UniversalTicketId &other) const;
};

/**
 * Information cached locally about tickets
 */
struct LocalTicketInfo
{
  std::string source_workspace, result_workspace;
};

} // namespace


/**
 * @brief Model behind the distributed segmentation GUI
 */
class DistributedSegmentationModel : public AbstractModel
{
public:
  irisITKObjectMacro(DistributedSegmentationModel, AbstractModel)

  // Server connection status
  enum ServerStatus
    { NOT_CONNECTED = 0, CONNECTED_NOT_AUTHORIZED, CONNECTED_AUTHORIZED };

  // What kind of load action should be triggered for the current tag
  enum LoadAction
    { LOAD_MAIN = 0, LOAD_OVERLAY, LOAD_NONE };

  // What is done with the project when downloading
  enum DownloadAction
    { DL_OPEN_CURRENT_WINDOW, DL_OPEN_NEW_WINDOW, DL_DONT_OPEN };

  enum UIState {
    UIF_AUTHENTICATED,
    UIF_TAGS_ASSIGNED,
    UIF_CAN_DOWNLOAD,
    UIF_TICKET_HAS_LOCAL_SOURCE,
    UIF_TICKET_HAS_LOCAL_RESULT
  };


  // A custom event fired when the server configuration changes
  itkEventMacro(ServerChangeEvent, IRISEvent)
  itkEventMacro(ServiceChangeEvent, IRISEvent)
  FIRES(ServerChangeEvent)
  FIRES(ServiceChangeEvent)

  void SetParentModel(GlobalUIModel *model);
  irisGetMacro(Parent, GlobalUIModel *)

  /** Load preferences from registry */
  void LoadPreferences(Registry &folder);

  /** Write preferences to registry */
  void SavePreferences(Registry &folder);

  /** Check state */
  bool CheckState(UIState state);

  /** Respond to events from up-stream models */
  virtual void OnUpdate() ITK_OVERRIDE;

  /** Server URL property model */
  typedef STLVectorWrapperItemSetDomain<int, std::string> ServerURLDomain;
  irisGenericPropertyAccessMacro(ServerURL, int, ServerURLDomain)

  /** Set the list of servers */
  std::vector<std::string> GetUserServerList() const;
  void SetUserServerList(const std::vector<std::string> &servers);

  /** Token model */
  irisSimplePropertyAccessMacro(Token, std::string)

  /** Server status model */
  irisSimplePropertyAccessMacro(ServerStatus, dss_model::AuthResponse)

  /** Download location model */
  irisSimplePropertyAccessMacro(DownloadLocation, std::string)

  /** Default location for downloads */
  std::string GetDefaultDownloadLocation();

  /** Long description of the current service */
  irisSimplePropertyAccessMacro(ServiceDescription, std::string)

  /** Get the full URL */
  std::string GetURL(const std::string &path);

  /** Registry describing the service listing */
  irisGetMacro(ServiceListing, const dss_model::ServiceListing &)
  void SetServiceListing(const dss_model::ServiceListing &listing);

  /** Selected service model */
  typedef SimpleItemSetDomain<int, std::string> CurrentServiceDomain;
  irisGenericPropertyAccessMacro(CurrentService, int, CurrentServiceDomain)

  /** Tag listing model */
  typedef STLVectorWrapperItemSetDomain<int, dss_model::TagTargetSpec> TagDomainType;
  irisGenericPropertyAccessMacro(TagList, int, TagDomainType)

  /** Layer domain for tag assignment */
  typedef SimpleItemSetDomain<unsigned long, std::string> LayerSelectionDomain;
  irisGenericPropertyAccessMacro(CurrentTagWorkspaceObject, unsigned long, LayerSelectionDomain)

  /** Get the type of the current tag */
  LoadAction GetTagLoadAction(int tag_index) const;
  
  /** Reset the tag assignment */
  void ResetTagAssignment();

  /** The progress of the current ticket */
  irisRangedPropertyAccessMacro(SelectedTicketProgress, double)

  /** The queue of the current ticket */
  irisSimplePropertyAccessMacro(SelectedTicketQueuePosition, int)

  /** The status of the currently selected ticket */
  irisSimplePropertyAccessMacro(SelectedTicketStatus, dss_model::TicketStatus)

  /** Log messages from the current ticket */
  typedef STLVectorWrapperItemSetDomain<dss_model::IdType, dss_model::TicketLogEntry> LogDomainType;
  irisGenericPropertyAccessMacro(SelectedTicketLog, dss_model::IdType, LogDomainType)

  /** Local workspace for the selected ticket */
  irisSimplePropertyAccessMacro(SelectedTicketLocalWorkspace, std::string)

  /** Result workspace for the selected ticket (if downloaded before) */
  irisSimplePropertyAccessMacro(SelectedTicketResultWorkspace, std::string)

  /** Get the git hash of the current service */
  std::string GetCurrentServiceGitHash() const;

  /** Check if all the required tags have a target */
  bool AreAllRequiredTagsAssignedTarget();

  /** Assign tags to their targets */
  void ApplyTagsToTargets();

  /** Submit the workspace */
  void SubmitWorkspace(ProgressReporterDelegate *pdel);

  /** Can the ticket be downloaded? */
  bool IsSelectedTicketSuccessful();

  /** Suggest a download location based on current settings */
  std::string SuggestDownloadFilename();

  typedef SimpleItemSetDomain<DownloadAction, std::string> DownloadActionDomain;

  /** What to do after download */
  irisGenericPropertyAccessMacro(DownloadAction, DownloadAction, DownloadActionDomain)

  /** Download the workspace */
  std::string DownloadWorkspace(const std::string &target_fn, ProgressReporterDelegate *pdel);

  /** Delete the ticket */
  void DeleteSelectedTicket();

  /** Property for selecting and listing tickets */
  typedef STLMapWrapperItemSetDomain<dss_model::IdType, dss_model::TicketStatusSummary> TicketListingDomain;
  irisGenericPropertyAccessMacro(TicketList, dss_model::IdType, TicketListingDomain)

  /** Get the last available log id for the currently selected ticket or 0 if none available */
  dss_model::IdType GetLastLogIdOfSelectedTicket();

  /** Get the detail for the current ticket - or NULL if no ticket is selected */
  const dss_model::TicketDetailResponse *GetSelectedTicketDetail();

  /** Static function that runs asynchronously to perform server authentication */
  static dss_model::StatusCheckResponse AsyncCheckStatus(std::string url, std::string token);

  /** Apply the results of async server authentication to the model */
  void ApplyStatusCheckResponse(const dss_model::StatusCheckResponse &result);

  /** Static function that runs asynchronously to get service details */
  static dss_model::ServiceDetailResponse AsyncGetServiceDetails(std::string githash);

  /** Apply the results of async server authentication to the model */
  void ApplyServiceDetailResponse(const dss_model::ServiceDetailResponse &resp);

  /** Static function that runs asynchronously to get a list of tickets */
  static dss_model::TicketListingResponse AsyncGetTicketListing();

  /** Apply the results of ticket listing call to the model */
  void ApplyTicketListingResponse(const dss_model::TicketListingResponse &resp);

  /** Static function that runs asynchronously to get service details */
  static dss_model::TicketDetailResponse AsyncGetTicketDetails(
      dss_model::IdType ticket_id, dss_model::IdType last_log);

  /** Apply the results of async server authentication to the model */
  void ApplyTicketDetailResponse(const dss_model::TicketDetailResponse &resp);

protected:

  // List of known server URLs
  std::vector<std::string> m_ServerURLList, m_SystemServerURLList;

  // Property model for server selection
  typedef ConcretePropertyModel<int, ServerURLDomain> ServerURLModelType;
  SmartPtr<ServerURLModelType> m_ServerURLModel;

  // Property model for token
  SmartPtr<ConcreteSimpleStringProperty> m_TokenModel;

  // Response from the server statuc call
  dss_model::AuthResponse m_AuthResponse;

  // Property model for server status
  typedef ConcretePropertyModel<dss_model::AuthResponse, TrivialDomain> ServerStatusModelType;
  SmartPtr<ServerStatusModelType> m_ServerStatusModel;

  // Property model for server status string
  SmartPtr<AbstractSimpleStringProperty> m_DownloadLocationModel;

  bool GetDownloadLocationValue(std::string &value);
  void SetDownloadLocationValue(std::string value);

  // Mapping from servers to download locations
  std::map<std::string, std::string> m_ServerDownloadLocationMap;

  // Property model for current service
  typedef ConcretePropertyModel<int, CurrentServiceDomain> CurrentServiceModel;
  SmartPtr<CurrentServiceModel> m_CurrentServiceModel;

  // Property model for service description
  SmartPtr<ConcreteSimpleStringProperty> m_ServiceDescriptionModel;

  // Property model for the ticket listing
  typedef ConcretePropertyModel<dss_model::IdType, TicketListingDomain> TicketListingModel;
  SmartPtr<TicketListingModel> m_TicketListModel;

  // Vector of current tag-specs
  std::vector<dss_model::TagTargetSpec> m_TagSpecArray;

  // Property model for the tag table
  typedef AbstractPropertyModel<int, TagDomainType> TagListModel;
  SmartPtr<TagListModel> m_TagListModel;

  // Getter and setter for the tag table
  bool GetTagListValueAndRange(int &value, TagDomainType *domain);
  void SetTagListValue(int value);

  // The currently selected tag
  int m_CurrentTag;

  // Id of the last submitted ticket
  dss_model::IdType m_SubmittedTicketId;

  // Property model for the current tag's image layer
  typedef AbstractPropertyModel<unsigned long, LayerSelectionDomain> CurrentTagWorkspaceObjectModel;
  SmartPtr<CurrentTagWorkspaceObjectModel> m_CurrentTagWorkspaceObjectModel;
  bool GetCurrentTagWorkspaceObjectValueAndRange(unsigned long &value, LayerSelectionDomain *range);
  void SetCurrentTagWorkspaceObjectValue(unsigned long value);

  // Stuff about the current ticket
  SmartPtr<ConcreteRangedDoubleProperty> m_SelectedTicketProgressModel;

  // Stuff about the queue position
  SmartPtr<ConcreteSimpleIntProperty> m_SelectedTicketQueuePositionModel;

  // Status of the selected ticket
  typedef AbstractPropertyModel<dss_model::TicketStatus, TrivialDomain> TicketStatusModel;
  SmartPtr<TicketStatusModel> m_SelectedTicketStatusModel;

  // Get the status of the selected ticket
  bool GetSelectedTicketStatusValue(dss_model::TicketStatus &value);

  // Log model for the current ticket
  typedef ConcretePropertyModel<dss_model::IdType, LogDomainType> LogModel;
  SmartPtr<LogModel> m_SelectedTicketLogModel;

  // Workspace associated with the selected ticket
  SmartPtr<AbstractSimpleStringProperty> m_SelectedTicketLocalWorkspaceModel;

  // Getter for the selected ticket workspace
  bool GetSelectedTicketLocalWorkspaceValue(std::string &value);

  // Result workspace associated with the selected ticket
  SmartPtr<AbstractSimpleStringProperty> m_SelectedTicketResultWorkspaceModel;

  // Getter for the selected ticket workspace
  bool GetSelectedTicketResultWorkspaceValue(std::string &value);

  // Download action model
  typedef ConcretePropertyModel<DownloadAction, DownloadActionDomain> DownloadActionModel;
  SmartPtr<DownloadActionModel> m_DownloadActionModel;

  // Registry holding the service listing
  dss_model::ServiceListing m_ServiceListing;

  // Listing of tickets
  dss_model::TicketListingResponse m_TicketListing;

  // Detail for the current ticket
  dss_model::TicketDetailResponse m_SelectedTicketDetail;

  // Download location

  typedef std::map<dss_model::UniversalTicketId, dss_model::LocalTicketInfo> TicketWorkspaceMap;
  TicketWorkspaceMap m_TicketWorkspaceMap;

  DistributedSegmentationModel();
  ~DistributedSegmentationModel() {}

  // Parent model
  GlobalUIModel *m_Parent;

  // Find a unique object that matches a tag, and if found, assign it to the tag
  bool FindUniqueObjectForTag(dss_model::TagTargetSpec &tag);
  void UpdateTagObjectIds(bool clear_ids_first);

  // Helper function to get listing of services
  static bool AsyncGetServiceListing(std::vector<dss_model::ServiceSummary> &services);
};

#endif // DISTRIBUTEDSEGMENTATIONMODEL_H
