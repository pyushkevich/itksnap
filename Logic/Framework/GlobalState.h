/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GlobalState.h,v $
  Language:  C++
  Date:      $Date: 2009/08/29 23:18:42 $
  Version:   $Revision: 1.19 $
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

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#ifndef __GlobalState_h_
#define __GlobalState_h_

class IRISApplication;
class MeshOptions;
class DefaultBehaviorSettings;

#include <vector>
#include "SNAPCommon.h"
#include "SnakeParameters.h"
#include "ThresholdSettings.h"
#include "SNAPSegmentationROISettings.h"
#include "itkImageRegion.h"
#include "PropertyModel.h"
#include "ColorLabelPropertyModel.h"

enum MeshFilterType
{
  SELECT_MESH_SINGLE,
  SELECT_MESH_VISIBLE,
  SELECT_MESH_ALL
};

enum SnakeType 
{
  IN_OUT_SNAKE,
  EDGE_SNAKE
};

/**
  Available modes for speed image generation
  */
enum PreprocessingMode
{
  PREPROCESS_NONE = 0,
  PREPROCESS_THRESHOLD,
  PREPROCESS_EDGE,
  PREPROCESS_GMM,
  PREPROCESS_RF
};

enum ConstraintsType 
{
  SAPIRO,
  SCHLEGEL,
  TURELLO,
  USER
};
  
enum PaintbrushMode
{
  PAINTBRUSH_RECTANGULAR = 0,
  PAINTBRUSH_ROUND = 1,
  PAINTBRUSH_WATERSHED = 2
};
  
enum DisplayPanel
{
  PANEL_AXIAL = 0,
  PANEL_SAGITTAL,
  PANEL_CORONAL,
  PANEL_3D
};

enum ToolbarModeType
{
  CROSSHAIRS_MODE = 0,
  NAVIGATION_MODE,
  POLYGON_DRAWING_MODE,
  PAINTBRUSH_MODE,
  ROI_MODE,
  ANNOTATION_MODE
};

enum ToolbarMode3DType
{
  TRACKBALL_MODE = 0,
  CROSSHAIRS_3D_MODE,
  SPRAYPAINT_MODE,
  SCALPEL_MODE
};

enum AnnotationMode
{
  ANNOTATION_RULER = 0,
  ANNOTATION_SELECT,
  ANNOTATION_LANDMARK
};

/** Layout of overlays in a slice view */
enum LayerLayout {
  LAYOUT_STACKED = 0, LAYOUT_TILED
};

/** Watershed settings for paintbrush */
struct PaintbrushWatershedSettings
{
  // Level of thresholding
  double level;

  // Amount of smoothing
  unsigned int smooth_iterations;

};

/** Paintbrush settings */
struct PaintbrushSettings
{
  double radius;
  PaintbrushMode mode;
  bool volumetric;
  bool isotropic;
  bool chase;

  PaintbrushWatershedSettings watershed;
};

/** Annotation settings */
struct AnnotationSettings
{
  bool shownOnAllSlices;
};

/**
 * Bubble structure: an object of this class stores information about an 
 * individual bubble initialized in the Snake window
 */
struct Bubble {
  // center of the bubble (voxel index)
  Vector3i center;
  
  // radius of the bubble (in mm.)
  double radius;     
};
                     
/**
 * \class GlobalState
 * \brief Contains global variables describing the state of the application.
 *
 * TODO: this class should be refactored to use PropertyModel objects for all
 * the various settings, rather than mixing PropertyModels with simple attributes.
 *
 */
class GlobalState : public AbstractModel
{
public:
  irisITKObjectMacro(GlobalState, AbstractModel)

  // Region of interest definition
  typedef itk::ImageRegion<3> RegionType;

  // Define the bubble array
  typedef std::vector<Bubble> BubbleArray;

  void SetDriver(IRISApplication *app);

  /** Get whether the grey image display uses linear interpolation */
  irisSetMacro(InterpolateGrey,bool );

  /** Set whether the grey image display uses linear interpolation */
  irisGetMacro(InterpolateGrey,bool );

  /** Get whether the segmentation image uses linear interpolation */
  irisSetMacro(InterpolateSegmentation,bool );

  /** Set whether the segmentation image uses linear interpolation */
  irisGetMacro(InterpolateSegmentation,bool );

  /** Get/Set the on/off state of the segmentation overlay */
  irisRangedPropertyAccessMacro(SegmentationAlpha, double)

  /** Get whether the slice requires an update or not (TODO: obsolete?) */
  irisSetMacro(UpdateSliceFlag,int );

  /** Set whether the slice requires an update or not (TODO: obsolete?) */
  irisGetMacro(UpdateSliceFlag,int );

  /** Get current mode of polygon/snake painting (over all, over label) */
  void SetCoverageMode(CoverageModeType coverage);

  /** Set current mode of polygon/snake painting (over all, over label) */
  CoverageModeType GetCoverageMode() const;

  /** Set whether the region of interest is valid */
  bool isSegmentationROIValid();

  /** Get whether the region of interest is visible */
  irisSetMacro(ShowROI,bool );

  /** Set whether the region of interest is visible */
  irisGetMacro(ShowROI,bool );

  /** Get whether the region of interest is being dragged */
  irisSetMacro(DraggingROI,bool );

  /** Set whether the region of interest is being dragged */
  irisGetMacro(DraggingROI,bool );

  /** Get whether SNAP is currently active */
  irisSetMacro(SNAPActive,bool );

  /** Set whether SNAP is currently active */
  irisGetMacro(SNAPActive,bool );

  /** Get whether the speed (preprocessing) image is valid */
  irisSetMacro(SpeedValid,bool );

  /** Set whether the speed (preprocessing) image is valid */
  irisGetMacro(SpeedValid,bool );

  /** Get whether the zero level of the speed image is being displayed */
  irisSetMacro(SpeedViewZero,bool );

  /** Set whether the zero level of the speed image is being displayed */
  irisGetMacro(SpeedViewZero,bool );

  /**
   * The last 'real' preprocessing mode used in the application. This
   * is stored so that the preprocessing mode can be remembered next
   * time that we enter active contour mode.
   */
  irisSimplePropertyAccessMacro(LastUsedPreprocessingMode, PreprocessingMode)

  /** The domain of values for snake type (edge/region) */
  typedef SimpleItemSetDomain<SnakeType, std::string> SnakeTypeDomain;

  /** Get the type of the snake being used */
  irisGenericPropertyAccessMacro(SnakeType, SnakeType, SnakeTypeDomain)

  /** Get the current parameters of the snake algorithm */
  irisSimplePropertyAccessMacro(SnakeParameters, SnakeParameters)

  /** Get the current mesh rendering options */
  irisGetMacro(MeshOptions, MeshOptions *)

  /** Get the default behavior settings */
  irisGetMacro(DefaultBehaviorSettings, DefaultBehaviorSettings *)

  /** Settings associated with the segmentation ROI */
  irisSimplePropertyAccessMacro(SegmentationROISettings, SNAPSegmentationROISettings)

  /** Shortcut ot set the actual bounding box in the ROI from the settings */
  void SetSegmentationROI(const RegionType &roi);

  /** Shortcut ot get the actual bounding box in the ROI from the settings */
  RegionType GetSegmentationROI();

  /** Get the current paintbrush settings */
  irisGetMacro(PaintbrushSettings, const PaintbrushSettings &);

  /** Set the current paintbrush settings */
  irisSetMacro(PaintbrushSettings, const PaintbrushSettings &);

  /** Get the current annotation settings */
  irisGetMacro(AnnotationSettings, const AnnotationSettings &);

  /** Set the current annotation settings */
  irisSetMacro(AnnotationSettings, const AnnotationSettings &);

#ifdef DRAWING_LOCK
  int GetDrawingLock( short );
  int ReleaseDrawingLock( short );
#endif /* DRAWING_LOCK */

  /** Set the segmentation image file name */
  irisSetStringMacro(LastAssociatedSegmentationFileName);

  /** Get the segmentation image file name */
  irisGetStringMacro(LastAssociatedSegmentationFileName);

  /** Set the preprocessing image file name */
  irisSetStringMacro(LastAssociatedPreprocessingFileName);

  /** Get the preprocessing image file name */
  irisGetStringMacro(LastAssociatedPreprocessingFileName);

  /** Get the array of bubbles */
  irisGetMacro(BubbleArray, BubbleArray);

  /** Set the array of bubbles */
  irisSetMacro(BubbleArray, BubbleArray);

  /** 
   * Get the active bubble number. This can be -1 indicating that there is no
   * active bubble or 0 .. N-1, where N is the size of the bubble array. The
   * active bubble is really a GUI concept for the time being. In general, the
   * bubbles will be eventually replaced by more advanced class hierarchy of
   * seeds, sensors, attractors, and detractors.
   */
  irisGetMacro(ActiveBubble, int);

  /** Set the active bubble */
  irisSetMacro(ActiveBubble, int);

  /** Check if there is an active bubble (i.e., ActiveBubble == -1) */
  bool IsActiveBubbleOn() const
    { return (m_ActiveBubble >= 0); }

  /** Disable active bubble (i.e., set ActiveBubble to -1) */
  void UnsetActiveBubble()
    { m_ActiveBubble = -1; }

  /** Get the polygon access menu flag */
  irisSimplePropertyAccessMacro(PolygonDrawingContextMenu, bool)

  /** Get the drawing label */
  irisGenericPropertyAccessMacro(DrawingColorLabel, LabelType,
                                 ColorLabelItemSetDomain)

  /** Get the draw over label */
  irisGenericPropertyAccessMacro(DrawOverFilter, DrawOverFilter,
                                 DrawOverLabelItemSetDomain)

  /** Whether drawing operations are inverted */
  irisSimplePropertyAccessMacro(PolygonInvert, bool)

  /** Whether the snake has been initialized with a manual segmentation, i.e.
    before running segmentation with label X, the user has labeled some voxels
    with label X. Only applicable in snake mode. */
  irisSimplePropertyAccessMacro(SnakeInitializedWithManualSegmentation, bool)

  /** Get/Set the current toolbar mode */
  irisSimplePropertyAccessMacro(ToolbarMode,ToolbarModeType)

  /** Set/Get the current 3D toolbar mode */
  irisSimplePropertyAccessMacro(ToolbarMode3D,ToolbarMode3DType)

  /** Set/Get the layout of multiple layers in slice views */
  irisSimplePropertyAccessMacro(SliceViewLayerLayout, LayerLayout)

  /** Set/Get selected layer id */
  irisSimplePropertyAccessMacro(SelectedLayerId, unsigned long)

  // ----------------------- Project support ------------------------------

  /**
   * Model for the filename of the current project. This is an empty string if
   * there is no project.
   */
  irisSimplePropertyAccessMacro(ProjectFilename, std::string)

  // --------------------- End Project support ----------------------------

  /**
   * Annotation mode
   */
  irisSimplePropertyAccessMacro(AnnotationMode, AnnotationMode)

  /**
   * Color for Annotations
   */
  irisSimplePropertyAccessMacro(AnnotationColor, Vector3d)

protected:

  GlobalState();
  virtual ~GlobalState();

private:

  /** Get the current crosshairs position */
  irisSetMacro(CrosshairsPosition,Vector3ui );

  /** Set the current crosshairs position */
  irisGetMacro(CrosshairsPosition,Vector3ui );

  friend class IRISApplication;

  /** Color label used to draw polygons */
  SmartPtr<ConcreteColorLabelPropertyModel> m_DrawingColorLabelModel;

  /** Color label over which we can draw */
  SmartPtr<ConcreteDrawOverFilterPropertyModel> m_DrawOverFilterModel;

  /** Whether the grey image display uses linear interpolation */
  bool m_InterpolateGrey;

  /** Whether the segmentation image uses linear interpolation */
  bool m_InterpolateSegmentation;

  /** The transparency of the segmentation overlay */
  SmartPtr<ConcreteRangedDoubleProperty> m_SegmentationAlphaModel;

  /** The current crosshairs position */
  Vector3ui m_CrosshairsPosition;

  /** Whether the slice requires an update or not (TODO: obsolete?) */
  int m_UpdateSliceFlag;

  /** Whether the region of interest is visible */
  bool m_ShowROI;

  /** Whether the region of interest is being dragged */
  bool m_DraggingROI;

  /** Whether SNAP is currently active */
  bool m_SNAPActive;

  /** Whether the speed (preprocessing) image is valid */
  bool m_SpeedValid;

  /** Whether the zero level of the speed image is being displayed */
  bool m_SpeedViewZero;

  /** The type of the snake being used */
  typedef ConcretePropertyModel<SnakeType, SnakeTypeDomain> SnakeTypeModel;
  SmartPtr<SnakeTypeModel> m_SnakeTypeModel;

  /** The last real preprocessing mode used in the program */
  SmartPtr<ConcretePropertyModel<PreprocessingMode> > m_LastUsedPreprocessingModeModel;

  /** Grey image file extension */
  char * m_GreyFileExtension; 

  /** The region of interest for the segmentation (drawn by the user) */
  SmartPtr<ConcretePropertyModel<SNAPSegmentationROISettings> > m_SegmentationROISettingsModel;

  /** Whether the context menu is enabled for polygon drawing. I don't think
    this is the right place to put this, because it's quite GUI specific, but
    for the time being, I stick it here */
  SmartPtr<ConcreteSimpleBooleanProperty> m_PolygonDrawingContextMenuModel;

  /** Whether drawing operations are inverted */
  SmartPtr<ConcreteSimpleBooleanProperty> m_PolygonInvertModel;

  /** Whether snake has been initialized with manual seg voxels */
  SmartPtr<ConcreteSimpleBooleanProperty> m_SnakeInitializedWithManualSegmentationModel;

  // The current 2D toolbar mode
  SmartPtr<ConcretePropertyModel<ToolbarModeType> > m_ToolbarModeModel;

  // The current 3D toolbar mode
  SmartPtr<ConcretePropertyModel<ToolbarMode3DType> > m_ToolbarMode3DModel;

  // Slice view layout model
  SmartPtr<ConcretePropertyModel<LayerLayout> > m_SliceViewLayerLayoutModel;
  
  int m_LockHeld; 
  int m_LockOwner;

  // Current mesh options
  SmartPtr<MeshOptions> m_MeshOptions;

  // Default behavior settings
  SmartPtr<DefaultBehaviorSettings> m_DefaultBehaviorSettings;

  // Current settings for the snake algorithm
  typedef ConcretePropertyModel<SnakeParameters, TrivialDomain> ConcreteSnakeParametersModel;
  SmartPtr<ConcreteSnakeParametersModel> m_SnakeParametersModel;

  // File name of the current grey file
  std::string m_LastAssociatedSegmentationFileName;

  // File name of the current preprocessing file
  std::string m_LastAssociatedPreprocessingFileName;

  // Array of bubbles
  BubbleArray m_BubbleArray;

  // Current bubble
  int m_ActiveBubble;

  // Paintbrush settings
  PaintbrushSettings m_PaintbrushSettings;

  IRISApplication *m_Driver;

  // ------------------- Selected Image ID ---------------------------------
  SmartPtr<ConcreteSimpleULongProperty> m_SelectedLayerIdModel;

  // ------------------- Project Related -----------------------------------
  SmartPtr<ConcreteSimpleStringProperty> m_ProjectFilenameModel;

  // ------------------- Annotation Mode -----------------------------------
  AnnotationSettings m_AnnotationSettings;

  typedef ConcretePropertyModel<AnnotationMode> ConcreteAnnotationModeModel;
  SmartPtr<ConcreteAnnotationModeModel> m_AnnotationModeModel;

  SmartPtr<ConcreteSimpleDoubleVec3Property> m_AnnotationColorModel;
};

#endif // __GlobalState_h_

/*
 *$Log: GlobalState.h,v $
 *Revision 1.19  2009/08/29 23:18:42  garyhuizhang
 *ENH: GreyImageWrapper uses the new ColorMap class
 *
 *Revision 1.18  2009/06/16 06:47:52  garyhuizhang
 *ENH: color map supports for grey overlay
 *
 *Revision 1.17  2009/06/16 04:55:45  garyhuizhang
 *ENH: per overlay opacity adjustment
 *
 *Revision 1.16  2009/06/09 04:34:00  garyhuizhang
 *ENH: color map for grey to RGB mapping
 *
 *Revision 1.15  2009/02/18 00:22:44  garyhuizhang
 *FIX: minor clean up
 *
 *Revision 1.14  2009/02/10 00:10:12  garyhuizhang
 *ENH: Support two drawing options in the Annotation mode: 1) each line shown only on the slice level it is drawn; 2) each line is shown on all slice levels
 *
 *Revision 1.13  2009/01/23 21:48:59  pyushkevich
 *ENH: Added hidden annotation mode (very bad code)
 *
 *Revision 1.12  2009/01/17 10:40:28  pyushkevich
 *Added synchronization to 3D window viewpoint
 *
 *Revision 1.11  2008/12/02 05:14:19  pyushkevich
 *New feature: watershed-based adaptive paint brush. Based on the similar tool in ITK-Grey (which was derived from ITK-SNAP).
 *
 *Revision 1.10  2008/11/17 19:38:23  pyushkevich
 *Added tools dialog to label editor window
 *
 *Revision 1.9  2008/11/15 12:20:38  pyushkevich
 *Several new features added for release 1.8, including (1) support for reading floating point and mapping to short range; (2) use of image direction cosines to determine image orientation; (3) new reorient image dialog and changes to the IO wizard; (4) display of NIFTI world coordinates and yoking based on them; (5) multi-session zoom; (6) fixes to the way we keep track of unsaved changes to segmentation, including a new discard dialog; (7) more streamlined code for offline loading; (8) new command-line options, allowing RGB files to be read and opening SNAP by doubleclicking images; (9) versioning for IPC communications; (10) ruler for display windows; (11) bug fixes and other stuff I can't remember
 *
 *Revision 1.8  2008/03/25 19:31:31  pyushkevich
 *Bug fixes for release 1.6.0
 *
 *Revision 1.7  2008/02/10 23:55:22  pyushkevich
 *Added "Auto" button to the intensity curve window; Added prompt before quitting on unsaved data; Fixed issues with undo on segmentation image load; Added synchronization between SNAP sessions.
 *
 *Revision 1.6  2007/12/30 04:05:13  pyushkevich
 *GPL License
 *
 *Revision 1.5  2007/09/15 15:59:20  pyushkevich
 *Improved the paintbrush mode, allowed more variety of brush sizes
 *
 *Revision 1.4  2007/06/06 22:27:20  garyhuizhang
 *Added support for RGB images in SNAP
 *
 *Revision 1.3  2007/05/10 20:19:50  pyushkevich
 *Added VTK mesh export code and GUI
 *
 *Revision 1.2  2006/12/06 01:26:06  pyushkevich
 *Preparing for 1.4.1. Seems to be stable in Windows but some bugs might be still there
 *
 *Revision 1.1  2006/12/02 04:22:11  pyushkevich
 *Initial sf checkin
 *
 *Revision 1.1.1.1  2006/09/26 23:56:18  pauly2
 *Import
 *
 *Revision 1.11  2006/02/01 20:21:23  pauly
 *ENH: An improvement to the main SNAP UI structure: one set of GL windows is used to support SNAP and IRIS modes
 *
 *Revision 1.10  2005/12/19 03:43:11  pauly
 *ENH: SNAP enhancements and bug fixes for 1.4 release
 *
 *Revision 1.9  2005/10/29 14:00:13  pauly
 *ENH: SNAP enhacements like color maps and progress bar for 3D rendering
 *
 *Revision 1.8  2004/07/24 19:00:03  pauly
 *ENH: Thumbnail UI for slice zooming
 *
 *Revision 1.7  2004/03/19 00:54:47  pauly
 *ENH: Added the ability to externally load the advection image
 *
 *Revision 1.6  2003/12/07 19:48:41  pauly
 *ENH: Resampling, multiresolution
 *
 *Revision 1.5  2003/10/09 22:45:12  pauly
 *EMH: Improvements in 3D functionality and snake parameter preview
 *
 *Revision 1.4  2003/10/02 14:54:52  pauly
 *ENH: Development during the September code freeze
 *
 *Revision 1.1  2003/09/11 13:50:29  pauly
 *FIX: Enabled loading of images with different orientations
 *ENH: Implemented image save and load operations
 *
 *Revision 1.3  2003/08/27 14:03:20  pauly
 *FIX: Made sure that -Wall option in gcc generates 0 warnings.
 *FIX: Removed 'comment within comment' problem in the cvs log.
 *
 *Revision 1.2  2003/08/27 04:57:45  pauly
 *FIX: A large number of bugs has been fixed for 1.4 release
 *
 *Revision 1.1  2003/07/12 04:52:23  pauly
 *Initial checkin of SNAP application  to the InsightApplications tree
 *
 *Revision 1.10  2003/07/12 01:34:18  pauly
 *More final changes before ITK checkin
 *
 *Revision 1.9  2003/07/11 23:28:52  pauly
 **** empty log message ***
 *
 *Revision 1.8  2003/06/08 16:11:42  pauly
 *User interface changes
 *Automatic mesh updating in SNAP mode
 *
 *Revision 1.7  2003/06/03 00:06:46  pauly
 *Almost ready for Pittsburgh demo
 *
 *Revision 1.6  2003/05/22 17:36:19  pauly
 *Edge preprocessing settings
 *
 *Revision 1.5  2003/05/14 18:33:58  pauly
 *SNAP Component is working. Double thresholds have been enabled.  Many other changes.
 *
 *Revision 1.4  2003/05/07 19:14:46  pauly
 *More progress on getting old segmentation working in the new SNAP.  Almost there, region of interest and bubbles are working.
 *
 *Revision 1.3  2003/04/23 20:36:23  pauly
 **** empty log message ***
 *
 *Revision 1.2  2003/04/23 06:05:18  pauly
 **** empty log message ***
 *
 *Revision 1.1  2003/03/07 19:29:47  pauly
 *Initial checkin
 *
 *Revision 1.1.1.1  2002/12/10 01:35:36  pauly
 *Started the project repository
 *
 *
 *Revision 1.16  2002/05/08 17:33:34  moon
 *I made some changes Guido wanted in the GUI, including removing
 *Turello/Sapiro/Schlegel options (I only hid them, the code is all
 *still there), changing a bunch of the ranges, defaults, etc. of the
 *snake parameters.
 *
 *Revision 1.15  2002/04/27 18:30:14  moon
 *Finished commenting
 *
 *Revision 1.14  2002/04/27 00:08:43  talbert
 *Final commenting run through . . . no functional changes.
 *
 *Revision 1.13  2002/04/26 17:38:05  moon
 *Added global variable used by the Apply+ button in the in/out dialog so that
 *2D windows know to show zero level visualization rather than seg data.
 *
 *Revision 1.12  2002/04/24 19:51:34  moon
 *Added a flag for when the ROI was being dragged.  The roi dragging is now better in
 *some ways, although it is not completely perfect.
 *
 *Revision 1.11  2002/04/24 14:14:01  moon
 *Implemented separate brightness/contrast settings for grey/preproc data
 *
 *Revision 1.10  2002/04/20 21:57:20  talbert
 *Added some code to access the extension of the grey file.
 *
 *Revision 1.9  2002/04/20 18:20:40  talbert
 *Added functions to the global state which allowed access to the new
 *data member m_GreyFileExtension.
 *
 *Revision 1.8  2002/04/19 23:04:12  moon
 *Changed more stuff to get the snake params state synched with the global state.
 *
 *Revision 1.7  2002/04/19 20:35:43  moon
 *Made preproc dialogs check global state and only preproc if parameters have changed.
 *So no if you hit apply, then ok, it doesn't re process on the ok.
 *Added global state for preproc params and snake params.  Still need to get snake
 *params synched.
 *
 *Revision 1.6  2002/04/18 21:05:20  moon
 *Changed the IRIS window ROI stuff.  Now the ROI is always valid if an image is
 *loaded, but there is a toggle to show it or not.  This will work better with
 *Konstantin's addition of being able to drag the roi box.  Added global state
 *as appropIriate.
 *
 *Revision 1.5  2002/04/01 22:31:30  moon
 *Added snakeMode and snakeActive to global state
 *snakeMode is in/out or edge, snakeActive is whether the snake
 *has been initialized, meaning it should be drawn in the windows
 *
 *Revision 1.4  2002/03/26 18:16:40  scheuerm
 *Added loading and display of preprocessed data:
 *- added vtkImageDeepCopy function
 *- added flags indicating which dataset to display in GlobalState
 *- added flag indicating whether to load gray or preprocessed data
 *  in the GUI class
 *
 *Revision 1.3  2002/03/08 14:06:29  moon
 *Added Header and Log tags to all files
 **/
