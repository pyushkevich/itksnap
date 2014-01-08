#ifndef GLOBALPREFERENCESMODEL_H
#define GLOBALPREFERENCESMODEL_H

#include "PropertyModel.h"
#include "ColorMap.h"
#include "SNAPAppearanceSettings.h"

class MeshOptions;
class GlobalUIModel;
class DefaultBehaviorSettings;

/**
 * This model exposes the different global preferences to the GUI. It is set
 * up for a dialog in which the user must press Apply for the changes to
 * be propagated to the GUI. So this model caches the values the preferences
 * until the apply function is called
 */
class GlobalPreferencesModel : public AbstractModel
{
public:
  irisITKObjectMacro(GlobalPreferencesModel, AbstractModel)

  // Typedefs for interpolation mode
  enum InterpolationMode { NEAREST = 0, LINEAR };
  typedef SimpleItemSetDomain<InterpolationMode, std::string> InterpolationDomain;

  // Typedefs for appearance
  typedef SNAPAppearanceSettings::UIElements UIElement;

  // State flags
  enum UIState {
    UIF_VALID_UI_ELEMENT_SELECTED
  };

  bool CheckState(UIState state);


  // Default behaviors and permissions
  irisGetMacro(DefaultBehaviorSettings, DefaultBehaviorSettings *)

  // The model controlling whether updates are enabled or disabled must
  // be exposed to the GUI as a boolean model, whereas internally it is
  // of an ENUM type
  irisSimplePropertyAccessMacro(CheckForUpdate, bool)

  // Screen layout
  irisGetMacro(GlobalDisplaySettings, GlobalDisplaySettings *)

  // Other slice display properties
  irisSimplePropertyAccessMacro(DefaultColorMapPreset, std::string)

  // Current UI element whose apperance is being edited
  irisSimplePropertyAccessMacro(ActiveUIElement, UIElement)

  // Apperance of the current element
  irisGetMacro(ActiveUIElementAppearance, OpenGLAppearanceElement *)

  // Mesh options
  irisGetMacro(MeshOptions, MeshOptions *)

  // Screen layout labels
  AbstractSimpleStringProperty *GetLayoutLabelModel(int i)
    { return m_LayoutLabelModel[i]; }

  /**
   * Set the parent model
   */
  void SetParentModel(GlobalUIModel *parent);

  irisGetMacro(ParentModel, GlobalUIModel *)

  /**
   * Initialize the internal cached properties based on current system state.
   * This method should be called when opening the properties dialog, or on revert
   */
  void InitializePreferences();

  /**
   * Update the system state with the current cached preferences
   */
  void ApplyPreferences();

  /** Reset visual element to default */
  void ResetCurrentElement();

  /** Reset all visual elements to default */
  void ResetAllElements();

protected:

  GlobalPreferencesModel();

  // Default behaviors and permissions (copy of the system's settings)
  SmartPtr<DefaultBehaviorSettings> m_DefaultBehaviorSettings;

  // Updates model
  SmartPtr<AbstractSimpleBooleanProperty> m_CheckForUpdateModel;
  bool GetCheckForUpdateValue(bool &outValue);
  void SetCheckForUpdateValue(bool inValue);

  // Color map preset model (tricky)
  SmartPtr<ConcreteSimpleStringProperty> m_DefaultColorMapPresetModel;

  // Current appearance element
  UIElement m_ActiveUIElement;

  // The model for the appearance element. This is a model that wraps around the
  // getter and setter below, since changes to the active element require additional
  // changes to this class.
  SmartPtr<AbstractPropertyModel<UIElement, TrivialDomain> > m_ActiveUIElementModel;
  bool GetActiveUIElementValue(UIElement &value);
  void SetActiveUIElementValue(UIElement value);

  // The cached apperance settings for the active element
  SmartPtr<OpenGLAppearanceElement> m_ActiveUIElementAppearance;

  // The apperance settings for all elements (held until user presses apply)
  SmartPtr<OpenGLAppearanceElement> m_ElementAppearance[SNAPAppearanceSettings::ELEMENT_COUNT];

  // Mesh options (we keep a copy)
  SmartPtr<MeshOptions> m_MeshOptions;

  // Copy of the system's global display preferences (thumbnail, etc)
  SmartPtr<GlobalDisplaySettings> m_GlobalDisplaySettings;

  // Layout label
  SmartPtr<AbstractSimpleStringProperty> m_LayoutLabelModel[3];
  bool GetLayoutLabelIndexedValue(int index, std::string &value);

  // Parent model
  GlobalUIModel *m_ParentModel;
};

#endif // GLOBALPREFERENCESMODEL_H
