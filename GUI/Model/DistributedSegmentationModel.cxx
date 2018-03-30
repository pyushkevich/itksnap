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
#include <sstream>

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

} // namespace

using namespace dss_model;

void DistributedSegmentationModel::SetParentModel(GlobalUIModel *model)
{
  m_Parent = model;
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
    case DistributedSegmentationModel::UIF_TAGS_ASSIGNED:
      return AreAllRequiredTagsAssignedTarget();
      break;

    }

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
    if(ts.type == TAG_LAYER_MAIN || ts.type == TAG_LAYER_ANATOMICAL)
      {
      ImageWrapperBase *wrapper = id->FindLayer(m_TagSpecArray[i].object_id, false);
      if(wrapper)
        {
        std::list<std::string> tags = wrapper->GetTags();
        if(std::find(tags.begin(), tags.end(), ts.name) == tags.end())
          {
          tags.push_back(ts.name);
          wrapper->SetTags(tags);
          }
        for(LayerIterator it = id->GetLayers(); !it.IsAtEnd(); ++it)
          if(it.GetLayer() != wrapper)
            {
            std::list<std::string> tags = it.GetLayer()->GetTags();
            if(std::find(tags.begin(), tags.end(), ts.name) != tags.end())
              {
              tags.remove(ts.name);
              it.GetLayer()->SetTags(tags);
              }
            }
        }
      }
    }
}


void DistributedSegmentationModel::SubmitWorkspace()
{
  // At this point the project had to be saved. We read it using the API object
  WorkspaceAPI ws;
  ws.ReadFromXMLFile(m_Parent->GetGlobalState()->GetProjectFilename().c_str());

  // Do the upload magic
  int ticket_id = ws.CreateWorkspaceTicket(this->GetCurrentServiceGitHash().c_str());

  // Stick into the model
  m_SubmittedTicketIdModel->SetValue(ticket_id);
  m_SubmittedTicketIdModel->SetIsValid(true);
}

bool
DistributedSegmentationModel::AsyncGetServiceListing(
    std::vector<dss_model::ServiceSummary> &services)
{
  try
  {
  // Second, try to get service listing
  RESTClient rc;
  if(rc.Get("api/services"))
    {
    FormattedTable ft;
    ft.ParseCSV(rc.GetOutput());

    for(int i = 0; i < ft.Rows(); i++)
      {
      dss_model::ServiceSummary service;
      service.name = ft(i, 0);
      service.githash = ft(i, 1);
      service.version = ft(i, 2);
      service.desc = ft(i, 3);
      services.push_back(service);
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
  response.auth_response.connected = false;
  response.auth_response.authenticated = false;

  // Try to get the service listing using the cached cookie
  if(AsyncGetServiceListing(response.service_listing))
    {
    response.auth_response.connected = true;
    response.auth_response.authenticated = true;
    return response;
    }

  // Failer likely means that we are not authenticated
  try
    {
    RESTClient rc;
    if(rc.Authenticate(url.c_str(), token.c_str()))
      {
      response.auth_response.connected = true;
      response.auth_response.authenticated = true;
      }
    else
      {
      response.auth_response.connected = true;
      return response;
      }
    }
  catch(...)
    {
    return response;
    }

  // Get the service listing again now that we are in
  AsyncGetServiceListing(response.service_listing);

  return response;
}

void DistributedSegmentationModel::ApplyStatusCheckResponse(const StatusCheckResponse &result)
{
  if(result.auth_response.connected)
    {
    if(result.auth_response.authenticated)
      SetServerStatus(DistributedSegmentationModel::CONNECTED_AUTHORIZED);
    else
      SetServerStatus(DistributedSegmentationModel::CONNECTED_NOT_AUTHORIZED);
    }
  else
    {
    SetServerStatus(DistributedSegmentationModel::NOT_CONNECTED);
    }

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

void DistributedSegmentationModel::AssignTagObjectIds()
{
  // Get the driver
  IRISApplication *driver = this->GetParent()->GetDriver();

  // Handle the main image assignment
  for(int i = 0; i < m_TagSpecArray.size(); i++)
    {
    TagTargetSpec &tag = m_TagSpecArray[i];
    tag.object_id = 0;
    tag.desc = std::string();

    if(driver->IsMainImageLoaded())
      {
      // If the tag is for the main image, then it has to be assigned to the main image
      if(tag.tag_spec.type == TAG_LAYER_MAIN)
        {
        ImageWrapperBase *main = driver->GetIRISImageData()->GetMain();
        tag.object_id = main->GetUniqueId();
        tag.desc = main->GetNickname();
        }
      else if(tag.tag_spec.type == TAG_LAYER_ANATOMICAL)
        {
        int role_filter = MAIN_ROLE | OVERLAY_ROLE;
        std::list<ImageWrapperBase*> matches =
            driver->GetIRISImageData()->FindLayersByTag(tag.tag_spec.name, role_filter);
        if(matches.size() == 1)
          {
          tag.object_id = matches.front()->GetUniqueId();
          tag.desc = matches.front()->GetNickname();
          }
        }
      }
    }
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
  this->AssignTagObjectIds();

  // Fire off a domain modified event
  m_TagListModel->InvokeEvent(DomainChangedEvent());
  if(resp.tag_specs.size())
    {
    m_TagListModel->SetValue(0);
    m_TagListModel->SetIsValid(true);
    }
  else
    {
    m_TagListModel->SetIsValid(false);
    }
}

bool DistributedSegmentationModel::GetServerStatusStringValue(std::string &value)
{
  ServerStatus status;
  ServerStatusDomain domain;
  if(m_ServerStatusModel->GetValueAndDomain(status, &domain))
    {
    value = domain[status];
    return true;
    }
  return false;
}

bool DistributedSegmentationModel
::GetCurrentTagImageLayerValueAndRange(unsigned long &value, LayerSelectionDomain *domain)
{
  int curr_tag;
  if(m_TagListModel->GetValueAndDomain(curr_tag, NULL))
    {
    TagTargetSpec &tag = m_TagSpecArray[curr_tag];
    value = tag.object_id;
    if(domain)
      {
      domain->clear();
      IRISApplication *driver = this->GetParent()->GetDriver();

      if(tag.tag_spec.type == TAG_LAYER_MAIN && driver->IsMainImageLoaded())
        {
        (*domain)[driver->GetIRISImageData()->GetMain()->GetUniqueId()] =
            driver->GetIRISImageData()->GetMain()->GetNickname();
        }
      else if(tag.tag_spec.type == TAG_LAYER_ANATOMICAL && driver->IsMainImageLoaded())
        {
        for(LayerIterator it = driver->GetIRISImageData()->GetLayers(MAIN_ROLE | OVERLAY_ROLE);
            !it.IsAtEnd(); ++it)
          {
          (*domain)[it.GetLayer()->GetUniqueId()] = it.GetLayer()->GetNickname();
          }
        }
      }
    return true;
    }
  return false;
}

void DistributedSegmentationModel
::SetCurrentTagImageLayerValue(unsigned long value)
{
  int curr_tag;
  IRISApplication *driver = this->GetParent()->GetDriver();
  if(m_TagListModel->GetValueAndDomain(curr_tag, NULL))
    {
    // Set the target id
    m_TagSpecArray[curr_tag].object_id = value;

    // Set the target description
    ImageWrapperBase *w = driver->GetIRISImageData()->FindLayer(value, false);
    m_TagSpecArray[curr_tag].desc = w ? w->GetNickname() : std::string();

    // Update the domain
    m_TagListModel->InvokeEvent(DomainChangedEvent());
    }
}

DistributedSegmentationModel::DistributedSegmentationModel()
{
  // Build a list of available URLs
  m_ServerURLList.push_back("https://dss.itksnap.org");

  // Create the server model that references the URL list
  m_ServerURLModel = NewConcreteProperty(0, ServerURLDomain(&m_ServerURLList));

  // Create the token model
  m_TokenModel = NewSimpleConcreteProperty(std::string(""));

  // Server status model
  ServerStatusDomain server_status_dom;
  server_status_dom[NOT_CONNECTED] = "Not connected";
  server_status_dom[CONNECTED_NOT_AUTHORIZED] = "Connected, Not Authorized";
  server_status_dom[CONNECTED_AUTHORIZED] = "Connected and Authorized";
  m_ServerStatusModel = NewConcreteProperty(NOT_CONNECTED, server_status_dom);

  // Server status string
  m_ServerStatusStringModel
      = wrapGetterSetterPairAsProperty(this, &Self::GetServerStatusStringValue);
  m_ServerStatusStringModel->RebroadcastFromSourceProperty(m_ServerStatusModel);

  // Initialize the service model
  m_CurrentServiceModel = NewConcreteProperty(-1, CurrentServiceDomain());
  m_CurrentServiceModel->SetIsValid(false);

  // Service description
  m_ServiceDescriptionModel = NewSimpleConcreteProperty(std::string());

  // Tag selection model
  m_TagListModel = NewConcreteProperty(-1, TagDomainType(&m_TagSpecArray));
  m_TagListModel->SetIsValid(false);

  // Model for current tag selection
  m_CurrentTagImageLayerModel = wrapGetterSetterPairAsProperty(
                                  this,
                                  &Self::GetCurrentTagImageLayerValueAndRange,
                                  &Self::SetCurrentTagImageLayerValue);
  m_CurrentTagImageLayerModel->Rebroadcast(m_TagListModel, ValueChangedEvent(), ValueChangedEvent());
  m_CurrentTagImageLayerModel->Rebroadcast(m_TagListModel, ValueChangedEvent(), DomainChangedEvent());

  // Last submitted ticket
  m_SubmittedTicketIdModel = NewSimpleConcreteProperty(-1);
  m_SubmittedTicketIdModel->SetIsValid(false);

  // Changes to the server and token result in a server change event
  this->Rebroadcast(m_ServerURLModel, ValueChangedEvent(), ServerChangeEvent());
  this->Rebroadcast(m_ServerURLModel, DomainChangedEvent(), ServerChangeEvent());
  this->Rebroadcast(m_TokenModel, ValueChangedEvent(), ServerChangeEvent());

  // Changes to the selected service also propagated
  this->Rebroadcast(m_CurrentServiceModel, ValueChangedEvent(), ServiceChangeEvent());
  this->Rebroadcast(m_CurrentServiceModel, DomainChangedEvent(), ServiceChangeEvent());

  // Changes to the tags table require a state update
  this->Rebroadcast(m_CurrentTagImageLayerModel, DomainChangedEvent(), StateMachineChangeEvent());

}


