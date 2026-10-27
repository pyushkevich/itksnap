
#include "GlobalPreferencesModel.h"
#include "MeshOptions.h"
#include "GlobalUIModel.h"
#include "GlobalState.h"
#include "DefaultBehaviorSettings.h"
#include "IRISApplication.h"
#include "SNAPEventListenerCallbacks.h"

GlobalPreferencesModel::GlobalPreferencesModel()
{
  // Create the copies of the preferences stored for revert. The preferences
  // themselves belong to the system, and are set in SetParentModel()
  m_SavedDefaultBehaviorSettings = DefaultBehaviorSettings::New();
  m_SavedGlobalDisplaySettings = GlobalDisplaySettings::New();
  m_SavedMeshOptions = MeshOptions::New();
  for(int i = 0; i < SNAPAppearanceSettings::ELEMENT_COUNT; i++)
    m_SavedElementAppearance[i] = OpenGLAppearanceElement::New();

  m_CheckForUpdateModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetCheckForUpdateValue, &Self::SetCheckForUpdateValue);

  // Current appearance element
  m_ActiveUIElement = SNAPAppearanceSettings::CROSSHAIRS;
  m_ActiveUIElementModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetActiveUIElementValue, &Self::SetActiveUIElementValue);

  // Changes to the appearance of the active element go to the system right away
  m_ActiveUIElementAppearance = OpenGLAppearanceElement::New();
  AddListener(m_ActiveUIElementAppearance, ChildPropertyChangedEvent(),
              this, &Self::OnActiveUIElementAppearanceChange);

  // Layout labels
  for(int j = 0; j < 3; j++)
    {
    m_LayoutLabelModel[j] = wrapIndexedGetterSetterPairAsProperty(
          this, j, &Self::GetLayoutLabelIndexedValue);
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
    // Update the element
    m_ActiveUIElement = value;

    // Update the apperance shown to the user (unless this is a bad value)
    this->UpdateActiveUIElementAppearance();

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
    case GlobalPreferencesModel::UIF_UI_LANGUAGE_CHANGED:
      {
      std::string selected = m_DefaultBehaviorSettings->GetUILanguage();
      if (selected.empty())
        selected = m_SystemUILanguage;
      return selected != m_ActiveUILanguage;
      }
    }
  return false;
}

void GlobalPreferencesModel::SetParentModel(GlobalUIModel *parent)
{
  m_ParentModel = parent;

  // The preferences shown to the user are the system's preferences
  GlobalState *gs = m_ParentModel->GetGlobalState();
  m_DefaultBehaviorSettings = gs->GetDefaultBehaviorSettings();
  m_GlobalDisplaySettings = m_ParentModel->GetGlobalDisplaySettings();
  m_MeshOptions = gs->GetMeshOptions();

  m_CheckForUpdateModel->RebroadcastFromSourceProperty(
        m_DefaultBehaviorSettings->GetCheckForUpdatesModel());

  for(int j = 0; j < 3; j++)
    m_LayoutLabelModel[j]->RebroadcastFromSourceProperty(
          m_GlobalDisplaySettings->GetSliceLayoutModel());

  // Choosing a language changes whether a restart is needed
  Rebroadcast(m_DefaultBehaviorSettings->GetUILanguageModel(),
              ValueChangedEvent(), StateMachineChangeEvent());

  // Show changes made to the active element elsewhere (e.g., the crosshair
  // can be hidden from the menu)
  SNAPAppearanceSettings *as = m_ParentModel->GetAppearanceSettings();
  for(int i = 0; i < SNAPAppearanceSettings::ELEMENT_COUNT; i++)
    AddListener(as->GetUIElement(i), ChildPropertyChangedEvent(),
                this, &Self::OnUIElementChange);

  this->UpdateActiveUIElementAppearance();
}

void GlobalPreferencesModel::UpdateActiveUIElementAppearance()
{
  if(m_ActiveUIElement != SNAPAppearanceSettings::ELEMENT_COUNT)
    {
    SNAPAppearanceSettings *as = m_ParentModel->GetAppearanceSettings();
    m_SyncingActiveUIElement = true;
    m_ActiveUIElementAppearance->DeepCopy(as->GetUIElement(m_ActiveUIElement));
    m_SyncingActiveUIElement = false;
    }
}

void GlobalPreferencesModel::OnActiveUIElementAppearanceChange()
{
  if(!m_SyncingActiveUIElement && m_ActiveUIElement != SNAPAppearanceSettings::ELEMENT_COUNT)
    {
    SNAPAppearanceSettings *as = m_ParentModel->GetAppearanceSettings();
    m_SyncingActiveUIElement = true;
    as->GetUIElement(m_ActiveUIElement)->DeepCopy(m_ActiveUIElementAppearance);
    m_SyncingActiveUIElement = false;
    }
}

void GlobalPreferencesModel::OnUIElementChange(itk::Object *caller, const itk::EventObject &)
{
  SNAPAppearanceSettings *as = m_ParentModel->GetAppearanceSettings();
  if(!m_SyncingActiveUIElement && m_ActiveUIElement != SNAPAppearanceSettings::ELEMENT_COUNT
     && caller == as->GetUIElement(m_ActiveUIElement))
    this->UpdateActiveUIElementAppearance();
}

void GlobalPreferencesModel::StorePreferencesForRevert()
{
  SNAPAppearanceSettings *as = m_ParentModel->GetAppearanceSettings();

  m_SavedDefaultBehaviorSettings->DeepCopy(m_DefaultBehaviorSettings);
  m_SavedGlobalDisplaySettings->DeepCopy(m_GlobalDisplaySettings);
  m_SavedMeshOptions->DeepCopy(m_MeshOptions);
  for(int i = 0; i < SNAPAppearanceSettings::ELEMENT_COUNT; i++)
    m_SavedElementAppearance[i]->DeepCopy(as->GetUIElement(i));
}

void GlobalPreferencesModel::RevertPreferences()
{
  SNAPAppearanceSettings *as = m_ParentModel->GetAppearanceSettings();

  m_DefaultBehaviorSettings->DeepCopy(m_SavedDefaultBehaviorSettings);
  m_ParentModel->SetGlobalDisplaySettings(m_SavedGlobalDisplaySettings);
  m_MeshOptions->DeepCopy(m_SavedMeshOptions);
  for(int i = 0; i < SNAPAppearanceSettings::ELEMENT_COUNT; i++)
    as->GetUIElement(i)->DeepCopy(m_SavedElementAppearance[i]);
}

void GlobalPreferencesModel::ResetCurrentElement()
{
  assert(m_ActiveUIElement != SNAPAppearanceSettings::ELEMENT_COUNT);

  SNAPAppearanceSettings *as = m_ParentModel->GetAppearanceSettings();
  as->GetUIElement(m_ActiveUIElement)->DeepCopy(
        as->GetUIElementDefaultSettings(m_ActiveUIElement));
}

void GlobalPreferencesModel::ResetAllElements()
{
  SNAPAppearanceSettings *as = m_ParentModel->GetAppearanceSettings();
  for(int i = 0; i < SNAPAppearanceSettings::ELEMENT_COUNT; i++)
    as->GetUIElement(i)->DeepCopy(as->GetUIElementDefaultSettings(i));
}

void GlobalPreferencesModel::ResetTabToDefaults(PreferencesTab tab)
{
  // Settings created with their default values
  SmartPtr<DefaultBehaviorSettings> dbs = DefaultBehaviorSettings::New();
  SmartPtr<GlobalDisplaySettings> gds = GlobalDisplaySettings::New();
  SmartPtr<MeshOptions> mo = MeshOptions::New();

  // Copy the settings shown on the tab
  switch(tab)
    {
    case TAB_DEFAULT_BEHAVIOR:
      m_DefaultBehaviorSettings->DeepCopyProperties(
            dbs, { "UILanguage", "AutoContrast", "HideFileExtensions", "LinkedZoom",
                   "ContinuousMeshUpdate", "Synchronization", "SyncCursor", "SyncZoom",
                   "SyncPan" });
      break;
    case TAB_SLICE_VIEWS_DISPLAY:
      m_GlobalDisplaySettings->DeepCopyProperties(
            gds, { "GreyInterpolationMode", "LayerLayout", "FlagDisplayZoomThumbnail",
                   "ZoomThumbnailSizeInPercent", "ZoomThumbnailMaximumSize" });
      m_DefaultBehaviorSettings->DeepCopyProperties(dbs, { "OverlayColorMapPreset" });
      break;
    case TAB_SLICE_VIEWS_LAYOUT:
      m_GlobalDisplaySettings->DeepCopyProperties(
            gds, { "SliceLayout", "FlagLayoutPatientAnteriorShownLeft",
                   "FlagLayoutPatientRightShownLeft", "FlagRemindLayoutSettings" });
      break;
    case TAB_MESH_SMOOTHING:
      m_MeshOptions->DeepCopyProperties(
            mo, { "UseGaussianSmoothing", "GaussianStandardDeviation", "GaussianError",
                  "UseMeshSmoothing", "MeshSmoothingIterations",
                  "MeshSmoothingRelaxationFactor", "MeshSmoothingFeatureAngle",
                  "MeshSmoothingConvergence", "MeshSmoothingFeatureEdgeSmoothing",
                  "MeshSmoothingBoundarySmoothing" });
      break;
    case TAB_MESH_DECIMATION:
      m_MeshOptions->DeepCopyProperties(
            mo, { "UseDecimation", "DecimateTargetReduction", "DecimateMaximumError",
                  "DecimateFeatureAngle", "DecimatePreserveTopology" });
      break;
    case TAB_TOOLS:
      m_DefaultBehaviorSettings->DeepCopyProperties(
            dbs, { "PaintbrushDefaultInitialSize", "PaintbrushDefaultMaximumSize" });
      break;
    }
}
