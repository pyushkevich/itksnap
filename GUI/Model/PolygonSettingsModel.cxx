#include "PolygonSettingsModel.h"
#include "GlobalUIModel.h"
#include "PolygonDrawingModel.h"
#include "NumericPropertyToggleAdaptor.h"
#include "DeepLearningSegmentationModel.h"
#include "SNAPEventListenerCallbacks.h"
#include "Registry.h"

void PolygonSettingsModel::SetParentModel(GlobalUIModel *model)
{
  m_ParentModel = model;

  m_FreehandSegmentLengthModel->RebroadcastFromSourceProperty(
        m_ParentModel->GetPolygonDrawingModel(0)->GetFreehandFittingRateModel());

  m_FreehandIsPiecewiseModel->RebroadcastFromSourceProperty(
        m_ParentModel->GetPolygonDrawingModel(0)->GetFreehandFittingRateModel());

  m_DeepLearningModeModel = NewSimpleConcreteProperty(false);

  // Turning on deep learning mode activates the connection to the server, as
  // selecting the deep learning paintbrush does
  AddListener(m_DeepLearningModeModel, ValueChangedEvent(), this, &Self::OnDeepLearningModeChange);

  // The list of pipelines changes when the deep learning server connects or
  // disconnects
  auto *status = m_ParentModel->GetDeepLearningSegmentationModel()->GetServerStatusModel();
  m_DeepLearningPipelineModel->Rebroadcast(status, ValueChangedEvent(), ValueChangedEvent());
  m_DeepLearningPipelineModel->Rebroadcast(status, ValueChangedEvent(), DomainChangedEvent());
}

void PolygonSettingsModel::OnDeepLearningModeChange()
{
  if(m_DeepLearningModeModel->GetValue())
    m_ParentModel->GetDeepLearningSegmentationModel()->SetIsActive(true);
}

std::string PolygonSettingsModel::GetEffectiveDeepLearningPipeline()
{
  auto *dlm = m_ParentModel->GetDeepLearningSegmentationModel();
  if(!dlm->GetIsActive())
    return std::string();

  auto pipelines = dlm->GetRemotePipelines();
  auto it_sel = pipelines.find(m_DeepLearningPipelineId);
  if(it_sel != pipelines.end() && it_sel->second.supports_lasso)
    return m_DeepLearningPipelineId;

  // Default to the first model listed by the server that supports lasso
  for(auto &id : dlm->GetRemotePipelineIdsInServerOrder())
    if(pipelines[id].supports_lasso)
      return id;

  return std::string();
}

bool PolygonSettingsModel::GetDeepLearningPipelineValueAndRange(
    std::string &value, DeepLearningPipelineDomain *range)
{
  value = this->GetEffectiveDeepLearningPipeline();
  if(value.empty())
    return false;

  if(range)
    {
    auto *dlm = m_ParentModel->GetDeepLearningSegmentationModel();
    auto pipelines = dlm->GetRemotePipelines();
    range->clear();
    for(auto &id : dlm->GetRemotePipelineIdsInServerOrder())
      if(pipelines[id].supports_lasso)
        range->push_back(id, id);
    }

  return true;
}

void PolygonSettingsModel::SetDeepLearningPipelineValue(std::string value)
{
  m_DeepLearningPipelineId = value;
}

void PolygonSettingsModel::LoadFromRegistry(Registry &folder)
{
  this->SetFreehandSegmentLength(folder["SegmentLength"][8]);
  this->SetFreehandIsPiecewise(folder["IsPiecewise"][true]);

}

void PolygonSettingsModel::SaveToRegistry(Registry &folder)
{
  folder["IsPiecewise"] << this->GetFreehandIsPiecewise();
  folder["SegmentLength"] << this->GetFreehandSegmentLength();
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

  m_DeepLearningPipelineModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetDeepLearningPipelineValueAndRange,
        &Self::SetDeepLearningPipelineValue);
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
    if(m_LastFreehandRate == 0)
      m_LastFreehandRate = PolygonDrawingModel::DefaultFreehandFittingRate;

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


