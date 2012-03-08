#ifndef COLORMAPMODEL_H
#define COLORMAPMODEL_H

#include "AbstractLayerAssociatedModel.h"
#include "EditableNumericValueModel.h"
#include "GreyImageWrapper.h"
#include <UIReporterDelegates.h>


class ColorMap;
class SystemInterface;

class ColorMapLayerProperties
{
public:

  irisGetSetMacro(ObserverTag, unsigned long)

  ColorMapLayerProperties()
  {
    m_SelectedControlIndex = -1;
    m_SelectedControlSide = NA;
    m_ObserverTag = 0;
  }

  enum Side {LEFT = 0, RIGHT, NA};

  irisGetSetMacro(SelectedControlIndex, int)
  irisGetSetMacro(SelectedControlSide, Side)
  irisGetSetMacro(SelectedPreset, std::string)

protected:

  // Control point being edited
  int m_SelectedControlIndex;

  // Side of the control point, if discontinuous
  Side m_SelectedControlSide;

  // The index of the current preset
  std::string m_SelectedPreset;

  // Whether or not we are already listening to events from this layer
  unsigned long m_ObserverTag;
};

typedef AbstractLayerAssociatedModel<
    ColorMapLayerProperties,
    GreyImageWrapperBase> ColorMapModelBase;


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
    UIF_USER_PRESET_SELECTED
    };

  void SetParentModel(GlobalUIModel *parent);

  /**
    Check the state flags above
    */
  bool CheckState(UIState state);

  // Implementation of virtual functions from parent class
  void RegisterWithLayer(GreyImageWrapperBase *layer);
  void UnRegisterFromLayer(GreyImageWrapperBase *layer);

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

  typedef AbstractEditableNumericValueModel<double> RealValueModel;
  typedef AbstractEditableNumericValueModel<int> IntegerValueModel;
  typedef AbstractEditableNumericValueModel<bool> BooleanValueModel;
  typedef AbstractEditableNumericValueModel<Continuity> ContinuityValueModel;
  typedef AbstractEditableNumericValueModel<Side> SideValueModel;

  /** The model for setting moving control point position */
  irisGetMacro(MovingControlPositionModel, RealValueModel *)

  /** The model for setting moving control point opacity */
  irisGetMacro(MovingControlOpacityModel, RealValueModel *)

  /** The model for setting moving control point continuity */
  irisGetMacro(MovingControlContinuityModel, ContinuityValueModel *)

  /** The model for setting moving control point side */
  irisGetMacro(MovingControlSideModel, SideValueModel *)

  /** Get the color map in associated layer */
  ColorMap *GetColorMap();

  /** Check whether a particular control point is selected */
  bool IsControlSelected(int cp, Side side);

  /** Set the selected control point, return true if selection
      changed as the result */
  bool SetSelection(int cp, Side side = ColorMapLayerProperties::NA);

  /** Get the color of the selected point */
  Vector3d GetSelectedColor();

  /** Set the color of the selected point */
  void SetSelectedColor(Vector3d rgb);

  /** Get the list of color map presets */
  void GetPresets(PresetList &system, PresetList &user);

  /** Select one of the presets. The index is into the combined list
    of system and user presets */
  void SelectPreset(const char *preset);

  /** Save the current state as a preset */
  void SaveAsPreset(std::string name);

  /** Delete a selected preset */
  void DeletePreset(std::string name);

protected:

  ColorMapModel();

  SmartPtr<RealValueModel> m_MovingControlPositionModel;
  SmartPtr<RealValueModel> m_MovingControlOpacityModel;
  SmartPtr<SideValueModel> m_MovingControlSideModel;
  SmartPtr<ContinuityValueModel> m_MovingControlContinuityModel;

  // A pointer to the system interface object
  SystemInterface *m_System;

  // A size reporter delegate (notifies the model when the size of the
  // corresponding widget changes).
  ViewportSizeReporter *m_ViewportReporter;

  // Colormap presets
  PresetList m_PresetSystem, m_PresetUser;

  // Control point position
  bool GetMovingControlPositionValueAndRange(
      double &value, NumericValueRange<double> *range);
  void SetMovingControlPosition(double value);

  // Control point opacity
  typedef iris_vector_fixed<double, 4> Vec4d;

  bool GetMovingControlRGBAValueAndRange(
      Vec4d &value, NumericValueRange< Vec4d > *range);
  void SetMovingControlRGBA(Vec4d value);

  // Control point style
  bool GetMovingControlTypeValueAndRange(
      Continuity &value, NumericValueRange<Continuity> *range);
  void SetMovingControlType(Continuity value);

  // Control point style
  bool GetMovingControlSideValueAndRange(
      Side &value, NumericValueRange<Side> *range);
  void SetMovingControlSide(Side value);
};

#endif // COLORMAPMODEL_H
