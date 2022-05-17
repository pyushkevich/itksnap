#include "IntensityUnderCursorRenderer.h"

#include <vtkChartXY.h>
#include <vtkPlot.h>
#include <vtkDoubleArray.h>
#include <vtkIdTypeArray.h>
#include <vtkTable.h>
#include <vtkContextView.h>
#include <vtkContextScene.h>
#include <vtkAxis.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkChartLegend.h>
#include "ImageInfoModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "IntensityCurveInterface.h"

void IntensityUnderCursorRenderer::SetModel(ImageInfoModel *model)
{
  m_Model = model;

  // Crosshair changes and layer changes are captured by this
  Rebroadcast(m_Model, ModelUpdateEvent(), ModelUpdateEvent());

  // Metadata changes should also be captures
  Rebroadcast(m_Model->GetParentModel()->GetDriver(),
              WrapperDisplayMappingChangeEvent(),
              ModelUpdateEvent());
}

void IntensityUnderCursorRenderer::SetRenderWindow(vtkRenderWindow *rwin)
{
  Superclass::SetRenderWindow(rwin);
  rwin->SetMultiSamples(0);
  rwin->SetLineSmoothing(1);
  rwin->SetPolygonSmoothing(1);
}

void IntensityUnderCursorRenderer::OnUpdate()
{
  itkAssertOrThrowMacro(
        m_Model,
        "Model cannot be NULL in IntensityUnderCursorRenderer::OnUpdate");

  m_Model->Update();
  this->UpdatePlotValues();
}

void IntensityUnderCursorRenderer::UpdatePlotValues()
{
  // There must be a currently selected layer
  ImageWrapperBase *layer = dynamic_cast<ImageWrapperBase*>(m_Model->GetLayer());
  if(layer)
    {
    // Sample data from the wrapper in native format across all time points
    unsigned int nc = layer->GetNumberOfComponents();
    unsigned int nt = layer->GetNumberOfTimePoints();
    vnl_vector<double> samples(nc * nt, 0.0);
    layer->SampleIntensityAtReferenceIndex(layer->GetSliceIndex(), -1, true, samples);

    // Figure out which indices are currently selected
    std::vector<int> selection;

    // Are we looking at a single component?
    int act_comp = -1;
    AbstractMultiChannelDisplayMappingPolicy *dp =
        dynamic_cast<AbstractMultiChannelDisplayMappingPolicy *>(layer->GetDisplayMapping());
    if(dp && dp->GetDisplayMode().IsSingleComponent())
      act_comp = dp->GetDisplayMode().SelectedComponent;

    // Now highlight the selected component or range of components
    if(act_comp >= 0)
      {
      // One selected bar
      selection.push_back(layer->GetTimePointIndex() * nc + act_comp);
      }
    else if(nt > 1)
      {
      // A range of selected bars
      for(unsigned int c = 0; c < nc; c++)
        selection.push_back(layer->GetTimePointIndex() * nc + c);
      }

    // Fill out the curve
    unsigned int n = samples.size();
    if(n != m_CurveX->GetNumberOfTuples())
      {
      m_CurveX->SetNumberOfValues(n);
      m_CurveY->SetNumberOfValues(n);

      for(unsigned int i = 0; i < n; i++)
        m_CurveX->SetValue(i, i+1);

      if(n < 10)
        m_CurvePlot->GetXAxis()->SetCustomTickPositions(m_CurveX);
      else
        m_CurvePlot->GetXAxis()->SetCustomTickPositions(NULL);
      }

    // Set the y-values
    for(unsigned int i = 0; i < n; i++)
      m_CurveY->SetValue(i, samples[i]);

    // Set the selection
    m_Selection->SetNumberOfValues(selection.size());
    for(unsigned int s = 0; s < selection.size(); s++)
      m_Selection->SetValue(s, selection[s]);

    // Set the x range
    m_CurvePlot->GetXAxis()->SetMinimumLimit(0.4);
    m_CurvePlot->GetXAxis()->SetMinimum(0.4);
    m_CurvePlot->GetXAxis()->SetMaximumLimit(n + 0.6);
    m_CurvePlot->GetXAxis()->SetMaximum(n + 0.6);

    // Get the intensity range
    Vector2d y_range = dp ? dp->GetCurveMinMaxNative() :
                            Vector2d(layer->GetImageMinNative(),
                                     layer->GetImageMaxNative());

    // Set the y range
    m_CurvePlot->GetYAxis()->SetMinimumLimit(y_range[0]);
    m_CurvePlot->GetYAxis()->SetMinimum(y_range[0]);
    m_CurvePlot->GetYAxis()->SetMaximumLimit(y_range[1]);
    m_CurvePlot->GetYAxis()->SetMaximum(y_range[1]);

    // Set the axis labels
    std::string axis_name =
        nt > 1 ? (nc > 1 ? "Time Point x Component" : "Time Point") : "Component";
    m_CurvePlot->GetXAxis()->SetTitle(axis_name.c_str());
    }
  else
    {
    m_CurveX->Reset();
    m_CurveY->Reset();
    m_Selection->Reset();
    }

  // Mark plot table as modified
  m_PlotTable->Modified();
  m_Chart->RecalculateBounds();
}

void IntensityUnderCursorRenderer::OnDevicePixelRatioChange(int old_ratio, int new_ratio)
{
  this->UpdateChartDevicePixelRatio(m_Chart, old_ratio, new_ratio);
}

IntensityUnderCursorRenderer::IntensityUnderCursorRenderer()
{
  m_Model = NULL;

  // Set up the scene for rendering
  m_Chart = vtkSmartPointer<vtkChartXY>::New();

  // Configure chart behavior
  m_Chart->ForceAxesToBoundsOff();
  m_Chart->SetShowLegend(false);
  m_Chart->GetLegend()->SetDragEnabled(false);

  // Add the chart to the renderer
  this->GetScene()->AddItem(m_Chart);

  // Set up the data
  m_CurveX = vtkSmartPointer<vtkDoubleArray>::New();
  m_CurveX->SetName("Component");
  m_CurveY = vtkSmartPointer<vtkDoubleArray>::New();
  m_CurveY->SetName("Intensity");
  m_Selection = vtkSmartPointer<vtkIdTypeArray>::New();

  // Set up the table
  m_PlotTable = vtkSmartPointer<vtkTable>::New();
  m_PlotTable->AddColumn(m_CurveX);
  m_PlotTable->AddColumn(m_CurveY);

  // Set up the curve plot
  m_CurvePlot = m_Chart->AddPlot(vtkChart::BAR);
  m_CurvePlot->SetInputData(m_PlotTable, 0, 1);
  m_CurvePlot->SetSelection(m_Selection);

  m_CurvePlot->GetXAxis()->SetBehavior(vtkAxis::FIXED);
  m_CurvePlot->GetYAxis()->SetBehavior(vtkAxis::FIXED);
  m_CurvePlot->GetYAxis()->SetTitle("Intensity");
}

IntensityUnderCursorRenderer::~IntensityUnderCursorRenderer()
{

}
