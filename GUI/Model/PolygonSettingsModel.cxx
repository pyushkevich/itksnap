#include "PolygonSettingsModel.h"
#include "GlobalUIModel.h"
#include "PolygonDrawingModel.h"
#include "NumericPropertyToggleAdaptor.h"

void PolygonSettingsModel::SetParentModel(GlobalUIModel *model)
{
  m_ParentModel = model;

  m_FreehandSegmentLengthModel->RebroadcastFromSourceProperty(
        m_ParentModel->GetPolygonDrawingModel(0)->GetFreehandFittingRateModel());

  m_FreehandIsPiecewiseModel->RebroadcastFromSourceProperty(
        m_ParentModel->GetPolygonDrawingModel(0)->GetFreehandFittingRateModel());

}

PolygonSettingsModel::PolygonSettingsModel()
{
  m_ParentModel = NULL;

  m_FreehandSegmentLengthModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetFreehandSegmentLengthValueAndRange,
        &Self::SetFreehandSegmentLengthValue);

  m_FreehandIsPiecewiseModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetFreehandIsPiecewiseValue,
        &Self::SetFreehandIsPiecewiseValue);
}

bool PolygonSettingsModel::GetFreehandIsPiecewiseValue(bool &value)
{
  if(!m_ParentModel)
    return false;

  int rate = (int) m_ParentModel->GetPolygonDrawingModel(0)->GetFreehandFittingRate();
  value = (rate != 0);

  return true;
}

void PolygonSettingsModel::SetFreehandIsPiecewiseValue(bool value)
{
  int rate = (int) m_ParentModel->GetPolygonDrawingModel(0)->GetFreehandFittingRate();
  if(value)
    {
    for(int i = 0; i < 3; i++)
      m_ParentModel->GetPolygonDrawingModel(i)->SetFreehandFittingRate(m_LastFreehandRate);
    }
  else
    {
    m_LastFreehandRate = rate;
    for(int i = 0; i < 3; i++)
      m_ParentModel->GetPolygonDrawingModel(i)->SetFreehandFittingRate(0.0);
    }
}

bool PolygonSettingsModel
::GetFreehandSegmentLengthValueAndRange(int &value, NumericValueRange<int> *range)
{
  if(!m_ParentModel)
    return false;

  value = (int) m_ParentModel->GetPolygonDrawingModel(0)->GetFreehandFittingRate();
  if(value == 0)
    value = m_LastFreehandRate;

  if(range)
    range->Set(1, 20, 1);

  return true;
}

void PolygonSettingsModel::SetFreehandSegmentLengthValue(int value)
{
  for(int i = 0; i < 3; i++)
    m_ParentModel->GetPolygonDrawingModel(i)->SetFreehandFittingRate(value);
}


