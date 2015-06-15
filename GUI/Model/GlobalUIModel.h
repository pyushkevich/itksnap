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
#include <PropertyModel.h>
#include <ColorLabelPropertyModel.h>

class IRISApplication;
class SNAPAppearanceSettings;
class GenericSliceModel;
class OrthogonalSliceCursorNavigationModel;
class PolygonDrawingModel;
class AnnotationModel;
class SliceWindowCoordinator;
class GuidedNativeImageIO;
class SystemInterface;
class GlobalState;
class AbstractLoadImageDelegate;
class IRISWarningList;
class IntensityCurveModel;
class LayerSelectionModel;
class ColorMapModel;
class ImageInfoModel;
class Generic3DModel;
class LabelEditorModel;
class ConcreteColorLabelPropertyModel;
class CursorInspectionModel;
class SnakeROIModel;
class SnakeWizardModel;
class SnakeROIResampleModel;
class ProgressReporterDelegate;
class ReorientImageModel;
class DisplayLayoutModel;
class PaintbrushModel;
class PaintbrushSettingsModel;
class PolygonSettingsModel;
class LayerGeneralPropertiesModel;
class SynchronizationModel;
class SnakeParameterModel;
class MeshExportModel;
class GlobalPreferencesModel;
class GlobalDisplaySettings;
class ImageIOWizardModel;
class ImageWrapperBase;
class ColorLabelQuickListModel;

namespace itk
{
class Command;
}


class SystemInfoDelegate;


/**


  */
class GlobalUIModel : public AbstractModel
{

public:

  typedef AbstractModel Superclass;
  typedef GlobalUIModel Self;
  typedef SmartPtr<Self> Pointer;
  typedef SmartPtr<const Self> ConstPointer;

  itkNewMacro(Self)
  itkTypeMacro(GlobalUIModel, AbstractModel)


  // Events fired by this object
  FIRES(CursorUpdateEvent)
  FIRES(LayerChangeEvent)
  FIRES(LinkedZoomUpdateEvent)
  FIRES(LabelUnderCursorChangedEvent)
  FIRES(ToolbarModeChangedEvent)



  irisGetMacro(Driver, IRISApplication *)

  irisGetMacro(AppearanceSettings, SNAPAppearanceSettings *)

  /** Get the global display settings (thumbnail properties, etc.) */
  irisGetMacro(GlobalDisplaySettings, const GlobalDisplaySettings *)

  /** Update the global display settings (thumbnail properties, etc.) */
  void SetGlobalDisplaySettings(const GlobalDisplaySettings *settings);

  irisGetMacro(SliceCoordinator, SliceWindowCoordinator *)

  // Convenience access to the SystemInfterface
  SystemInterface *GetSystemInterface() const;

  // I don't know why this is in a separate class
  GlobalState *GetGlobalState() const;

  /**
   * Load the global user preferences at startup. This high-level method
   * loads the preference file into m_SystemInterface and then pulls out
   * of the registry folders various settings.
   *
   * TODO: this is coded badly. SystemInterface is a poorly designed class
   * that combines low-level and high-level functionality. It needs to be
   * recoded
   */
  void LoadUserPreferences();

  /**
   * Save user preferences to disk before quitting the application
   */
  void SaveUserPreferences();

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

  /** Get the polygon drawing model for each slice */
  SnakeROIModel *GetSnakeROIModel(unsigned int i) const
  {
    return m_SnakeROIModel[i];
  }

  PaintbrushModel *GetPaintbrushModel(unsigned int i) const
  {
    return m_PaintbrushModel[i];
  }

  /** Get the annotation model for each slice */
  AnnotationModel *GetAnnotationModel(unsigned int i) const
  {
    return m_AnnotationModel[i];
  }

  /** Get the model for intensity curve navigation */
  irisGetMacro(IntensityCurveModel, IntensityCurveModel *)

  /** Get the model for color map interation */
  irisGetMacro(ColorMapModel, ColorMapModel *)

  /** Get the model for obtaining image info for layers */
  irisGetMacro(ImageInfoModel, ImageInfoModel *)

  /** Get the model for selecting channels in a multi-channel image */
  irisGetMacro(LayerGeneralPropertiesModel, LayerGeneralPropertiesModel *)

  /** Get the model encapsulating the main images and all overlays */
  irisGetMacro(LoadedLayersSelectionModel, LayerSelectionModel *)

  /** Get the model for 3D window interaction */
  irisGetMacro(Model3D, Generic3DModel *)

  /** Get the model for the label editor */
  irisGetMacro(LabelEditorModel, LabelEditorModel *)

  /** Get the model for image reorientation */
  irisGetMacro(ReorientImageModel, ReorientImageModel *)

  /** Get the model that handles UI for the cursor inspector */
  irisGetMacro(CursorInspectionModel, CursorInspectionModel *)

  /** The model that handles snake wizard interaction */
  irisGetMacro(SnakeWizardModel, SnakeWizardModel *)

  /** The model handling display layout properties */
  irisGetMacro(DisplayLayoutModel, DisplayLayoutModel *)

  /** Model for managing paintbrush settings */
  irisGetMacro(PaintbrushSettingsModel, PaintbrushSettingsModel *)

  /** Model for managing polygon settings */
  irisGetMacro(PolygonSettingsModel, PolygonSettingsModel *)

  /** Model for multi-session sync */
  irisGetMacro(SynchronizationModel, SynchronizationModel *)

  /** Model for snake parameter editing */
  irisGetMacro(SnakeParameterModel, SnakeParameterModel *)

  /** Model for the snake ROI resampling */
  irisGetMacro(SnakeROIResampleModel, SnakeROIResampleModel *)

  /** Model for the mesh export wizard */
  irisGetMacro(MeshExportModel, MeshExportModel *)

  /** Model for the preferences dialog */
  irisGetMacro(GlobalPreferencesModel, GlobalPreferencesModel *)

  /** Model for the list of recently used color labels */
  irisGetMacro(ColorLabelQuickListModel, ColorLabelQuickListModel *)

  /**
    Check the state of the system. This class will issue StateChangeEvent()
    when one of the flags has changed. This method can be used together with
    the SNAPUIFlag object to construct listeners to complex state changes.
   */
  bool CheckState(UIState state);

  /** Get the model for the cursor coordinates */
  irisGetMacro(CursorPositionModel, AbstractRangedUIntVec3Property *)

  /** Get the models for the snake ROI */
  irisGetMacro(SnakeROIIndexModel, AbstractRangedUIntVec3Property *)
  irisGetMacro(SnakeROISizeModel, AbstractRangedUIntVec3Property *)

  /** A model for overall segmentation opacity (int, range 0..100) */
  irisRangedPropertyAccessMacro(SegmentationOpacity, int)

  /** A model for the segmentation visibility on/off state */
  irisSimplePropertyAccessMacro(SegmentationVisibility, bool)

  /** Method to toggle overlay visibility (all or selected overlays) */
  void ToggleOverlayVisibility();

  /** Method to adjust overlay opacity (all or selected overlays) */
  void AdjustOverlayOpacity(int delta);

  /** Get a list of k recent "things" that are tracked in history */
  std::vector<std::string> GetRecentHistoryItems(const char *historyCategory,
                                                 unsigned int k = 5, bool global_history = true);

  /** Check if a particular history is empty */
  bool IsHistoryEmpty(const char *historyCategory);

  typedef AbstractPropertyModel< std::vector<std::string> > AbstractHistoryModel;

  /** A quick-access method to the global history models, same as calling the
   * GetGlobalHistoryModel() in the HistoryManager() */
  AbstractHistoryModel *GetHistoryModel(const std::string &category);

  /** Get the progress command */
  irisGetMacro(ProgressCommand, itk::Command *)

  /** Get and set the progress reporter delegate */
  irisGetSetMacro(ProgressReporterDelegate, ProgressReporterDelegate *)

  /** Get and set the last screenshot filename */
  irisGetSetMacro(LastScreenshotFileName, std::string)

  /** Generate a suggested filename for saving screenshots, by incrementing
   *  from thelast saved screenshot filename */
  std::string GenerateScreenshotFilename();

  /**
   * Create a temporary model for saving an image layer to a file, to use in
   * conjunction with an IO wizard. We pass in the Layer and the LayerRole
   */
  SmartPtr<ImageIOWizardModel> CreateIOWizardModelForSave(
      ImageWrapperBase *layer, LayerRole role);

  /**
   * Perform an animation step
   */
  void AnimateLayerComponents();

  /** Increment the current color label (delta = 1 or -1) */
  void IncrementDrawingColorLabel(int delta);

  /** Increment the draw over color label (delta = 1 or -1) */
  void IncrementDrawOverColorLabel(int delta);

  /** Auto-adjust contrast in all image layers */
  void AutoContrastAllLayers();

  /** Auto-adjust contrast in all image layers */
  void ResetContrastAllLayers();

protected:

  GlobalUIModel();
  ~GlobalUIModel();

  // Callback for reporting progress
  void ProgressCallback(itk::Object *source, const itk::EventObject &event);

  SmartPtr<IRISApplication> m_Driver;

  SmartPtr<SNAPAppearanceSettings> m_AppearanceSettings;

  SmartPtr<GlobalDisplaySettings> m_GlobalDisplaySettings;

  // A set of three slice models, representing the UI state of each
  // of the 2D slice panels the user interacts with
  SmartPtr<GenericSliceModel> m_SliceModel[3];

  // A set of models that support cursor navigation
  SmartPtr<OrthogonalSliceCursorNavigationModel> m_CursorNavigationModel[3];

  // Models for polygon drawing
  SmartPtr<PolygonDrawingModel> m_PolygonDrawingModel[3];

  // Models for snake ROI drawing
  SmartPtr<SnakeROIModel> m_SnakeROIModel[3];

  // Models for paintbrush drawing
  SmartPtr<PaintbrushModel> m_PaintbrushModel[3];

  // Models for annotation
  SmartPtr<AnnotationModel> m_AnnotationModel[3];

  // Window coordinator
  SmartPtr<SliceWindowCoordinator> m_SliceCoordinator;

  // Model for intensity curve manipulation
  SmartPtr<IntensityCurveModel> m_IntensityCurveModel;

  // Model for color map manipulation
  SmartPtr<ColorMapModel> m_ColorMapModel;

  // Model for image info interaction
  SmartPtr<ImageInfoModel> m_ImageInfoModel;

  // Model for multi-channel image component selection
  SmartPtr<LayerGeneralPropertiesModel> m_LayerGeneralPropertiesModel;

  // Layer selection model encapsulating the main image and overlays
  SmartPtr<LayerSelectionModel> m_LoadedLayersSelectionModel;

  // Label editor model
  SmartPtr<LabelEditorModel> m_LabelEditorModel;

  // Reorient image model
  SmartPtr<ReorientImageModel> m_ReorientImageModel;

  // Cursor interaction model
  SmartPtr<CursorInspectionModel> m_CursorInspectionModel;

  // 3D Model
  SmartPtr<Generic3DModel> m_Model3D;

  // The snake wizard model
  SmartPtr<SnakeWizardModel> m_SnakeWizardModel;

  // Display layout model
  SmartPtr<DisplayLayoutModel> m_DisplayLayoutModel;

  // Paintbrush settings
  SmartPtr<PaintbrushSettingsModel> m_PaintbrushSettingsModel;

  // Polygon settings
  SmartPtr<PolygonSettingsModel> m_PolygonSettingsModel;

  // Synchronization
  SmartPtr<SynchronizationModel> m_SynchronizationModel;

  // Snake parameters
  SmartPtr<SnakeParameterModel> m_SnakeParameterModel;

  // Snake resampling model
  SmartPtr<SnakeROIResampleModel> m_SnakeROIResampleModel;

  // Model for the quick list of recently used labels
  SmartPtr<ColorLabelQuickListModel> m_ColorLabelQuickListModel;

  // Current coordinates of the cursor
  SmartPtr<AbstractRangedUIntVec3Property> m_CursorPositionModel;
  bool GetCursorPositionValueAndRange(
      Vector3ui &value, NumericValueRange<Vector3ui> *range);
  void SetCursorPosition(Vector3ui value);

  // Current ROI for snake mode
  SmartPtr<AbstractRangedUIntVec3Property> m_SnakeROIIndexModel;
  bool GetSnakeROIIndexValueAndRange(
      Vector3ui &value, NumericValueRange<Vector3ui> *range);
  void SetSnakeROIIndexValue(Vector3ui value);

  SmartPtr<AbstractRangedUIntVec3Property> m_SnakeROISizeModel;
  bool GetSnakeROISizeValueAndRange(
      Vector3ui &value, NumericValueRange<Vector3ui> *range);
  void SetSnakeROISizeValue(Vector3ui value);

  // The model for the mesh export wizard
  SmartPtr<MeshExportModel> m_MeshExportModel;

  // Global preferences model
  SmartPtr<GlobalPreferencesModel> m_GlobalPreferencesModel;

  // A pointer to the progress reporter delegate object
  ProgressReporterDelegate *m_ProgressReporterDelegate;

  // An ITK command used to handle progress
  SmartPtr<itk::Command> m_ProgressCommand;

  // Screenshot filename
  std::string m_LastScreenshotFileName;

  // Segmentation opacity and visibility models
  SmartPtr<AbstractRangedIntProperty> m_SegmentationOpacityModel;
  SmartPtr<AbstractSimpleBooleanProperty> m_SegmentationVisibilityModel;

  // Callbacks for the opacity model
  bool GetSegmentationOpacityValueAndRange(int &value, NumericValueRange<int> *domain);
  void SetSegmentationOpacityValue(int value);

};

#endif // GLOBALUIMODEL_H
