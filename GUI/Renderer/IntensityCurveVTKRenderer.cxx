#include "IntensityCurveVTKRenderer.h"

#include <vtkChartXY.h>
#include <vtkPlot.h>
#include <vtkFloatArray.h>
#include <vtkTable.h>
#include <vtkContextView.h>
#include <vtkContextScene.h>
#include <vtkAxis.h>
#include "vtkControlPointsItem.h"
#include "vtkObjectFactory.h"
#include "vtkContext2D.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkIdTypeArray.h"
#include "vtkPlotPoints.h"
#include "vtkTransform2D.h"
#include "vtkScalarsToColorsItem.h"
#include "vtkImageData.h"
#include "vtkContextInteractorStyle.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkContextMouseEvent.h"
#include "vtkChartLegend.h"
#include <vtkTextProperty.h>


#include "IntensityCurveModel.h"
#include "IntensityCurveInterface.h"
#include "LayerHistogramPlotAssembly.h"
#include "ScalarImageHistogram.h"
#include "ColorMapModel.h"


class AbstractColorMapContextItem : public vtkScalarsToColorsItem
{
public:
  vtkTypeMacro(AbstractColorMapContextItem, vtkScalarsToColorsItem)

  void SetModel(IntensityCurveModel *model)
  {
    m_Model = model;
    m_ColorMapModel = model->GetParentModel()->GetColorMapModel();
    this->Modified();
  }

protected:

  AbstractColorMapContextItem()
  {
    m_Model = NULL;
    m_ColorMapModel = NULL;
  }


  virtual ~AbstractColorMapContextItem() {}

  IntensityCurveModel *m_Model;
  ColorMapModel *m_ColorMapModel;

};



class HorizontalColorMapContextItem : public AbstractColorMapContextItem
{
public:
  vtkTypeMacro(HorizontalColorMapContextItem, AbstractColorMapContextItem)

  static HorizontalColorMapContextItem *New();


protected:

  HorizontalColorMapContextItem() {}
  virtual ~HorizontalColorMapContextItem() {}

  virtual void GetBounds(double bounds[4])
  {
    if(m_Model && m_Model->GetLayer())
      {
      Vector2d vis_range = m_Model->GetVisibleImageRange();
      bounds[0] = vis_range[0];
      bounds[1] = vis_range[1];

      // Draw under the axes
      bounds[2] = -0.1;
      bounds[3] = -0.02;
      }
  }

  virtual void ComputeTexture()
  {
    // The size of the texture - fixed
    const unsigned int dim = 256;

    // Compute the bounds
    double bounds[4];
    this->GetBounds(bounds);
    if (bounds[0] == bounds[1] || !m_Model)
      return;

    // Get the curve object
    IntensityCurveInterface *curve = m_Model->GetCurve();

    // Define the texture
    if (this->Texture == 0)
      {
      this->Texture = vtkImageData::New();
      this->Texture->SetExtent(0, dim-1, 0, 0, 0, 0);
      this->Texture->SetPointDataActiveScalarInfo(
            this->Texture->GetInformation(), VTK_UNSIGNED_CHAR, 4);
      this->Texture->AllocateScalars(this->Texture->GetInformation());
      }

    // Get the texture array
    unsigned char *data =
        reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0,0,0));
    Vector2d range = m_Model->GetNativeImageRangeForCurve();

    // Get the colormap, and if there is none (RGB display), then use
    // the default greyscale colormap
    ColorMap *cm = m_ColorMapModel->GetColorMap();

    // Draw the texture. The texture is being drawn on the x-axis, so it must
    // first be interpolated through the intensity curve
    for(int i = 0; i < dim; i++)
      {
      // Here x is the intensity value for which we want to look up the color
      double x = bounds[0] + i * (bounds[1] - bounds[0]) / (dim - 1);

      // We first map this to a value t, which is normalized for intensity range
      double t = (x - range[0]) / (range[1] - range[0]);

      // Map the intensity value to the colormap index
      double y = curve->Evaluate(t);

      // Below we handle the null colormap case, used when adjusting contrast
      // on RGB-mode images
      ColorMap::RGBAType rgba;
      if(cm)
        rgba = cm->MapIndexToRGBA(y);
      else
        rgba[0] = rgba[1] = rgba[2] = (unsigned char)(255.0 * y);

      for(int j = 0; j < 3; j++)
        *data++ = rgba[j];

      // Skip the opacity part
      *data++ = 255;
      }

    this->Texture->Modified();
  }
};



class VerticalColorMapContextItem : public AbstractColorMapContextItem
{
public:
  vtkTypeMacro(VerticalColorMapContextItem, AbstractColorMapContextItem)

  static VerticalColorMapContextItem *New();


protected:

  VerticalColorMapContextItem() {}
  virtual ~VerticalColorMapContextItem() {}

  virtual void GetBounds(double bounds[4])
  {
    if(m_Model && m_Model->GetLayer())
      {
      Vector2d vis_range = m_Model->GetVisibleImageRange();
      float margin = (vis_range[1] - vis_range[0]) / 40.0;
      bounds[0] = vis_range[0] - 0.9 * margin;
      bounds[1] = vis_range[0] - 0.1 * margin;
      bounds[2] = 0.0;
      bounds[3] = 1.0;
      }
  }

  virtual void ComputeTexture()
  {
    if (!m_Model)
      return;

    // The size of the texture - fixed
    const unsigned int dim = 256;

    // Compute the bounds
    double bounds[4];
    this->GetBounds(bounds);

    // Define the texture
    if (this->Texture == 0)
      {
      vtkInformation *info = this->Texture->GetInformation();
      this->Texture = vtkImageData::New();
      this->Texture->SetExtent(0, 0, 0, 0, 0, dim-1);
      this->Texture->SetPointDataActiveScalarInfo(info, VTK_UNSIGNED_CHAR, 4);
      this->Texture->AllocateScalars(info);
      }

    // Get the texture array
    unsigned char *data =
        reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0,0,0));

    // Get the colormap (may be NULL)
    ColorMap *cm = m_ColorMapModel->GetColorMap();

    // Draw the texture. The texture is being drawn on the y-axis, so we use
    // simple interpolation
    for(int i = 0; i < dim; i++)
      {
      // Here x is the intensity value for which we want to look up the color
      double t = i * 1.0 / (dim - 1);

      // Below we handle the null colormap case, used when adjusting contrast
      // on RGB-mode images
      ColorMap::RGBAType rgba;
      if(cm)
        rgba = cm->MapIndexToRGBA(t);
      else
        rgba[0] = rgba[1] = rgba[2] = (unsigned char)(255.0 * t);

      for(int j = 0; j < 3; j++)
        *data++ = rgba[j];

      // Skip the opacity part
      *data++ = 255;
      }

    this->Texture->Modified();
  }
};


#include "vtkPiecewiseControlPointsItem.h"


class IntensityCurveControlPointsContextItem : public vtkControlPointsItem
{
public:
  vtkTypeMacro(IntensityCurveControlPointsContextItem, vtkControlPointsItem)

  static IntensityCurveControlPointsContextItem *New();

  virtual void GetControlPoint(vtkIdType index, double *point) const
  {
    // Return the coordinates of the point, in plot units
    IntensityCurveInterface *curve = m_Model->GetCurve();
    Vector2d range = m_Model->GetNativeImageRangeForCurve();

    float t, x, y;
    curve->GetControlPoint(index, t, y);
    x = range[0] * (1 - t) + range[1] * t;

    point[0] = x;
    point[1] = y;
  }

  virtual vtkIdType GetNumberOfPoints() const
  {
    if(m_Model && m_Model->GetLayer())
      return m_Model->GetCurve()->GetControlPointCount();
    else return 0;
  }

  virtual vtkIdType RemovePoint(double *pos) { return -1; }

  virtual vtkIdType AddPoint(double *newPos) { return -1; }

  virtual void emitEvent(unsigned long event, void* params = 0) {};

  virtual void SetControlPoint(vtkIdType index, double *point)
  {
    // Return the coordinates of the point, in plot units
    Vector2d range = m_Model->GetNativeImageRangeForCurve();

    // Force the positions of the starting and ending points
    float t = (point[0] - range[0]) / (range[1] - range[0]);
    float y = point[1];

    m_Model->UpdateControlPoint(index, t, y);
  }

  virtual void DrawSelectedPoints(vtkContext2D *painter)
  {
    painter->GetBrush()->SetColor(255, 255, 0, 255);
    Superclass::DrawSelectedPoints(painter);
  }

  // I had to cannibalize the paint method because VTK hardcoded some basic
  // properties such as the point color
  virtual bool Paint(vtkContext2D *painter)
  {
    painter->GetBrush()->SetColor(255, 0, 0, 255);
    painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
    painter->GetPen()->SetWidth(1.2);
    painter->GetPen()->SetColor(0, 0, 0, 128);

    // When expressing anything in screen pixel units, we need to watch out
    // for retina displays
    float vppr = m_Model->GetViewportReporter()->GetViewportPixelRatio();
    this->ScreenPointRadius = 5 * vppr;
    this->DrawUnselectedPoints(painter);

    if (this->CurrentPoint != -1)
      {

      painter->GetBrush()->SetColor(255, 255, 0, 255);
      painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);

      this->ScreenPointRadius = 6 * vppr;
      this->DrawPoint(painter, this->CurrentPoint);
      }

    this->Transform->SetMatrix(painter->GetTransform()->GetMatrix());
    return true;
  }

  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
  {
    this->MouseMoved = false;

    if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
      {
      double pos[2];
      pos[0] = mouse.Pos[0];
      pos[1] = mouse.Pos[1];
      vtkIdType pointUnderMouse = this->FindPoint(pos);
      this->SetCurrentPoint(pointUnderMouse);
      return true;
      }

    else return false;
  }

  virtual unsigned long int GetControlPointsMTime()
  {
    // TODO: figure this out!
    return this->GetMTime();
  }


  void SetModel(IntensityCurveModel *model)
  {
    m_Model = model;
    this->Modified();
  }

protected:

  IntensityCurveControlPointsContextItem()
  {
    m_Model = NULL;
  }

  ~IntensityCurveControlPointsContextItem() {}

  IntensityCurveModel *m_Model;

};


vtkStandardNewMacro(HorizontalColorMapContextItem)
vtkStandardNewMacro(VerticalColorMapContextItem)
vtkStandardNewMacro(IntensityCurveControlPointsContextItem);


IntensityCurveVTKRenderer::IntensityCurveVTKRenderer()
  : AbstractVTKSceneRenderer()
{
  this->m_RenderWindow->SetMultiSamples(0);
  this->m_RenderWindow->SetLineSmoothing(1);
  this->m_RenderWindow->SetPolygonSmoothing(1);

  m_Model = NULL;

  // Set up the scene for rendering
  m_Chart = vtkSmartPointer<vtkChartXY>::New();

  // Configure chart behavior
  m_Chart->ForceAxesToBoundsOn();

  // Add the chart to the renderer
  m_ContextView->GetScene()->AddItem(m_Chart);

  // Set up the data
  m_CurveX = vtkSmartPointer<vtkFloatArray>::New();
  m_CurveX->SetName("Image Intensity");
  m_CurveY = vtkSmartPointer<vtkFloatArray>::New();
  m_CurveY->SetName("Output Intensity");

  // Set up the table
  m_PlotTable = vtkSmartPointer<vtkTable>::New();
  m_PlotTable->AddColumn(m_CurveX);
  m_PlotTable->AddColumn(m_CurveY);

  // There are additional points at the tails of the curve so it looks like
  // the curve runs over the entire range of the axis
  m_PlotTable->SetNumberOfRows(CURVE_RESOLUTION+3);

  // Set up the histogram plot
  m_HistogramAssembly = new LayerHistogramPlotAssembly();
  m_HistogramAssembly->AddToChart(m_Chart);

  // Set up the curve plot
  m_CurvePlot = m_Chart->AddPlot(vtkChart::LINE);
  m_CurvePlot->SetInputData(m_PlotTable, 0, 1);
  m_CurvePlot->SetColor(1, 0, 0);
  m_CurvePlot->SetWidth(1.0);
  m_CurvePlot->GetYAxis()->SetBehavior(vtkAxis::FIXED);
  m_CurvePlot->GetYAxis()->SetMinimumLimit(-0.1);
  m_CurvePlot->GetYAxis()->SetMinimum(-0.1);
  m_CurvePlot->GetYAxis()->SetMaximumLimit(1.1);
  m_CurvePlot->GetYAxis()->SetMaximum(1.1);  
  m_CurvePlot->GetXAxis()->SetTitle("Image Intensity");
  m_CurvePlot->GetYAxis()->SetTitle("Index into Color Map");
  m_CurvePlot->GetXAxis()->SetBehavior(vtkAxis::FIXED);

  // Set up the color map plot
  m_XColorMapItem = vtkSmartPointer<HorizontalColorMapContextItem>::New();
  m_YColorMapItem = vtkSmartPointer<VerticalColorMapContextItem>::New();
  m_Chart->AddPlot(m_XColorMapItem);

  // I commented this out, because the chart looks too busy
  // m_Chart->AddPlot(m_YColorMapItem);

  m_Controls = vtkSmartPointer<IntensityCurveControlPointsContextItem>::New();
  m_Chart->AddPlot(m_Controls);

  // Disable the legend in the plot. This seems like somewhat of a bug, but
  // the Hit() method in vtkChartLegend responds to events even when the
  // legend is not visible. So we have to instead make the legend not draggable
  m_Chart->SetShowLegend(false);
  m_Chart->GetLegend()->SetDragEnabled(false);

  // Listen to events from the control points item, in order to synchronize
  // the selected control point in the model with the context item
  AddListenerVTK(m_Controls, vtkControlPointsItem::CurrentPointChangedEvent,
                 this, &Self::OnCurrentControlPointChangedInScene);

}

IntensityCurveVTKRenderer::~IntensityCurveVTKRenderer()
{
  delete m_HistogramAssembly;
}

void
IntensityCurveVTKRenderer
::SetModel(IntensityCurveModel *model)
{
  m_Model = model;

  m_Controls->SetModel(model);
  m_XColorMapItem->SetModel(model);
  m_YColorMapItem->SetModel(model);

  // Rebroadcast the relevant events from the model in order for the
  // widget that uses this renderer to cause an update
  Rebroadcast(model, ModelUpdateEvent(), ModelUpdateEvent());
}



void
IntensityCurveVTKRenderer
::UpdatePlotValues()
{
  if(m_Model->GetLayer())
    {
    IntensityCurveInterface *curve = m_Model->GetCurve();
    Vector2d range = m_Model->GetNativeImageRangeForCurve();

    // Get the range over which to draw the curve
    float t0, t1, y0, y1;
    curve->GetControlPoint(0, t0, y0);
    curve->GetControlPoint(curve->GetControlPointCount() - 1, t1, y1);

    // Compute the range over which the curve is plotted, where [0 1] is the
    // image intensity range
    float z0 = std::min(t0, 0.0f);
    float z1 = std::max(t1, 1.0f);

    // Compute the range over which the curve is plotted, in intensity units
    float x0 = range[0] * (1 - z0) + range[1] * z0;
    float x1 = range[0] * (1 - z1) + range[1] * z1;

    // Sample the curve
    for(int i = 0; i <= CURVE_RESOLUTION; i++)
      {
      float p = i * 1.0 / CURVE_RESOLUTION;
      float t = z0 * (1.0 - p) + z1 * p;
      float x = x0 * (1.0 - p) + x1 * p;
      float y = curve->Evaluate(t);
      m_CurveX->SetValue(i+1, x);
      m_CurveY->SetValue(i+1, y);
      }

    // Set the range of the plot. In order for the control points to be visible,
    // we include a small margin on the left and right.
    float margin = (x1 - x0) / 40.0;
    m_CurvePlot->GetXAxis()->SetMinimumLimit(x0 - margin);
    m_CurvePlot->GetXAxis()->SetMaximumLimit(x1 + margin);
    m_CurvePlot->GetXAxis()->SetMinimum(x0 - margin);
    m_CurvePlot->GetXAxis()->SetMaximum(x1 + margin);

    // While we are here, also update the range of the y axis, because it seems
    // that it sometimes gets messed up due to mouse interaction
    m_CurvePlot->GetYAxis()->SetMinimumLimit(-0.1);
    m_CurvePlot->GetYAxis()->SetMinimum(-0.1);
    m_CurvePlot->GetYAxis()->SetMaximumLimit(1.1);
    m_CurvePlot->GetYAxis()->SetMaximum(1.1);


    // While we are here, we need to set the bounds on the control point plot
    m_Controls->SetUserBounds(x0, x1, 0.0, 1.0);

    // And also make sure the selected point is correctly set
    int cpoint;
    if(m_Model->GetMovingControlIdModel()->GetValueAndDomain(cpoint, NULL))
      m_Controls->SetCurrentPoint(cpoint - 1);
    else
      m_Controls->SetCurrentPoint(-1);

    // Set the terminal points of the curve as well
    m_CurveX->SetValue(0, x0 - margin);
    m_CurveY->SetValue(0, 0.0);
    m_CurveX->SetValue(CURVE_RESOLUTION+2, x1 + margin);
    m_CurveY->SetValue(CURVE_RESOLUTION+2, 1.0);

    m_PlotTable->Modified();

    // Compute the histogram entries
    m_HistogramAssembly->PlotWithFixedLimits(
          m_Model->GetHistogram(), 0.0, 1.0,
          m_Model->GetProperties().GetHistogramCutoff(),
          m_Model->GetProperties().IsHistogramLog());

    m_XColorMapItem->Modified();
    m_YColorMapItem->Modified();

    m_Chart->RecalculateBounds();


    }
}

void IntensityCurveVTKRenderer::paintGL()
{
  if(m_Model && m_Model->GetLayer())
    {
    Superclass::paintGL();
    }
}

void
IntensityCurveVTKRenderer
::OnUpdate()
{
  m_Model->Update();
  if(m_Model && m_Model->GetLayer())
    {
    this->UpdatePlotValues();
    }
}

void
IntensityCurveVTKRenderer
::OnCurrentControlPointChangedInScene(vtkObject *, unsigned long , void *)
{
  m_Model->GetMovingControlIdModel()->SetValue(
        m_Controls->GetCurrentPoint() + 1);
}

void IntensityCurveVTKRenderer::OnDevicePixelRatioChange(int old_ratio, int new_ratio)
{
  this->UpdateChartDevicePixelRatio(m_Chart, old_ratio, new_ratio);
}
