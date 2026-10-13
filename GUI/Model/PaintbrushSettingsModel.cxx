#include "PaintbrushSettingsModel.h"
#include "GlobalUIModel.h"
#include "GlobalState.h"
#include "GlobalPreferencesModel.h"
#include "DefaultBehaviorSettings.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "DeepLearningSegmentationModel.h"

PaintbrushSettingsModel::PaintbrushSettingsModel()
{
  m_PaintbrushSettingsModel =
      wrapGetterSetterPairAsProperty(this,
                                     &Self::GetPaintbrushSettings,
                                     &Self::SetPaintbrushSettings);

  // Create models for the fields of PaintbrushSettings
  m_PaintbrushShapeModel =
      wrapStructMemberAsSimpleProperty<PaintbrushSettings, PaintbrushShape>(
        m_PaintbrushSettingsModel, offsetof(PaintbrushSettings, shape));

  m_PaintbrushSmartModeModel =
    wrapGetterSetterPairAsProperty(this,
                                   &Self::GetPaintbrushSmartModeValue,
                                   &Self::SetPaintbrushSmartModeValue);

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

  m_DeepLearningModeModel = wrapGetterSetterPairAsProperty(
    this, &Self::GetDeepLearningModeValue);

  m_DeepLearningPipelineModel = wrapGetterSetterPairAsProperty(
    this, &Self::GetDeepLearningPipelineValueAndRange, &Self::SetDeepLearningPipelineValue);

  m_ThresholdLevelModel = wrapGetterSetterPairAsProperty(
    this, &Self::GetThresholdLevelValueAndRange, &Self::SetThresholdLevelValue);

  m_SmoothingIterationsModel = wrapGetterSetterPairAsProperty(
    this, &Self::GetSmoothingIterationValueAndRange, &Self::SetSmoothingIterationValue);
}

void PaintbrushSettingsModel::SetParentModel(GlobalUIModel *parent)
{
  m_ParentModel = parent;

  // We need to watch for changes to the GlobalState defaults on paintbrush size
  DefaultBehaviorSettings *dbs =
      m_ParentModel->GetGlobalState()->GetDefaultBehaviorSettings();

  // For the initial size, we actually need some custom code - to set the brush value to default
  Rebroadcast(dbs->GetPaintbrushDefaultInitialSizeModel(), ValueChangedEvent(), ModelUpdateEvent());

  Rebroadcast(m_ParentModel->GetDriver(), WrapperDisplayMappingChangeEvent(), StateMachineChangeEvent());

  // Whether the adaptive brush is available depends on the reference space
  Rebroadcast(m_ParentModel->GetDriver(), ReferenceSpaceGeometryChangeEvent(), StateMachineChangeEvent());

  // For the maximum size, we just need the size model to be updated, no custom code
  m_BrushSizeModel->RebroadcastFromSourceProperty(dbs->GetPaintbrushDefaultMaximumSizeModel());

  // The brush size model also depends on the deep learning mode
  m_BrushSizeModel->Rebroadcast(m_DeepLearningModeModel, ValueChangedEvent(), ValueChangedEvent());
  m_BrushSizeModel->Rebroadcast(m_DeepLearningModeModel, ValueChangedEvent(), DomainChangedEvent());

  // The deep learning pipeline model needs to be refreshed when the deep learning extension model
  // connects to a new server, etc.
  m_DeepLearningPipelineModel->Rebroadcast(
    m_ParentModel->GetDeepLearningSegmentationModel()->GetServerStatusModel(),
    ValueChangedEvent(),
    ValueChangedEvent());
  m_DeepLearningPipelineModel->Rebroadcast(
    m_ParentModel->GetDeepLearningSegmentationModel()->GetServerStatusModel(),
    ValueChangedEvent(),
    DomainChangedEvent());
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

bool
PaintbrushSettingsModel
::CheckState(PaintbrushSettingsModel::UIState state)
{
  if (!m_ParentModel->GetDriver()->IsMainImageLoaded())
    return false;

  GenericImageData *gid = m_ParentModel->GetDriver()->GetCurrentImageData();
  bool free_rotation = gid->IsFreeRotation();

  switch(state)
    {
    case UIF_VOLUMETRIC_OK:
      return !free_rotation;
    case UIF_ADAPTIVE_OK:
      // The adaptive brush works on the voxels of the image, so it needs the
      // image to be on the same grid as the active segmentation
      return !free_rotation && gid->GetMain()->ImageSpaceMatchesReferenceSpace();
    case UIF_DEEPLEARNING_OK:
      return !free_rotation;
    default:
      return false;
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

bool
PaintbrushSettingsModel::GetPaintbrushSmartModeValue(PaintbrushSmartMode &value)
{
  value = GetPaintbrushSettings().smart_mode;
  return true;
}

void
PaintbrushSettingsModel::SetPaintbrushSmartModeValue(PaintbrushSmartMode value)
{
  auto pbs = GetPaintbrushSettings();
  pbs.smart_mode = value;
  SetPaintbrushSettings(pbs);

  // We need to active the smart paintbrush model
  if(value == PAINTBRUSH_DLS)
    m_ParentModel->GetDeepLearningSegmentationModel()->SetIsActive(true);
}


bool
PaintbrushSettingsModel ::GetBrushSizeValueAndRange(int &value, NumericValueRange<int> *domain)
{
  PaintbrushSettings pbs = GetPaintbrushSettings();

  // In AI mode, the paintbrush should be one pixel thick
  if (pbs.smart_mode == PAINTBRUSH_DLS)
  {
    value = 1;
    if (domain)
      domain->Set(1, 1, 1);
  }
  else
  {
    // Round just in case
    value = (int)(pbs.radius * 2 + 0.5);
    if (domain)
    {
      int max_size = m_ParentModel->GetGlobalPreferencesModel()
                       ->GetDefaultBehaviorSettings()
                       ->GetPaintbrushDefaultMaximumSize();

      domain->Set(1, max_size, 1);
    }
  }
  return true;
}

void PaintbrushSettingsModel::SetBrushSizeValue(int value)
{
  PaintbrushSettings pbs = GetPaintbrushSettings();

  if (pbs.smart_mode != PAINTBRUSH_DLS)
  {
    pbs.radius = 0.5 * value;
    SetPaintbrushSettings(pbs);
  }
}

bool PaintbrushSettingsModel::GetAdaptiveModeValue(bool &value)
{
  PaintbrushSettings pbs = GetPaintbrushSettings();
  value = (pbs.smart_mode == PAINTBRUSH_WATERSHED);
  return true;
}

bool
PaintbrushSettingsModel::GetDeepLearningModeValue(bool &value)
{
  PaintbrushSettings pbs = GetPaintbrushSettings();
  value = (pbs.smart_mode == PAINTBRUSH_DLS);
  return true;
}

// Get the deep learning models usable with the paintbrush, in the order listed
// by the server (the first one is the default). Uses the list cached by the
// deep learning model, so there is no network access here.
static PaintbrushSettingsModel::DeepLearningPipelineDomain
GetUsablePaintbrushPipelines(DeepLearningSegmentationModel *dlm)
{
  PaintbrushSettingsModel::DeepLearningPipelineDomain usable_pipelines;
  if(!dlm->GetIsActive())
    return usable_pipelines;

  auto pipelines = dlm->GetRemotePipelines();
  for(auto &id : dlm->GetRemotePipelineIdsInServerOrder())
    if(pipelines[id].supports_point || pipelines[id].supports_scribble)
      usable_pipelines.push_back(id, id);

  return usable_pipelines;
}

std::string
PaintbrushSettingsModel::GetEffectiveDeepLearningPipeline()
{
  auto usable_pipelines =
    GetUsablePaintbrushPipelines(m_ParentModel->GetDeepLearningSegmentationModel());
  if(usable_pipelines.size() == 0)
    return std::string();

  // The selected pipeline is kept even when the server does not offer it, so
  // that it is used again when connecting to a server that does
  std::string selected = GetPaintbrushSettings().dl_pipeline_id;
  if(usable_pipelines.find(selected) != usable_pipelines.end())
    return selected;

  return usable_pipelines.begin()->first;
}

bool
PaintbrushSettingsModel::GetDeepLearningPipelineValueAndRange(std::string                &value,
                                                              DeepLearningPipelineDomain *range)
{
  auto usable_pipelines =
    GetUsablePaintbrushPipelines(m_ParentModel->GetDeepLearningSegmentationModel());
  if(usable_pipelines.size() == 0)
    return false;

  value = GetEffectiveDeepLearningPipeline();
  if(range)
    *range = usable_pipelines;

  return true;
}

void
PaintbrushSettingsModel::SetDeepLearningPipelineValue(std::string value)
{
  PaintbrushSettings pbs = GetPaintbrushSettings();
  pbs.dl_pipeline_id = value;
  SetPaintbrushSettings(pbs);
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





