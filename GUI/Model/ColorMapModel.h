#ifndef COLORMAPMODEL_H
#define COLORMAPMODEL_H

#include "AbstractLayerAssociatedModel.h"
#include "PropertyModel.h"
#include <UIReporterDelegates.h>
#include "ColorMap.h"
#include "ColorMapPresetManager.h"

class ColorMap;
class SystemInterface;

class ColorMapLayerProperties
{
public:

  irisGetSetMacro(LayerObserverTag, unsigned long)

  ColorMapLayerProperties()
  {
    m_SelectedControlIndex = -1;
    m_SelectedControlSide = NA;
    m_LayerObserverTag = 0;
    m_SelectedPreset =
        std::make_pair(ColorMapPresetManager::PRESET_NONE, std::string());
  }

  virtual ~ColorMapLayerProperties() {}

  enum Side {LEFT = 0, RIGHT, NA};

  irisGetSetMacro(SelectedControlIndex, int)
  irisGetSetMacro(SelectedControlSide, Side)
  irisGetSetMacro(SelectedPreset, ColorMapPresetManager::PresetMatch)

protected:

  // Control point being edited
  int m_SelectedControlIndex;

  // Side of the control point, if discontinuous
  Side m_SelectedControlSide;

  // Cached information about the currently selected preset
  ColorMapPresetManager::PresetMatch m_SelectedPreset;

  // Whether or not we are already listening to events from this layer
  unsigned long m_ColorMapObserverTag, m_LayerObserverTag;
};

typedef AbstractLayerAssociatedModel<
    ColorMapLayerProperties, ImageWrapperBase> ColorMapModelBase;


/**
  The UI model for color map editing
  */
class ColorMapModel : public ColorMapModelBase
{
public:
  irisITKObjectMacro(ColorMapModel, ColorMapModelBase)

  typedef ColorMapLayerProperties::Side Side;
  typedef ColorMap::CMPointType Continuity;
  typedef std::vector<std::string> PresetList;

  // This event only affects this model
  itkEventMacro(PresetUpdateEvent, IRISEvent)

  // This event is fired when the presets are changed
  FIRES(PresetUpdateEvent)

  /**
    States in which the model can be, which allow the activation and
    deactivation of various widgets in the interface
    */
  enum UIState {
    UIF_LAYER_ACTIVE,
    UIF_CONTROL_SELECTED,
    UIF_CONTROL_SELECTED_IS_NOT_ENDPOINT,
    UIF_CONTROL_SELECTED_IS_DISCONTINUOUS,
    UIF_PRESET_SELECTED,
    UIF_USER_PRESET_SELECTED
    };

  void SetParentModel(GlobalUIModel *parent);

  /**
    Check the state flags above
    */
  bool CheckState(UIState state);

  // Implementation of virtual functions from parent class
  void RegisterWithLayer(ImageWrapperBase *layer);
  void UnRegisterFromLayer(ImageWrapperBase *layer, bool being_deleted);

  /** Before using the model, it must be coupled with a size reporter */
  irisGetSetMacro(ViewportReporter, ViewportSizeReporter *)

  /**
    Process colormap box interaction events
    */
  bool ProcessMousePressEvent(const Vector3d &x);
  bool ProcessMouseDragEvent(const Vector3d &x);
  bool ProcessMouseReleaseEvent(const Vector3d &x);

  /** Update the model in response to upstream events */
  virtual void OnUpdate();

  typedef AbstractPropertyModel<Continuity> ContinuityValueModel;
  typedef AbstractPropertyModel<Side> SideValueModel;

  /** The model for setting moving control point position */
  irisGetMacro(MovingControlPositionModel, AbstractRangedDoubleProperty *)

  /** The model for setting moving control point opacity */
  irisGetMacro(MovingControlOpacityModel, AbstractRangedDoubleProperty *)

  /** The model for setting moving control point continuity */
  irisGetMacro(MovingControlContinuityModel, ContinuityValueModel *)

  /** The model for setting moving control point side */
  irisGetMacro(MovingControlSideModel, SideValueModel *)

  /** The index of the moving control point */
  irisGetMacro(MovingControlIndexModel, AbstractRangedIntProperty *)

  /** The overall opacity of the selected layer */
  irisGetMacro(LayerOpacityModel, AbstractRangedDoubleProperty *)

  /** The overall visibility of the selected layer */
  irisGetMacro(LayerVisibilityModel, AbstractSimpleBooleanProperty *)

  /** Get the color map in associated layer */
  ColorMap *GetColorMap();

  /** Check whether a particular control point is selected */
  bool IsControlSelected(int cp, Side side);

  /** Set the selected control point, return true if selection
      changed as the result */
  bool SetSelection(int cp, Side side = ColorMapLayerProperties::NA);

  /** Delete the selected control */
  void DeleteSelectedControl();

  /** Get the color of the selected point */
  Vector3d GetSelectedColor();

  /** Set the color of the selected point */
  void SetSelectedColor(Vector3d rgb);

  /** Get the list of color map presets */
  void GetPresets(PresetList &system, PresetList &user);

  /** Get the preset manager object */
  irisGetMacro(PresetManager, ColorMapPresetManager *)

  /** Select one of the presets. The index is into the combined list
    of system and user presets */
  void SelectPreset(const std::string &preset);

  /** Save the current state as a preset */
  void SaveAsPreset(std::string name);

  /** Delete a selected preset */
  void DeletePreset(std::string name);

  /** Get the currently selected preset */
  std::string GetSelectedPreset();

protected:

  ColorMapModel();

  SmartPtr<AbstractRangedDoubleProperty> m_MovingControlPositionModel;
  SmartPtr<AbstractRangedDoubleProperty> m_MovingControlOpacityModel;
  SmartPtr<SideValueModel> m_MovingControlSideModel;
  SmartPtr<ContinuityValueModel> m_MovingControlContinuityModel;
  SmartPtr<AbstractRangedIntProperty> m_MovingControlIndexModel;
  SmartPtr<AbstractRangedDoubleProperty> m_LayerOpacityModel;
  SmartPtr<AbstractSimpleBooleanProperty> m_LayerVisibilityModel;

  // A pointer to the system interface object
  SystemInterface *m_System;

  // A size reporter delegate (notifies the model when the size of the
  // corresponding widget changes).
  ViewportSizeReporter *m_ViewportReporter;

  // Color map preset manager
  ColorMapPresetManager *m_PresetManager;

  // Colormap presets
  // PresetList m_PresetSystem, m_PresetUser;

  // Get the RGBA for selected point
  bool GetSelectedRGBA(ColorMap::RGBAType &rgba);
  void SetSelectedRGBA(ColorMap::RGBAType rgba);

  // Control point position
  bool GetMovingControlPositionValueAndRange(
      double &value, NumericValueRange<double> *range);
  void SetMovingControlPosition(double value);

  // Control point opacity
  bool GetMovingControlOpacityValueAndRange(
      double &value, NumericValueRange<double> *range);
  void SetMovingControlOpacity(double value);

  // Control point style
  bool GetMovingControlType(Continuity &value);
  void SetMovingControlType(Continuity value);

  // Control point style
  bool GetMovingControlSide(Side &value);
  void SetMovingControlSide(Side value);

  // Selected control point index
  bool GetMovingControlIndexValueAndRange(
      int &value, NumericValueRange<int> *range);
  void SetMovingControlIndex(int value);

  // Opacity of the selected layer
  bool GetLayerOpacityValueAndRange(
      double &value, NumericValueRange<double> *range);
  void SetLayerOpacity(double value);

  // Extract a colormap from the layer if it has one
  ColorMap *GetColorMap(ImageWrapperBase *layer);

};

#endif // COLORMAPMODEL_H
