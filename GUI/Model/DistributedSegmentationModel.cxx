/*=========================================================================

  Program:   ITK-SNAP
  Module:    DistributedSegmentationModel.cxx
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
#include "DistributedSegmentationModel.h"

#include "GlobalUIModel.h"
#include "RESTClient.h"
#include "FormattedTable.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "WorkspaceAPI.h"
#include "json/json.h"
#include "UIReporterDelegates.h"
#include <sstream>
#include <algorithm>
#include "itksys/SystemTools.hxx"
#include "AllPurposeProgressAccumulator.h"
#include "ImageAnnotationData.h"


namespace dss_model {

/** TODO: make this sort versions properly */
bool service_summary_cmp(const ServiceSummary &a, const ServiceSummary &b)
{
  if(a.name < b.name)
    return true;

  if(a.name == b.name && a.version < b.version)
    return true;

  return false;
}

bool TagSpec::operator ==(const TagSpec &o) const
{
  return
      name == o.name && type == o.type &&
      required == o.required && hint == o.hint &&
      object_id == o.object_id;
}

bool TagSpec::IsLayerType() const
{
  return (type == TAG_LAYER_ANATOMICAL
          || type == TAG_LAYER_MAIN
          || type == TAG_LAYER_OVERLAY);
}

const char *ticket_status_emap_initializer[] =
{
  "init", "ready", "claimed", "success", "failed", "timeout", "unknown", NULL
};
RegistryEnumMap<TicketStatus> ticket_status_emap(ticket_status_emap_initializer);

const char *log_type_emap_initializer[] =
{
  "info", "warning", "error", "unknown", NULL
};
RegistryEnumMap<LogType> log_type_emap(log_type_emap_initializer);


std::string ticket_status_strings[] =
{
  "initialized", "ready", "claimed", "success", "failed", "timed out"
};

std::string tag_type_strings[] =
{
  "Image Layer", "Main Image", "Overlay Image", "Segmentation Label", "Point Landmark", "Unknown"
};

UniversalTicketId::UniversalTicketId(std::string in_url, IdType in_id)
  : server_url(in_url), ticket_id(in_id)
{

}

bool UniversalTicketId::operator <(const UniversalTicketId &other) const
{
  return (server_url < other.server_url)
      || ((server_url == other.server_url) && ticket_id < other.ticket_id);
}


} // namespace

using namespace dss_model;

void DistributedSegmentationModel::SetParentModel(GlobalUIModel *model)
{
  m_Parent = model;

  // Changes in the layer structure result in changes in the tag configuration and
  // must be responded to
  this->Rebroadcast(m_Parent->GetDriver(), LayerChangeEvent(), ModelUpdateEvent());

  // Initialize the download location
  m_DownloadLocationModel->SetValue(this->GetDefaultDownloadLocation());
}



void DistributedSegmentationModel::LoadPreferences(Registry &folder)
{
  // Read the list of servers
  std::vector<std::string> user_servers = folder.Folder("UserServerList").GetArray(std::string());
  this->SetUserServerList(user_servers);

  // Read the preferred server
  int pref_server = folder["PreferredServerIndex"][0];
  if(pref_server < m_ServerURLList.size())
    m_ServerURLModel->SetValue(pref_server);

  // Read the workspace locations
  Registry &twm = folder.Folder("TicketWorkspaceMap");
  m_TicketWorkspaceMap.clear();
  for(int k = 0; k < twm["ArraySize"][0]; k++)
    {
    Registry &f = twm.Folder(twm.Key("Element[%d]", k));
    UniversalTicketId uti(f["URL"][""], f["Ticket"][0]);
    if(uti.server_url.size() && uti.ticket_id)
      {
      std::string workspace = f["SourceWorkspace"][""];
      if(workspace.size())
        m_TicketWorkspaceMap[uti].source_workspace = workspace;

      std::string dl_workspace = f["ResultWorkspace"][""];
      if(dl_workspace.size())
        m_TicketWorkspaceMap[uti].result_workspace = dl_workspace;
      }
    }

  // Read the server-specific data
  m_ServerDownloadLocationMap.clear();
  Registry &server_prefs = folder.Folder("ServerData");
  for(int k = 0; k < server_prefs["ArraySize"][0]; k++)
    {
    Registry &f = server_prefs.Folder(twm.Key("Element[%d]", k));
    std::string url = f["URL"][""];
    std::string dl = f["DownloadLocation"][""];

    if(url.size() && dl.size()
       && std::find(m_ServerURLList.begin(), m_ServerURLList.end(), url) != m_ServerURLList.end())
      {
      m_ServerDownloadLocationMap[url] = dl;
      }
    }
}

void DistributedSegmentationModel::SavePreferences(Registry &folder)
{
  // Save the list of servers
  std::vector<std::string> user_servers = this->GetUserServerList();
  folder.Folder("UserServerList").PutArray(user_servers);

  // Save the preferred server index
  folder["PreferredServerIndex"] << m_ServerURLModel->GetValue();

  // Save the workspace locations
  Registry &twm = folder.Folder("TicketWorkspaceMap");
  twm.Clear();
  twm["ArraySize"] << m_TicketWorkspaceMap.size();
  int k = 0;
  for(TicketWorkspaceMap::const_iterator it = m_TicketWorkspaceMap.begin();
      it != m_TicketWorkspaceMap.end(); ++it, ++k)
    {
    Registry &f = twm.Folder(twm.Key("Element[%d]", k));
    f["URL"] << it->first.server_url;
    f["Ticket"] << it->first.ticket_id;
    if(it->second.source_workspace.size())
      f["SourceWorkspace"] << it->second.source_workspace;
    if(it->second.result_workspace.size())
      f["ResultWorkspace"] << it->second.result_workspace;
    }

  // Read the server-specific data
  Registry &server_prefs = folder.Folder("ServerData");
  server_prefs.Clear();
  server_prefs["ArraySize"] << m_ServerDownloadLocationMap.size();
  k = 0;
  for(std::map<std::string, std::string>::const_iterator it = m_ServerDownloadLocationMap.begin();
      it != m_ServerDownloadLocationMap.end(); ++it, ++k)
    {
    Registry &f = server_prefs.Folder(server_prefs.Key("Element[%d]", k));
    f["URL"] << it->first;
    f["DownloadLocation"] << it->second;
    }
}

bool DistributedSegmentationModel::AreAllRequiredTagsAssignedTarget()
{
  for(int i = 0; i < m_TagSpecArray.size(); i++)
    {
    if(m_TagSpecArray[i].tag_spec.required && m_TagSpecArray[i].object_id == 0)
      return false;
    }
  return true;
}

bool DistributedSegmentationModel::CheckState(DistributedSegmentationModel::UIState state)
{
  switch(state)
    {
    case DistributedSegmentationModel::UIF_TICKET_HAS_LOCAL_SOURCE:
      return this->GetSelectedTicketLocalWorkspace().size() > 0;
    case DistributedSegmentationModel::UIF_TICKET_HAS_LOCAL_RESULT:
      return this->GetSelectedTicketResultWorkspace().size() > 0;
    case DistributedSegmentationModel::UIF_AUTHENTICATED:
      return this->GetServerStatus().status == AUTH_AUTHENTICATED;
    case DistributedSegmentationModel::UIF_TAGS_ASSIGNED:
      return AreAllRequiredTagsAssignedTarget();
    case DistributedSegmentationModel::UIF_CAN_DOWNLOAD:
      return IsSelectedTicketSuccessful();
    }

  return false;
}

void DistributedSegmentationModel::OnUpdate()
{
  if(this->m_EventBucket->HasEvent(LayerChangeEvent())
     || this->m_EventBucket->HasEvent(WrapperMetadataChangeEvent()))
    {
    // Layers have changed! Make sure that all layers currently referenced in the tag
    // table are still valid layers, otherwise change to unassigned
    this->UpdateTagObjectIds(false);
    }
}

std::vector<std::string> DistributedSegmentationModel::GetUserServerList() const
{
  std::vector<std::string> user_servers;
  user_servers.insert(user_servers.end(),
                      m_ServerURLList.begin() + m_SystemServerURLList.size(),
                      m_ServerURLList.end());
  return user_servers;
}

void DistributedSegmentationModel::SetUserServerList(const std::vector<std::string> &servers)
{
  // Get the current server (the URL model is always valid)
  std::string my_server = m_ServerURLList[this->GetServerURL()];

  // Reset the list of servers
  m_ServerURLList = m_SystemServerURLList;
  m_ServerURLList.insert(m_ServerURLList.end(), servers.begin(), servers.end());

  // Is the selected server still on the list
  std::vector<std::string>::const_iterator it =
      std::find(m_ServerURLList.begin(), m_ServerURLList.end(), my_server);
  if(it == m_ServerURLList.end())
    this->SetServerURL(0);
  else
    this->SetServerURL(it - m_ServerURLList.begin());

  // Update the domain
  m_ServerURLModel->InvokeEvent(DomainChangedEvent());

}

std::string DistributedSegmentationModel::GetURL(const std::string &path)
{
  // Get the main part of the URL
  std::string server = m_ServerURLList[m_ServerURLModel->GetValue()];
  return path.length() ? server + "/" + path : server;
}

void DistributedSegmentationModel::SetServiceListing(const ServiceListing &listing)
{
  // Get the current service info
  int curr_service_id;
  bool is_curr_service_valid = m_CurrentServiceModel->GetValueAndDomain(curr_service_id, NULL);
  std::string curr_service_hash;
  if(is_curr_service_valid)
    curr_service_hash = m_ServiceListing[curr_service_id].githash;

  // Set the service listing
  m_ServiceListing = listing;

  // Deal with empty listing
  if(m_ServiceListing.size() == 0)
    {
    m_CurrentServiceModel->SetDomain(CurrentServiceDomain());
    m_CurrentServiceModel->SetIsValid(false);
    return;
    }

  // Sort the service listing
  std::sort(m_ServiceListing.begin(), m_ServiceListing.end(), dss_model::service_summary_cmp);

  // Generate the domain for the selected service model
  CurrentServiceDomain domain;
  int new_service_id = 0;
  for(int i = 0; i < m_ServiceListing.size(); i++)
    {
    std::ostringstream oss;
    oss << m_ServiceListing[i].name << " " << m_ServiceListing[i].version
        << " : " << m_ServiceListing[i].desc;
    domain[i] = oss.str();

    if(is_curr_service_valid && m_ServiceListing[i].githash == curr_service_hash)
      new_service_id = i;
    }

  // Set the current service
  m_CurrentServiceModel->SetIsValid(true);
  m_CurrentServiceModel->SetDomain(domain);
  m_CurrentServiceModel->SetValue(new_service_id);
}

DistributedSegmentationModel::LoadAction
DistributedSegmentationModel::GetTagLoadAction(int tag_index) const
{
  if(tag_index < 0 || tag_index >= m_TagSpecArray.size())
    return LOAD_NONE;

  TagType type = m_TagSpecArray[tag_index].tag_spec.type;

  bool have_main = m_Parent->GetDriver()->IsMainImageLoaded();
  if(type == TAG_LAYER_MAIN || (type == TAG_LAYER_ANATOMICAL && !have_main))
    {
    return LOAD_MAIN;
    }
  else if((type == TAG_LAYER_OVERLAY || type == TAG_LAYER_ANATOMICAL) && have_main)
    {
    return LOAD_OVERLAY;
    }
  else return LOAD_NONE;
}

void DistributedSegmentationModel::ResetTagAssignment()
{
  this->UpdateTagObjectIds(true);
}

std::string DistributedSegmentationModel::GetCurrentServiceGitHash() const
{
  int index;
  if(m_CurrentServiceModel->GetValueAndDomain(index, NULL))
    return m_ServiceListing[index].githash;
  else
    return std::string();
}

void DistributedSegmentationModel::ApplyTagsToTargets()
{
  GenericImageData *id = m_Parent->GetDriver()->GetIRISImageData();
  for(int i = 0; i < m_TagSpecArray.size(); i++)
    {
    TagSpec &ts = m_TagSpecArray[i].tag_spec;
    if(ts.IsLayerType())
      {
      ImageWrapperBase *wrapper = id->FindLayer(m_TagSpecArray[i].object_id, false);
      if(wrapper)
        {
        // Add tag to the target wrapper
        TagList tags = wrapper->GetTags();
        if(tags.AddTag(ts.name))
          wrapper->SetTags(tags);

        // Remove tag from all other wrappers
        for(LayerIterator it = id->GetLayers(); !it.IsAtEnd(); ++it)
          if(it.GetLayer() != wrapper)
            {
            TagList tags = it.GetLayer()->GetTags();
            if(tags.RemoveTag(ts.name))
              it.GetLayer()->SetTags(tags);
            }
        }
      }
    else if(ts.type == TAG_POINT_LANDMARK)
      {
      // Iterate over the available annotations
      ImageAnnotationData *iad = id->GetAnnotations();
      for(ImageAnnotationIterator<annot::LandmarkAnnotation *> it(iad); !it.IsAtEnd(); ++it)
        {
        TagList tags = (*it)->GetTags();
        if((*it)->GetUniqueId() == m_TagSpecArray[i].object_id)
          {
          if(tags.AddTag(ts.name))
            (*it)->SetTags(tags);
          }
        else if(tags.RemoveTag(ts.name))
          {
          (*it)->SetTags(tags);
          }
        }
      }
    }
}



void DistributedSegmentationModel::SubmitWorkspace(ProgressReporterDelegate *pdel)
{
  // At this point the project had to be saved. We read it using the API object
  WorkspaceAPI ws;
  ws.ReadFromXMLFile(m_Parent->GetGlobalState()->GetProjectFilename().c_str());

  // Create a command that reports accumulated progress
  SmartPtr<itk::Command> cmd = pdel->CreateCommand();

  // Do the upload magic
  int ticket_id = ws.CreateWorkspaceTicket(this->GetCurrentServiceGitHash().c_str(), cmd);

  // Associate the ticket id with the workspace file for the future
  UniversalTicketId uti(this->GetURL(""), ticket_id);
  m_TicketWorkspaceMap[uti].source_workspace = m_Parent->GetGlobalState()->GetProjectFilename();
  m_SelectedTicketLocalWorkspaceModel->InvokeEvent(ValueChangedEvent());

  // Stick into the model
  m_SubmittedTicketId = ticket_id;
}

bool DistributedSegmentationModel::IsSelectedTicketSuccessful()
{
  IdType selected_ticket_id;
  return
      m_TicketListModel->GetValueAndDomain(selected_ticket_id, NULL)
      && (m_TicketListing.find(selected_ticket_id) != m_TicketListing.end())
      && (m_TicketListing[selected_ticket_id].status == STATUS_SUCCESS);
}

std::string DistributedSegmentationModel::DownloadWorkspace(
    const std::string &target_fn, ProgressReporterDelegate *pdel)
{
  // Is there a valid ticket id with status of success?
  IdType selected_ticket_id;
  if(!m_TicketListModel->GetValueAndDomain(selected_ticket_id, NULL)
     || m_TicketListing.find(selected_ticket_id) == m_TicketListing.end()
     || m_TicketListing[selected_ticket_id].status != STATUS_SUCCESS)
    return "";

  // Split the target filename into a diretory and a file
  std::string dirname, filename;
  if(itksys::SystemTools::FileIsDirectory(target_fn) ||
     (target_fn.find_last_of(".itksnap") == std::string::npos))
    {
    dirname = target_fn.c_str();
    }
  else
    {
    dirname = itksys::SystemTools::GetFilenamePath(target_fn);
    filename = itksys::SystemTools::GetFilenameName(target_fn);
    }

  // Create the directory if it does not exist
  if(!itksys::SystemTools::MakeDirectory(dirname))
    throw IRISException("Could not create directory %s", dirname.c_str());

  // Create a command that reports accumulated progress
  SmartPtr<itk::Command> cmd = pdel->CreateCommand();

  // Download into this directory
  std::string file_list_str =
      WorkspaceAPI::DownloadTicketFiles(selected_ticket_id, dirname.c_str(), false, "results",
                                        filename.size() ? filename.c_str() : NULL, cmd);
  std::vector<std::string> file_list;
  itksys::SystemTools::Split(file_list_str, file_list);

  // Find the .itksnap file
  for(int i = 0; i < file_list.size(); i++)
    {
    if(itksys::SystemTools::GetFilenameLastExtension(file_list[i]) == ".itksnap")
      {
      UniversalTicketId uti(this->GetURL(""), selected_ticket_id);
      m_TicketWorkspaceMap[uti].result_workspace = file_list[i];
      m_SelectedTicketResultWorkspaceModel->InvokeEvent(ValueChangedEvent());
      return file_list[i];
      }
    }

  throw IRISException("A workspace file was not among the files that were downloaded.\n"
                      "File list: \n%s",
                      file_list_str.c_str());
}

std::string DistributedSegmentationModel::SuggestDownloadFilename()
{
  // Is there a valid ticket id selected?
  IdType selected_ticket_id;
  if(m_TicketListModel->GetValueAndDomain(selected_ticket_id, NULL))
    {
    // The path under which we will be saving the project
    char ticket_file[4096];

    // Do we have a workspace location for this ticket?
    std::string local_ws = this->GetSelectedTicketLocalWorkspace();
    if(local_ws.size())
      {
      // Create the ticket in the same folder as the local workspace
      sprintf(ticket_file, "%s/ticket_%08ld_results.itksnap",
              itksys::SystemTools::GetFilenamePath(local_ws).c_str(),
              selected_ticket_id);
      }
    else
      {
      // Generate a default path for download
      sprintf(ticket_file, "%s/ticket_%08ld/ticket_%08ld_results.itksnap",
              this->GetDownloadLocation().c_str(),
              selected_ticket_id, selected_ticket_id);
      }

    return ticket_file;
    }

  return std::string();
}

void DistributedSegmentationModel::DeleteSelectedTicket()
{
  // Is there a valid ticket id with status of success?
  IdType selected_ticket_id;
  if(!m_TicketListModel->GetValueAndDomain(selected_ticket_id, NULL)
     || m_TicketListing.find(selected_ticket_id) == m_TicketListing.end())
    return;

  // Delete the ticket
  RESTClient rc;
  if(!rc.Get("api/tickets/%d/delete", selected_ticket_id))
    throw IRISException("Error deleting ticket %d: %s", selected_ticket_id, rc.GetResponseText());

  // Select the next ticket in the list
  TicketListingResponse::const_iterator it = m_TicketListing.find(selected_ticket_id);
  if(++it != m_TicketListing.end())
    m_TicketListModel->SetValue(it->first);
  else if(m_TicketListing.size())
    m_TicketListModel->SetValue(m_TicketListing.rbegin()->first);
  else
    m_TicketListModel->SetIsValid(false);

  // Remove the ticket
  m_TicketListing.erase(selected_ticket_id);
  m_TicketListModel->InvokeEvent(DomainChangedEvent());
}

IdType DistributedSegmentationModel::GetLastLogIdOfSelectedTicket()
{
  // There must be a selected ticket, the detail must be for that ticket and there
  // must be some log messages in the detail
  IdType selected_ticket_id;
  if(!m_TicketListModel->GetValueAndDomain(selected_ticket_id, NULL)
     || selected_ticket_id != m_SelectedTicketDetail.ticket_id
     || m_SelectedTicketDetail.log.size() == 0)
    {
    return 0;
    }

  // Get the latest id
  return m_SelectedTicketDetail.log.back().id;
}

const TicketDetailResponse *DistributedSegmentationModel::GetSelectedTicketDetail()
{
  // There must be a selected ticket, the detail must be for that ticket and there
  // must be some log messages in the detail
  IdType selected_ticket_id;
  if(!m_TicketListModel->GetValueAndDomain(selected_ticket_id, NULL)
     || selected_ticket_id != m_SelectedTicketDetail.ticket_id)
    {
    return NULL;
    }

  return &m_SelectedTicketDetail;
}

bool
DistributedSegmentationModel::AsyncGetServiceListing(
    std::vector<dss_model::ServiceSummary> &services)
{
  try
  {
  // Second, try to get service listing
  RESTClient rc;

  if(rc.Get("api/services?format=json"))
    {
    Json::Reader json_reader; Json::Value root;
    if(json_reader.parse(rc.GetOutput(), root, false))
      {
      const Json::Value res = root["result"];
      for(int i = 0; i < res.size(); i++)
        {
        dss_model::ServiceSummary service;
        service.name = res[i].get("name","").asString();
        service.githash = res[i].get("githash","").asString();
        service.version = res[i].get("version","").asString();
        service.desc = res[i].get("shortdesc","").asString();
        services.push_back(service);
        }
      }

    return true;
    }
  }
  catch(...)
  {
  }

  return false;
}

StatusCheckResponse
DistributedSegmentationModel::AsyncCheckStatus(std::string url, std::string token)
{
  dss_model::StatusCheckResponse response;

  try
    {
    // Set the server URL to the new location
    RESTClient rc;
    rc.SetServerURL(url.c_str());

    // If there is a token, post it
    bool status_login;
    if(token.size() > 0)
      {
      rc.SetReceiveCookieMode(true);
      status_login = rc.Post("api/login?format=json", "token=%s", token.c_str());
      }
    else
      {
      status_login = rc.Get("api/login?format=json");
      }

    // If we got here without an exception, that means we are connected
    response.auth_response.status = AUTH_CONNECTED_NOT_AUTHENTICATED;

    // If status is not ok, that's it
    if(status_login == false)
      return response;

    // Check if we got an email address
    Json::Reader json_reader;
    Json::Value root;
    if(json_reader.parse(rc.GetOutput(), root, false))
      {
      const Json::Value data = root["result"];
      response.auth_response.user_email = data.get("email","").asString();
      }

    // If no email address we are not logged in
    if(response.auth_response.user_email.length() == 0)
      return response;
    }
  catch(...)
    {
    response.auth_response.status = AUTH_NOT_CONNECTED;
    }

  // See if we can successfully get a listing
  if(AsyncGetServiceListing(response.service_listing))
    {
    response.auth_response.status = AUTH_AUTHENTICATED;
    }

  return response;
}

void DistributedSegmentationModel::ApplyStatusCheckResponse(const StatusCheckResponse &result)
{
  // Set the status
  this->SetServerStatus(result.auth_response);

  // If successfully logged in, clear the token field
  if(result.auth_response.status == AUTH_AUTHENTICATED)
    this->SetToken("");

  // Set the service listing
  SetServiceListing(result.service_listing);
}

ServiceDetailResponse
DistributedSegmentationModel::AsyncGetServiceDetails(std::string githash)
{
  ServiceDetailResponse result;
  result.valid = false;

  RegistryEnumMap<TagType> type_map;
  type_map.AddPair(TAG_POINT_LANDMARK, "PointLandmark");
  type_map.AddPair(TAG_LAYER_MAIN, "MainImage");
  type_map.AddPair(TAG_LAYER_OVERLAY, "OverlayImage");
  type_map.AddPair(TAG_LAYER_ANATOMICAL, "AnatomicalImage");
  type_map.AddPair(TAG_SEGMENTATION_LABEL, "SegmentationLabel");
  type_map.AddPair(TAG_UNKNOWN, "Unknown");

  try {
    RESTClient rc;
    if(rc.Get("api/services/%s/detail", githash.c_str()))
      {
      Json::Reader json_reader;
      Json::Value root;
      if(json_reader.parse(rc.GetOutput(), root, false))
        {
        result.longdesc = root.get("longdesc","").asString();
        result.url = root.get("url","").asString();
        result.valid = true;
        const Json::Value tag_group = root["tags"];
        for(int i = 0; i < tag_group.size(); i++)
          {
          TagSpec tag_spec;
          tag_spec.required = tag_group[i].get("required", false).asBool();
          tag_spec.type = type_map.GetEnumValueWithDefault(
                            tag_group[i].get("type","").asString(), TAG_UNKNOWN);
          tag_spec.name = tag_group[i].get("name","").asString();
          tag_spec.hint = tag_group[i].get("hint","").asString();
          result.tag_specs.push_back(tag_spec);
          }
        }
      }
    }
  catch (...) {

    }

  return result;
}

// Find a unique object that matches a tag, and if found, assign it to the tag
bool DistributedSegmentationModel::FindUniqueObjectForTag(TagTargetSpec &tag)
{
  // Get the driver
  IRISApplication *driver = this->GetParent()->GetDriver();
  if(driver->IsMainImageLoaded())
    {
    int role_filter = -1;
    switch(tag.tag_spec.type)
      {
      case TAG_LAYER_MAIN: role_filter = MAIN_ROLE; break;
      case TAG_LAYER_OVERLAY: role_filter = OVERLAY_ROLE; break;
      case TAG_LAYER_ANATOMICAL: role_filter = MAIN_ROLE | OVERLAY_ROLE; break;
      default: break;
      }

    // Is there an image to search for?
    if(role_filter >= 0)
      {
      std::list<ImageWrapperBase*> matches = driver->GetIRISImageData()->FindLayersByRole(role_filter);
      if(matches.size() == 1)
        {
        // Unique match found. Assign the object to the tag.
        tag.object_id = matches.front()->GetUniqueId();
        tag.desc = matches.front()->GetNickname();
        return true;
        }
      else if (matches.size() > 1)
        {
        // There are multiple matches. Look through them to see if the present tag fits
        std::list<ImageWrapperBase*> tag_matching_layers;
        for(std::list<ImageWrapperBase*>::iterator it = matches.begin(); it != matches.end(); ++it)
          {
          ImageWrapperBase *w = *it;
          if(w->GetUniqueId() == tag.object_id)
            {
            // The currently assigned object still matches. Keep the assignment
            tag.desc = w->GetNickname();
            return true;
            }
          else if(w->GetTags().Contains(tag.tag_spec.name))
            {
            // Found another object with the matching tag. Then it may be assignable
            tag_matching_layers.push_back(w);
            }
          }

        // The current id is not in the list of matching layers.
        if(tag_matching_layers.size() == 1)
          {
          // There was one layer that contained the current tag! We can assign it to the layer
          tag.object_id = tag_matching_layers.front()->GetUniqueId();
          tag.desc = tag_matching_layers.front()->GetNickname();
          return true;
          }
        }
      }

    else if(tag.tag_spec.type == TAG_POINT_LANDMARK)
      {
      // For landmarks, we only assign them to tags if the name of the landmark matches
      // the name of the tag, or the landmark already has a tag.
      std::list<const annot::LandmarkAnnotation *> matches;

      // Iterate over the available annotations
      ImageAnnotationData *iad = driver->GetIRISImageData()->GetAnnotations();
      for(ImageAnnotationIterator<annot::LandmarkAnnotation *> it(iad); !it.IsAtEnd(); ++it)
        {
        if((*it)->GetUniqueId() == tag.object_id)
          matches.push_back((*it));
        else if((*it)->GetTags().Contains(tag.tag_spec.name))
          matches.push_back((*it));
        else if((*it)->GetLandmark().Text == tag.tag_spec.name)
          matches.push_back((*it));
        }

      // Handle the matches. We only assign a match if there is one matching object per list
      if(matches.size() == 1)
        {
        tag.object_id = matches.front()->GetUniqueId();
        tag.desc = matches.front()->GetLandmark().Text;
        return true;
        }
      }
    }

  // The tag was not matched. Assign id 0 and desc unassigned
  tag.object_id = 0L;
  tag.desc = "Unassigned";
  return false;
}

void DistributedSegmentationModel::UpdateTagObjectIds(bool clear_ids_first)
{
  // Handle the main image assignment
  for(int i = 0; i < m_TagSpecArray.size(); i++)
    {
    // Reset the tag to 'unassigned'
    TagTargetSpec &tag = m_TagSpecArray[i];
    if(clear_ids_first)
      {
      tag.object_id = 0;
      tag.desc = "Unassigned";
      }

    // Find a match for the tag
    FindUniqueObjectForTag(tag);
    }

  // The domain for the tag list has changed!
  m_TagListModel->InvokeEvent(DomainDescriptionChangedEvent());
}

void DistributedSegmentationModel::ApplyServiceDetailResponse(const ServiceDetailResponse &resp)
{
  this->SetServiceDescription(resp.longdesc);

  // Store the tag spec array
  m_TagSpecArray.clear();
  for(int i = 0; i < resp.tag_specs.size(); i++)
    {
    TagTargetSpec ttspec;
    ttspec.tag_spec = resp.tag_specs[i];
    ttspec.object_id = 0;
    m_TagSpecArray.push_back(ttspec);
    }

  // Assign tag ids to objects in current workspace
  this->UpdateTagObjectIds(true);

  // Fire off a domain modified event
  m_TagListModel->InvokeEvent(DomainChangedEvent());
}

TicketListingResponse DistributedSegmentationModel::AsyncGetTicketListing()
{
  TicketListingResponse result;

  try {
    RESTClient rc;
    if(rc.Get("api/tickets?format=json"))
      {
      Json::Reader json_reader;
      Json::Value root;
      if(json_reader.parse(rc.GetOutput(), root, false))
        {
        const Json::Value tickets = root["result"];
        for(int i = 0; i < tickets.size(); i++)
          {
          TicketStatusSummary tss;
          tss.id = (IdType) tickets[i].get("id",0).asLargestInt();
          tss.service_name = tickets[i].get("service","").asString();
          tss.status = ticket_status_emap.GetEnumValueWithDefault(
                         tickets[i].get("status","").asString(), STATUS_UNKNOWN);
          result[tss.id] = tss;
          }
        }
      }
    }
  catch (...)
  {
  }

  return result;
}

void DistributedSegmentationModel::ApplyTicketListingResponse(const TicketListingResponse &resp)
{
  // Check if the ticket listing has changed
  bool same_keys = true;
  if(m_TicketListing.size() != resp.size())
    {
    same_keys = false;
    }
  else
    {
    TicketListingResponse::const_iterator it_old = m_TicketListing.begin();
    TicketListingResponse::const_iterator it_new = resp.begin();

    for(; it_old != m_TicketListing.end() && it_new != resp.end(); it_old++, it_new++)
      {
      if(it_old->first != it_new->first)
        {
        same_keys = false;
        break;
        }
      }
    }

  // Just store the ticket listing
  m_TicketListing = resp;

  // Set the status of the model
  if(m_TicketListing.size() > 0)
    { 
    m_TicketListModel->SetIsValid(true);

    // If the current value of the model is not in the listing point it to the first entry
    if(m_TicketListing.find(m_TicketListModel->GetValue()) == m_TicketListing.end())
      m_TicketListModel->SetValue(m_TicketListing.begin()->first);
    }
  else
    {
    m_TicketListModel->SetIsValid(false);
    m_TicketListModel->SetValue(-1);
    }

  // If we have recently submitted a ticket and it now appears in the list, select it
  if(m_TicketListing.find(m_SubmittedTicketId) != m_TicketListing.end())
    {
    m_TicketListModel->SetValue(m_SubmittedTicketId);
    m_SubmittedTicketId = -1;
    }

  // Fire the right domain event
  if(same_keys)
    m_TicketListModel->InvokeEvent(DomainDescriptionChangedEvent());
  else
    m_TicketListModel->InvokeEvent(DomainChangedEvent());
}

TicketDetailResponse DistributedSegmentationModel::AsyncGetTicketDetails(IdType ticket_id, IdType last_log)
{
  TicketDetailResponse tdr;
  tdr.ticket_id = ticket_id;
  tdr.progress = 0.0;

  try {

    // Get a full update on this ticket
    RESTClient rc;
    if(rc.Get("api/tickets/%ld/detail?since=%ld", ticket_id, last_log))
      {
      Json::Reader json_reader;
      Json::Value root;
      if(json_reader.parse(rc.GetOutput(), root, false))
        {
        const Json::Value result = root["result"];

        // Read progress
        tdr.progress = result.get("progress",0.0).asDouble();
        tdr.queue_position = result.get("queue_position",0).asInt();

        // Read logs
        const Json::Value log_entry = result["log"];
        for(int i = 0; i < log_entry.size(); i++)
          {
          TicketLogEntry loglet;
          loglet.id = log_entry[i].get("id",0).asLargestInt();
          loglet.type = log_type_emap.GetEnumValueWithDefault(
                             log_entry[i].get("category","").asString(), LOG_UNKNOWN);
          loglet.atime = log_entry[i].get("atime","").asString();
          loglet.text = log_entry[i].get("message","").asString();

          const Json::Value att_entry = log_entry[i]["attachments"];
          for(int i = 0; i < att_entry.size(); i++)
            {
            Attachment att;
            att.desc = att_entry[i].get("description","").asString();
            att.url = att_entry[i].get("url","").asString();
            att.mimetype = att_entry[i].get("mime_type","").asString();
            loglet.attachments.push_back(att);
            }

          tdr.log.push_back(loglet);
          }
        }
      }
    }
  catch (...) {

    }

  return tdr;
}

void DistributedSegmentationModel::ApplyTicketDetailResponse(const TicketDetailResponse &resp)
{
  // Make sure that the detail is for the ticket that is currently selected
  IdType selected_ticket_id;
  if(!m_TicketListModel->GetValueAndDomain(selected_ticket_id, NULL)
     || selected_ticket_id != resp.ticket_id)
    {
    // Just ignore this update - it is irrelevant because we selected another ticket already
    return;
    }

  // Store the progress
  m_SelectedTicketProgressModel->SetValue(resp.progress);
  m_SelectedTicketProgressModel->SetIsValid(true);

  // Store the queue position
  m_SelectedTicketQueuePositionModel->SetValue(resp.queue_position);
  m_SelectedTicketQueuePositionModel->SetIsValid(true);

  // Store the log for the current ticket
  bool log_modified = false;
  if(m_SelectedTicketDetail.ticket_id != resp.ticket_id)
    {
    m_SelectedTicketDetail.log.clear();
    log_modified = true;
    }

  // Append the log
  for(std::vector<TicketLogEntry>::const_iterator it = resp.log.begin(); it != resp.log.end(); ++it)
    {
    m_SelectedTicketDetail.log.push_back(*it);
    log_modified = true;
    }

  // Update the other fields
  m_SelectedTicketDetail.progress = resp.progress;
  m_SelectedTicketDetail.ticket_id = resp.ticket_id;

  // Cause update in the log model
  m_SelectedTicketLogModel->SetIsValid(true);
  if(log_modified)
    m_SelectedTicketLogModel->InvokeEvent(DomainChangedEvent());
}

bool DistributedSegmentationModel::GetDownloadLocationValue(std::string &value)
{
  std::string url = this->GetURL("");
  std::map<std::string, std::string>::const_iterator it = m_ServerDownloadLocationMap.find(url);
  if(it != m_ServerDownloadLocationMap.end())
    value = it->second;
  else
    m_ServerDownloadLocationMap[url] = value = this->GetDefaultDownloadLocation();
  return true;
}

void DistributedSegmentationModel::SetDownloadLocationValue(std::string value)
{
  std::string url = this->GetURL("");
  m_ServerDownloadLocationMap[url] = value;
}

bool DistributedSegmentationModel::GetTagListValueAndRange(int &value, DistributedSegmentationModel::TagDomainType *domain)
{
  // Must be logged in and have a list of tags
  if(this->GetServerStatus().status != AUTH_AUTHENTICATED || this->m_TagSpecArray.size() == 0)
    return false;

  // Set the value
  value = m_CurrentTag;

  // Handle the domain
  if(domain)
    domain->SetWrappedVector(&m_TagSpecArray);

  return true;
}

void DistributedSegmentationModel::SetTagListValue(int value)
{
  m_CurrentTag = value;
}

bool DistributedSegmentationModel
::GetCurrentTagWorkspaceObjectValueAndRange(unsigned long &value, LayerSelectionDomain *domain)
{
  int curr_tag;
  if(m_TagListModel->GetValueAndDomain(curr_tag, NULL))
    {
    TagTargetSpec &tag = m_TagSpecArray[curr_tag];
    value = tag.object_id;
    if(domain)
      {
      domain->clear();
      (*domain)[0] = "Unassigned";
      IRISApplication *driver = this->GetParent()->GetDriver();

      // Handle tags to image layers
      if(tag.tag_spec.IsLayerType())
        {
        int role_filter = -1;
        switch(tag.tag_spec.type)
          {
          case TAG_LAYER_MAIN: role_filter = MAIN_ROLE; break;
          case TAG_LAYER_OVERLAY: role_filter = OVERLAY_ROLE; break;
          case TAG_LAYER_ANATOMICAL: role_filter = MAIN_ROLE | OVERLAY_ROLE; break;
          default: break;
          }
        if(role_filter >= 0 && driver->IsMainImageLoaded())
          {
          for(LayerIterator it = driver->GetIRISImageData()->GetLayers(role_filter);
              !it.IsAtEnd(); ++it)
            {
            (*domain)[it.GetLayer()->GetUniqueId()] = it.GetLayer()->GetNickname();
            }
          }
        }

      // Handle tags to point landmarks
      else if(tag.tag_spec.type == TAG_POINT_LANDMARK)
        {
        // Iterate over the available annotations
        ImageAnnotationData *iad = driver->GetIRISImageData()->GetAnnotations();
        for(ImageAnnotationIterator<annot::LandmarkAnnotation *> it(iad); !it.IsAtEnd(); ++it)
          {
          std::ostringstream oss;
          oss << "Landmark " << (*it)->GetLandmark().Text
              << " [" << (*it)->GetLandmark().Pos << "]";
          (*domain)[(*it)->GetUniqueId()] = oss.str();
          }
        }
      }
    return true;
    }
  return false;
}

void DistributedSegmentationModel
::SetCurrentTagWorkspaceObjectValue(unsigned long value)
{
  int curr_tag;
  IRISApplication *driver = this->GetParent()->GetDriver();
  if(m_TagListModel->GetValueAndDomain(curr_tag, NULL))
    {
    // Get the tag spec
    dss_model::TagTargetSpec &ts = m_TagSpecArray[curr_tag];

    // Set the target id
    ts.object_id = value;

    // Set the target description
    if(ts.tag_spec.IsLayerType())
      {
      ImageWrapperBase *w = driver->GetIRISImageData()->FindLayer(value, false);
      ts.desc = w ? w->GetNickname() : "Unassigned";
      }
    else if(ts.tag_spec.type == TAG_POINT_LANDMARK)
      {
      ts.desc = "Unassigned";
      ImageAnnotationData *iad = driver->GetIRISImageData()->GetAnnotations();
      for(ImageAnnotationIterator<annot::LandmarkAnnotation *> it(iad); !it.IsAtEnd(); ++it)
        {
        if((*it)->GetUniqueId() == ts.object_id)
          {
          std::ostringstream oss;
          oss << "Landmark " << (*it)->GetLandmark().Text
              << " [" << (*it)->GetLandmark().Pos << "]";
          ts.desc = oss.str();
          break;
          }
        }
      }

    // Update the domain
    m_TagListModel->InvokeEvent(DomainChangedEvent());
    }
}

bool DistributedSegmentationModel::GetSelectedTicketStatusValue(TicketStatus &value)
{
  IdType selected_ticket_id;
  if(m_TicketListModel->GetValueAndDomain(selected_ticket_id, NULL))
    {
    TicketListingResponse::const_iterator it = m_TicketListing.find(selected_ticket_id);
    if(it != m_TicketListing.end())
      {
      value = it->second.status;
      return true;
      }
    }

  return false;
}

bool DistributedSegmentationModel::GetSelectedTicketLocalWorkspaceValue(std::string &value)
{
  IdType selected_ticket_id;
  if(m_TicketListModel->GetValueAndDomain(selected_ticket_id, NULL))
    {
    UniversalTicketId uti(this->GetURL(""), selected_ticket_id);
    TicketWorkspaceMap::const_iterator it = m_TicketWorkspaceMap.find(uti);
    if(it != m_TicketWorkspaceMap.end())
      value = it->second.source_workspace;
    else
      value = "";
    return true;
    }

  return false;
}

bool DistributedSegmentationModel::GetSelectedTicketResultWorkspaceValue(std::string &value)
{
  IdType selected_ticket_id;
  if(m_TicketListModel->GetValueAndDomain(selected_ticket_id, NULL))
    {
    UniversalTicketId uti(this->GetURL(""), selected_ticket_id);
    TicketWorkspaceMap::const_iterator it = m_TicketWorkspaceMap.find(uti);
    if(it != m_TicketWorkspaceMap.end())
      value = it->second.result_workspace;
    else
      value = "";
    return true;
    }

  return false;
}


std::string
DistributedSegmentationModel::GetDefaultDownloadLocation()
{
  // Get the standard Documents location
  std::string dir_docs =
      m_Parent->GetSystemInterface()->GetSystemInfoDelegate()->GetUserDocumentsLocation();

  // Append the current URL
  std::string encoded_url =
      m_Parent->GetSystemInterface()->GetSystemInfoDelegate()->EncodeServerURL(this->GetURL(""));

  // Replace reserved characters with underscores
  std::string reschar(";/?:@=&.");
  for(int i = 0; i < encoded_url.size(); i++)
    {
    char c = encoded_url[i];
    if(reschar.find(c) != std::string::npos)
      encoded_url[i] = '_';
    }

  // Remove characters before and after
  size_t p1 = encoded_url.find_first_not_of('_');
  size_t p2 = encoded_url.find_last_not_of('_');
  encoded_url = encoded_url.substr(p1, 1 + p2 - p1);

  // Append ITK-SNAP
  std::string full_dir = dir_docs + std::string("/") + "ITK-SNAP" + std::string("/") + encoded_url;

  // Convert to correct slashes
  return itksys::SystemTools::ConvertToOutputPath(full_dir);
}

DistributedSegmentationModel::DistributedSegmentationModel()
{
  // Build a list of available URLs
  m_SystemServerURLList.push_back("https://dss.itksnap.org");

  // Add system URLs to the url list
  m_ServerURLList = m_SystemServerURLList;

  // Create the server model that references the URL list
  m_ServerURLModel = NewConcreteProperty(0, ServerURLDomain(&m_ServerURLList));

  // Create the token model
  m_TokenModel = NewSimpleConcreteProperty(std::string(""));

  // Server status model
  m_ServerStatusModel = NewSimpleConcreteProperty(AuthResponse());

  // Initialize the service model
  m_CurrentServiceModel = NewConcreteProperty(-1, CurrentServiceDomain());
  m_CurrentServiceModel->SetIsValid(false);

  // Download location model
  m_DownloadLocationModel = wrapGetterSetterPairAsProperty(
                              this,
                              &Self::GetDownloadLocationValue,
                              &Self::SetDownloadLocationValue);
  m_DownloadLocationModel->RebroadcastFromSourceProperty(m_ServerStatusModel);

  // Service description
  m_ServiceDescriptionModel = NewSimpleConcreteProperty(std::string());

  // Tag selection model
  m_TagListModel = wrapGetterSetterPairAsProperty(
                     this,
                     &Self::GetTagListValueAndRange,
                     &Self::SetTagListValue);

  // Ticket listing model
  m_TicketListModel = NewConcreteProperty((IdType) -1, TicketListingDomain(&m_TicketListing));
  m_TicketListModel->SetIsValid(false);

  // Model for current tag selection
  m_CurrentTagWorkspaceObjectModel = wrapGetterSetterPairAsProperty(
                                  this,
                                  &Self::GetCurrentTagWorkspaceObjectValueAndRange,
                                  &Self::SetCurrentTagWorkspaceObjectValue);
  m_CurrentTagWorkspaceObjectModel->Rebroadcast(m_TagListModel, ValueChangedEvent(), ValueChangedEvent());
  m_CurrentTagWorkspaceObjectModel->Rebroadcast(m_TagListModel, ValueChangedEvent(), DomainChangedEvent());

  // Last submitted ticket
  m_SubmittedTicketId = -1;

  // Selected ticket progress model
  m_SelectedTicketProgressModel = NewRangedConcreteProperty(0.0, 0.0, 1.0, 0.01);
  m_SelectedTicketProgressModel->SetIsValid(false);

  // Selected ticket queue position
  m_SelectedTicketQueuePositionModel = NewSimpleConcreteProperty(0);
  m_SelectedTicketQueuePositionModel->SetIsValid(false);

  // Selected ticket logs
  m_SelectedTicketLogModel = NewConcreteProperty((IdType) -1, LogDomainType(&m_SelectedTicketDetail.log));
  m_SelectedTicketLogModel->SetIsValid(false);

  // Selected ticket status
  m_SelectedTicketStatusModel = wrapGetterSetterPairAsProperty(
                                  this,
                                  &Self::GetSelectedTicketStatusValue);
  m_SelectedTicketStatusModel->Rebroadcast(m_TicketListModel, ValueChangedEvent(), ValueChangedEvent());
  m_SelectedTicketStatusModel->Rebroadcast(m_TicketListModel, DomainChangedEvent(), ValueChangedEvent());
  m_SelectedTicketStatusModel->Rebroadcast(m_TicketListModel, DomainDescriptionChangedEvent(), ValueChangedEvent());

  // Selected ticket local workspace
  m_SelectedTicketLocalWorkspaceModel = wrapGetterSetterPairAsProperty(
                                          this,
                                          &Self::GetSelectedTicketLocalWorkspaceValue);
  m_SelectedTicketLocalWorkspaceModel->Rebroadcast(m_TicketListModel, ValueChangedEvent(), ValueChangedEvent());
  m_SelectedTicketLocalWorkspaceModel->Rebroadcast(m_TicketListModel, DomainChangedEvent(), ValueChangedEvent());


  m_SelectedTicketResultWorkspaceModel = wrapGetterSetterPairAsProperty(
                                          this,
                                          &Self::GetSelectedTicketResultWorkspaceValue);
  m_SelectedTicketResultWorkspaceModel->Rebroadcast(m_TicketListModel, ValueChangedEvent(), ValueChangedEvent());
  m_SelectedTicketResultWorkspaceModel->Rebroadcast(m_TicketListModel, DomainChangedEvent(), ValueChangedEvent());

  // Download action model
  DownloadActionDomain dl_action_domain;
  dl_action_domain[DL_OPEN_CURRENT_WINDOW] = "Open workspace in the current ITK-SNAP window";
  dl_action_domain[DL_OPEN_NEW_WINDOW] = "Open workspace in a new ITK-SNAP window";
  dl_action_domain[DL_DONT_OPEN] = "Do not open workspace";
  m_DownloadActionModel = NewConcreteProperty(DL_OPEN_CURRENT_WINDOW, dl_action_domain);

  // Changes to the server and token result in a server change event
  this->Rebroadcast(m_ServerURLModel, ValueChangedEvent(), ServerChangeEvent());
  this->Rebroadcast(m_ServerURLModel, DomainChangedEvent(), ServerChangeEvent());
  this->Rebroadcast(m_TokenModel, ValueChangedEvent(), ServerChangeEvent());

  // Changes to the selected service also propagated
  this->Rebroadcast(m_CurrentServiceModel, ValueChangedEvent(), ServiceChangeEvent());
  this->Rebroadcast(m_CurrentServiceModel, DomainChangedEvent(), ServiceChangeEvent());

  // Changes to the tags table require a state update
  this->Rebroadcast(m_CurrentTagWorkspaceObjectModel, DomainChangedEvent(), StateMachineChangeEvent());
  this->Rebroadcast(m_ServerStatusModel, ValueChangedEvent(), StateMachineChangeEvent());
  this->Rebroadcast(m_TagListModel, DomainChangedEvent(), StateMachineChangeEvent());
  this->Rebroadcast(m_TagListModel, DomainDescriptionChangedEvent(), StateMachineChangeEvent());
  this->Rebroadcast(m_TicketListModel, ValueChangedEvent(), StateMachineChangeEvent());
  this->Rebroadcast(m_TicketListModel, DomainChangedEvent(), StateMachineChangeEvent());
  this->Rebroadcast(m_TicketListModel, DomainDescriptionChangedEvent(), StateMachineChangeEvent());


}


