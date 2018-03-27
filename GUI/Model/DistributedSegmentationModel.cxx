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

void DistributedSegmentationModel::SetParentModel(GlobalUIModel *model)
{
  m_Parent = model;
}

std::string DistributedSegmentationModel::GetURL(const std::string &path)
{
  // Get the main part of the URL
  std::string server = m_ServerURLList[m_ServerURLModel->GetValue()];
  return server + "/" + path;
}

DistributedSegmentationModel::DistributedSegmentationModel()
{
  // Build a list of available URLs
  m_ServerURLList.push_back("https://dss.itksnap.org");

  // Create the server model that references the URL list
  m_ServerURLModel = NewConcreteProperty(0, ServerURLDomain(&m_ServerURLList));

  // Create the token model
  m_TokenModel = NewSimpleConcreteProperty(std::string(""));

  // Changes to the server and token result in a server change event
  this->Rebroadcast(m_ServerURLModel, ValueChangedEvent(), ServerChangeEvent());
  this->Rebroadcast(m_ServerURLModel, DomainChangedEvent(), ServerChangeEvent());
  this->Rebroadcast(m_TokenModel, ValueChangedEvent(), ServerChangeEvent());
}
