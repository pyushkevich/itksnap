#include "OptimizationProgressRenderer.h"

#include "RegistrationModel.h"

#include <vtkChartXY.h>
#include <vtkPlot.h>
#include <vtkPlotBar.h>
#include <vtkFloatArray.h>
#include <vtkTable.h>
#include <vtkContextView.h>
#include <vtkContextScene.h>
#include <vtkAxis.h>
#include <vtkContextMouseEvent.h>
#include "vtkGenericOpenGLRenderWindow.h"

#include <cstdlib>
#include <algorithm>

OptimizationProgressRenderer::OptimizationProgressRenderer()
{
  m_Model = NULL;

  // Set up the scene for rendering
  m_Chart = vtkSmartPointer<vtkChartXY>::New();
  m_Chart->SetActionToButton(vtkChartXY::PAN, vtkContextMouseEvent::LEFT_BUTTON);
  m_Chart->SetActionToButton(vtkChartXY::ZOOM_AXIS, vtkContextMouseEvent::RIGHT_BUTTON);

  // Add the chart to the renderer
  m_ContextView->GetScene()->AddItem(m_Chart);

  // Set up the data
  m_DataX = vtkSmartPointer<vtkFloatArray>::New();
  m_DataX->SetName("Optimization Iteration");
  m_DataY = vtkSmartPointer<vtkFloatArray>::New();
  m_DataY->SetName("Metric Value");

  // Set up the table
  m_PlotTable = vtkSmartPointer<vtkTable>::New();
  m_PlotTable->AddColumn(m_DataX);
  m_PlotTable->AddColumn(m_DataY);
  m_PlotTable->SetNumberOfRows(0);

  // Set up the plot
  m_Plot = m_Chart->AddPlot(vtkChart::LINE);
  m_Plot->SetInputData(m_PlotTable, 0, 1);
  m_Plot->SetColor(1, 0, 0);
  m_Plot->SetWidth(2.0);
  m_Plot->GetYAxis()->SetBehavior(vtkAxis::FIXED);
  m_Plot->GetYAxis()->SetMinimum(-0.05);
  m_Plot->GetYAxis()->SetMaximum(1.05);
  m_Plot->GetXAxis()->SetTitle("Iteration");
  m_Plot->GetXAxis()->SetBehavior(vtkAxis::FIXED);
  m_Plot->GetYAxis()->SetTitle("Metric");

  // Set the background to white
  m_BackgroundColor.fill(1.0);

  // Customize the render window
  this->m_RenderWindow->SetMultiSamples(0);
  this->m_RenderWindow->SetLineSmoothing(1);
  this->m_RenderWindow->SetPolygonSmoothing(1);

  m_PyramidLevel = 0;
}

void OptimizationProgressRenderer::SetModel(RegistrationModel *model)
{
  m_Model = model;

  // Rebroadcast the relevant events from the model in order for the
  // widget that uses this renderer to cause an update
  Rebroadcast(model->GetLastMetricValueModel(), ValueChangedEvent(), ModelUpdateEvent());

}

void OptimizationProgressRenderer::OnUpdate()
{
  // Get the metric log
  const RegistrationModel::MetricLog &mlog = m_Model->GetRegistrationMetricLog();

  // Set the data points
  m_DataX->Reset();
  m_DataY->Reset();

  int x = 0;
  if(mlog.size() > m_PyramidLevel)
    {
    for(int i = 0; i < mlog[m_PyramidLevel].size(); i++, x++)
      {
      double y = mlog[m_PyramidLevel][i].TotalMetric;
      m_DataX->InsertNextValue(x);
      m_DataY->InsertNextValue(y);

      m_MinValue = (x == 0) ? y : std::min(m_MinValue, y);
      m_MaxValue = (x == 0) ? y : std::max(m_MaxValue, y);
      }
    }

  m_PlotTable->Modified();

  m_Plot->GetXAxis()->SetRange(0.0, ((m_DataX->GetNumberOfTuples()+ 5) / 20 + 1) * 20.0);

  double min_rnd = (((int) floor(m_MinValue * 10)) - 1) * 0.1;
  double max_rnd = (((int) ceil(m_MaxValue * 10)) + 1) * 0.1;

  m_Plot->GetYAxis()->SetRange(min_rnd, max_rnd);

  char plotLabel[64];
  sprintf(plotLabel, "%dx Level", m_PyramidZoom);
  m_Chart->SetTitle(plotLabel);
}
