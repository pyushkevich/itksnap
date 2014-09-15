#include "SnakeWizardModel.h"
#include "GlobalUIModel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "GlobalState.h"
#include "GenericImageData.h"
#include "SNAPImageData.h"
#include "SmoothBinaryThresholdImageFilter.h"
#include "EdgePreprocessingImageFilter.h"
#include "ColorMap.h"
#include "SlicePreviewFilterWrapper.h"
#include "UnsupervisedClustering.h"
#include "PreprocessingFilterConfigTraits.h"
#include "GMMClassifyImageFilter.h"
#include "RFClassificationEngine.h"
#include "RandomForestClassifier.h"
#include "RandomForestClassifyImageFilter.h"

SnakeWizardModel::SnakeWizardModel()
{
  // Set up the child models
  m_ThresholdUpperModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetThresholdUpperValueAndRange,
        &Self::SetThresholdUpperValue,
        ThresholdSettingsUpdateEvent(),
        ThresholdSettingsUpdateEvent());

  m_ThresholdLowerModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetThresholdLowerValueAndRange,
        &Self::SetThresholdLowerValue,
        ThresholdSettingsUpdateEvent(),
        ThresholdSettingsUpdateEvent());

  m_ThresholdSmoothnessModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetThresholdSmoothnessValueAndRange,
        &Self::SetThresholdSmoothnessValue,
        ThresholdSettingsUpdateEvent(),
        ThresholdSettingsUpdateEvent());

  m_ThresholdModeModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetThresholdModeValue,
        &Self::SetThresholdModeValue,
        ThresholdSettingsUpdateEvent(),
        ThresholdSettingsUpdateEvent());

  m_ThresholdActiveLayerModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetThresholdActiveLayerValueAndRange,
        &Self::SetThresholdActiveLayerValue,
        ThresholdSettingsUpdateEvent(),
        ThresholdSettingsUpdateEvent());

  m_ThresholdActiveScalarRepModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetThresholdActiveScalarRepValueAndRange,
        &Self::SetThresholdActiveScalarRepValue,
        ThresholdSettingsUpdateEvent(),
        ThresholdSettingsUpdateEvent());

  m_PreviewModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetPreviewValue,
        &Self::SetPreviewValue);

  m_BlueWhiteSpeedModeModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetBlueWhiteSpeedModeValue,
        &Self::SetBlueWhiteSpeedModeValue);

  m_RedTransparentSpeedModeModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetRedTransparentSpeedModeValue,
        &Self::SetRedTransparentSpeedModeValue);

  // TODO: which events from the parent model should be rebroadcast by the
  // preview model?

  // EdgePreprocessingSettingsUpdateEvent(),
  // EdgePreprocessingSettingsUpdateEvent()

  m_EdgePreprocessingSigmaModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetEdgePreprocessingSigmaValueAndRange,
        &Self::SetEdgePreprocessingSigmaValue,
        EdgePreprocessingSettingsUpdateEvent(),
        EdgePreprocessingSettingsUpdateEvent());

  m_EdgePreprocessingKappaModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetEdgePreprocessingKappaValueAndRange,
        &Self::SetEdgePreprocessingKappaValue,
        EdgePreprocessingSettingsUpdateEvent(),
        EdgePreprocessingSettingsUpdateEvent());

  m_EdgePreprocessingExponentModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetEdgePreprocessingExponentValueAndRange,
        &Self::SetEdgePreprocessingExponentValue,
        EdgePreprocessingSettingsUpdateEvent(),
        EdgePreprocessingSettingsUpdateEvent());

  m_SnakeTypeModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetSnakeTypeValueAndRange,
        &Self::SetSnakeTypeValue);

  // Preprocessing mode model initialization
  m_PreprocessingModeModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetPreprocessingModeValueAndRange,
        &Self::SetPreprocessingModeValue);

  m_ActiveBubbleModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetActiveBubbleValue,
        &Self::SetActiveBubbleValue,
        ActiveBubbleUpdateEvent());

  m_BubbleRadiusModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetBubbleRadiusValueAndRange,
        &Self::SetBubbleRadiusValue,
        BubbleDefaultRadiusUpdateEvent(),
        BubbleDefaultRadiusUpdateEvent());

  m_StepSizeModel = NewRangedConcreteProperty(1, 1, 100, 1);

  // Need to define a null setter function
  void (Self::*nullsetter)(int) = NULL;

  m_EvolutionIterationModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetEvolutionIterationValue,
        nullsetter,
        EvolutionIterationEvent());

  m_NumberOfClustersModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetNumberOfClustersValueAndRange,
        &Self::SetNumberOfClustersValue,
        GMMModifiedEvent(),
        GMMModifiedEvent());

  m_NumberOfGMMSamplesModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetNumberOfGMMSamplesValueAndRange,
        &Self::SetNumberOfGMMSamplesValue,
        GMMModifiedEvent(),
        GMMModifiedEvent());

  m_ForegroundClusterModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetForegroundClusterValueAndRange,
        &Self::SetForegroundClusterValue,
        GMMModifiedEvent(),
        GMMModifiedEvent());

  m_ForegroundClassColorLabelModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetForegroundClassColorLabelValueAndRange,
        &Self::SetForegroundClassColorLabelValue,
        RFClassifierModifiedEvent(),
        RFClassifierModifiedEvent());

  m_ForestSizeModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetForestSizeValueAndRange,
        &Self::SetForestSizeValue,
        RFClassifierModifiedEvent(),
        RFClassifierModifiedEvent());

  // The domain of the foreground cluster model depends on the number of clusters
  m_ForegroundClusterModel->Rebroadcast(
        m_NumberOfClustersModel, ValueChangedEvent(), DomainChangedEvent());

  m_ClusterPlottedComponentModel = ComponentIndexModel::New();
  this->UpdateClusterPlottedComponentModel();

  m_InteractionMode = MODE_NONE;
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
  Rebroadcast(m_Driver,
              WrapperProcessingSettingsChangeEvent(), ThresholdSettingsUpdateEvent());

  // Changes to the preview pipeline (preview status) are broadcast as events
  Rebroadcast(m_Driver->GetPreprocessingFilterPreviewer(PREPROCESS_THRESHOLD),
              itk::ModifiedEvent(), ThresholdSettingsUpdateEvent());

  // Repeat the same code for the edge preprocessing
  Rebroadcast(this, ModelUpdateEvent(), EdgePreprocessingSettingsUpdateEvent());

  Rebroadcast(m_Driver->GetEdgePreprocessingSettings(),
              itk::ModifiedEvent(), EdgePreprocessingSettingsUpdateEvent());

  Rebroadcast(m_Driver->GetPreprocessingFilterPreviewer(PREPROCESS_EDGE),
              itk::ModifiedEvent(), EdgePreprocessingSettingsUpdateEvent());

  Rebroadcast(m_Driver->GetPreprocessingFilterPreviewer(PREPROCESS_GMM),
              itk::ModifiedEvent(), GMMModifiedEvent());

  Rebroadcast(m_Driver->GetPreprocessingFilterPreviewer(PREPROCESS_RF),
              itk::ModifiedEvent(), RFClassifierModifiedEvent());

  // Changes to the snake mode are cast as model update events
  Rebroadcast(m_GlobalState->GetSnakeTypeModel(),
              ValueChangedEvent(), ModelUpdateEvent());

  // We also need to rebroadcast these events as state change events
  Rebroadcast(this, ThresholdSettingsUpdateEvent(), StateMachineChangeEvent());
  Rebroadcast(this, EdgePreprocessingSettingsUpdateEvent(), StateMachineChangeEvent());
  Rebroadcast(this, ModelUpdateEvent(), StateMachineChangeEvent());
  Rebroadcast(this, ActiveBubbleUpdateEvent(), StateMachineChangeEvent());
  Rebroadcast(this, RFClassifierModifiedEvent(), StateMachineChangeEvent());

  // The two appearance mode models depend on changes to the color map and
  // the metadata of the speed image wrapper
  m_BlueWhiteSpeedModeModel->Rebroadcast(
        m_Driver, WrapperChangeEvent(), ValueChangedEvent());

  m_RedTransparentSpeedModeModel->Rebroadcast(
        m_Driver, WrapperChangeEvent(), ValueChangedEvent());
}


bool SnakeWizardModel::CheckState(SnakeWizardModel::UIState state)
{
  ThresholdSettings *ts = this->GetThresholdSettings();
  switch(state)
    {
    case UIF_BUBBLE_MODE:
      return m_InteractionMode == MODE_BUBBLES;
    case UIF_THESHOLDING_ENABLED:
      return AreThresholdModelsActive();
    case UIF_LOWER_THRESHOLD_ENABLED:
      return ts && ts->IsLowerThresholdEnabled();
    case UIF_UPPER_THRESHOLD_ENABLED:
      return ts && ts->IsUpperThresholdEnabled();
    case UIF_EDGEPROCESSING_ENABLED:
      return AreEdgePreprocessingModelsActive();
    case UIF_CAN_GENERATE_SPEED:
      return CanGenerateSpeedVolume();
    case UIF_SPEED_AVAILABLE:
      return m_GlobalState->GetSpeedValid();
    case UIF_PREPROCESSING_ACTIVE:
      return m_Driver->GetPreprocessingMode() != PREPROCESS_NONE;
    case UIF_BUBBLE_SELECTED:
      return m_GlobalState->GetActiveBubble() >= 0;
    case UIF_INITIALIZATION_VALID:
      return m_GlobalState->GetSnakeInitializedWithManualSegmentation()
          || m_Driver->GetBubbleArray().size() > 0;
    }

  return false;
}


void SnakeWizardModel::OnUpdate()
{
  // If there is a change in available layers, we must rebuild the list
  // of available components.
  if(m_EventBucket->HasEvent(LayerChangeEvent()))
    {
    m_ComponentInfo.clear();
    LayerIterator it = m_Driver->GetSNAPImageData()->GetLayers(
          MAIN_ROLE | OVERLAY_ROLE);
    for(; !it.IsAtEnd(); ++it)
      {
      if(VectorImageWrapperBase *viw = it.GetLayerAsVector())
        {
        for(int comp = 0; comp < it.GetLayerAsVector()->GetNumberOfComponents(); ++comp)
          {
          ComponentInfo ci;
          ci.ImageWrapper = viw;
          ci.ComponentWrapper = viw->GetScalarRepresentation(
                SCALAR_REP_COMPONENT, comp);
          ci.ComponentIndex = comp;
          m_ComponentInfo.push_back(ci);
          }
        }
      else
        {
        ComponentInfo ci;
        ci.ImageWrapper = ci.ComponentWrapper = it.GetLayerAsScalar();
        ci.ComponentIndex = 0;
        m_ComponentInfo.push_back(ci);
        }
      }

    this->UpdateClusterPlottedComponentModel();
    }
}

ScalarImageWrapperBase *SnakeWizardModel::GetActiveScalarLayer(PreprocessingMode mode)
{
  return m_Driver->GetPreprocessingFilterPreviewer(mode)->GetActiveScalarLayer();
}

void SnakeWizardModel::OnBubbleModeBack()
{
  SetInteractionMode(MODE_PREPROCESSING);

  // Set the current preprocessing mode.
  PreprocessingMode lastMode = m_GlobalState->GetLastUsedPreprocessingMode();
  m_PreprocessingModeModel->SetValue(lastMode);
}

void SnakeWizardModel::UpdateClusterPlottedComponentModel()
{
  this->m_ClusterPlottedComponentModel->SetValue(0);
  ComponentDomain cd;
  for(int i = 0; i < m_ComponentInfo.size(); i++)
    {
    std::ostringstream oss;
    oss << (i+1) << " : " << m_ComponentInfo[i].ImageWrapper->GetNickname();
    cd[i] = oss.str();
    }
  this->m_ClusterPlottedComponentModel->SetDomain(cd);
}

bool SnakeWizardModel::GetForegroundClassColorLabelValueAndRange(
    LabelType &value, SnakeWizardModel::ForegroundClassDomain *range)
{
  // Must have a classification engine
  RFClassificationEngine *rfe = m_Driver->GetClassificationEngine();
  if(!rfe)
    return false;

  // Must have a classifier
  RandomForestClassifier *rfc = rfe->GetClassifier();
  if(!rfc)
    return false;

  // The classifier must be valid (2 or more classes)
  if(!rfc->IsValidClassifier())
    return false;

  // Set the class label to the one stored in the classifier
  value = rfc->GetForegroundClassLabel();

  // Set the range to the correct range
  if(range)
    range->SetWrappedMap(&m_ActiveClasses);

  return true;
}

void SnakeWizardModel::SetForegroundClassColorLabelValue(LabelType value)
{
  RFClassificationEngine *rfe = m_Driver->GetClassificationEngine();
  assert(rfe);

  RandomForestClassifier *rfc = rfe->GetClassifier();
  assert(rfc);

  rfc->SetForegroundClassLabel(value);

  InvokeEvent(RFClassifierModifiedEvent());

  // TODO: this is a hack!
  TagRFPreprocessingFilterModified();

}

bool SnakeWizardModel::GetForestSizeValueAndRange(int &value, NumericValueRange<int> *range)
{
  // Must have a classification engine
  RFClassificationEngine *rfe = m_Driver->GetClassificationEngine();
  if(!rfe)
    return false;

  value = rfe->GetForestSize();
  if(range)
    range->Set(10, 500, 10);

  return true;
}

void SnakeWizardModel::SetForestSizeValue(int value)
{
  RFClassificationEngine *rfe = m_Driver->GetClassificationEngine();
  assert(rfe);

  rfe->SetForestSize(value);
}

void SnakeWizardModel::OnBubbleModeEnter()
{
  // When entering bubble mode, we should not use the overlay display of the
  // speed image, as tht clashes with bubble placement visually
  if(this->GetRedTransparentSpeedModeModel()->GetValue())
    this->SetBlueWhiteSpeedModeValue(true);

  // In bubble mode, we want the main toolbar to be in crosshairs mode
  m_GlobalState->SetToolbarMode(CROSSHAIRS_MODE);

  // We are in bubble mode
  SetInteractionMode(MODE_BUBBLES);
}


bool SnakeWizardModel::AreThresholdModelsActive()
{
  return (m_Driver->IsSnakeModeActive() &&
          m_Driver->GetPreprocessingMode() == PREPROCESS_THRESHOLD);
}

bool SnakeWizardModel::AreEdgePreprocessingModelsActive()
{
  return (m_Driver->IsSnakeModeActive() &&
          m_Driver->GetPreprocessingMode() == PREPROCESS_EDGE);
}

bool SnakeWizardModel::CanGenerateSpeedVolume()
{
  // The answer depends on the proprocessing mode
  switch(m_Driver->GetPreprocessingMode())
    {
    case PREPROCESS_NONE:
      return false;
    case PREPROCESS_THRESHOLD:
      return true;
    case PREPROCESS_EDGE:
      return true;
    case PREPROCESS_GMM:
      return true;
    case PREPROCESS_RF:
      {
      RFClassificationEngine *cfe = m_Driver->GetClassificationEngine();
      RandomForestClassifier *rfc = cfe->GetClassifier();
      return rfc->IsValidClassifier();
      }
    }
}

ScalarImageWrapperBase *SnakeWizardModel::GetSelectedScalarLayer()
{
  // TODO: this should be set by the wizard through user interaction.
  // This is just a placeholder
  return m_Driver->GetCurrentImageData()->GetMain()->GetDefaultScalarRepresentation();
}

bool SnakeWizardModel
::GetThresholdUpperValueAndRange(
    double &x, NumericValueRange<double> *range)
{
  if(!AreThresholdModelsActive())
    return false;

  ScalarImageWrapperBase *iw = this->GetActiveScalarLayer(PREPROCESS_THRESHOLD);
  ThresholdSettings *ts = this->GetThresholdSettings();

  // The thresholds are stored in internal image representation, but are
  // presented to the user in native image representation.
  x = iw->GetNativeIntensityMapping()->MapInternalToNative(ts->GetUpperThreshold());

  if(range)
    {
    range->Minimum = iw->GetImageMinNative();
    range->Maximum = iw->GetImageMaxNative();
    range->StepSize = CalculatePowerOfTenStepSize(range->Minimum, range->Maximum, 1000);
    }

  return true;
}

bool SnakeWizardModel
::GetThresholdLowerValueAndRange(
    double &x, NumericValueRange<double> *range)
{
  if(!AreThresholdModelsActive())
    return false;

  ScalarImageWrapperBase *iw = this->GetActiveScalarLayer(PREPROCESS_THRESHOLD);
  ThresholdSettings *ts = this->GetThresholdSettings();

  // The thresholds are stored in internal image representation, but are
  // presented to the user in native image representation.
  x = iw->GetNativeIntensityMapping()->MapInternalToNative(ts->GetLowerThreshold());

  if(range)
    {
    range->Minimum = iw->GetImageMinNative();
    range->Maximum = iw->GetImageMaxNative();
    range->StepSize = CalculatePowerOfTenStepSize(range->Minimum, range->Maximum, 1000);
    }

  return true;
}

void SnakeWizardModel
::SetThresholdUpperValue(double x)
{
  // Map the value to internal format
  ScalarImageWrapperBase *iw = this->GetActiveScalarLayer(PREPROCESS_THRESHOLD);
  float z = (float) iw->GetNativeIntensityMapping()->MapNativeToInternal(x);

  // Get the current settings
  ThresholdSettings *ts = this->GetThresholdSettings();
  if(z < ts->GetLowerThreshold())
    ts->SetLowerThreshold(z);

  ts->SetUpperThreshold(z);
}

void SnakeWizardModel
::SetThresholdLowerValue(double x)
{
  // Map the value to internal format
  ScalarImageWrapperBase *iw = this->GetActiveScalarLayer(PREPROCESS_THRESHOLD);
  float z = (float) iw->GetNativeIntensityMapping()->MapNativeToInternal(x);

  // Get the current settings
  ThresholdSettings *ts = this->GetThresholdSettings();
  if(z > ts->GetUpperThreshold())
    ts->SetUpperThreshold(z);

  ts->SetLowerThreshold(z);
}

bool
SnakeWizardModel
::GetThresholdSmoothnessValueAndRange(double &x, NumericValueRange<double> *range)
{
  if(!AreThresholdModelsActive())
    return false;

  ThresholdSettings *ts = this->GetThresholdSettings();
  x = ts->GetSmoothness();
  if(range)
    range->Set(0, 10, 0.1);
  return true;
}

void SnakeWizardModel::SetThresholdSmoothnessValue(double x)
{
  ThresholdSettings *ts = this->GetThresholdSettings();
  ts->SetSmoothness(x);
}

bool SnakeWizardModel::GetThresholdModeValue(ThresholdSettings::ThresholdMode &x)
{
  if(!AreThresholdModelsActive())
    return false;

  ThresholdSettings *ts = this->GetThresholdSettings();
  x = ts->GetThresholdMode();
  return true;
}

void SnakeWizardModel::SetThresholdModeValue(ThresholdSettings::ThresholdMode x)
{
  ThresholdSettings *ts = this->GetThresholdSettings();
  ts->SetThresholdMode(x);
}

bool SnakeWizardModel::GetThresholdActiveLayerValueAndRange(unsigned long &value, SnakeWizardModel::LayerSelectionDomain *range)
{
  // Get the currently selected layer
  ScalarImageWrapperBase *active = GetActiveScalarLayer(PREPROCESS_THRESHOLD);

  if(active)
    {
    // Figure out the top level wrapper for the selected component
    ImageWrapperBase *parent = active->GetParentWrapper();
    if(parent)
      value = parent->GetUniqueId();
    else
      value = active->GetUniqueId();

    // Set up the domain
    if(range)
      {
      range->clear();
      for(LayerIterator it = m_Driver->GetSNAPImageData()->GetLayers(
            MAIN_ROLE | OVERLAY_ROLE);
          !it.IsAtEnd(); ++it)
        {
        (*range)[it.GetLayer()->GetUniqueId()] = it.GetLayer()->GetNickname();
        }
      }

    return true;
    }

  return false;
}

void SnakeWizardModel::SetThresholdActiveLayerValue(unsigned long value)
{
  // Find the layer
  ImageWrapperBase *layer = m_Driver->GetSNAPImageData()->FindLayer(value, false);
  if(layer)
    {
    m_Driver->GetPreprocessingFilterPreviewer(PREPROCESS_THRESHOLD)->SetActiveScalarLayer(
          layer->GetDefaultScalarRepresentation());
    }
}

bool
SnakeWizardModel
::GetThresholdActiveScalarRepValueAndRange(
    SnakeWizardModel::LayerScalarRepIndex &value, SnakeWizardModel::ScalarRepSelectionDomain *range)
{
  // Get the currently selected scalar layer
  ScalarImageWrapperBase *active = GetActiveScalarLayer(PREPROCESS_THRESHOLD);
  VectorImageWrapperBase *parent =
      (active) ? dynamic_cast<VectorImageWrapperBase*>(active->GetParentWrapper()) : NULL;

  // If the scalar layer is its own parent, there should be nothing selected
  if(active && parent && parent->GetNumberOfComponents() > 1)
    {
    // Now we must figure out the index of the layer in the parent
    if(parent->FindScalarRepresentation(active, value.first, value.second))
      {
      // Configure the domain
      if(range)
        {
        range->clear();
        for(int sr = SCALAR_REP_COMPONENT; sr < NUMBER_OF_SCALAR_REPS; sr++)
          {
          (*range)[std::make_pair(SCALAR_REP_MAGNITUDE, 0)] = "Magnitude of Components";
          (*range)[std::make_pair(SCALAR_REP_MAX, 0)] = "Maximum Component";
          (*range)[std::make_pair(SCALAR_REP_AVERAGE, 0)] = "Average Component";

          for(int k = 0; k < parent->GetNumberOfComponents(); k++)
            {
            std::ostringstream oss;
            oss << "Component " << (k+1);
            (*range)[std::make_pair(SCALAR_REP_COMPONENT, k)] = oss.str();
            }
          }
        }

      return true;
      }
    }

  return false;
}

void
SnakeWizardModel
::SetThresholdActiveScalarRepValue(LayerScalarRepIndex value)
{
  // Get the currently selected scalar layer
  ScalarImageWrapperBase *active =GetActiveScalarLayer(PREPROCESS_THRESHOLD);
  VectorImageWrapperBase *parent =
      (active) ? dynamic_cast<VectorImageWrapperBase*>(active->GetParentWrapper()) : NULL;

  // The parent should be not null!
  assert(parent);

  // Set the component within the parent
  ScalarImageWrapperBase *comp = parent->GetScalarRepresentation(value.first, value.second);
  m_Driver->GetPreprocessingFilterPreviewer(PREPROCESS_THRESHOLD)->SetActiveScalarLayer(comp);
}





bool SnakeWizardModel::GetPreviewValue(bool &value)
{
  PreprocessingMode mode = m_Driver->GetPreprocessingMode();
  if(mode != PREPROCESS_NONE)
    {
    value = m_Driver->GetPreprocessingFilterPreviewer(mode)->IsPreviewMode();
    return true;
    }
  return false;
}

void SnakeWizardModel::SetPreviewValue(bool value)
{
  PreprocessingMode mode = m_Driver->GetPreprocessingMode();
  if(mode != PREPROCESS_NONE)
    {
    m_Driver->GetPreprocessingFilterPreviewer(mode)->SetPreviewMode(value);
    }
}

bool SnakeWizardModel::GetBlueWhiteSpeedModeValue(bool &value)
{
  PreprocessingMode mode = m_Driver->GetPreprocessingMode();
  if(mode != PREPROCESS_NONE)
    {
    SpeedImageWrapper *speed = m_Driver->GetSNAPImageData()->GetSpeed();
    if(speed->GetColorMap()->GetSystemPreset() == ColorMap::COLORMAP_SPEED
       && !speed->IsSticky() && speed->GetAlpha() == 1.0)
      {
      value = true;
      }
    else
      {
      value = false;
      }
    return true;
    }
  return false;
}

void SnakeWizardModel::SetBlueWhiteSpeedModeValue(bool value)
{
  SpeedImageWrapper *speed = m_Driver->GetSNAPImageData()->GetSpeed();
  if(value)
    {
    speed->GetColorMap()->SetToSystemPreset(ColorMap::COLORMAP_SPEED);
    speed->SetSticky(false);
    speed->SetAlpha(1.0);
    }
}

bool SnakeWizardModel::GetRedTransparentSpeedModeValue(bool &value)
{
  PreprocessingMode mode = m_Driver->GetPreprocessingMode();
  if(mode != PREPROCESS_NONE)
    {
    SpeedImageWrapper *speed = m_Driver->GetSNAPImageData()->GetSpeed();
    if(speed->GetColorMap()->GetSystemPreset() == ColorMap::COLORMAP_SPEED_OVERLAY
       && speed->IsSticky() && speed->GetAlpha() == 0.5)
      {
      value = true;
      }
    else
      {
      value = false;
      }
    return true;
    }
  return false;
}

void SnakeWizardModel::SetRedTransparentSpeedModeValue(bool value)
{
  SpeedImageWrapper *speed = m_Driver->GetSNAPImageData()->GetSpeed();
  if(value)
    {
    speed->GetColorMap()->SetToSystemPreset(ColorMap::COLORMAP_SPEED_OVERLAY);
    speed->SetSticky(true);
    speed->SetAlpha(0.5);
    }
}



void SnakeWizardModel
::EvaluateThresholdFunction(unsigned int n, float *x, float *y)
{
  assert(m_Driver->IsSnakeModeActive());

  ScalarImageWrapperBase *grey = this->GetActiveScalarLayer(PREPROCESS_THRESHOLD);
  ThresholdSettings *ts = this->GetThresholdSettings();
  SpeedImageWrapper *speed = m_Driver->GetSNAPImageData()->GetSpeed();

  double imin = grey->GetImageMinAsDouble();
  double imax = grey->GetImageMaxAsDouble();

  SmoothBinaryThresholdFunctor<float> functor;
  functor.SetParameters(ts, imin, imax);

  for(int i = 0; i < n; i++)
    {
    float t = i * 1.0 / (n - 1);
    float x_internal = imin + t * (imax - imin);
    x[i] = grey->GetNativeIntensityMapping()->MapInternalToNative(x_internal);
    y[i] = speed->GetNativeIntensityMapping()->MapInternalToNative(functor(x_internal));

    // We are actually plotting the threshold function itself, not the speed
    // function, so we will scale further to the range [0 1]
    y[i] = 0.5 * (y[i] + 1.0);
    }
}

bool
SnakeWizardModel
::GetEdgePreprocessingSigmaValueAndRange(
    double &x, NumericValueRange<double> *range)
{
  if(!AreEdgePreprocessingModelsActive())
    return false;

  EdgePreprocessingSettings *eps = m_Driver->GetEdgePreprocessingSettings();
  x = eps->GetGaussianBlurScale();
  if(range)
    range->Set(0.1, 3, 0.01);

  return true;
}

void
SnakeWizardModel
::SetEdgePreprocessingSigmaValue(double x)
{
  EdgePreprocessingSettings *eps = m_Driver->GetEdgePreprocessingSettings();
  eps->SetGaussianBlurScale(x);
}

bool
SnakeWizardModel
::GetEdgePreprocessingKappaValueAndRange(
    double &x, NumericValueRange<double> *range)
{
  if(!AreEdgePreprocessingModelsActive())
    return false;

  EdgePreprocessingSettings *eps = m_Driver->GetEdgePreprocessingSettings();
  x = eps->GetRemappingSteepness();
  if(range)
    range->Set(0.001, 0.2, 0.001);

  return true;
}

void
SnakeWizardModel
::SetEdgePreprocessingKappaValue(double x)
{
  EdgePreprocessingSettings *eps = m_Driver->GetEdgePreprocessingSettings();
  eps->SetRemappingSteepness(x);
}


bool
SnakeWizardModel
::GetEdgePreprocessingExponentValueAndRange(
    double &x, NumericValueRange<double> *range)
{
  if(!AreEdgePreprocessingModelsActive())
    return false;

  EdgePreprocessingSettings *eps = m_Driver->GetEdgePreprocessingSettings();
  x = eps->GetRemappingExponent();
  if(range)
    range->Set(1, 4, 0.01);

  return true;
}

void
SnakeWizardModel
::SetEdgePreprocessingExponentValue(double x)
{
  EdgePreprocessingSettings *eps = m_Driver->GetEdgePreprocessingSettings();
  eps->SetRemappingExponent(x);
}


void SnakeWizardModel
::EvaluateEdgePreprocessingFunction(unsigned int n, float *x, float *y)
{
  assert(m_Driver->IsSnakeModeActive());

  EdgePreprocessingSettings *eps = m_Driver->GetEdgePreprocessingSettings();
  ScalarImageWrapperBase *grey = this->GetSelectedScalarLayer();
  SpeedImageWrapper *speed = m_Driver->GetSNAPImageData()->GetSpeed();

  // Get the range of gradient magnitude in native units
  double xlim = grey->GetImageGradientMagnitudeUpperLimitNative();
  EdgeRemappingFunctor<float> functor;
  functor.SetParameters(0, xlim,
                        eps->GetRemappingExponent(),
                        eps->GetRemappingSteepness());

  for(int i = 0; i < n; i++)
    {
    float t = i * 1.0 / (n - 1);
    float x_internal = t * xlim;
    x[i] = x_internal;
    y[i] = speed->GetNativeIntensityMapping()->MapInternalToNative(functor(x_internal));
    }
}

void SnakeWizardModel::ApplyPreprocessing()
{
  // Compute the speed image
  m_Driver->ApplyCurrentPreprocessingModeToSpeedVolume(m_Parent->GetProgressCommand());

  // Invoke an event so we get a screen update
  InvokeEvent(ModelUpdateEvent());
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

bool SnakeWizardModel::GetPreprocessingModeValueAndRange(PreprocessingMode &value, SnakeWizardModel::PreprocessingModeDomain *range)
{
  PreprocessingMode mode = m_Driver->GetPreprocessingMode();
  if(mode == PREPROCESS_NONE)
    return false;

  value = mode;

  if(range)
    {
    (*range)[PREPROCESS_THRESHOLD] = "Thresholding";
    (*range)[PREPROCESS_EDGE] = "Edge Attraction";
    (*range)[PREPROCESS_GMM] = "Clustering";
    (*range)[PREPROCESS_RF] = "Classification";
    }
  return true;
}

void SnakeWizardModel::SetPreprocessingModeValue(PreprocessingMode value)
{
  m_Driver->EnterPreprocessingMode(value);
  InvokeEvent(ModelUpdateEvent());
  InvokeEvent(GMMModifiedEvent());
  InvokeEvent(RFClassifierModifiedEvent());
}

void SnakeWizardModel::CompletePreprocessing()
{
  // If we are in classification pre-segmentation mode, set the active drawing label
  // to match the foreground class - otherwise it's confusing to the user
  if(m_Driver->GetPreprocessingMode() == PREPROCESS_RF)
    m_Parent->GetGlobalState()->SetDrawingColorLabel(
          this->GetForegroundClassColorLabel());

  // Disconnect preview pipeline
  m_Driver->EnterPreprocessingMode(PREPROCESS_NONE);
  InvokeEvent(ModelUpdateEvent());
}

bool
SnakeWizardModel
::GetActiveBubbleValue(int &value)
{
  // This is irrelevant when the snake is inactive
  if(!m_Driver->IsSnakeModeActive())
    return false;

  // This may be -1 if no bubbles are selected
  value = m_GlobalState->GetActiveBubble();
  return true;
}

void
SnakeWizardModel
::SetActiveBubbleValue(int value)
{
  m_GlobalState->SetActiveBubble(value);
  InvokeEvent(ActiveBubbleUpdateEvent());
}

void SnakeWizardModel::AddBubbleAtCursor()
{
  // Create a new bubble, using the default radius value
  Bubble bub;
  bub.center = to_int(m_Driver->GetCursorPosition());
  bub.radius = m_BubbleRadiusDefaultValue;

  // Add the bubble to the global state
  m_Driver->GetBubbleArray().push_back(bub);

  // Set the bubble's position
  m_GlobalState->SetActiveBubble(m_Driver->GetBubbleArray().size() - 1);

  // Update the bubble list in the GUI
  InvokeEvent(ActiveBubbleUpdateEvent());
  InvokeEvent(BubbleListUpdateEvent());
  InvokeEvent(BubbleDefaultRadiusUpdateEvent());
}

void SnakeWizardModel::RemoveBubbleAtCursor()
{
  int ibub = m_GlobalState->GetActiveBubble();
  IRISApplication::BubbleArray &ba = m_Driver->GetBubbleArray();

  if(ibub >= 0 && ibub < (int) ba.size())
    {
    // Remove the bubble from the global state
    ba.erase(ba.begin() + ibub);

    // Update the active bubble
    if(ibub == (int) ba.size())
      m_GlobalState->SetActiveBubble(ibub - 1);

    // Update the bubble list in the GUI
    InvokeEvent(ActiveBubbleUpdateEvent());
    InvokeEvent(BubbleListUpdateEvent());
    InvokeEvent(BubbleDefaultRadiusUpdateEvent());
    }
  else
    {
    throw IRISException("Invalid bubble index %d selected for removal.", ibub);
    }
}

bool SnakeWizardModel::UpdateBubble(int index, Bubble bubble)
{
  if(m_Driver->GetCurrentImageData()->GetImageRegion().IsInside(
       to_itkIndex(bubble.center)))
    {
    m_Driver->GetBubbleArray()[index] = bubble;
    InvokeEvent(BubbleDefaultRadiusUpdateEvent());
    return true;
    }
  return false;
}

void SnakeWizardModel::OnSnakeModeEnter()
{
  // Initialize the image data
  m_Driver->InitializeSNAPImageData(
        m_Driver->GetGlobalState()->GetSegmentationROISettings(),
        m_Parent->GetProgressCommand());

  m_Driver->SetCurrentImageDataToSNAP();

  // Some preparatory stuff
  this->ComputeBubbleRadiusDefaultAndRange();

  // Reset the bubbles
  m_Driver->GetBubbleArray().clear();
  m_GlobalState->UnsetActiveBubble();

  // We begin in preprocessing mode
  SetInteractionMode(MODE_PREPROCESSING);

  // Set the current preprocessing mode.
  PreprocessingMode lastMode = m_GlobalState->GetLastUsedPreprocessingMode();
  m_PreprocessingModeModel->SetValue(lastMode);
}

void SnakeWizardModel::ComputeBubbleRadiusDefaultAndRange()
{
  // Set bubble radius range according to volume dimensions (world dimensions)
  Vector3ui size = m_Driver->GetSNAPImageData()->GetVolumeExtents();
  Vector3d voxdims = m_Driver->GetSNAPImageData()->GetImageSpacing();
  double mindim =
      vector_multiply_mixed<double,unsigned int,3>(voxdims, size).min_value();

  // The largest value of the bubble radius is mindim / 2
  double xBubbleMax = 0.5 * mindim ;

  // The unit step should be equal or smaller than the smallest voxel edge length
  // divided by two, and should be a power of 10. Since FLTK accepts rational step
  // size, we compute it as a ratio two numbers
  double xMinVoxelEdge = 0.5 * voxdims.min_value();
  int xBubbleStepA = 1, xBubbleStepB = 1;
  int xLogVoxelEdge = (int) floor(log10(xMinVoxelEdge));
  if(xLogVoxelEdge > 0)
    xBubbleStepA = (int)(0.5 + pow(10.0, xLogVoxelEdge));
  else if(xLogVoxelEdge < 0)
    xBubbleStepB = (int)(0.5 + pow(10.0, -xLogVoxelEdge));

  // It is likely however that 0.1 is not an appropriate step size when min
  // voxel size is 0.99, so we try 0.5 and 0.2 as candidates
  if(xBubbleStepA * 5.0 / xBubbleStepB <= xMinVoxelEdge)
    xBubbleStepA *= 5;
  else if(xBubbleStepA * 2.0 / xBubbleStepB <= xMinVoxelEdge)
    xBubbleStepA *= 2;

  // Set the bubble min value
  double xBubbleStep = xBubbleStepA * 1.0 / xBubbleStepB;
  double xBubbleMin = xBubbleStep;

  // Set the default value so that it falls on the step boundary
  m_BubbleRadiusDefaultValue = floor(0.25 * xBubbleMax / xBubbleStep) * xBubbleStep;

  // Set the domain and value for the radius slider
  m_BubbleRadiusDomain.Set(xBubbleMin, xBubbleMax, xBubbleStep);

  // Let the GUI know that the values have changed
  InvokeEvent(BubbleDefaultRadiusUpdateEvent());
}

void SnakeWizardModel::SetInteractionMode(SnakeWizardModel::InteractionMode mode)
{
  m_InteractionMode = mode;
  InvokeEvent(StateMachineChangeEvent());
}


bool
SnakeWizardModel
::GetBubbleRadiusValueAndRange(
    double &value, NumericValueRange<double> *range)
{
  // Bail out if not in snake mode
  if(!m_Driver->IsSnakeModeActive())
    return false;

  int activeBubble;
  if(m_ActiveBubbleModel->GetValueAndDomain(activeBubble, NULL)
     && activeBubble >= 0)
    {
    // If a bubble is currently selected, we change the value of the
    // currently selected bubble
    value = m_Driver->GetBubbleArray()[activeBubble].radius;
    }
  else
    {
    // Otherwise, we return the default value computed for this image
    value = m_BubbleRadiusDefaultValue;
    }

  // Set the range as well
  if(range)
    *range = m_BubbleRadiusDomain;

  return true;
}

void
SnakeWizardModel
::SetBubbleRadiusValue(double value)
{
  int activeBubble;
  if(m_ActiveBubbleModel->GetValueAndDomain(activeBubble, NULL)
     && activeBubble >= 0)
    {
    // There is an active bubble - change its radius
    m_Driver->GetBubbleArray()[activeBubble].radius = value;
    InvokeEvent(BubbleListUpdateEvent());
    }

  // Always store as the default value
  m_BubbleRadiusDefaultValue = value;

  // Radius has updated
  InvokeEvent(BubbleDefaultRadiusUpdateEvent());
}

void SnakeWizardModel::OnEvolutionPageEnter()
{
  // We are no longer bubble
  SetInteractionMode(MODE_EVOLUTION);

  // Initialize the segmentation
  if(!m_Driver->InitializeActiveContourPipeline())
    {
    throw IRISException("Failed to initialize the active contour. "
                        "Check that the initialization bubbles are "
                        "present and cover the image region.");
    }
}

bool SnakeWizardModel::PerformEvolutionStep()
{
  // Do the segmentation step!
  m_Driver->GetSNAPImageData()->RunSegmentation(m_StepSizeModel->GetValue());

  // Fire an event
  InvokeEvent(EvolutionIterationEvent());

  // Return the status. In the future, this should check for convergence, but
  // the way the parallel sparse filter is behaving in the more recent versions
  // of ITK, the RMSChange flag is just meaningless.
  return false;
}

int SnakeWizardModel::GetEvolutionIterationValue()
{
  if(m_Driver->IsSnakeModeActive() &&
     m_Driver->GetSNAPImageData()->IsSegmentationActive())
    {
    return m_Driver->GetSNAPImageData()->GetElapsedSegmentationIterations();
    }
  else return 0;
}

ThresholdSettings *SnakeWizardModel::GetThresholdSettings()
{
  // Get the layer currently being thresholded
  ScalarImageWrapperBase *layer = GetActiveScalarLayer(PREPROCESS_THRESHOLD);

  // Get the threshold settings from that layer
  return layer
      ? dynamic_cast<ThresholdSettings *>(layer->GetUserData("ThresholdSettings"))
      : NULL;
}

void SnakeWizardModel::OnEvolutionPageBack()
{

  // Abort the segmentation (stops segmentation and resets the snake
  SNAPImageData *sid = m_Driver->GetSNAPImageData();
  if(sid->IsSegmentationActive())
    {
    sid->TerminateSegmentation();
    sid->ClearSnake();
    }

  SetInteractionMode(MODE_BUBBLES);
}

void SnakeWizardModel::OnEvolutionPageFinish()
{
  // Stop the segmentation pipeline
  if(m_Driver->GetSNAPImageData()->IsSegmentationActive())
    m_Driver->GetSNAPImageData()->TerminateSegmentation();

  // Update IRIS with SNAP images
  m_Driver->UpdateIRISWithSnapImageData(NULL);

  // Set an undo point
  m_Driver->StoreUndoPoint("Automatic Segmentation");

  // Return to IRIS mode
  m_Driver->SetCurrentImageDataToIRIS();
  m_Driver->ReleaseSNAPImageData();
}

void SnakeWizardModel::OnCancelSegmentation()
{
  // Stop the segmentation pipeline
  if(m_Driver->GetSNAPImageData()->IsSegmentationActive())
    m_Driver->GetSNAPImageData()->TerminateSegmentation();

  // Leave the preprocessing mode
  m_PreprocessingModeModel->SetValue(PREPROCESS_NONE);

  // Return to IRIS mode
  m_Driver->SetCurrentImageDataToIRIS();
  m_Driver->ReleaseSNAPImageData();
}

void SnakeWizardModel::RewindEvolution()
{
  if(m_Driver->GetSNAPImageData()->IsSegmentationActive())
    m_Driver->GetSNAPImageData()->RestartSegmentation();

  // Fire an event
  InvokeEvent(EvolutionIterationEvent());
}


bool SnakeWizardModel
::GetNumberOfClustersValueAndRange(
    int &value, NumericValueRange<int> *range)
{
  UnsupervisedClustering *uc = m_Driver->GetClusteringEngine();
  if(uc)
    {
    value = uc->GetNumberOfClusters();
    if(range)
      range->Set(2, 20, 1);
    return true;
    }

  return false;
}

void SnakeWizardModel
::SetNumberOfClustersValue(
    int value)
{
  UnsupervisedClustering *uc = m_Driver->GetClusteringEngine();
  assert(uc);

  uc->SetNumberOfClusters(value);
  uc->InitializeClusters();
  this->TagGMMPreprocessingFilterModified();
  this->InvokeEvent(GMMModifiedEvent());
}

bool SnakeWizardModel::GetNumberOfGMMSamplesValueAndRange(int &value, NumericValueRange<int> *range)
{
  UnsupervisedClustering *uc = m_Driver->GetClusteringEngine();
  if(uc)
    {
    value = uc->GetNumberOfSamples();
    if(range)
      {
      int nvox = m_Driver->GetCurrentImageData()->GetMain()->GetNumberOfVoxels();
      range->Set(std::min(nvox, 5000), nvox, 5000);
      }

    return true;
    }

  return false;
}

void SnakeWizardModel::SetNumberOfGMMSamplesValue(int value)
{
  UnsupervisedClustering *uc = m_Driver->GetClusteringEngine();
  assert(uc);

  uc->SetNumberOfSamples(value);
  uc->InitializeClusters();
  this->TagGMMPreprocessingFilterModified();
  this->InvokeEvent(GMMModifiedEvent());
}

bool SnakeWizardModel::GetForegroundClusterValueAndRange(int &value, NumericValueRange<int> *range)
{
  UnsupervisedClustering *uc = m_Driver->GetClusteringEngine();
  if(!uc)
    return false;

  // Go through the clusters, and find which cluster is marked as foreground
  int foreCluster = -1;
  for(int i = 0; i < uc->GetNumberOfClusters(); i++)
    {
    if(uc->GetMixtureModel()->IsForeground(i))
      {
      if(foreCluster >= 0)
        // Oops! more than one foreground cluster!
        return false;
      else
        foreCluster = i;
      }
    }

  // Set the value
  value = foreCluster + 1;

  // Set the range
  if(range)
    {
    range->Set(1, uc->GetNumberOfClusters(), 1);
    }

  return true;
}

void SnakeWizardModel::SetForegroundClusterValue(int value)
{
  UnsupervisedClustering *uc = m_Driver->GetClusteringEngine();
  assert(uc);

  // Go through the clusters, and find which cluster is marked as foreground
  int foreCluster = value - 1;
  for(int i = 0; i < uc->GetNumberOfClusters(); i++)
    {
    if(i == foreCluster)
      uc->GetMixtureModel()->SetForeground(i);
    else
      uc->GetMixtureModel()->SetBackground(i);
    }

  this->TagGMMPreprocessingFilterModified();
  InvokeEvent(GMMModifiedEvent());
}

void SnakeWizardModel::TagGMMPreprocessingFilterModified()
{
  // TODO: this is not the right way to do this! Make MixtureModel an itkObject
  // and an inout to the filter, so we don't have to update the filter itself!!
  // THIS IS HACKY!!!
  UnsupervisedClustering *uc = m_Driver->GetClusteringEngine();
  typedef SlicePreviewFilterWrapper<GMMPreprocessingFilterConfigTraits>
                                            GMMPreprocessingPreviewWrapperType;
  GMMPreprocessingPreviewWrapperType *junk =
      (GMMPreprocessingPreviewWrapperType *) m_Driver->GetPreprocessingFilterPreviewer(PREPROCESS_GMM);
  junk->SetParameters(uc->GetMixtureModel());
}

void SnakeWizardModel::TagRFPreprocessingFilterModified()
{
  // TODO: this is not the right way to do this! Make RandomForestClassifier an itkObject
  // and an inout to the filter, so we don't have to update the filter itself!!
  // THIS IS HACKY!!!
  RFClassificationEngine *ce = m_Driver->GetClassificationEngine();
  typedef SlicePreviewFilterWrapper<RFPreprocessingFilterConfigTraits>
                                            RFPreprocessingPreviewWrapperType;
  RFPreprocessingPreviewWrapperType *junk =
      (RFPreprocessingPreviewWrapperType *) m_Driver->GetPreprocessingFilterPreviewer(PREPROCESS_RF);
  junk->SetParameters(ce->GetClassifier());

}


void SnakeWizardModel::PerformClusteringIteration()
{
  UnsupervisedClustering *uc = m_Driver->GetClusteringEngine();
  assert(uc);

  uc->Iterate();
  this->InvokeEvent(GMMModifiedEvent());

  TagGMMPreprocessingFilterModified();
}

bool SnakeWizardModel::SetClusterForegroundState(int cluster, bool state)
{
  UnsupervisedClustering *uc = m_Driver->GetClusteringEngine();
  assert(uc);

  GaussianMixtureModel *gmm = uc->GetMixtureModel();

  // Currently this implements mutually exclusive behavior
  if(state && !gmm->IsForeground(cluster))
    {
    for(int i = 0; i < gmm->GetNumberOfGaussians(); i++)
      {
      if(cluster == i)
        gmm->SetForeground(i);
      else
        gmm->SetBackground(i);
      }

    TagGMMPreprocessingFilterModified();
    this->InvokeEvent(GMMModifiedEvent());
    return true;
    }
  else
    {
    return false;
    }
}

double SnakeWizardModel::GetClusterWeight(int cluster)
{
  UnsupervisedClustering *uc = m_Driver->GetClusteringEngine();
  GaussianMixtureModel *gmm = uc->GetMixtureModel();
  return gmm->GetWeight(cluster);
}

bool SnakeWizardModel::SetClusterWeight(int cluster, double weight)
{
  UnsupervisedClustering *uc = m_Driver->GetClusteringEngine();
  assert(uc);

  GaussianMixtureModel *gmm = uc->GetMixtureModel();

  if(weight != gmm->GetWeight(cluster))
    {
    gmm->SetWeightAndRenormalize(cluster, weight);

    TagGMMPreprocessingFilterModified();
    this->InvokeEvent(GMMModifiedEvent());
    return true;
    }
  else
    return false;
}

double SnakeWizardModel::GetClusterNativeMean(int cluster, int component)
{
  UnsupervisedClustering *uc = m_Driver->GetClusteringEngine();
  GaussianMixtureModel *gmm = uc->GetMixtureModel();
  ImageWrapperBase *aiw = this->GetLayerAndIndexForNthComponent(component).ImageWrapper;
  const AbstractNativeIntensityMapping *nim = aiw->GetNativeIntensityMapping();
  return nim->MapInternalToNative(gmm->GetMean(cluster)[component]);
}

bool SnakeWizardModel::SetClusterNativeMean(int cluster, int component, double x)
{
  UnsupervisedClustering *uc = m_Driver->GetClusteringEngine();
  GaussianMixtureModel *gmm = uc->GetMixtureModel();
  ImageWrapperBase *aiw = this->GetLayerAndIndexForNthComponent(component).ImageWrapper;
  const AbstractNativeIntensityMapping *nim = aiw->GetNativeIntensityMapping();

  vnl_vector<double> mean_raw = gmm->GetMean(cluster);
  double mk = nim->MapNativeToInternal(x);
  if(mk != mean_raw[component])
    {
    mean_raw[component] = mk;
    gmm->SetMean(cluster, mean_raw);

    TagGMMPreprocessingFilterModified();
    this->InvokeEvent(GMMModifiedEvent());

    return true;
    }

  return false;
}

double SnakeWizardModel::GetClusterNativeCovariance(int cluster, int comp1, int comp2)
{
  UnsupervisedClustering *uc = m_Driver->GetClusteringEngine();
  GaussianMixtureModel *gmm = uc->GetMixtureModel();

  const AbstractNativeIntensityMapping *nim1 =
      this->GetLayerAndIndexForNthComponent(comp1).ImageWrapper->GetNativeIntensityMapping();

  const AbstractNativeIntensityMapping *nim2 =
      this->GetLayerAndIndexForNthComponent(comp2).ImageWrapper->GetNativeIntensityMapping();

  double cov_raw = gmm->GetGaussian(cluster)->GetCovariance()(comp1, comp2);
  return cov_raw * nim1->GetScale() * nim2->GetScale();
}

double SnakeWizardModel::GetClusterNativeTotalVariance(int cluster)
{
  UnsupervisedClustering *uc = m_Driver->GetClusteringEngine();
  GaussianMixtureModel *gmm = uc->GetMixtureModel();
  int ng = gmm->GetNumberOfGaussians();
  double var = 0;
  for(int i = 0; i < gmm->GetNumberOfComponents(); i++)
    var += this->GetClusterNativeCovariance(cluster, i, i);
  return var;
}

void SnakeWizardModel::ReinitializeClustering()
{
  UnsupervisedClustering *uc = m_Driver->GetClusteringEngine();
  assert(uc);

  uc->InitializeClusters();
  this->InvokeEvent(GMMModifiedEvent());

  TagGMMPreprocessingFilterModified();
}


int SnakeWizardModel::GetNumberOfComponentsForSegmentation()
{
  this->Update();
  return m_ComponentInfo.size();
}

SnakeWizardModel::ComponentInfo
SnakeWizardModel::GetLayerAndIndexForNthComponent(int n)
{
  this->Update();
  assert(n < m_ComponentInfo.size());
  return m_ComponentInfo[n];
}

void SnakeWizardModel::TrainClassifier()
{
  // Get the classification engine
  RFClassificationEngine *rfengine = m_Driver->GetClassificationEngine();

  // Perform the classification
  rfengine->TrainClassifier();

  // Get the current foreground label
  LabelType curr_foreground;
  bool fg_valid =
      m_ForegroundClassColorLabelModel->GetValueAndDomain(curr_foreground, NULL);

  // Populate the available labels for the foreground label
  m_ActiveClasses.clear();
  const RandomForestClassifier::MappingType &mapping =
      rfengine->GetClassifier()->GetClassToLabelMapping();
  for(RandomForestClassifier::MappingType::const_iterator it = mapping.begin();
      it != mapping.end(); ++it)
    {
    m_ActiveClasses[it->second] =
        m_Driver->GetColorLabelTable()->GetColorLabel(it->second);
    }

  // Fire the appropriate event
  InvokeEvent(RFClassifierModifiedEvent());

  // TODO: this is a hack!
  TagRFPreprocessingFilterModified();
}

void SnakeWizardModel::ClearSegmentation()
{
  m_Driver->ResetSNAPSegmentationImage();
}



