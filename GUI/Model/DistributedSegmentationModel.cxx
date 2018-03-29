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

} // namespace

using namespace dss_model;

void DistributedSegmentationModel::SetParentModel(GlobalUIModel *model)
{
  m_Parent = model;
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

StatusCheckResponse
DistributedSegmentationModel::AsyncCheckStatus(std::string url, std::string token)
{
  dss_model::StatusCheckResponse response;
  response.auth_response.connected = false;
  response.auth_response.authenticated = false;

  // First try to authenticate
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
      response.service_listing.push_back(service);
      }
    }
  }
  catch(...)
  {
  }

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
        }
      }
    }
  catch (...) {

    }

  return result;
}

void DistributedSegmentationModel::ApplyServiceDetailResponse(const ServiceDetailResponse &resp)
{
  this->SetServiceDescription(resp.longdesc);
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

  // Changes to the server and token result in a server change event
  this->Rebroadcast(m_ServerURLModel, ValueChangedEvent(), ServerChangeEvent());
  this->Rebroadcast(m_ServerURLModel, DomainChangedEvent(), ServerChangeEvent());
  this->Rebroadcast(m_TokenModel, ValueChangedEvent(), ServerChangeEvent());

  // Changes to the selected service also propagated
  this->Rebroadcast(m_CurrentServiceModel, ValueChangedEvent(), ServiceChangeEvent());
  this->Rebroadcast(m_CurrentServiceModel, DomainChangedEvent(), ServiceChangeEvent());

}


