#include "PaintbrushSettingsModel.h"
#include "GlobalUIModel.h"
#include "GlobalState.h"
#include "GlobalPreferencesModel.h"
#include "DefaultBehaviorSettings.h"


PaintbrushSettingsModel::PaintbrushSettingsModel()
{
  m_PaintbrushSettingsModel =
      wrapGetterSetterPairAsProperty(this,
                                     &Self::GetPaintbrushSettings,
                                     &Self::SetPaintbrushSettings);

  // Create models for the fields of PaintbrushSettings
  m_PaintbrushModeModel =
      wrapStructMemberAsSimpleProperty<PaintbrushSettings, PaintbrushMode>(
        m_PaintbrushSettingsModel, offsetof(PaintbrushSettings, mode));

  m_VolumetricBrushModel =
      wrapStructMemberAsSimpleProperty<PaintbrushSettings, bool>(
        m_PaintbrushSettingsModel, offsetof(PaintbrushSettings, volumetric));

  m_IsotropicBrushModel =
      wrapStructMemberAsSimpleProperty<PaintbrushSettings, bool>(
        m_PaintbrushSettingsModel, offsetof(PaintbrushSettings, isotropic));

  m_ChaseCursorModel =
      wrapStructMemberAsSimpleProperty<PaintbrushSettings, bool>(
        m_PaintbrushSettingsModel, offsetof(PaintbrushSettings, chase));

  // The paintbrush size model requires special processing, so it is implemeted
  // using a getter/setter pair
  m_BrushSizeModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetBrushSizeValueAndRange,
        &Self::SetBrushSizeValue);

  m_AdaptiveModeModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetAdaptiveModeValue);

  m_ThresholdLevelModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetThresholdLevelValueAndRange,
        &Self::SetThresholdLevelValue);

  m_SmoothingIterationsModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetSmoothingIterationValueAndRange,
        &Self::SetSmoothingIterationValue);
}

void PaintbrushSettingsModel::SetParentModel(GlobalUIModel *parent)
{
  m_ParentModel = parent;

  // We need to watch for changes to the GlobalState defaults on paintbrush size
  DefaultBehaviorSettings *dbs =
      m_ParentModel->GetGlobalState()->GetDefaultBehaviorSettings();

  // For the initial size, we actually need some custom code - to set the brush value to default
  Rebroadcast(dbs->GetPaintbrushDefaultInitialSizeModel(), ValueChangedEvent(), ModelUpdateEvent());

  // For the maximum size, we just need the size model to be updated, no custom code
  m_BrushSizeModel->RebroadcastFromSourceProperty(dbs->GetPaintbrushDefaultMaximumSizeModel());
}

void PaintbrushSettingsModel::OnUpdate()
{
  DefaultBehaviorSettings *dbs =
      m_ParentModel->GetGlobalState()->GetDefaultBehaviorSettings();

  if(m_EventBucket->HasEvent(ValueChangedEvent(), dbs->GetPaintbrushDefaultInitialSizeModel()))
    {
    this->SetBrushSizeValue(dbs->GetPaintbrushDefaultInitialSizeModel()->GetValue());
    }
}


PaintbrushSettingsModel::~PaintbrushSettingsModel()
{
}

PaintbrushSettings PaintbrushSettingsModel::GetPaintbrushSettings()
{
  return m_ParentModel->GetGlobalState()->GetPaintbrushSettings();
}

void PaintbrushSettingsModel::SetPaintbrushSettings(PaintbrushSettings ps)
{
  m_ParentModel->GetGlobalState()->SetPaintbrushSettings(ps);
  InvokeEvent(ModelUpdateEvent());
}


bool PaintbrushSettingsModel
::GetBrushSizeValueAndRange(int &value, NumericValueRange<int> *domain)
{
  PaintbrushSettings pbs = GetPaintbrushSettings();

  // Round just in case
  value = (int) (pbs.radius * 2 + 0.5);
  if(domain)
    {
    int max_size = m_ParentModel->GetGlobalPreferencesModel()
                   ->GetDefaultBehaviorSettings()->GetPaintbrushDefaultMaximumSize();

    domain->Set(1, max_size, 1);
    }
  return true;
}

void PaintbrushSettingsModel::SetBrushSizeValue(int value)
{
  PaintbrushSettings pbs = GetPaintbrushSettings();

  pbs.radius = 0.5 * value;
  SetPaintbrushSettings(pbs);
}

bool PaintbrushSettingsModel::GetAdaptiveModeValue(bool &value)
{
  PaintbrushSettings pbs = GetPaintbrushSettings();

  value = (pbs.mode == PAINTBRUSH_WATERSHED);
  return true;
}

bool PaintbrushSettingsModel::GetThresholdLevelValueAndRange(double &value, NumericValueRange<double> *domain)
{
  PaintbrushSettings pbs = GetPaintbrushSettings();

  value = pbs.watershed.level * 100.0;
  if(domain)
    {
    domain->Set(0, 100, 1);
    }
  return true;
}

void PaintbrushSettingsModel::SetThresholdLevelValue(double value)
{
  PaintbrushSettings pbs = GetPaintbrushSettings();
  pbs.watershed.level = 0.01 * value;
  SetPaintbrushSettings(pbs);
}

bool PaintbrushSettingsModel::GetSmoothingIterationValueAndRange(
    int &value, NumericValueRange<int> *domain)
{
  PaintbrushSettings pbs = GetPaintbrushSettings();

  value = pbs.watershed.smooth_iterations;
  if(domain)
    {
    domain->Set(0, 100, 1);
    }
  return true;

}

void PaintbrushSettingsModel::SetSmoothingIterationValue(int value)
{
  PaintbrushSettings pbs = GetPaintbrushSettings();
  pbs.watershed.smooth_iterations = value;
  SetPaintbrushSettings(pbs);
}





