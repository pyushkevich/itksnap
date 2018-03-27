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

class GlobalUIModel;
class IRISApplication;

/**
 * @brief Model behind the distributed segmentation GUI
 */
class DistributedSegmentationModel : public AbstractModel
{
public:
  irisITKObjectMacro(DistributedSegmentationModel, AbstractModel)

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

  /** Get the full URL */
  std::string GetURL(const std::string &path);

protected:

  // List of known server URLs
  std::vector<std::string> m_ServerURLList;

  // Property model for server selection
  typedef ConcretePropertyModel<int, ServerURLDomain> ServerURLModelType;
  SmartPtr<ServerURLModelType> m_ServerURLModel;

  // Property model for token
  SmartPtr<ConcreteSimpleStringProperty> m_TokenModel;



  DistributedSegmentationModel();
  ~DistributedSegmentationModel() {}

  // Parent model
  GlobalUIModel *m_Parent;
};

#endif // DISTRIBUTEDSEGMENTATIONMODEL_H
