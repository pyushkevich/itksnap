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
#include <PolygonDrawingModel.h>
#include <SliceWindowCoordinator.h>
#include <GenericImageData.h>
#include <GuidedNativeImageIO.h>
#include <ImageIODelegates.h>
#include <IntensityCurveModel.h>

#include <SNAPUIFlag.h>
#include <SNAPUIFlag.txx>

// Enable this model to be used with the flag engine
template class SNAPUIFlag<GlobalUIModel, UIState>;


GlobalUIModel::GlobalUIModel()
  : AbstractModel()
{
  // Create the appearance settings objects
  m_AppearanceSettings = new SNAPAppearanceSettings();

  // Create the IRIS application login
  m_Driver = IRISApplication::New();

  // Create the slice models
  for (unsigned int i = 0; i < 3; i++)
    {
    m_SliceModel[i] = GenericSliceModel::New();
    m_SliceModel[i]->Initialize(this, i);
    m_CursorNavigationModel[i] =
        OrthogonalSliceCursorNavigationModel::New();
    m_CursorNavigationModel[i]->SetParent(m_SliceModel[i]);

    m_PolygonDrawingModel[i] = PolygonDrawingModel::New();
    m_PolygonDrawingModel[i]->SetParent(m_SliceModel[i]);
    }

  // Connect them together with the coordinator
  m_SliceCoordinator = SliceWindowCoordinator::New();
  GenericSliceModel *ptr[3] =
    { m_SliceModel[0], m_SliceModel[1], m_SliceModel[2] };
  m_SliceCoordinator->RegisterSliceModels(ptr);

  // Intensity curve model
  m_IntensityCurveModel = IntensityCurveModel::New();
  m_IntensityCurveModel->SetParentModel(this);

  // Listen to state changes from the slice coordinator
  Rebroadcast(m_SliceCoordinator, LinkedZoomUpdateEvent(), LinkedZoomUpdateEvent());
  Rebroadcast(m_SliceCoordinator, LinkedZoomUpdateEvent(), StateMachineChangeEvent());

  // Rebroadcast cursor change events
  Rebroadcast(m_Driver, CursorUpdateEvent(), CursorUpdateEvent());

  // Rebroadcast image layer change events
  Rebroadcast(m_Driver, LayerChangeEvent(), LayerChangeEvent());
  Rebroadcast(m_Driver, LayerChangeEvent(), StateMachineChangeEvent());

  Rebroadcast(m_ToolbarMode, ToolbarModeChangeEvent());

  // Set the defaults for properties
  m_ToolbarMode = CROSSHAIRS_MODE;
}

GlobalUIModel::~GlobalUIModel()
{
  delete m_AppearanceSettings;
}

bool GlobalUIModel::CheckState(UIState state)
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

void GlobalUIModel::LoadGrayImage(GuidedNativeImageIO *io)
{
  m_Driver->UnloadOverlays();
  m_Driver->UpdateIRISMainImage(io, IRISApplication::MAIN_SCALAR);
}

void GlobalUIModel
::LoadImageNonInteractive(const char *fname,
                          AbstractLoadImageDelegate &del,
                          IRISWarningList &wl)
{
  // Load the settings associated with this file
  Registry reg;
  m_Driver->GetSystemInterface()->FindRegistryAssociatedWithFile(fname, reg);

  // Get the folder dealing with grey image properties
  Registry &folder = reg.Folder("Files.Grey");

  // Create a native image IO object
  GuidedNativeImageIO io;

  // Load the header of the image
  io.ReadNativeImageHeader(fname, folder);

  // Validate the header
  del.ValidateHeader(&io, wl);

  // Read the image body
  io.ReadNativeImageData();

  // Validate the image data
  del.ValidateImage(&io, wl);

  // Put the image in the right place
  del.UpdateApplicationWithImage(&io);
}


SystemInterface * GlobalUIModel::GetSystemInterface() const
{
  return m_Driver->GetSystemInterface();
}

GlobalState * GlobalUIModel::GetGlobalState() const
{
  return m_Driver->GetGlobalState();
}


