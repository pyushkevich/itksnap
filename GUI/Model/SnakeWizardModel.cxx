#include "SnakeWizardModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GlobalState.h"
#include "GenericImageData.h"
#include "SNAPImageData.h"
#include "SmoothBinaryThresholdImageFilter.h"
#include "ColorMap.h"
#include "SlicePreviewFilterWrapper.h"

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


  m_ActiveBubbleModel = makeChildPropertyModel(
        this,
        &Self::GetActiveBubbleValueAndRange,
        &Self::SetActiveBubbleValue,
        ActiveBubbleUpdateEvent(),
        BubbleListUpdateEvent());

  m_BubbleRadiusModel = makeChildPropertyModel(
        this,
        &Self::GetBubbleRadiusValueAndRange,
        &Self::SetBubbleRadiusValue,
        BubbleDefaultRadiusUpdateEvent(),
        BubbleDefaultRadiusUpdateEvent());

  m_StepSizeModel = NewRangedConcreteProperty(1, 1, 100, 1);

  // Need to define a null setter function
  void (Self::*nullsetter)(int) = NULL;

  m_EvolutionIterationModel = makeChildPropertyModel(
        this,
        &Self::GetEvolutionIterationValue,
        nullsetter,
        EvolutionIterationEvent());
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

  // Changes to the preview pipeline (preview status) are broadcast as events
  Rebroadcast(m_Driver->GetPreprocessingFilterPreviewer(PREPROCESS_THRESHOLD),
              itk::ModifiedEvent(), ThresholdSettingsUpdateEvent());

  // Changes to the snake mode are cast as model update events
  Rebroadcast(m_GlobalState->GetSnakeTypeModel(),
              ValueChangedEvent(), ModelUpdateEvent());

  // We also need to rebroadcast these events as state change events
  Rebroadcast(this, ThresholdSettingsUpdateEvent(), StateMachineChangeEvent());
  Rebroadcast(this, ModelUpdateEvent(), StateMachineChangeEvent());
  Rebroadcast(this, ActiveBubbleUpdateEvent(), StateMachineChangeEvent());
}

bool SnakeWizardModel
::GetThresholdUpperValueAndRange(
    double &x, NumericValueRange<double> *range)
{
  if(m_Driver->GetCurrentImageData()->IsGreyLoaded())
    {
    ThresholdSettings *ts = m_Driver->GetThresholdSettings();

    // The thresholds are stored in internal image representation, but are
    // presented to the user in native image representation.
    x = m_Driver->GetCurrentImageData()->GetGrey()->
        GetNativeMapping().MapInternalToNative(ts->GetUpperThreshold());

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

    // The thresholds are stored in internal image representation, but are
    // presented to the user in native image representation.
    x = m_Driver->GetCurrentImageData()->GetGrey()->
        GetNativeMapping().MapInternalToNative(ts->GetLowerThreshold());

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
  // Map the value to internal format
  float z = (float) m_Driver->GetCurrentImageData()->GetGrey()->
      GetNativeMapping().MapNativeToInternal(x);

  // Get the current settings
  ThresholdSettings *ts = m_Driver->GetThresholdSettings();
  if(z < ts->GetLowerThreshold())
    ts->SetLowerThreshold(z);

  ts->SetUpperThreshold(z);
}

void SnakeWizardModel
::SetThresholdLowerValue(double x)
{
  // Map the value to internal format
  float z = (float) m_Driver->GetCurrentImageData()->GetGrey()->
      GetNativeMapping().MapNativeToInternal(x);

  // Get the current settings
  ThresholdSettings *ts = m_Driver->GetThresholdSettings();
  if(z > ts->GetUpperThreshold())
    ts->SetUpperThreshold(z);

  ts->SetLowerThreshold(z);
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
}

bool
SnakeWizardModel
::GetThresholdSmoothnessValueAndRange(double &x, NumericValueRange<double> *range)
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
  GreyImageWrapperBase *grey = m_Driver->GetSNAPImageData()->GetGrey();
  SpeedImageWrapper *speed = m_Driver->GetSNAPImageData()->GetSpeed();

  double imin = grey->GetImageMinAsDouble();
  double imax = grey->GetImageMaxAsDouble();

  SmoothBinaryThresholdFunctor<float> functor;
  functor.SetParameters(ts, imin, imax);

  double x_internal = t * (imax - imin) + imin;
  x = grey->GetNativeMapping().MapInternalToNative(x_internal);
  y = speed->GetNativeMapping().MapInternalToNative(functor(x_internal));
}

void SnakeWizardModel::ApplyThresholdPreprocessing()
{
  // Compute the speed image
  m_Driver->ApplyCurrentPreprocessingModeToSpeedVolume(NULL);

  // Invoke an event so we get a screen update
  InvokeEvent(ModelUpdateEvent());
}

bool SnakeWizardModel::GetThresholdPreviewValue(bool &value)
{
  if(m_Driver->IsSnakeModeActive())
    {
    value = m_Driver->GetPreprocessingFilterPreviewer(PREPROCESS_THRESHOLD)
        ->IsPreviewMode();
    return true;
    }
  else return false;
}

void SnakeWizardModel::SetThresholdPreviewValue(bool value)
{
  assert(m_Driver->IsSnakeModeActive());
  m_Driver->GetPreprocessingFilterPreviewer(PREPROCESS_THRESHOLD)
      ->SetPreviewMode(value);
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

void SnakeWizardModel::OnPreprocessingDialogClose()
{
  // Disconnect preview pipeline
  m_Driver->EnterPreprocessingMode(PREPROCESS_NONE);
  InvokeEvent(ModelUpdateEvent());
}

void SnakeWizardModel::OnThresholdingPageEnter()
{
  m_Driver->EnterPreprocessingMode(PREPROCESS_THRESHOLD);
  InvokeEvent(ModelUpdateEvent());
}

void SnakeWizardModel::OnEdgePreprocessingPageEnter()
{
  m_Driver->EnterPreprocessingMode(PREPROCESS_EDGE);
  InvokeEvent(ModelUpdateEvent());
}

bool
SnakeWizardModel
::GetActiveBubbleValueAndRange(int &value, SnakeWizardModel::BubbleDomain *range)
{
  // This is irrelevant when the snake is inactive
  if(!m_Driver->IsSnakeModeActive())
    return false;

  // This may be -1 if no bubbles are selected
  value = m_GlobalState->GetActiveBubble();

  // Provide the actual bubble list
  if(range)
    {
    *range = BubbleDomain(&m_Driver->GetBubbleArray());
    }
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

void SnakeWizardModel::OnSnakeModeEnter()
{
  // Initialize the image data
  // TODO: how to deal with the progress dialog?
  m_Driver->InitializeSNAPImageData(
        m_Driver->GetGlobalState()->GetSegmentationROISettings());

  m_Driver->SetCurrentImageDataToSNAP();

  // Some preparatory stuff
  this->ComputeBubbleRadiusDefaultAndRange();
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
  // Initialize the segmentation
  if(!m_Driver->InitializeActiveContourPipeline())
    {
    throw IRISException("Failed to initialize the active contour. "
                        "Check that the initialization bubbles are "
                        "present and cover the image region.");
    }
}

void SnakeWizardModel::PerformEvolutionStep()
{
  // Do the segmentation step!
  m_Driver->GetSNAPImageData()->RunSegmentation(m_StepSizeModel->GetValue());

  // Fire an event
  InvokeEvent(EvolutionIterationEvent());
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






