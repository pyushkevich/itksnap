#include "ThresholdSettingsRenderer.h"
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


const unsigned int ThresholdSettingsRenderer::NUM_POINTS = 256;

ThresholdSettingsRenderer::ThresholdSettingsRenderer()
  : AbstractVTKSceneRenderer()
{
  m_Model = NULL;

  // Set up the scene for rendering
  m_Chart = vtkSmartPointer<vtkChartXY>::New();

  // Add the chart to the renderer
  m_ContextView->GetScene()->AddItem(m_Chart);

  // Set up the data
  m_DataX = vtkSmartPointer<vtkFloatArray>::New();
  m_DataX->SetName("Grayscale Intensity");
  m_DataY = vtkSmartPointer<vtkFloatArray>::New();
  m_DataY->SetName("Speed Image Value");

  // Set up the table
  m_PlotTable = vtkSmartPointer<vtkTable>::New();
  m_PlotTable->AddColumn(m_DataX);
  m_PlotTable->AddColumn(m_DataY);
  m_PlotTable->SetNumberOfRows(NUM_POINTS);

  // Set up the plot
  vtkPlot *plot = m_Chart->AddPlot(vtkChart::LINE);
  plot->SetInput(m_PlotTable, 0, 1);
  plot->SetColor(1, 0, 0);
  plot->SetWidth(1.0);
  plot->GetYAxis()->SetBehavior(vtkAxis::FIXED);
  plot->GetYAxis()->SetMinimum(-1.05);
  plot->GetYAxis()->SetMaximum(1.05);
  plot->GetXAxis()->SetTitle("Input Image Intensity");
  plot->GetYAxis()->SetTitle("Speed Value");

  // TODO: we could also render a histogram here

  // Set the background to white
  m_BackgroundColor.fill(1.0);

}

void ThresholdSettingsRenderer::SetModel(SnakeWizardModel *model)
{
  this->m_Model = model;

  // Rebroadcast the relevant events from the model in order for the
  // widget that uses this renderer to cause an update
  Rebroadcast(model, SnakeWizardModel::ThresholdSettingsUpdateEvent(),
              ModelUpdateEvent());

  // TODO: We also need to listen to appearance changes....
}

void ThresholdSettingsRenderer::UpdatePlotValues()
{
  if(m_Model->CheckState(SnakeWizardModel::UIF_THESHOLDING_ENABLED))
    {
    // Update the x and y coordinates
    for(unsigned int i = 0; i < NUM_POINTS; i++)
      {
      double t = i * 1.0 / (NUM_POINTS - 1);
      double x, y;
      m_Model->EvaluateThresholdFunction(t, x, y);
      m_PlotTable->SetValue(i, 0, x);
      m_PlotTable->SetValue(i, 1, y);
      }

    m_PlotTable->Modified();
    }
}


void ThresholdSettingsRenderer::OnUpdate()
{
  // Update if any events took place
  this->UpdatePlotValues();

}

