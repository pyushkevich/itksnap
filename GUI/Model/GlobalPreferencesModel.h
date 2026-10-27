#ifndef GLOBALPREFERENCESMODEL_H
#define GLOBALPREFERENCESMODEL_H

#include "PropertyModel.h"
#include "ColorMap.h"
#include "SNAPAppearanceSettings.h"

class MeshOptions;
class GlobalUIModel;
class DefaultBehaviorSettings;

/**
 * This model exposes the different global preferences to the GUI. Changes
 * made through this model take effect right away. The model can store the
 * preferences when the dialog opens, so that the user can revert to them.
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
    UIF_VALID_UI_ELEMENT_SELECTED,
    UIF_UI_LANGUAGE_CHANGED
  };

  bool CheckState(UIState state);


  // Default behaviors and permissions
  irisGetMacro(DefaultBehaviorSettings, DefaultBehaviorSettings *)

  // The model controlling whether updates are enabled or disabled must
  // be exposed to the GUI as a boolean model, whereas internally it is
  // of an ENUM type
  irisSimplePropertyAccessMacro(CheckForUpdate, bool)

  /**
   * The translation in use (e.g., "de", or "en" for the untranslated user
   * interface) and the one that "System default" selects. These are set at
   * startup, so that the dialog can tell whether a new language choice needs
   * a restart to take effect.
   */
  irisGetSetMacro(ActiveUILanguage, std::string)
  irisGetSetMacro(SystemUILanguage, std::string)

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
   * Store the current preferences, so that RevertPreferences() can restore
   * them. This method should be called when opening the preferences dialog
   */
  void StorePreferencesForRevert();

  /**
   * Restore the preferences stored by StorePreferencesForRevert()
   */
  void RevertPreferences();

  /** Reset visual element to default */
  void ResetCurrentElement();

  /** Reset all visual elements to default */
  void ResetAllElements();

  /** Tabs of the preferences dialog whose settings can be reset to defaults */
  enum PreferencesTab {
    TAB_DEFAULT_BEHAVIOR,
    TAB_SLICE_VIEWS_DISPLAY,
    TAB_SLICE_VIEWS_LAYOUT,
    TAB_MESH_SMOOTHING,
    TAB_MESH_DECIMATION,
    TAB_TOOLS
  };

  /** Reset the settings shown on a tab of the preferences dialog to default */
  void ResetTabToDefaults(PreferencesTab tab);

protected:

  GlobalPreferencesModel();

  // Default behaviors and permissions (the system's settings)
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

  // The apperance settings for the active element, which the dialog edits.
  // These are kept in sync with the system's settings for that element
  SmartPtr<OpenGLAppearanceElement> m_ActiveUIElementAppearance;
  bool m_SyncingActiveUIElement = false;
  void OnActiveUIElementAppearanceChange();
  void OnUIElementChange(itk::Object *caller, const itk::EventObject &event);
  void UpdateActiveUIElementAppearance();

  // Mesh options (the system's options)
  SmartPtr<MeshOptions> m_MeshOptions;

  // The system's global display preferences (thumbnail, etc)
  SmartPtr<GlobalDisplaySettings> m_GlobalDisplaySettings;

  // Preferences stored for revert
  SmartPtr<DefaultBehaviorSettings> m_SavedDefaultBehaviorSettings;
  SmartPtr<GlobalDisplaySettings> m_SavedGlobalDisplaySettings;
  SmartPtr<MeshOptions> m_SavedMeshOptions;
  SmartPtr<OpenGLAppearanceElement> m_SavedElementAppearance[SNAPAppearanceSettings::ELEMENT_COUNT];

  // Layout label
  SmartPtr<AbstractSimpleStringProperty> m_LayoutLabelModel[3];
  bool GetLayoutLabelIndexedValue(int index, std::string &value);

  // Translation in use and the one selected by "System default"
  std::string m_ActiveUILanguage, m_SystemUILanguage;

  // Parent model
  GlobalUIModel *m_ParentModel;
};

#endif // GLOBALPREFERENCESMODEL_H
