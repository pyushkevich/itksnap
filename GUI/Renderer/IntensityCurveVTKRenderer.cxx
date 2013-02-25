#include "IntensityCurveVTKRenderer.h"

#include <vtkChartXY.h>
#include <vtkPlot.h>
#include <vtkFloatArray.h>
#include <vtkTable.h>
#include <vtkContextView.h>
#include <vtkContextScene.h>
#include <vtkAxis.h>
#include "IntensityCurveModel.h"
#include "IntensityCurveInterface.h"
#include "vtkControlPointsItem.h"
#include "vtkPiecewiseControlPointsItem.h"
#include "vtkObjectFactory.h"
#include "vtkContext2D.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkIdTypeArray.h"
#include "vtkPlotPoints.h"
#include "vtkTransform2D.h"

class IntensityCurveControlPointsContextItem : public vtkPlot
{
public:
  vtkTypeMacro(IntensityCurveControlPointsContextItem, vtkPlot)

  static IntensityCurveControlPointsContextItem *New();

  // Description:
  // Paint event for the item.
  virtual bool Paint(vtkContext2D *painter)
  {
    if(!m_Model)
      return false;


    painter->ApplyPen(m_PenNormal);

    IntensityCurveInterface *curve = m_Model->GetCurve();
    Vector2d range = m_Model->GetNativeImageRangeForCurve();
    vtkTransform2D *tran = painter->GetTransform();
    vtkSmartPointer<vtkTransform2D> isotran = vtkSmartPointer<vtkTransform2D>::New();

    // Get the range over which to draw the curve
    float t0, t1, y0, y1;
    curve->GetControlPoint(0, t0, y0);
    curve->GetControlPoint(curve->GetControlPointCount() - 1, t1, y1);

    for(int i = 0; i < curve->GetControlPointCount(); i++)
      {
      float t, xy[2], xy_screen[2];
      curve->GetControlPoint(i, t, xy[1]);
      xy[0] = range[0] * (1 - t) + range[1] * t;

      // Transform the points into screen coordinates
      tran->TransformPoints(xy, xy_screen, 1);

      // TODO: I don't like the look of these control points. For some reason
      // VTK does not handle aliasing well. Maybe point sprites would be better

      // Handle selected point
      double radius;
      if(i == m_Model->GetProperties().GetMovingControlPoint())
        {
        painter->ApplyBrush(m_BrushSelected);
        radius = 5.0;
        }
      else
        {
        painter->ApplyBrush(m_BrushNormal);
        radius = 4.0;
        }

      // Set the isotropic transform for rendering a point
      isotran->Identity();
      isotran->Translate(xy_screen);
      painter->PushMatrix();
      painter->SetTransform(isotran);
      painter->DrawArc(0, 0, radius, 0.f, 360.f);
      painter->PopMatrix();
      }

    return true;
  }

  void GetBounds(double bounds[4])
  {
    if(m_Model)
      {
      float t0, y0, t1, y1;
      IntensityCurveInterface *curve = m_Model->GetCurve();
      curve->GetControlPoint(0, t0, y0);
      curve->GetControlPoint(curve->GetControlPointCount() - 1, t1, y1);
      Vector2d range = m_Model->GetNativeImageRangeForCurve();

      // Compute the range over which the curve is plotted, where [0 1] is the
      // image intensity range
      float z0 = std::min(t0, 0.0f);
      float z1 = std::max(t1, 1.0f);

      // Compute the range over which the curve is plotted, in intensity units
      float x0 = range[0] * (1 - z0) + range[1] * z0;
      float x1 = range[0] * (1 - z1) + range[1] * z1;

      bounds[0] = x0;
      bounds[1] = x1;
      bounds[2] = 0.0;
      bounds[3] = 1.0;
      }
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
    m_PenNormal = vtkSmartPointer<vtkPen>::New();
    m_PenNormal->SetColor(0, 0, 0, 255);
    m_PenNormal->SetWidth(2.0);

    m_BrushNormal = vtkSmartPointer<vtkBrush>::New();
    m_BrushNormal->SetColor(255, 0, 0, 255);

    m_BrushSelected = vtkSmartPointer<vtkBrush>::New();
    m_BrushSelected->SetColor(255, 255, 0, 255);

  }

  ~IntensityCurveControlPointsContextItem() {}

  IntensityCurveModel *m_Model;
  vtkSmartPointer<vtkPen> m_PenNormal, m_PenSelected;
  vtkSmartPointer<vtkBrush> m_BrushNormal, m_BrushSelected;
};

vtkStandardNewMacro(IntensityCurveControlPointsContextItem);


IntensityCurveVTKRenderer::IntensityCurveVTKRenderer()
{
  this->m_RenderWindow->SetMultiSamples(0);
  this->m_RenderWindow->SetLineSmoothing(1);
  this->m_RenderWindow->SetPolygonSmoothing(1);

  m_Model = NULL;

  // Set up the scene for rendering
  m_Chart = vtkSmartPointer<vtkChartXY>::New();

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

  // Set up the plot
  m_CurvePlot = m_Chart->AddPlot(vtkChart::LINE);
  m_CurvePlot->SetInput(m_PlotTable, 0, 1);
  m_CurvePlot->SetColor(1, 0, 0);
  m_CurvePlot->SetWidth(1.0);
  m_CurvePlot->GetYAxis()->SetBehavior(vtkAxis::FIXED);
  m_CurvePlot->GetYAxis()->SetMinimum(-0.05);
  m_CurvePlot->GetYAxis()->SetMaximum(1.05);
  m_CurvePlot->GetXAxis()->SetTitle("Image Intensity");
  m_CurvePlot->GetYAxis()->SetTitle("Output Intensity");
  m_CurvePlot->GetXAxis()->SetBehavior(vtkAxis::FIXED);

  // TODO: we could also render a histogram here

  m_Controls = vtkSmartPointer<IntensityCurveControlPointsContextItem>::New();
  m_Chart->AddPlot(m_Controls);

  // Set the background to white
  m_BackgroundColor.fill(1.0);
}

IntensityCurveVTKRenderer::~IntensityCurveVTKRenderer()
{
}

void
IntensityCurveVTKRenderer
::SetModel(IntensityCurveModel *model)
{
  m_Model = model;

  m_Controls->SetModel(model);

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
    float margin = (x1 - x0) / 20.0;
    m_CurvePlot->GetXAxis()->SetMinimum(x0 - margin);
    m_CurvePlot->GetXAxis()->SetMaximum(x1 + margin);

    // Set the terminal points of the curve as well
    m_CurveX->SetValue(0, x0 - margin);
    m_CurveY->SetValue(0, 0.0);
    m_CurveX->SetValue(CURVE_RESOLUTION+2, x1 + margin);
    m_CurveY->SetValue(CURVE_RESOLUTION+2, 1.0);

    m_PlotTable->Modified();

    // Update the control points
    /*
    m_ControlTable->SetNumberOfRows(curve->GetControlPointCount());
    for(int i = 0; i < curve->GetControlPointCount(); i++)
      {
      float t, y;
      curve->GetControlPoint(i, t, y);
      float x = range[0] * (1.0 - t) + range[1] * t;
      m_ControlX->SetValue(i, x);
      m_ControlY->SetValue(i, y);
      m_ControlTable->Modified();

      vtkSmartPointer<vtkIdTypeArray> sel = vtkSmartPointer<vtkIdTypeArray>::New();
      int id;
      if(m_Model->GetMovingControlIdModel()->GetValueAndDomain(id, NULL))
        {
        sel->Allocate(1);
        sel->InsertNextValue(id);
        }
      m_ControlPlot->SetSelection(sel);
      }
      */

    m_Chart->RecalculateBounds();


    }
}

void
IntensityCurveVTKRenderer
::OnUpdate()
{
  UpdatePlotValues();
}
