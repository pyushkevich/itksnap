#include "IntensityCurveModel.h"
#include "ImageWrapperBase.h"
#include "IRISApplication.h"
#include "itkImageBase.h"
#include "ScalarImageHistogram.h"
#include "GlobalUIModel.h"

#include "LayerAssociation.txx"

template class LayerAssociation<IntensityCurveLayerProperties,
                                GreyImageWrapperBase,
                                IntensityCurveModelBase::PropertiesFactory>;


IntensityCurveModel::IntensityCurveModel()
  : IntensityCurveModelBase()
{
  // Create the child model for the moving control point id
  m_MovingControlIdModel = makeChildPropertyModel(
        this,
        &Self::GetMovingControlPointIdValueAndRange,
        &Self::SetMovingControlPointId);

  // Create the child model for the control point coordinates
  m_MovingControlXYModel = makeChildPropertyModel(
        this,
        &Self::GetMovingControlPointPositionAndRange,
        &Self::SetMovingControlPointPosition);

  // Window/level model
  m_LevelWindowModel = makeChildPropertyModel(
        this,
        &Self::GetLevelAndWindowValueAndRange,
        &Self::SetLevelAndWindow);

  // Histogram bin size and other controls
  m_HistogramBinSizeModel = makeChildPropertyModel(
        this,
        &Self::GetHistogramBinSizeValueAndRange,
        &Self::SetHistogramBinSize);

  m_HistogramCutoffModel = makeChildPropertyModel(
        this,
        &Self::GetHistogramCutoffValueAndRange,
        &Self::SetHistogramCutoff);

  m_HistogramScaleModel = makeChildPropertyModel(
        this,
        &Self::GetHistogramScale,
        &Self::SetHistogramScale);

  // Layer change events rebroadcast as ModelUpdateEvents
  Rebroadcast(this, ActiveLayerChangedEvent(), ModelUpdateEvent());
}



IntensityCurveModel::~IntensityCurveModel()
{

}

void
IntensityCurveModel
::RegisterWithLayer(GreyImageWrapperBase *layer)
{
  // Listen to events on the layer's curve
  unsigned long tag =
      Rebroadcast(layer->GetIntensityMapFunction(),
                  IntensityCurveChangeEvent(), ModelUpdateEvent());

  // Set a flag so we don't register a listener again
  GetProperties().SetObserverTag(tag);
}

void
IntensityCurveModel
::UnRegisterFromLayer(GreyImageWrapperBase *layer)
{
  // It's safe to call GetProperties()
  unsigned long tag = GetProperties().GetObserverTag();
  if(tag)
    {
    layer->GetIntensityMapFunction()->RemoveObserver(tag);
    }
}


const ScalarImageHistogram *
IntensityCurveModel
::GetHistogram()
{
  // Check that we have a layer
  assert(m_Layer);

  // Get the properties for the layer
  IntensityCurveLayerProperties *p = m_LayerProperties[m_Layer];

  // Figure out the number of bins that we want
  unsigned int nBins = 64;
  if(m_ViewportReporter && m_ViewportReporter->CanReportSize())
    {
    unsigned int width = m_ViewportReporter->GetViewportSize()[0];
    nBins = width / p->GetHistogramBinSize();
    }

  // Get the histogram
  return m_Layer->GetHistogram(nBins);
}

IntensityCurveLayerProperties::IntensityCurveLayerProperties()
{
  m_ObserverTag = 0;
  m_HistogramLog = false;
  m_MovingControlPoint = false;
  m_HistogramBinSize = 4;
  m_HistogramCutoff = 1;
}

IntensityCurveLayerProperties::~IntensityCurveLayerProperties()
{
}

IntensityCurveInterface * IntensityCurveModel::GetCurve()
{
  assert(m_Layer);
  return m_Layer->GetIntensityMapFunction();
}

bool
IntensityCurveModel
::UpdateControlPoint(size_t i, float t, float x)
{
  IntensityCurveInterface *curve = this->GetCurve();

  // Must be in range
  assert(i < curve->GetControlPointCount());

  // Get the current values
  float told, xold;
  curve->GetControlPoint(i, told, xold);

  // The control value should be in range
  // if(t < 0.0 || t > 1.0)
  //  return false;

  // First and last control points are treated specially because they
  // provide windowing style behavior
  int last = curve->GetControlPointCount()-1;
  if (i == 0 || i == (size_t) last)
    {
    // Get the current domain
    float tMin,tMax,x;
    curve->GetControlPoint(0,   tMin, x);
    curve->GetControlPoint(last,tMax, x);

    // Check if the new domain is valid
    float epsilon = 0.02;
    if (i == 0 && t < tMax - epsilon)
      tMin = t;
    else if (i == (size_t) last && t > tMin + epsilon)
      tMax = t;
    else
      // One of the conditions failed; the window has size <= 0
      return false;

    // Change the domain of the curve
    curve->ScaleControlPointsToWindow(tMin, tMax);
    }
  else
    {
    // Check whether the X coordinate is in range
    if (x < 0.0 || x > 1.0)
      return false;

    // Update the control point
    curve->UpdateControlPoint(i, t, x);

    // Check the curve for monotonicity
    if(!curve->IsMonotonic())
      {
      curve->UpdateControlPoint(i, told, xold);
      return false;
      }
    }
  return true;
}

int
IntensityCurveModel
::GetControlPointInVicinity(float x, float y, int pixelRadius)
{
  assert(m_ViewportReporter && m_ViewportReporter->CanReportSize());
  Vector2ui vp = m_ViewportReporter->GetViewportSize();
  IntensityCurveInterface *curve = GetCurve();

  float rx = pixelRadius * 1.0f / vp[0];
  float ry = pixelRadius * 1.0f / vp[1];
  float fx = 1.0f / (rx * rx);
  float fy = 1.0f / (ry * ry);

  float minDistance = 1.0f;
  int nearestPoint = -1;

  for (unsigned int c=0;c<curve->GetControlPointCount();c++) {

    // Get the next control point
    float cx,cy;
    curve->GetControlPoint(c,cx,cy);

    // Check the distance to the control point
    float d = (cx - x) * (cx - x) * fx + (cy - y) * (cy - y) * fy;
    if (minDistance >= d) {
      minDistance = d;
      nearestPoint = c;
    }
  }

  // Negative: return -1
  return nearestPoint;
}

Vector3d IntensityCurveModel::GetEventCurveCoordiantes(const Vector3d &x)
{
  float t0, t1, xDummy;
  GetCurve()->GetControlPoint(0, t0, xDummy);
  GetCurve()->GetControlPoint(
        GetCurve()->GetControlPointCount() - 1, t1, xDummy);
  float z0 = std::min(t0, 0.0f);
  float z1 = std::max(t1, 1.0f);

  // Scale the display so that leftmost point to plot maps to 0, rightmost to 1
  return Vector3d(x[0] * (z1 - z0) + z0, x[1], x[2]);
}


bool IntensityCurveModel::ProcessMousePressEvent(const Vector3d &x)
{
  // Check the control point affected by the event
  Vector3d xCurve = this->GetEventCurveCoordiantes(x);
  GetProperties().SetMovingControlPoint(
      GetControlPointInVicinity(xCurve[0], xCurve[1], 5));

  // SetCursor(m_MovingControlPoint);

  // Clear the dragged flag
  m_FlagDraggedControlPoint = false;

  return true;
}

bool IntensityCurveModel::ProcessMouseDragEvent(const Vector3d &x)
{
  Vector3d xCurve = this->GetEventCurveCoordiantes(x);
  if (GetProperties().GetMovingControlPoint() >= 0)
    {
    // Update the moving control point
    if(UpdateControlPoint(GetProperties().GetMovingControlPoint(),
                           xCurve[0], xCurve[1]))
      {
      InvokeEvent(ModelUpdateEvent());
      }

    // Set the dragged flag
    m_FlagDraggedControlPoint = true;
    return true;
    }

  return false;
}

bool IntensityCurveModel::ProcessMouseReleaseEvent(const Vector3d &x)
{
  Vector3d xCurve = this->GetEventCurveCoordiantes(x);
  if (GetProperties().GetMovingControlPoint() >= 0)
    {
    // Update the moving control point
    if (UpdateControlPoint(GetProperties().GetMovingControlPoint(),
                           xCurve[0], xCurve[1]))
      {
      InvokeEvent(ModelUpdateEvent());
      }

    // Set the dragged flag
    m_FlagDraggedControlPoint = true;
    return true;
    }
  return false;
}



bool
IntensityCurveModel
::GetMovingControlPointIdValueAndRange(int &value,
                                       NumericValueRange<int> *range)
{
  if(range)
    {
    range->Minimum = 1;
    range->Maximum = GetCurve()->GetControlPointCount();
    range->StepSize = 1;
    }
  value = GetProperties().GetMovingControlPoint() + 1;
  return value >= 1;
}

void
IntensityCurveModel
::SetMovingControlPointId(int value)
{
  GetProperties().SetMovingControlPoint(value - 1);
  InvokeEvent(ModelUpdateEvent());
}

bool
IntensityCurveModel
::GetLevelAndWindowValueAndRange(Vector2d &lw,
                                 NumericValueRange<Vector2d> *range)
{
  if(!m_Layer)
    return false;

  IntensityCurveInterface *curve = this->GetCurve();

  // Get the absolute range
  double iAbsMin = m_Layer->GetImageMinNative();
  double iAbsMax = m_Layer->GetImageMaxNative();
  double iAbsSpan = iAbsMax - iAbsMin;

  // Get the starting and ending control points
  float t0, x0, t1, x1;
  curve->GetControlPoint(0,t0,x0);
  curve->GetControlPoint(curve->GetControlPointCount()-1,t1,x1);

  // The the curve intensity range
  double iMin = iAbsMin + iAbsSpan * t0;
  double iMax = iAbsMin + iAbsSpan * t1;

  // Level and window
  lw = Vector2d(iMin, iMax - iMin);

  // Compute range and step if needed
  if(range)
    {
    // The range for the window and level are basically unlimited. To be safe, we
    // set it to be two orders of magnitude greater than the largest absolute
    // value in the image.
    double step = pow(10, floor(0.5 + log10(iAbsSpan) - 3));
    double order = log10(std::max(fabs(iAbsMin), fabs(iAbsMax)));
    double maxabsval = pow(10, ceil(order)+2);

    range->Minimum = Vector2d(-maxabsval, step);
    range->Maximum = Vector2d(maxabsval, maxabsval);
    range->StepSize = Vector2d(step, step);
    }

  // Value is valid
  return true;
}


void
IntensityCurveModel
::SetLevelAndWindow(Vector2d p)
{
  IntensityCurveInterface *curve = this->GetCurve();

  // Assure that input and output outside of the image range
  // is handled gracefully
  // m_InLevel->value(m_InLevel->clamp(m_InLevel->value()));
  // m_InWindow->value(m_InWindow->clamp(m_InWindow->value()));

  // Get the absolute range
  double iAbsMin = m_Layer->GetImageMinNative();
  double iAbsMax = m_Layer->GetImageMaxNative();

  // Get the new values of min and max
  double iMin = p(0);
  double iMax = iMin + p(1);

  // Min better be less than max
  assert(iMin < iMax);

  // Compute the unit coordinate values that correspond to min and max
  double factor = 1.0 / (iAbsMax - iAbsMin);
  double t0 = factor * (iMin - iAbsMin);
  double t1 = factor * (iMax - iAbsMin);

  // Update the curve boundary
  curve->ScaleControlPointsToWindow((float) t0, (float) t1);
}

bool
IntensityCurveModel
::GetMovingControlPointPositionAndRange(
    Vector2d &pos,
    NumericValueRange<Vector2d> *range)
{
  // If no control point selected, the value is invalid
  if(!m_Layer || GetProperties().GetMovingControlPoint() < 0)
    return false;

  IntensityCurveInterface *curve = this->GetCurve();
  int cp = GetProperties().GetMovingControlPoint();

  // Compute the position
  float x, t;
  curve->GetControlPoint(cp, t, x);
  double intensity =
      m_Layer->GetImageMinNative() * (1-t) +
      m_Layer->GetImageMaxNative() * t;

  pos = Vector2d(intensity,x);

  // Compute the range
  if(range)
    {
    float t0, x0, t1, x1;

    double iAbsMin = m_Layer->GetImageMinNative();
    double iAbsMax = m_Layer->GetImageMaxNative();
    double iAbsSpan = iAbsMax - iAbsMin;

    double xStep = pow(10, floor(0.5 + log10(iAbsSpan) - 3));
    double order = log10(std::max(fabs(iAbsMin), fabs(iAbsMax)));
    double maxabsval = pow(10, ceil(order)+2);

    if(cp == 0)
      {
      range->Minimum[0] = -maxabsval;
      }
    else
      {
      curve->GetControlPoint(cp - 1, t0, x0);
      range->Minimum[0] = iAbsMin + iAbsSpan * t0 + xStep;
      }

    if(cp == (int)(curve->GetControlPointCount() - 1))
      {
      range->Maximum[0] = maxabsval;
      }
    else
      {
      curve->GetControlPoint(cp + 1, t1, x1);
      range->Maximum[0] = iAbsMin + iAbsSpan * t1 - xStep;
      }

    if(cp == 0)
      {
      range->Minimum[1] = range->Maximum[1] = 0;
      }
    else if(cp == (int)(curve->GetControlPointCount() - 1))
      {
      range->Minimum[1] = range->Maximum[1] = 1;
      }
    else
      {
      range->Minimum[1] = x0 + 0.01;
      range->Maximum[1] = x1 - 0.01;
      }

    range->StepSize[0] = xStep;
    range->StepSize[1] = 0.01;
    }

  return true;
}

void IntensityCurveModel::SetMovingControlPointPosition(Vector2d p)
{
  assert(m_Layer);
  IntensityCurveInterface *curve = this->GetCurve();

  double t = (p[0] - m_Layer->GetImageMinNative()) /
      (m_Layer->GetImageMaxNative() - m_Layer->GetImageMinNative());
  curve->UpdateControlPoint(GetProperties().GetMovingControlPoint(),
                            t, p[1]);
}


void
IntensityCurveModel
::OnControlPointNumberDecreaseAction()
{
  IntensityCurveInterface *curve = this->GetCurve();

  if (curve->GetControlPointCount() > 3)
    {
    curve->Initialize(curve->GetControlPointCount() - 1);
    GetProperties().SetMovingControlPoint(0);

    // m_BoxCurve->GetInteractor()->SetMovingControlPoint(0);
    // OnWindowLevelChange();
    InvokeEvent(ModelUpdateEvent());
    }

  // if (m_Curve->GetControlPointCount() == 3)
  //  m_BtnCurveLessControlPoint->deactivate();
}

void
IntensityCurveModel
::OnControlPointNumberIncreaseAction()
{
  this->GetCurve()->Initialize(this->GetCurve()->GetControlPointCount() + 1);
  InvokeEvent(ModelUpdateEvent());
}

void IntensityCurveModel::OnResetCurveAction()
{
  this->GetCurve()->Initialize(this->GetCurve()->GetControlPointCount());
  InvokeEvent(ModelUpdateEvent());
}

void IntensityCurveModel::OnUpdate()
{
  if(m_EventBucket->HasEvent(IntensityCurveChangeEvent()))
    {
    // Inform the layer that it needs to recompute its intensity map function
    this->GetLayer()->UpdateIntensityMapFunction();
    }
}

void IntensityCurveModel::OnAutoFitWindow()
{
  // There must be a layer
  assert(m_Layer);

  // Get the histogram
  m_Layer->AutoFitContrast();
}

bool
IntensityCurveModel
::GetHistogramBinSizeValueAndRange(
    int &value, NumericValueRange<int> *range)
{
  if(m_Layer)
    {
    value = (int) GetProperties().GetHistogramBinSize();
    if(range)
      {
      range->Minimum = 1;
      range->Maximum = m_Layer->GetNumberOfVoxels() / 10;
      range->StepSize = 1;
      }
    return true;
    }
  return false;
}

void
IntensityCurveModel
::SetHistogramBinSize(int value)
{
  assert(m_Layer);
  GetProperties().SetHistogramBinSize((unsigned int) value);
  InvokeEvent(ModelUpdateEvent());
}

bool
IntensityCurveModel
::GetHistogramCutoffValueAndRange(
    double &value, NumericValueRange<double> *range)
{
  if(m_Layer)
    {
    value = GetProperties().GetHistogramCutoff() * 100.0;
    if(range)
      *range = NumericValueRange<double>(0.1, 100, 1);
    return true;
    }
  return false;
}

void
IntensityCurveModel
::SetHistogramCutoff(double value)
{
  assert(m_Layer);
  GetProperties().SetHistogramCutoff(value / 100.0);
  InvokeEvent(ModelUpdateEvent());
}

bool
IntensityCurveModel
::GetHistogramScale(bool &value)
{
  if(m_Layer)
    {
    value = GetProperties().IsHistogramLog();
    return true;
    }
  return false;
}

void
IntensityCurveModel
::SetHistogramScale(bool value)
{
  assert(m_Layer);
  GetProperties().SetHistogramLog(value);
  InvokeEvent(ModelUpdateEvent());
}


