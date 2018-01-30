#include "EdgePreprocessingSettingsRenderer.h"

#include <vtkChartXY.h>
#include <vtkPlot.h>
#include <vtkFloatArray.h>
#include <vtkTable.h>
#include <vtkContextView.h>
#include <vtkContextScene.h>
#include <vtkAxis.h>
#include <SnakeWizardModel.h>

#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

const unsigned int EdgePreprocessingSettingsRenderer::NUM_POINTS = 256;

EdgePreprocessingSettingsRenderer::EdgePreprocessingSettingsRenderer()
{
  m_Model = NULL;

  // Set up the scene for rendering
  m_Chart = vtkSmartPointer<vtkChartXY>::New();

  // Add the chart to the renderer
  m_ContextView->GetScene()->AddItem(m_Chart);

  // Set up the data
  m_DataX = vtkSmartPointer<vtkFloatArray>::New();
  m_DataX->SetName("Edge Strength (Gradient Mangitude)");
  m_DataY = vtkSmartPointer<vtkFloatArray>::New();
  m_DataY->SetName("Speed Image Value");

  // Set up the table
  m_PlotTable = vtkSmartPointer<vtkTable>::New();
  m_PlotTable->AddColumn(m_DataX);
  m_PlotTable->AddColumn(m_DataY);
  m_PlotTable->SetNumberOfRows(NUM_POINTS);

  // Set up the plot
  vtkPlot *plot = m_Chart->AddPlot(vtkChart::LINE);
  plot->SetInputData(m_PlotTable, 0, 1);
  plot->SetColor(1, 0, 0);
  plot->SetWidth(1.0);
  plot->GetYAxis()->SetBehavior(vtkAxis::FIXED);
  plot->GetYAxis()->SetMinimum(-0.05);
  plot->GetYAxis()->SetMaximum(1.05);
  plot->GetXAxis()->SetTitle("Edge Strength (Gradient Mangitude)");
  plot->GetYAxis()->SetTitle("Speed Value");
  plot->GetXAxis()->SetBehavior(vtkAxis::AUTO);

  // TODO: we could also render a histogram here

  // Set the background to white
  m_BackgroundColor.fill(1.0);

}

void EdgePreprocessingSettingsRenderer::OnDevicePixelRatioChange(int old_ratio, int new_ratio)
{
  this->UpdateChartDevicePixelRatio(m_Chart, old_ratio, new_ratio);
}

void EdgePreprocessingSettingsRenderer::SetModel(SnakeWizardModel *model)
{
  this->m_Model = model;

  // Rebroadcast the relevant events from the model in order for the
  // widget that uses this renderer to cause an update
  Rebroadcast(model, SnakeWizardModel::EdgePreprocessingSettingsUpdateEvent(),
              ModelUpdateEvent());
}

void EdgePreprocessingSettingsRenderer::UpdatePlotValues()
{
  if(m_Model->CheckState(SnakeWizardModel::UIF_EDGEPROCESSING_ENABLED))
    {
    // Compute the preprocessing function
    m_Model->EvaluateEdgePreprocessingFunction(NUM_POINTS,
                                               m_DataX->GetPointer(0),
                                               m_DataY->GetPointer(0));
    m_PlotTable->Modified();
    m_Chart->RecalculateBounds();
    }
}


void EdgePreprocessingSettingsRenderer::OnUpdate()
{
  // Update if any events took place
  this->UpdatePlotValues();
}

