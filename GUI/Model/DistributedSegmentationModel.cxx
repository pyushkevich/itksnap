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

  // Changes to the server and token result in a server change event
  this->Rebroadcast(m_ServerURLModel, ValueChangedEvent(), ServerChangeEvent());
  this->Rebroadcast(m_ServerURLModel, DomainChangedEvent(), ServerChangeEvent());
  this->Rebroadcast(m_TokenModel, ValueChangedEvent(), ServerChangeEvent());

  // Initialize the service model
  m_CurrentServiceModel = NewConcreteProperty(-1, CurrentServiceDomain());
  m_CurrentServiceModel->SetIsValid(false);
}


