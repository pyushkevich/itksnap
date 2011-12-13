#include "IntensityCurveModel.h"
#include "ImageWrapperBase.h"
#include "IRISApplication.h"
#include "itkImageBase.h"
#include "ScalarImageHistogram.h"
#include "GlobalUIModel.h"

#include "LayerAssociation.txx"

template class LayerAssociation<IntensityCurveLayerProperties,
                                GreyImageWrapperBase,
                                IntensityCurvePropertyAssociationFactory>;









IntensityCurveModel::IntensityCurveModel()
{
  // Set up the factory
  IntensityCurvePropertyAssociationFactory factory;
  factory.m_Model = this;
  m_LayerProperties.SetDelegate(factory);

  m_ParentModel = NULL;
  m_Layer = NULL;

  // Initialize the child models
  typedef FunctionWrapperNumericValueModel<
      Vector2d, IntensityCurveModel> VectorWrapperModel;

  // Control point coordinate models
  SmartPtr<VectorWrapperModel> modelControl = VectorWrapperModel::New();
  modelControl->Initialize(this,
                           &IntensityCurveModel::GetMovingControlPointPosition,
                           &IntensityCurveModel::SetMovingControlPointPosition,
                           &IntensityCurveModel::GetMovingControlPointRange,
                           &IntensityCurveModel::IsMovingControlPointAvailable);
  modelControl->SetEvents(ModelUpdateEvent(), ModelUpdateEvent());

  // Create component models for X and Y
  m_MovingControlXModel = static_cast<NumericValueModel *>(
      ComponentEditableNumericValueModel<double, 2>::New(modelControl, 0));

  m_MovingControlYModel = static_cast<NumericValueModel *>(
      ComponentEditableNumericValueModel<double, 2>::New(modelControl, 1));

}

IntensityCurveModel::~IntensityCurveModel()
{

}

void
IntensityCurveModel
::SetParentModel(GlobalUIModel *parent)
{
  // Store the parent model
  m_ParentModel = parent;

  // Associate the layers with properties.
  m_LayerProperties.SetImageData(
        m_ParentModel->GetDriver()->GetCurrentImageData());

  // Set active layer to NULL
  SetLayer(NULL);
}

void
IntensityCurveModel
::SetLayer(GreyImageWrapperBase *layer)
{

  // Make sure the layer-specific stuff is up to date
  m_LayerProperties.Update();

  // Unregister from the current layer
  if(m_LayerProperties.find(m_Layer) != m_LayerProperties.end())
    {
    // It's safe to call GetProperties()
    unsigned long tag = GetProperties().GetObserverTag();
    if(tag)
      {
      layer->GetIntensityMapFunction()->RemoveObserver(tag);
      }
    }

  // Set the layer
  m_Layer = layer;

  // Handle events. Need to be careful here, because layers are dynamically
  // changing, and we don't want to add more than one observer to any layer.
  // Note that we don't remove the observer from the old layer because when
  // this method is called, the old layer may have already been destroyed!
  if(m_Layer)
    {
    // Listen to events on the layer's curve
    unsigned long tag =
        Rebroadcast(m_Layer->GetIntensityMapFunction(),
                    IntensityCurveChangeEvent(), ModelUpdateEvent());

    // Set a flag so we don't register a listener again
    GetProperties().SetObserverTag(tag);
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
  m_HistogramBinSize = 1;
  m_HistogramCutoff = 1;
}

IntensityCurveLayerProperties::~IntensityCurveLayerProperties()
{
}

IntensityCurveLayerProperties *
IntensityCurvePropertyAssociationFactory
::New(GreyImageWrapperBase *layer)
{
  return m_Model->CreateProperty(layer);
}

IntensityCurveLayerProperties *
IntensityCurveModel
::CreateProperty(GreyImageWrapperBase *w)
{
  // Create the property
  IntensityCurveLayerProperties *p = new IntensityCurveLayerProperties();

  // Set the default bin size ...
  if(m_ViewportReporter && m_ViewportReporter->CanReportSize())
    {
    unsigned int width = m_ViewportReporter->GetViewportSize()[0];
    p->SetHistogramBinSize(std::max(width / 64, 1u));
    }

  return p;
}

IntensityCurveInterface * IntensityCurveModel::GetCurve()
{
  assert(m_Layer);
  return m_Layer->GetIntensityMapFunction();
}

IntensityCurveLayerProperties & IntensityCurveModel::GetProperties()
{
  assert(m_Layer);
  return *m_LayerProperties[m_Layer];
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

bool IntensityCurveModel::IsMovingControlPointAvailable()
{
  return (m_Layer && (GetProperties().GetMovingControlPoint() >= 0));
}

Vector2d IntensityCurveModel::GetMovingControlPointPosition()
{
  assert(IsMovingControlPointAvailable());
  IntensityCurveInterface *curve = this->GetCurve();
  float x, t;
  curve->GetControlPoint(GetProperties().GetMovingControlPoint(), t, x);
  double intensity =
      m_Layer->GetImageMinNative() * (1-t) +
      m_Layer->GetImageMaxNative() * t;
  return Vector2d(intensity,x);
}

void IntensityCurveModel::SetMovingControlPointPosition(Vector2d p)
{
  assert(IsMovingControlPointAvailable());
  IntensityCurveInterface *curve = this->GetCurve();

  double t = (p[0] - m_Layer->GetImageMinNative()) /
      (m_Layer->GetImageMaxNative() - m_Layer->GetImageMinNative());
  curve->UpdateControlPoint(GetProperties().GetMovingControlPoint(),
                            t, p[1]);
}

NumericValueRange<Vector2d> IntensityCurveModel::GetMovingControlPointRange()
{
  assert(IsMovingControlPointAvailable());

  float t0, x0, t1, x1;
  int cp = GetProperties().GetMovingControlPoint();
  IntensityCurveInterface *curve = this->GetCurve();

  double iAbsMin = m_Layer->GetImageMinNative();
  double iAbsMax = m_Layer->GetImageMaxNative();
  double iAbsSpan = iAbsMax - iAbsMin;

  double xStep = pow(10, floor(0.5 + log10(iAbsSpan) - 3));

  Vector2d rmin, rmax, step;

  if(cp == 0)
    {
    rmin[0] = iAbsMin;
    }
  else
    {
    curve->GetControlPoint(cp - 1, t0, x0);
    rmin[0] = iAbsMin + iAbsSpan * t0 + xStep;
    }

  if(cp == (int)(curve->GetControlPointCount() - 1))
    {
    rmax[0] = iAbsMax;
    }
  else
    {
    curve->GetControlPoint(cp + 1, t1, x1);
    rmax[0] = iAbsMin + iAbsSpan * t1 - xStep;
    }

  if(cp == 0)
    {
    rmin[1] = rmax[1] = 0;
    }
  else if(cp == (int)(curve->GetControlPointCount() - 1))
    {
    rmin[1] = rmax[1] = 1;
    }
  else
    {
    rmin[1] = x0 + 0.01;
    rmax[1] = x1 - 0.01;
    }

  step[0] = xStep;
  step[1] = 0.01;

  return NumericValueRange<Vector2d>(rmin, rmax, step);
}





