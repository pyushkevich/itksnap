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
#include <AbstractModel.h>
#include <UIState.h>
#include "Property.h"

class IRISApplication;
class SNAPAppearanceSettings;
class GenericSliceModel;
class OrthogonalSliceCursorNavigationModel;
class PolygonDrawingModel;
class SliceWindowCoordinator;
class GuidedNativeImageIO;
class SystemInterface;
class GlobalState;
class AbstractLoadImageDelegate;
class IRISWarningList;
class IntensityCurveModel;
class LayerSelectionModel;

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



/**


  */
class GlobalUIModel : public AbstractModel
{

public:

  irisITKObjectMacro(GlobalUIModel, AbstractModel)

  // Events fired by this object
  FIRES(CursorUpdateEvent)
  FIRES(LayerChangeEvent)
  FIRES(LinkedZoomUpdateEvent)


  irisGetMacro(Driver, IRISApplication *)

  irisGetMacro(AppearanceSettings, SNAPAppearanceSettings *)

  irisGetMacro(SliceCoordinator, SliceWindowCoordinator *)

  // Convenience access to the SystemInfterface
  SystemInterface *GetSystemInterface() const;

  // I don't know why this is in a separate class
  GlobalState *GetGlobalState() const;

  /** Get the current toolbar mode */
  irisSetPropertyMacro(ToolbarMode,ToolbarModeType)

  /** Get the current toolbar mode */
  irisGetPropertyMacro(ToolbarMode,ToolbarModeType)

  GenericSliceModel *GetSliceModel(unsigned int i) const
    { return m_SliceModel[i]; }

  /** Get the slice navigation model for each slice */
  OrthogonalSliceCursorNavigationModel *
  GetCursorNavigationModel(unsigned int i) const
    { return m_CursorNavigationModel[i]; }

  /** Get the polygon drawing model for each slice */
  PolygonDrawingModel *GetPolygonDrawingModel(unsigned int i) const
  {
    return m_PolygonDrawingModel[i];
  }

  /** Get the model for intensity curve navigation */
  irisGetMacro(IntensityCurveModel, IntensityCurveModel *)

  /** Get the model encapsulating the main images and all overlays */
  irisGetMacro(LoadedLayersSelectionModel, LayerSelectionModel *)

  /** Load the main image */
  void LoadGrayImage(GuidedNativeImageIO *io);

  /** Load an image non-interactively through a delegate */
  void LoadImageNonInteractive(const char *fname,
                               AbstractLoadImageDelegate &delegate,
                               IRISWarningList &wl);

  /**
    Check the state of the system. This class will issue StateChangeEvent()
    when one of the flags has changed. This method can be used together with
    the SNAPUIFlag object to construct listeners to complex state changes.
   */
  bool CheckState(UIState state);

protected:

  GlobalUIModel();
  ~GlobalUIModel();

  SmartPtr<IRISApplication> m_Driver;

  SNAPAppearanceSettings *m_AppearanceSettings;

  // A set of three slice models, representing the UI state of each
  // of the 2D slice panels the user interacts with
  SmartPtr<GenericSliceModel> m_SliceModel[3];

  // A set of models that support cursor navigation
  SmartPtr<OrthogonalSliceCursorNavigationModel> m_CursorNavigationModel[3];

  // Models for polygon drawing
  SmartPtr<PolygonDrawingModel> m_PolygonDrawingModel[3];

  // Window coordinator
  SmartPtr<SliceWindowCoordinator> m_SliceCoordinator;

  // Model for intensity curve manipulation
  SmartPtr<IntensityCurveModel> m_IntensityCurveModel;

  // Layer selection model encapsulating the main image and overlays
  SmartPtr<LayerSelectionModel> m_LoadedLayersSelectionModel;

  // The current 2D toolbar mode
  irisPropertyDeclMacro(ToolbarMode, ToolbarModeType)
};

#endif // GLOBALUIMODEL_H
