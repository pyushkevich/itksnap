#include "OptimizationProgressRenderer.h"

#include "ImageIOWizardModel.h"

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


OptimizationProgressRenderer::OptimizationProgressRenderer()
{
  m_Model = NULL;

  // Set up the scene for rendering
  m_Chart = vtkSmartPointer<vtkChartXY>::New();
  m_Chart->SetActionToButton(vtkChartXY::PAN, vtkContextMouseEvent::LEFT_BUTTON);
  m_Chart->SetActionToButton(vtkChartXY::ZOOM, vtkContextMouseEvent::RIGHT_BUTTON);

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
  m_Plot->GetXAxis()->SetTitle("Optimization iteration");
  m_Plot->GetXAxis()->SetBehavior(vtkAxis::FIXED);
  m_Plot->GetYAxis()->SetTitle("Metric value");

  // Set the background to white
  m_BackgroundColor.fill(1.0);

  // Customize the render window
  this->m_RenderWindow->SetMultiSamples(0);
  this->m_RenderWindow->SetLineSmoothing(1);
  this->m_RenderWindow->SetPolygonSmoothing(1);
}

void OptimizationProgressRenderer::SetModel(ImageIOWizardModel *model)
{
  m_Model = model;

  // Rebroadcast the relevant events from the model in order for the
  // widget that uses this renderer to cause an update
  Rebroadcast(model, ImageIOWizardModel::RegistrationProgressEvent(), ModelUpdateEvent());

  // Reset value range

}

void OptimizationProgressRenderer::OnUpdate()
{
  int x = m_DataX->GetNumberOfTuples()+1;
  double y = m_Model->GetRegistrationObjective();

  m_DataX->InsertNextValue(x);
  m_DataY->InsertNextValue(y);
  m_PlotTable->Modified();

  m_Plot->GetXAxis()->SetRange(0.0, ((x + 5) / 40 + 1) * 40.0);


  if (x == 1 || y <= m_MinValue)
    {
    m_MinValue = (((int) floor(y * 10)) - 1) * 0.1;
    }
  if (x == 1 || y >= m_MaxValue)
    {
    m_MaxValue = (((int) ceil(y * 10)) + 1) * 0.1;
    }

  m_Plot->GetYAxis()->SetRange(m_MinValue, m_MaxValue);
}
