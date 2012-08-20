#include "SnakeWizardModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GlobalState.h"
#include "GenericImageData.h"
#include "SNAPImageData.h"
#include "SmoothBinaryThresholdImageFilter.h"
#include "ColorMap.h"

SnakeWizardModel::SnakeWizardModel()
{
  // Set up the child models
  m_ThresholdUpperModel = makeChildPropertyModel(
        this,
        &Self::GetThresholdUpperValueAndRange,
        &Self::SetThresholdUpperValue,
        ThresholdSettingsUpdateEvent(),
        ThresholdSettingsUpdateEvent());

  m_ThresholdLowerModel = makeChildPropertyModel(
        this,
        &Self::GetThresholdLowerValueAndRange,
        &Self::SetThresholdLowerValue,
        ThresholdSettingsUpdateEvent(),
        ThresholdSettingsUpdateEvent());

  m_ThresholdSmoothnessModel = makeChildPropertyModel(
        this,
        &Self::GetThresholdSmoothnessValueAndRange,
        &Self::SetThresholdSmoothnessValue,
        ThresholdSettingsUpdateEvent(),
        ThresholdSettingsUpdateEvent());

  m_ThresholdModeModel = makeChildPropertyModel(
        this,
        &Self::GetThresholdModeValue,
        &Self::SetThresholdModeValue,
        ThresholdSettingsUpdateEvent(),
        ThresholdSettingsUpdateEvent());

  m_ThresholdPreviewModel = makeChildPropertyModel(
        this,
        &Self::GetThresholdPreviewValue,
        &Self::SetThresholdPreviewValue,
        ThresholdSettingsUpdateEvent(),
        ThresholdSettingsUpdateEvent());

  m_SnakeTypeModel = makeChildPropertyModel(
        this,
        &Self::GetSnakeTypeValueAndRange,
        &Self::SetSnakeTypeValue);
}

void SnakeWizardModel::SetParentModel(GlobalUIModel *model)
{
  m_Parent = model;
  m_Driver = m_Parent->GetDriver();
  m_GlobalState = m_Driver->GetGlobalState();

  // Layer changes are rebroadcast as model changes, causing all child
  // models to update themselves.
  Rebroadcast(m_Driver, LayerChangeEvent(), ModelUpdateEvent());

  // Model update events are the "big events", and are rebroadcast
  // as the specialized events as well.
  Rebroadcast(this, ModelUpdateEvent(), ThresholdSettingsUpdateEvent());

  // Changes to the threshold settings are rebroadcast as own own events
  Rebroadcast(m_Driver->GetThresholdSettings(),
              itk::ModifiedEvent(), ThresholdSettingsUpdateEvent());

  // Changes to the snake mode are cast as model update events
  Rebroadcast(m_GlobalState->GetSnakeTypeModel(),
              ValueChangedEvent(), ModelUpdateEvent());

  // We also need to rebroadcast these events as state change events
  Rebroadcast(this, ThresholdSettingsUpdateEvent(), StateMachineChangeEvent());
  Rebroadcast(this, ModelUpdateEvent(), StateMachineChangeEvent());
}

bool SnakeWizardModel
::GetThresholdUpperValueAndRange(
    double &x, NumericValueRange<double> *range)
{
  if(m_Driver->GetCurrentImageData()->IsGreyLoaded())
    {
    ThresholdSettings *ts = m_Driver->GetThresholdSettings();
    x = ts->GetUpperThreshold();
    if(range)
      {
      range->Minimum = m_Driver->GetCurrentImageData()->GetGrey()->GetImageMinNative();
      range->Maximum = m_Driver->GetCurrentImageData()->GetGrey()->GetImageMaxNative();
      range->StepSize = CalculatePowerOfTenStepSize(range->Minimum, range->Maximum, 100);
      }
    return true;
    }
  else return false;
}

bool SnakeWizardModel
::GetThresholdLowerValueAndRange(
    double &x, NumericValueRange<double> *range)
{
  if(m_Driver->GetCurrentImageData()->IsGreyLoaded())
    {
    ThresholdSettings *ts = m_Driver->GetThresholdSettings();
    x = ts->GetLowerThreshold();
    if(range)
      {
      range->Minimum = m_Driver->GetCurrentImageData()->GetGrey()->GetImageMinNative();
      range->Maximum = m_Driver->GetCurrentImageData()->GetGrey()->GetImageMaxNative();
      range->StepSize = CalculatePowerOfTenStepSize(range->Minimum, range->Maximum, 100);
      }
    return true;
    }
  else return false;
}

void SnakeWizardModel
::SetThresholdUpperValue(double x)
{
  // Get the current settings
  ThresholdSettings *ts = m_Driver->GetThresholdSettings();
  if(x < ts->GetLowerThreshold())
    ts->SetLowerThreshold(x);

  ts->SetUpperThreshold(x);
}

void SnakeWizardModel
::SetThresholdLowerValue(double x)
{
  // Get the current settings
  ThresholdSettings *ts = m_Driver->GetThresholdSettings();
  if(x > ts->GetUpperThreshold())
    ts->SetUpperThreshold(x);

  ts->SetLowerThreshold(x);
}

bool SnakeWizardModel::CheckState(SnakeWizardModel::UIState state)
{
  ThresholdSettings *ts = m_Driver->GetThresholdSettings();
  switch(state)
    {
    case UIF_THESHOLDING_ENABLED:
      return m_Driver->GetCurrentImageData()->IsGreyLoaded();
    case UIF_LOWER_THRESHOLD_ENABLED:
      return ts->IsLowerThresholdEnabled();
    case UIF_UPPER_THRESHOLD_ENABLED:
      return ts->IsUpperThresholdEnabled();
    }

  return false;
}


void SnakeWizardModel::OnUpdate()
{
}

bool SnakeWizardModel::GetThresholdSmoothnessValueAndRange(double &x, NumericValueRange<double> *range)
{
  if(m_Driver->GetCurrentImageData()->IsGreyLoaded())
    {
    ThresholdSettings *ts = m_Driver->GetThresholdSettings();
    x = ts->GetSmoothness();
    if(range)
      range->Set(0, 10, 0.1);
    return true;
    }
  else return false;
}

void SnakeWizardModel::SetThresholdSmoothnessValue(double x)
{
  ThresholdSettings *ts = m_Driver->GetThresholdSettings();
  ts->SetSmoothness(x);
}

bool SnakeWizardModel::GetThresholdModeValue(ThresholdSettings::ThresholdMode &x)
{
  if(m_Driver->GetCurrentImageData()->IsGreyLoaded())
    {
    ThresholdSettings *ts = m_Driver->GetThresholdSettings();
    x = ts->GetThresholdMode();
    return true;
    }
  else return false;
}

void SnakeWizardModel::SetThresholdModeValue(ThresholdSettings::ThresholdMode x)
{
  ThresholdSettings *ts = m_Driver->GetThresholdSettings();
  ts->SetThresholdMode(x);
}

void SnakeWizardModel::EvaluateThresholdFunction(double t, double &x, double &y)
{
  assert(m_Driver->IsSnakeModeActive());
  ThresholdSettings *ts = m_Driver->GetThresholdSettings();

  double imin = m_Driver->GetCurrentImageData()->GetGrey()->GetImageMinNative();
  double imax = m_Driver->GetCurrentImageData()->GetGrey()->GetImageMaxNative();

  SmoothBinaryThresholdFunctor<float> functor;
  functor.SetParameters(ts);

  x = t * (imax - imin) + imin;
  y = functor(x) * 1.0 / 0x7fff;
}

void SnakeWizardModel::ApplyThresholdPreprocessing()
{
  // Compute the speed image
  m_Driver->ComputeSNAPSpeedImage();

  // Set the color map for the speed image
  ColorMap *cm = m_Driver->GetSNAPImageData()->GetSpeed()->GetColorMap();
  cm->SetToSystemPreset(ColorMap::COLORMAP_SPEED);

  // The speed image should be shown
  m_GlobalState->SetShowSpeed(true);
}

bool SnakeWizardModel::GetThresholdPreviewValue(bool &value)
{
  if(m_Driver->IsSnakeModeActive())
    {
    value = m_Driver->GetSNAPImageData()->GetThresholdPreviewMode();
    return true;
    }
  else return false;
}

void SnakeWizardModel::SetThresholdPreviewValue(bool value)
{
  assert(m_Driver->IsSnakeModeActive());
  m_Driver->GetSNAPImageData()->SetThresholdPreviewMode(value);
  m_GlobalState->SetShowSpeed(value);
}

bool SnakeWizardModel::GetSnakeTypeValueAndRange(
    SnakeType &value, GlobalState::SnakeTypeDomain *range)
{
  return m_GlobalState->GetSnakeTypeModel()->GetValueAndDomain(value, range);
}

void SnakeWizardModel::SetSnakeTypeValue(SnakeType value)
{
  m_Driver->SetSnakeMode(value);
}



