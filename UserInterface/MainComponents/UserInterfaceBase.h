/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: UserInterfaceBase.h,v $
  Language:  C++
  Date:      $Date: 2011/04/18 15:06:07 $
  Version:   $Revision: 1.45 $
  Copyright (c) 2007 Paul A. Yushkevich
  
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
#ifndef __UserInterfaceBase__h_
#define __UserInterfaceBase__h_

// This should all be gone
#include "SNAPCommonUI.h"
#include "GlobalState.h" 
#include "FL/Fl_Double_Window.H"

// Borland compiler stuff. Note to whoever went through the code and added all 
// these borland things: you just can't add ITK headers to headers like this one
// that get included in lots of files. This makes compilation time insane!!!
#if defined(__BORLANDC__)
#include "SNAPBorlandDummyTypes.h"
#endif

// Forward refences to some classes
class IRISApplication;
class SystemInterface;
class SNAPAppearanceSettings;
class SliceWindowCoordinator;
class Fl_Window;
class Fl_Menu_Bar;
class UserInterfaceLogic;
struct Fl_Menu_Item;

namespace itk {
  class Object;
};

// Layout properties
enum SliceViewConfiguration { FOUR_SLICE=0, AXIAL, SAGITTAL, CORONAL, THREED };

enum WindowSize { FULL_SIZE, HALF_SIZE, CUSTOM_SIZE };

struct DisplayLayout
{
  bool full_screen;
  bool show_main_ui, show_panel_ui;
  SliceViewConfiguration slice_config;
  WindowSize size;
  int fs_restore[4];
};

class SNAPMainWindow : public Fl_Double_Window
{
public:
  SNAPMainWindow(int w, int h, const char *label=0)
    : Fl_Double_Window(w,h,label) { m_ParentUI = NULL; }

  SNAPMainWindow(int x, int y, int w, int h, const char *label=0)
    : Fl_Double_Window(x,y,w,h,label) { m_ParentUI = NULL; }

  virtual ~SNAPMainWindow() {}

  void resize(int x, int y, int w, int h);

  irisSetMacro(ParentUI, UserInterfaceLogic *);
  irisGetMacro(ParentUI, UserInterfaceLogic *);

private:
  UserInterfaceLogic *m_ParentUI;
};

/**
 * \class UserInterfaceBase
 * \brief Base class for the main user interface.
 */
class UserInterfaceBase {
public:
    virtual ~UserInterfaceBase(void) {} /* Needed to avoid compiler warning */

  // Methods for switching between SNAP and IRIS modes
  virtual void ShowIRIS() = 0;
  virtual void ShowSNAP() = 0;
  
  // Method for exiting IRIS
  virtual void OnMainWindowCloseAction() = 0;

  // Methods that set the IRIS Toolbar and 3D Toolbar modes
  virtual void SetToolbarMode(ToolbarModeType mode) = 0;
  virtual void SetToolbarMode3D(ToolbarMode3DType mode) = 0;
  
  // Menu item callbacks
  virtual void OnMenuNewSegmentation() = 0;
  virtual void OnMenuLoadGrey() = 0;
  virtual void OnMenuSaveGrey() = 0;
  virtual void OnMenuSaveGreyROI() = 0;
  virtual void OnMenuLoadRGB() = 0;
  virtual void OnMenuLoadGreyOverlay() = 0;
  virtual void OnMenuLoadRGBOverlay() = 0;
  virtual void OnMenuUnloadOverlayLast() = 0;
  virtual void OnMenuUnloadOverlays() = 0;
  virtual void OnMenuLoadSegmentation() = 0;
  virtual void OnMenuSaveSegmentation() = 0;
  virtual void OnMenuSaveSegmentationMesh() = 0;
  virtual void OnMenuSavePreprocessed() = 0;
  virtual void OnMenuSaveLevelSet() = 0;
  virtual void OnMenuLoadLabels() = 0;
  virtual void OnMenuSaveLabels() = 0;
  virtual void OnMenuSaveScreenshot(unsigned int iSlice) = 0;
  virtual void OnMenuSaveScreenshotSeries(unsigned int iSlice) = 0;
  virtual void OnMenuLoadAdvection() = 0;
  virtual void OnMenuWriteVoxelCounts() = 0;
  virtual void OnLoadRecentAction(unsigned int iRecent) = 0;
  virtual void OnMenuIntensityCurve() = 0;
  virtual void OnMenuColorMap() = 0;
  virtual void OnMenuShowDisplayOptions() = 0;
  virtual void OnMenuShowLayerInspector() = 0;
  virtual void OnMenuExportSlice(unsigned int iSlice) = 0;
  virtual void OnMenuImageInfo() = 0;
  virtual void OnMenuReorientImage() = 0;
  virtual void OnMenuShowVolumes() = 0;
  virtual void OnMenuQuit() = 0;
  virtual void OnMenuCheckForUpdate() = 0;
  virtual void OnMenuResetAll() = 0;
  virtual void OnMenuViewToggleUI() = 0;
  virtual void OnMenuViewToggleFullscreen() = 0;
  virtual void OnMenuViewRestoreDefault() = 0;
  virtual void OnMenuLaunchNewInstance() = 0;

  // IRIS: Slice selection actions
  virtual void OnSliceSliderChange(int id) = 0;
  virtual void UpdatePositionDisplay(int id) = 0;
  virtual void OnSynchronizeCursorAction() = 0;

  // IRIS: Zoom/pan interaction callbacks
  virtual void OnResetView2DAction(unsigned int window) = 0;
  virtual void OnResetAllViews2DAction() = 0;
  virtual void OnLinkedZoomChange() = 0;
  virtual void OnZoomLevelChange() = 0;
  virtual void OnMultisessionZoomChange() = 0;
  virtual void OnMultisessionPanChange() = 0;
  
  // IRIS: Color label selection and editing callbacks
  virtual void OnDrawingLabelUpdate() = 0;
  virtual void OnDrawOverLabelUpdate() = 0;
  virtual void UpdateEditLabelWindow() = 0;
  virtual void OnEditLabelsAction() = 0;
  
  // IRIS: Polygon buttons callbacks
  virtual void OnClosePolygonAction(unsigned int window) = 0;
  virtual void OnUndoPointPolygonAction(unsigned int window) = 0;
  virtual void OnClearPolygonAction(unsigned int window) = 0;
  virtual void OnAcceptPolygonAction(unsigned int window) = 0;
  virtual void OnDeletePolygonSelectedAction(unsigned int window) = 0;
  virtual void OnInsertIntoPolygonSelectedAction(unsigned int window) = 0;
  virtual void OnPastePolygonAction(unsigned int window) = 0;
  virtual void OnIRISFreehandFittingRateUpdate() = 0;

  // IRIS: Paintbrush tools
  virtual void OnPaintbrushAttributesUpdate() = 0;
  virtual void OnPaintbrushPaint() = 0;
  virtual void UpdatePaintbrushAttributes() = 0;

  // IRIS: Annotation mode
  virtual void OnAnnotationAttributesUpdate() = 0;

  // IRIS: 3D Window callbacks
  virtual void OnMeshResetViewAction() = 0;
  virtual void OnIRISMeshUpdateAction() = 0;
  virtual void OnIRISMeshAcceptAction() = 0;
  
  // IRIS: ROI manipulation callbacks
  virtual void OnResetROIAction() = 0;
  virtual void OnSnakeStartAction() = 0;

  // SNAP Preprocessing page actions
  virtual void OnInOutSnakeSelect() = 0;
  virtual void OnEdgeSnakeSelect() = 0;
  virtual void OnPreprocessAction() = 0;
  virtual void OnPreprocessClose() = 0;
  virtual void OnLoadPreprocessedImageAction() = 0;
  virtual void OnAcceptPreprocessingAction() = 0;

  // SNAP Color map operations
  virtual void OnPreprocessedColorMapAction() = 0;
  virtual void OnColorMapCloseAction() = 0;
  virtual void OnColorMapSelectAction() = 0;

  // SNAP Initialization page actions
  virtual void OnAddBubbleAction() = 0;
  virtual void OnRemoveBubbleAction() = 0;
  virtual void OnActiveBubblesChange() = 0;
  virtual void OnBubbleRadiusChange() = 0;
  virtual void OnAcceptInitializationAction() = 0;
  
  // SNAP Segmentation page actions
  virtual void OnRestartInitializationAction() = 0;
  virtual void OnSnakeParametersAction() = 0;
  virtual void OnAcceptSegmentationAction() = 0;
  virtual void OnSnakeRewindAction() = 0;
  virtual void OnSnakeStopAction() = 0;
  virtual void OnSnakePlayAction() = 0;
  virtual void OnSnakeStepAction() = 0;
  virtual void OnSnakeStepSizeChange() = 0;
  virtual void OnRestartPreprocessingAction() = 0;
  virtual void OnCancelSegmentationAction() = 0;
  
  // SNAP: display interaction actions  
  virtual void OnSNAPViewOriginalSelect() = 0;
  virtual void OnViewPreprocessedSelect() = 0;
  
  virtual void UpdateMainLabel() = 0;

  virtual void DeleteColorLabelMenu(Fl_Menu_Item *menu) = 0;
  virtual Fl_Menu_Item *GenerateColorLabelMenu(bool,bool,bool) = 0;

  // Opacity sliders
  virtual void OnIRISLabelOpacityChange() = 0;
  virtual void OnSNAPLabelOpacityChange() = 0;

  // Undo/Redo buttons
  virtual void OnUndoAction() = 0;
  virtual void OnRedoAction() = 0;

  // SNAP: 3D window related callbacks
  virtual void OnSNAPMeshUpdateAction() = 0;
  virtual void OnSNAPMeshContinuousUpdateAction() = 0;

  // Volume and statistics actions
  virtual void OnStatisticsUpdateAction() = 0;
  virtual void OnStatisticsExportAction() = 0;
  virtual void OnStatisticsCopyAction() = 0;

  // virtual void Activate3DAccept(bool on) = 0;

  // Help related callbacks
  virtual void OnLaunchTutorialAction() = 0;
  virtual void ShowHTMLPage(const char *link) = 0;

  // Window size manipulation calls
  virtual void OnWindowFocus(int i) = 0;
  virtual void OnWindowCollapse(int i) = 0;

  // Save as PNG
  virtual void OnActiveWindowSaveSnapshot(unsigned int window) = 0;

  // The following methods are not referenced in the .fl file, but are defined here
  // so that other classes in the project can include UserInterfaceBase.h instead of
  // UserInterfaceLogic.h; this way there is not a huge build every time we change
  // a user interface element
  virtual IRISApplication *GetDriver() const = 0;
  virtual SystemInterface *GetSystemInterface() const = 0;
  virtual SNAPAppearanceSettings *GetAppearanceSettings() const = 0;
  virtual SliceWindowCoordinator *GetSliceCoordinator() const = 0;

  virtual void OnMainImageUpdate() = 0;
  virtual void OnOverlayImageUpdate() = 0;
  virtual void OnImageGeometryUpdate() = 0;
  virtual void RedrawWindows() = 0;
  virtual void OnIRISMeshDisplaySettingsUpdate() = 0;
  virtual void ResetScrollbars() = 0;
  virtual void UpdateImageProbe() = 0;
  virtual void OnLabelListUpdate() = 0;
  virtual void OnSegmentationImageUpdate(bool) = 0;
  virtual void CenterChildWindowInMainWindow(Fl_Window *) = 0;
  virtual void OnPreprocessingPreviewStatusUpdate(bool) = 0;
  virtual void OnSpeedImageUpdate() = 0;
  virtual void OnCrosshairPositionUpdate(bool flagBroadcastUpdate = true) = 0;
  virtual void OnViewPositionsUpdate(bool flagBroadcastUpdate = true) = 0;
  virtual void OnTrackballUpdate(bool flagBroadcastUpdate = true) = 0;
  virtual void OnPolygonStateUpdate(unsigned int) = 0;
  virtual void OnZoomUpdate(bool flagBroadcastUpdate = true) = 0;
  virtual void OnIRISMeshEditingAction() = 0;

  // Progress callbacks
  virtual void OnITKProgressEvent(itk::Object *source, const itk::EventObject &event) = 0;

  // Non-callbacks
  virtual double GetFreehandFittingRate() = 0;

  virtual void StoreUndoPoint(const char *text) = 0;
  virtual void ClearUndoPoints() = 0;
  
  virtual void OnHiddenFeaturesToggleAction() = 0;

  virtual void DisplayTips() = 0;

  virtual void OpenDraggedContent(const char *fn_open, bool interactive) = 0;
  
  virtual void OnOpenDroppedAction(int selection) = 0;

  virtual DisplayLayout GetDisplayLayout() const = 0;
  virtual void SetDisplayLayout(DisplayLayout dlo) = 0;

  virtual Fl_Menu_Bar* GetMainMenuBar() = 0;
  virtual Fl_Window* GetMainWindow() = 0;
  virtual Fl_Window* GetPopupToolbarWindow() = 0;
  
  virtual void OnCollapsedViewPopupMenu() = 0;

protected:
    GlobalState *m_GlobalState;
};

#endif // __UserInterfaceBase__h_
