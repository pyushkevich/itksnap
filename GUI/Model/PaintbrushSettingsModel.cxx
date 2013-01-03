#include "PaintbrushSettingsModel.h"
#include "GlobalUIModel.h"
#include "GlobalState.h"

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
    domain->Set(1, 40, 1);
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





