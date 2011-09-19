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

#ifndef GLOBALUIMODEL_H
#define GLOBALUIMODEL_H

#include <SNAPCommon.h>
#include <SNAPEvents.h>
#include <StateManagement.h>

class IRISApplication;
class SNAPAppearanceSettings;
class GenericSliceModel;
class OrthogonalSliceCursorNavigationModel;
class SliceWindowCoordinator;
class GlobalUIModel;

// Events fired by this object
itkEventMacro(ToolbarModeChangeEvent, IRISEvent)

enum ToolbarModeType
{
  POLYGON_DRAWING_MODE,
  NAVIGATION_MODE,
  CROSSHAIRS_MODE,
  PAINTBRUSH_MODE,
  ANNOTATION_MODE,
  ROI_MODE
};

/** A list of states that the Global UI may be in. Whenever any of these
  states changes, the UI issues a StateChangedEvent */
enum UIState {
  UIF_GRAY_LOADED,
  UIF_RGB_LOADED,
  UIF_BASEIMG_LOADED,  // i.e., Gray or RGB loaded
  UIF_OVERLAY_LOADED,  // i.e., Baseimg loaded and at least one overlay
  UIF_IRIS_ACTIVE,     // i.e., system in main interaction mode
  UIF_MESH_DIRTY,
  UIF_MESH_ACTION_PENDING,
  UIF_ROI_VALID,
  UIF_LINKED_ZOOM,
  UIF_UNDO_POSSIBLE,
  UIF_REDO_POSSIBLE,
  UIF_UNSAVED_CHANGES,
  UIF_MESH_SAVEABLE
};

/**
  A BooleanCondition implementation that queries the GlobalUIModel
  about different UI states.
  */
class SNAPUIFlag : public BooleanCondition
{
public:

  SNAPUIFlag(GlobalUIModel *model, UIState state);
  bool operator() () const;

private:
  GlobalUIModel *m_Model;
  UIState m_State;
};


class GlobalUIModel : public itk::Object
{

public:

  GlobalUIModel();
  ~GlobalUIModel();

  irisGetMacro(Driver, IRISApplication *)

  irisGetMacro(AppearanceSettings, SNAPAppearanceSettings *)

  irisGetMacro(SliceCoordinator, SliceWindowCoordinator *)

  /** Get the current toolbar mode */
  irisSetWithEventMacro(ToolbarMode, ToolbarModeType, ToolbarModeChangeEvent)

  /** Get the current toolbar mode */
  irisGetMacro(ToolbarMode,ToolbarModeType)

  GenericSliceModel *GetSliceModel(unsigned int i) const
    { return m_SliceModel[i]; }

  OrthogonalSliceCursorNavigationModel *
  GetCursorNavigationModel(unsigned int i) const
    { return m_CursorNavigationModel[i]; }

  /**
    Check the state of the system. This class will issue StateChangeEvent()
    when one of the flags has changed. This method can be used together with
    the SNAPUIFlag object to construct listeners to complex state changes.
   */
  bool checkState(UIState state);

  // Callback used to map various events into the state change event
  void OnStateChange();

protected:

  IRISApplication *m_Driver;

  SNAPAppearanceSettings *m_AppearanceSettings;

  // A set of three slice models, representing the UI state of each
  // of the 2D slice panels the user interacts with
  GenericSliceModel *m_SliceModel[3];

  // A set of models that support cursor navigation
  OrthogonalSliceCursorNavigationModel *m_CursorNavigationModel[3];

  // Window coordinator
  SliceWindowCoordinator *m_SliceCoordinator;

  // The current 2D toolbar mode
  ToolbarModeType m_ToolbarMode;

};

#endif // GLOBALUIMODEL_H
