#include "IntensityCurveVTKRenderer.h"

#include <vtkChartXY.h>
#include <vtkPlot.h>
#include <vtkFloatArray.h>
#include <vtkTable.h>
#include <vtkContextView.h>
#include <vtkContextScene.h>
#include <vtkAxis.h>
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
#include "vtkScalarsToColorsItem.h"
#include "vtkImageData.h"

#include "IntensityCurveModel.h"
#include "IntensityCurveInterface.h"
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
    if(m_Model)
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
      this->Texture->SetNumberOfScalarComponents(4);
      this->Texture->SetScalarTypeToUnsignedChar();
      this->Texture->AllocateScalars();
      }

    // Get the texture array
    unsigned char *data =
        reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0,0,0));
    Vector2d range = m_Model->GetNativeImageRangeForCurve();


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
      ColorMap::RGBAType rgba = m_ColorMapModel->GetColorMap()->MapIndexToRGBA(y);
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
    if(m_Model)
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
      this->Texture = vtkImageData::New();
      this->Texture->SetExtent(0, 0, 0, 0, 0, dim-1);
      this->Texture->SetNumberOfScalarComponents(4);
      this->Texture->SetScalarTypeToUnsignedChar();
      this->Texture->AllocateScalars();
      }

    // Get the texture array
    unsigned char *data =
        reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0,0,0));

    // Draw the texture. The texture is being drawn on the y-axis, so we use
    // simple interpolation
    for(int i = 0; i < dim; i++)
      {
      // Here x is the intensity value for which we want to look up the color
      double t = i * 1.0 / (dim - 1);

      // Map the intensity value to the colormap index
      ColorMap::RGBAType rgba = m_ColorMapModel->GetColorMap()->MapIndexToRGBA(t);
      for(int j = 0; j < 3; j++)
        *data++ = rgba[j];

      // Skip the opacity part
      *data++ = 255;
      }

    this->Texture->Modified();
  }
};



vtkStandardNewMacro(HorizontalColorMapContextItem)
vtkStandardNewMacro(VerticalColorMapContextItem)


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
      Vector2d vis_range = m_Model->GetVisibleImageRange();
      bounds[0] = vis_range[0];
      bounds[1] = vis_range[1];
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

  // Set up the histogram plot
  m_HistogramX = vtkSmartPointer<vtkFloatArray>::New();
  m_HistogramX->SetName("Image Intensity");
  m_HistogramY = vtkSmartPointer<vtkFloatArray>::New();
  m_HistogramY->SetName("Frequency");
  m_HistogramTable = vtkSmartPointer<vtkTable>::New();
  m_HistogramTable->AddColumn(m_HistogramX);
  m_HistogramTable->AddColumn(m_HistogramY);
  m_HistogramPlot = m_Chart->AddPlot(vtkChart::BAR);
  m_HistogramPlot->SetInput(m_HistogramTable, 0, 1);
  m_HistogramPlot->SetColor(0.8, 0.8, 1.0);

  // Set up the curve plot
  m_CurvePlot = m_Chart->AddPlot(vtkChart::LINE);
  m_CurvePlot->SetInput(m_PlotTable, 0, 1);
  m_CurvePlot->SetColor(1, 0, 0);
  m_CurvePlot->SetWidth(1.0);
  m_CurvePlot->GetYAxis()->SetBehavior(vtkAxis::FIXED);
  m_CurvePlot->GetYAxis()->SetMinimum(-0.1);
  m_CurvePlot->GetYAxis()->SetMaximum(1.05);
  m_CurvePlot->GetXAxis()->SetTitle("Image Intensity");
  m_CurvePlot->GetYAxis()->SetTitle("Output Intensity");
  m_CurvePlot->GetXAxis()->SetBehavior(vtkAxis::FIXED);

  // Set up the color map plot
  m_XColorMapItem = vtkSmartPointer<HorizontalColorMapContextItem>::New();
  m_YColorMapItem = vtkSmartPointer<VerticalColorMapContextItem>::New();
  m_Chart->AddPlot(m_XColorMapItem);
  m_Chart->AddPlot(m_YColorMapItem);

  m_Controls = vtkSmartPointer<IntensityCurveControlPointsContextItem>::New();
  m_Chart->AddPlot(m_Controls);
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
    m_CurvePlot->GetXAxis()->SetMinimum(x0 - margin);
    m_CurvePlot->GetXAxis()->SetMaximum(x1 + margin);

    // Set the terminal points of the curve as well
    m_CurveX->SetValue(0, x0 - margin);
    m_CurveY->SetValue(0, 0.0);
    m_CurveX->SetValue(CURVE_RESOLUTION+2, x1 + margin);
    m_CurveY->SetValue(CURVE_RESOLUTION+2, 1.0);

    m_PlotTable->Modified();

    // Compute the histogram entries
    const ScalarImageHistogram *histogram = m_Model->GetHistogram();
    m_HistogramTable->SetNumberOfRows(histogram->GetSize());

    // Figure out how to scale the histogram
    double yspan = histogram->GetMaxFrequency() *
        m_Model->GetProperties().GetHistogramCutoff();
    if(m_Model->GetProperties().IsHistogramLog() && yspan > 0)
      yspan = log10(yspan);
    double scaleFactor = 1.0 / yspan;
    for(int i = 0; i < histogram->GetSize(); i++)
      {
      m_HistogramX->SetValue(i, histogram->GetBinCenter(i));
      double y = histogram->GetFrequency(i);
      if(m_Model->GetProperties().IsHistogramLog() && y > 0)
        y = log10(y);
      m_HistogramY->SetValue(i, y * scaleFactor);
      }

    m_HistogramTable->Modified();

    m_XColorMapItem->Modified();
    m_YColorMapItem->Modified();

    m_Chart->RecalculateBounds();


    }
}

void
IntensityCurveVTKRenderer
::OnUpdate()
{
  UpdatePlotValues();
}
