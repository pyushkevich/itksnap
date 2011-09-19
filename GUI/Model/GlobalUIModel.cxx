/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Filename.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
  Copyright (c) 2011 Paul A. Yushkevich

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

=========================================================================*/

#include "GlobalUIModel.h"

#include <IRISApplication.h>
#include <SNAPAppearanceSettings.h>
#include <GenericSliceModel.h>
#include <OrthogonalSliceCursorNavigationModel.h>
#include <SliceWindowCoordinator.h>
#include <GenericImageData.h>


SNAPUIFlag::SNAPUIFlag(GlobalUIModel *model, UIState state)
{
  m_Model = model;
  m_State = state;
  AddListener<SNAPUIFlag>(m_Model, StateChangeEvent(),
              this, &SNAPUIFlag::OnStateChange);
}

bool SNAPUIFlag::operator() () const
{
  return m_Model->checkState(m_State);
}



GlobalUIModel::GlobalUIModel()
{
  // Create the appearance settings objects
  m_AppearanceSettings = new SNAPAppearanceSettings();

  // Create the IRIS application login
  m_Driver = new IRISApplication();

  // Create the slice models
  for (unsigned int i = 0; i < 3; i++)
    {
    m_SliceModel[i] = new GenericSliceModel(this, i);
    m_CursorNavigationModel[i] =
        new OrthogonalSliceCursorNavigationModel(m_SliceModel[i]);
    }

  // Connect them together with the coordinator
  m_SliceCoordinator = new SliceWindowCoordinator();
  m_SliceCoordinator->RegisterSliceModels(m_SliceModel);

  // Listen to state changes from the slice coordinator
  AddListener(m_SliceCoordinator,
              SliceWindowCoordinator::LinkedZoomUpdateEvent(),
              this, &GlobalUIModel::OnStateChange);

  // Set the defaults for properties
  m_ToolbarMode = CROSSHAIRS_MODE;
}

void GlobalUIModel::OnStateChange()
{
  InvokeEvent(StateChangeEvent());
}

GlobalUIModel::~GlobalUIModel()
{
  delete m_AppearanceSettings;
  delete m_Driver;
  delete m_SliceCoordinator;
  for (unsigned int i = 0; i < 3; i++)
    {
    delete m_SliceModel[i];
    delete m_CursorNavigationModel[i];
    }
  }

bool GlobalUIModel::checkState(UIState state)
{
  // TODO: implement all the other cases
  switch(state)
    {
    case UIF_RGB_LOADED:
      return m_Driver->GetCurrentImageData()->IsRGBLoaded();
    case UIF_BASEIMG_LOADED:
      return m_Driver->GetCurrentImageData()->IsMainLoaded();
    case UIF_IRIS_ACTIVE:
      return true; // TODO: for now!
    case UIF_MESH_DIRTY:
      return false; // TODO:
    case UIF_MESH_ACTION_PENDING:
      break;
    case UIF_ROI_VALID:
      break;
    case UIF_LINKED_ZOOM:
      return m_SliceCoordinator->GetLinkedZoom();
    case UIF_UNDO_POSSIBLE:
      break;
    case UIF_REDO_POSSIBLE:
      break;
    case UIF_UNSAVED_CHANGES:
      break;
    case UIF_MESH_SAVEABLE:
      break;
    case UIF_GRAY_LOADED:
      return m_Driver->GetCurrentImageData()->IsMainLoaded();
    case UIF_OVERLAY_LOADED:
      return m_Driver->GetCurrentImageData()->IsOverlayLoaded();
    }

  return false;
}
