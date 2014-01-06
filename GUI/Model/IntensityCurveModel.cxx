#include "IntensityCurveModel.h"
#include "ImageWrapperBase.h"
#include "IRISApplication.h"
#include "itkImageBase.h"
#include "ScalarImageHistogram.h"
#include "GlobalUIModel.h"
#include "IntensityCurveInterface.h"
#include "DisplayMappingPolicy.h"
#include "LayerAssociation.txx"

template class LayerAssociation<IntensityCurveLayerProperties,
                                ImageWrapperBase,
                                IntensityCurveModelBase::PropertiesFactory>;


IntensityCurveModel::IntensityCurveModel()
  : IntensityCurveModelBase()
{
  // Create the child model for the moving control point id
  m_MovingControlIdModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetMovingControlPointIdValueAndRange,
        &Self::SetMovingControlPointId);

  // Create the child model for the control point coordinates
  m_MovingControlXYModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetMovingControlPointPositionAndRange,
        &Self::SetMovingControlPointPosition);

  // Min/max/level/window models
  for(int i = 0; i < 4; i++)
    m_IntensityRangeModel[i] = wrapIndexedGetterSetterPairAsProperty(
          this, i,
          &Self::GetIntensityRangeIndexedValueAndRange,
          &Self::SetIntensityRangeIndexedValue);

  // Histogram bin size and other controls
  m_HistogramBinSizeModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetHistogramBinSizeValueAndRange,
        &Self::SetHistogramBinSize);

  m_HistogramCutoffModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetHistogramCutoffValueAndRange,
        &Self::SetHistogramCutoff);

  m_HistogramScaleModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetHistogramScale,
        &Self::SetHistogramScale);

  // Model events are also state changes for GUI activation
  Rebroadcast(this, ModelUpdateEvent(), StateMachineChangeEvent());
}



IntensityCurveModel::~IntensityCurveModel()
{

}

AbstractContinuousImageDisplayMappingPolicy *
IntensityCurveModel
::GetDisplayPolicy()
{
  ImageWrapperBase *layer = this->GetLayer();
  if(layer)
    return dynamic_cast<AbstractContinuousImageDisplayMappingPolicy *>
        (layer->GetDisplayMapping());
  return NULL;
}

void
IntensityCurveModel
::RegisterWithLayer(ImageWrapperBase *layer)
{
  IntensityCurveLayerProperties &p = GetProperties();

  // Listen to changes in the layer's intensity curve
  unsigned long tag =
      Rebroadcast(layer, WrapperDisplayMappingChangeEvent(), ModelUpdateEvent());

  // Set a flag so we don't register a listener again
  p.SetObserverTag(tag);

  // If this is the first time we are registered with this layer, we are going
  // to set the histogram cutoff optimally. The user may change this later so
  // we only do this for the first-time registration.
  //
  // TODO: it may make more sense for this to be a property that's associated
  // with the image for future times that it is loaded. Then the cutoff would
  // have to be stored in the ImageWrapper.
  if(p.IsFirstTime())
    {
    // Set the cutoff automatically
    const ScalarImageHistogram *hist = layer->GetHistogram(0);
    p.SetHistogramCutoff(hist->GetReasonableDisplayCutoff(0.95, 0.6));
    p.SetFirstTime(false);
    }
}

void
IntensityCurveModel
::UnRegisterFromLayer(ImageWrapperBase *layer, bool being_deleted)
{
  if(!being_deleted)
    {
    // It's safe to call GetProperties()
    unsigned long tag = GetProperties().GetObserverTag();
    if(tag)
      {
      layer->GetDisplayMapping()->RemoveObserver(tag);
      }
    }
}


const ScalarImageHistogram *
IntensityCurveModel
::GetHistogram()
{
  AbstractContinuousImageDisplayMappingPolicy *dmp = this->GetDisplayPolicy();
  assert(dmp);

  // Get the properties for the layer
  IntensityCurveLayerProperties *p = m_LayerProperties[m_Layer];

  // Figure out the number of bins that we want
  unsigned int nBins = DEFAULT_HISTOGRAM_BINS;
  if(m_ViewportReporter && m_ViewportReporter->CanReportSize())
    {
    unsigned int width = m_ViewportReporter->GetViewportSize()[0];
    nBins = width / p->GetHistogramBinSize();
    }

  // Get the histogram
  return dmp->GetHistogram(nBins);
}

IntensityCurveLayerProperties::IntensityCurveLayerProperties()
{
  m_ObserverTag = 0;
  m_HistogramLog = false;
  m_MovingControlPoint = false;
  m_HistogramBinSize = 10;
  m_HistogramCutoff = 1;
  m_FirstTime = true;
}

IntensityCurveLayerProperties::~IntensityCurveLayerProperties()
{
}

IntensityCurveInterface * IntensityCurveModel::GetCurve()
{
  return (this->GetDisplayPolicy())
      ? this->GetDisplayPolicy()->GetIntensityCurve()
      : NULL;
}

Vector2d IntensityCurveModel::GetNativeImageRangeForCurve()
{
  assert(this->GetDisplayPolicy());
  return this->GetDisplayPolicy()->GetNativeImageRangeForCurve();
}

Vector2d IntensityCurveModel::GetCurveRange()
{
  IntensityCurveInterface *curve = this->GetCurve();
  assert(curve);

  // Get the control point range
  float t0, y0, t1, y1;
  curve->GetControlPoint(0, t0, y0);
  curve->GetControlPoint(curve->GetControlPointCount() - 1, t1, y1);

  // Get the reference intensity range
  Vector2d range = this->GetNativeImageRangeForCurve();

  // Map the extents of the control points to image units
  Vector2d outRange;
  outRange[0] = range[0] * (1 - t0) + range[1] * t0;
  outRange[1] = range[0] * (1 - t1) + range[1] * t1;
  return outRange;
}

Vector2d IntensityCurveModel::GetVisibleImageRange()
{
  IntensityCurveInterface *curve = this->GetCurve();
  assert(curve);

  // Get the control point range
  float t0, y0, t1, y1;
  curve->GetControlPoint(0, t0, y0);
  curve->GetControlPoint(curve->GetControlPointCount() - 1, t1, y1);

  // Get the reference intensity range
  Vector2d range = this->GetNativeImageRangeForCurve();

  // Compute the range over which the curve is plotted, where [0 1] is the
  // image intensity range
  float z0 = std::min(t0, 0.0f);
  float z1 = std::max(t1, 1.0f);

  // Compute the range over which the curve is plotted, in intensity units
  Vector2d outRange;
  outRange[0] = range[0] * (1 - z0) + range[1] * z0;
  outRange[1] = range[0] * (1 - z1) + range[1] * z1;
  return outRange;
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
  if(!m_Layer)
    return false;

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
::GetIntensityRangeIndexedValueAndRange(
    int index,
    double &value,
    NumericValueRange<double> *range)
{
  if(!this->GetCurve())
    return false;

  // Get the range of the curve in image units
  Vector2d crange = this->GetCurveRange();

  // Level and window
  switch(index)
    {
    case 0 : value = crange[0]; break;
    case 1 : value = crange[1]; break;
    case 2 : value = (crange[0] + crange[1]) / 2; break;
    case 3 : value = (crange[1] - crange[0]); break;
    }

  // Compute range and step if needed
  if(range)
    {
    // The range for the window and level are basically unlimited. To be safe, we
    // set it to be two orders of magnitude greater than the largest absolute
    // value in the image.
    Vector2d irange = this->GetNativeImageRangeForCurve();
    double step = pow(10, floor(0.5 + log10(irange[1] - irange[0]) - 3));
    double order = log10(std::max(fabs(irange[0]), fabs(irange[1])));
    double maxabsval = pow(10, ceil(order)+2);

    // Set the ranges for each of the four properties
    switch(index)
      {
      case 0 :
      case 1 :
      case 2 :
        range->Minimum = -maxabsval;
        range->Maximum = maxabsval;
        break;
      case 3 :
        range->Minimum = 0.0;
        range->Maximum = maxabsval;
        break;
      }
    range->StepSize = step;
    }

  // Value is valid
  return true;
}

void IntensityCurveModel::SetIntensityRangeIndexedValue(int index, double value)
{
  // Get the curve
  IntensityCurveInterface *curve = this->GetCurve();

  // Get the intensity range and curve range in image units
  Vector2d irange = this->GetNativeImageRangeForCurve();
  Vector2d crange = this->GetCurveRange();

  // Get the current window and level
  double win = crange[1] - crange[0];
  double level = (crange[0] + crange[1]) / 2;
  double step = pow(10, floor(0.5 + log10(irange[1] - irange[0]) - 3));

  // How we set the output range depends on what property was changed
  switch(index)
    {
    case 0:         // min
      crange[0] = value;
      if(crange[0] >= crange[1])
        crange[1] = crange[0] + step;
      break;
    case 1:         // max
      crange[1] = value;
      if(crange[1] <= crange[0])
        crange[0] = crange[1] - step;
      break;
    case 2:         // level (mid-range)
      crange[0] = value - win / 2;
      crange[1] = value + win / 2;
      break;
    case 3:         // window
      if(value <= 0)
        value = step;
      crange[0] = level - value / 2;
      crange[1] = level + value / 2;
      break;
    }

  // Map the range into curve units
  double t0 = (crange[0] - irange[0]) / (irange[1] - irange[0]);
  double t1 = (crange[1] - irange[0]) / (irange[1] - irange[0]);

  curve->ScaleControlPointsToWindow(t0, t1);
}



bool
IntensityCurveModel
::GetMovingControlPointPositionAndRange(
    Vector2d &pos,
    NumericValueRange<Vector2d> *range)
{
  AbstractContinuousImageDisplayMappingPolicy *dmp = this->GetDisplayPolicy();
  if(!dmp)
    return false;

  // If no control point selected, the value is invalid
  if(GetProperties().GetMovingControlPoint() < 0)
    return false;

  IntensityCurveInterface *curve = dmp->GetIntensityCurve();
  int cp = GetProperties().GetMovingControlPoint();

  // Get the absolute range
  Vector2d iAbsRange = dmp->GetNativeImageRangeForCurve();

  // Compute the position
  float x, t;
  curve->GetControlPoint(cp, t, x);
  double intensity = iAbsRange[0] * (1-t) + iAbsRange[1] * t;

  pos = Vector2d(intensity,x);

  // Compute the range
  if(range)
    {
    float t0, x0, t1, x1;

    double iAbsSpan = iAbsRange[1] - iAbsRange[0];

    double xStep = pow(10, floor(0.5 + log10(iAbsSpan) - 3));
    double order = log10(std::max(fabs(iAbsRange[0]), fabs(iAbsRange[1])));
    double maxabsval = pow(10, ceil(order)+2);

    if(cp == 0)
      {
      range->Minimum[0] = -maxabsval;
      }
    else
      {
      curve->GetControlPoint(cp - 1, t0, x0);
      range->Minimum[0] = iAbsRange[0] + iAbsSpan * t0 + xStep;
      }

    if(cp == (int)(curve->GetControlPointCount() - 1))
      {
      range->Maximum[0] = maxabsval;
      }
    else
      {
      curve->GetControlPoint(cp + 1, t1, x1);
      range->Maximum[0] = iAbsRange[0] + iAbsSpan * t1 - xStep;
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
  AbstractContinuousImageDisplayMappingPolicy *dmp = this->GetDisplayPolicy();
  assert(dmp);

  IntensityCurveInterface *curve = dmp->GetIntensityCurve();
  Vector2d iAbsRange = dmp->GetNativeImageRangeForCurve();

  double t = (p[0] - iAbsRange[0]) / (iAbsRange[1] - iAbsRange[0]);
  curve->UpdateControlPoint(GetProperties().GetMovingControlPoint(), t, p[1]);
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
  this->GetCurve()->Reset();
  InvokeEvent(ModelUpdateEvent());
}

void IntensityCurveModel::OnUpdate()
{
  Superclass::OnUpdate();
}

AbstractRangedDoubleProperty *
IntensityCurveModel::GetIntensityRangeModel(
    IntensityRangePropertyType index) const
{
  return m_IntensityRangeModel[index];
}

void IntensityCurveModel::OnAutoFitWindow()
{
  // There must be a layer
  AbstractContinuousImageDisplayMappingPolicy *dmp = this->GetDisplayPolicy();
  assert(dmp);

  // Get the histogram
  dmp->AutoFitContrast();
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

bool IntensityCurveModel::CheckState(IntensityCurveModel::UIState state)
{
  // All flags are false if no layer is loaded
  if(this->GetLayer() == NULL)
    return false;

  // Otherwise get the properties
  IntensityCurveLayerProperties &p = this->GetProperties();
  int cp = p.GetMovingControlPoint();

  switch(state)
    {
    case UIF_LAYER_ACTIVE:
      return true;
    case UIF_CONTROL_SELECTED:
      return cp >= 0;
    }
  return false;
}



