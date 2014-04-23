
#include "GlobalPreferencesModel.h"
#include "MeshOptions.h"
#include "GlobalUIModel.h"
#include "GlobalState.h"
#include "DefaultBehaviorSettings.h"

GlobalPreferencesModel::GlobalPreferencesModel()
{
  // Create all the property containers owned by the model. These hold copies
  // of the property containers stored in the system.
  m_DefaultBehaviorSettings = DefaultBehaviorSettings::New();
  m_GlobalDisplaySettings = GlobalDisplaySettings::New();
  m_MeshOptions = MeshOptions::New();

  m_CheckForUpdateModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetCheckForUpdateValue, &Self::SetCheckForUpdateValue);
  m_CheckForUpdateModel->RebroadcastFromSourceProperty(
        m_DefaultBehaviorSettings->GetCheckForUpdatesModel());

  // Current appearance element
  m_ActiveUIElement = SNAPAppearanceSettings::CROSSHAIRS;
  m_ActiveUIElementModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetActiveUIElementValue, &Self::SetActiveUIElementValue);

  // All appearance elements
  m_ActiveUIElementAppearance = OpenGLAppearanceElement::New();
  for(int i = 0; i < SNAPAppearanceSettings::ELEMENT_COUNT; i++)
    m_ElementAppearance[i] = OpenGLAppearanceElement::New();

  // Layout labels
  for(int j = 0; j < 3; j++)
    {
    m_LayoutLabelModel[j] = wrapIndexedGetterSetterPairAsProperty(
          this, j, &Self::GetLayoutLabelIndexedValue);

    m_LayoutLabelModel[j]->RebroadcastFromSourceProperty(
          m_GlobalDisplaySettings->GetSliceLayoutModel());
    }

}

bool GlobalPreferencesModel::GetCheckForUpdateValue(bool &outValue)
{
  switch(m_DefaultBehaviorSettings->GetCheckForUpdates())
    {
    case DefaultBehaviorSettings::UPDATE_YES:
      outValue = true; return true;
    case DefaultBehaviorSettings::UPDATE_NO:
      outValue = false; return true;
    case DefaultBehaviorSettings::UPDATE_UNKNOWN:
      return false;
    }
  return false;
}

void GlobalPreferencesModel::SetCheckForUpdateValue(bool inValue)
{
  m_DefaultBehaviorSettings->SetCheckForUpdates(
        inValue ? DefaultBehaviorSettings::UPDATE_YES : DefaultBehaviorSettings::UPDATE_NO);
}

bool GlobalPreferencesModel::GetActiveUIElementValue(GlobalPreferencesModel::UIElement &value)
{
  value = m_ActiveUIElement;
  return true;
}

void GlobalPreferencesModel::SetActiveUIElementValue(GlobalPreferencesModel::UIElement value)
{
  if(m_ActiveUIElement != value)
    {
    // Locally store the current settings edited by the user
    if(m_ActiveUIElement != SNAPAppearanceSettings::ELEMENT_COUNT)
      m_ElementAppearance[m_ActiveUIElement]->DeepCopy(m_ActiveUIElementAppearance);

    // Update the element
    m_ActiveUIElement = value;

    // Update the apperance shown to the user (unless this is a bad value)
    if(m_ActiveUIElement != SNAPAppearanceSettings::ELEMENT_COUNT)
      m_ActiveUIElementAppearance->DeepCopy(m_ElementAppearance[m_ActiveUIElement]);

    // The state has changed
    this->InvokeEvent(StateMachineChangeEvent());
    }
}

bool GlobalPreferencesModel::GetLayoutLabelIndexedValue(int index, std::string &value)
{
  static std::map<GlobalDisplaySettings::UISliceLayout, std::string> idxmap;
  if(!idxmap.size())
    {
    idxmap[GlobalDisplaySettings::LAYOUT_ACS] = "ACS";
    idxmap[GlobalDisplaySettings::LAYOUT_ASC] = "ASC";
    idxmap[GlobalDisplaySettings::LAYOUT_CAS] = "CAS";
    idxmap[GlobalDisplaySettings::LAYOUT_CSA] = "CSA";
    idxmap[GlobalDisplaySettings::LAYOUT_SAC] = "SAC";
    idxmap[GlobalDisplaySettings::LAYOUT_SCA] = "SCA";
    }

  GlobalDisplaySettings::UISliceLayout lo = m_GlobalDisplaySettings->GetSliceLayout();
  char letter = idxmap[lo][index];

  switch(letter)
    {
    case 'A' : value = "Axial"; break;
    case 'S' : value = "Sagittal"; break;
    case 'C' : value = "Coronal"; break;
    }

  return true;
}

bool GlobalPreferencesModel::CheckState(GlobalPreferencesModel::UIState state)
{
  switch(state)
    {
    case GlobalPreferencesModel::UIF_VALID_UI_ELEMENT_SELECTED:
      return m_ActiveUIElement != SNAPAppearanceSettings::ELEMENT_COUNT;
    }
  return false;
}

void GlobalPreferencesModel::SetParentModel(GlobalUIModel *parent)
{
  m_ParentModel = parent;
}

void GlobalPreferencesModel::InitializePreferences()
{
  // Pull the preferences from the system
  GlobalState *gs = m_ParentModel->GetGlobalState();
  SNAPAppearanceSettings *as = m_ParentModel->GetAppearanceSettings();

  // Default behaviors
  m_DefaultBehaviorSettings->DeepCopy(gs->GetDefaultBehaviorSettings());

  // Global display prefs
  m_GlobalDisplaySettings->DeepCopy(m_ParentModel->GetGlobalDisplaySettings());

  // Mesh options
  m_MeshOptions->DeepCopy(gs->GetMeshOptions());

  // Appearance of all the elements
  for(int i = 0; i < SNAPAppearanceSettings::ELEMENT_COUNT; i++)
    m_ElementAppearance[i]->DeepCopy(as->GetUIElement(i));

  // Apperance of selected element
  if(m_ActiveUIElement != SNAPAppearanceSettings::ELEMENT_COUNT)
    m_ActiveUIElementAppearance->DeepCopy(as->GetUIElement(m_ActiveUIElement));

}

void GlobalPreferencesModel::ApplyPreferences()
{
  // Pull the preferences from the system
  GlobalState *gs = m_ParentModel->GetGlobalState();
  SNAPAppearanceSettings *as = m_ParentModel->GetAppearanceSettings();

  // Default behaviors
  gs->GetDefaultBehaviorSettings()->DeepCopy(m_DefaultBehaviorSettings);

  // Global display prefs
  m_ParentModel->SetGlobalDisplaySettings(m_GlobalDisplaySettings);

  // Mesh options
  gs->GetMeshOptions()->DeepCopy(m_MeshOptions);

  // If the current element is valid, accept it
  if(m_ActiveUIElement != SNAPAppearanceSettings::ELEMENT_COUNT)
    m_ElementAppearance[m_ActiveUIElement]->DeepCopy(m_ActiveUIElementAppearance);

  // Appearance of all the elements
  for(int i = 0; i < SNAPAppearanceSettings::ELEMENT_COUNT; i++)
    as->GetUIElement(i)->DeepCopy(m_ElementAppearance[i]);
}

void GlobalPreferencesModel::ResetCurrentElement()
{
  assert(m_ActiveUIElement != SNAPAppearanceSettings::ELEMENT_COUNT);

  SNAPAppearanceSettings *as = m_ParentModel->GetAppearanceSettings();
  m_ActiveUIElementAppearance->DeepCopy(
        as->GetUIElementDefaultSettings(m_ActiveUIElement));
}

void GlobalPreferencesModel::ResetAllElements()
{
  assert(m_ActiveUIElement != SNAPAppearanceSettings::ELEMENT_COUNT);

  SNAPAppearanceSettings *as = m_ParentModel->GetAppearanceSettings();
  for(int i = 0; i < SNAPAppearanceSettings::ELEMENT_COUNT; i++)
    {
    m_ElementAppearance[i]->DeepCopy(as->GetUIElementDefaultSettings(i));
    }

  if(m_ActiveUIElement != SNAPAppearanceSettings::ELEMENT_COUNT)
    m_ActiveUIElementAppearance->DeepCopy(m_ElementAppearance[m_ActiveUIElement]);
}



