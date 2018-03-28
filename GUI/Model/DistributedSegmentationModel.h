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

namespace dss_model {

/** Structure describing the authentication status */
struct AuthResponse
{
  bool connected, authenticated;
  std::string user_id;
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

  // A custom event fired when the server configuration changes
  itkEventMacro(ServerChangeEvent, IRISEvent)
  FIRES(ServerChangeEvent)

  void SetParentModel(GlobalUIModel *model);
  irisGetMacro(Parent, GlobalUIModel *)

  /** Server URL property model */
  typedef STLVectorWrapperItemSetDomain<int, std::string> ServerURLDomain;
  irisGenericPropertyAccessMacro(ServerURL, int, ServerURLDomain)

  /** Token model */
  irisSimplePropertyAccessMacro(Token, std::string)

  /** Server status model */
  typedef SimpleItemSetDomain<ServerStatus, std::string> ServerStatusDomain;
  irisGenericPropertyAccessMacro(ServerStatus, ServerStatus, ServerStatusDomain)

  irisSimplePropertyAccessMacro(ServerStatusString, std::string)

  /** Get the full URL */
  std::string GetURL(const std::string &path);

  /** Registry describing the service listing */
  irisGetMacro(ServiceListing, const dss_model::ServiceListing &)
  void SetServiceListing(const dss_model::ServiceListing &listing);

  /** Selected service model (indexed by git hash) */
  typedef SimpleItemSetDomain<int, std::string> CurrentServiceDomain;
  irisGenericPropertyAccessMacro(CurrentService, int, CurrentServiceDomain)



protected:

  // List of known server URLs
  std::vector<std::string> m_ServerURLList;

  // Property model for server selection
  typedef ConcretePropertyModel<int, ServerURLDomain> ServerURLModelType;
  SmartPtr<ServerURLModelType> m_ServerURLModel;

  // Property model for token
  SmartPtr<ConcreteSimpleStringProperty> m_TokenModel;

  // Property model for server status
  typedef ConcretePropertyModel<ServerStatus, ServerStatusDomain> ServerStatusModelType;
  SmartPtr<ServerStatusModelType> m_ServerStatusModel;

  // Property model for server status string
  SmartPtr<AbstractSimpleStringProperty> m_ServerStatusStringModel;
  bool GetServerStatusStringValue(std::string &value);

  // Property model for current service
  typedef ConcretePropertyModel<int, CurrentServiceDomain> CurrentServiceModel;
  SmartPtr<CurrentServiceModel> m_CurrentServiceModel;

  // Registry holding the service listing
  dss_model::ServiceListing m_ServiceListing;

  DistributedSegmentationModel();
  ~DistributedSegmentationModel() {}

  // Parent model
  GlobalUIModel *m_Parent;
};

#endif // DISTRIBUTEDSEGMENTATIONMODEL_H
