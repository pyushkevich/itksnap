#include "ThresholdSettingsRenderer.h"
#include <vtkChartXY.h>
#include <vtkPlot.h>
#include <vtkPlotBar.h>
#include <vtkFloatArray.h>
#include <vtkTable.h>
#include <vtkContextView.h>
#include <vtkContextScene.h>
#include <vtkAxis.h>
#include <SnakeWizardModel.h>
#include "LayerHistogramPlotAssembly.h"
#include "ImageWrapperBase.h"
#include "ScalarImageHistogram.h"

#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include "vtkGenericOpenGLRenderWindow.h"

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

  // Create the histogram assembly
  m_HistogramAssembly = new LayerHistogramPlotAssembly();
  m_HistogramAssembly->AddToChart(m_Chart);

  // Set up the plot
  m_Plot = m_Chart->AddPlot(vtkChart::LINE);
  m_Plot->SetInputData(m_PlotTable, 0, 1);
  m_Plot->SetColor(1, 0, 0);
  m_Plot->SetWidth(2.0);
  m_Plot->GetYAxis()->SetBehavior(vtkAxis::FIXED);
  m_Plot->GetYAxis()->SetMinimum(-0.05);
  m_Plot->GetYAxis()->SetMaximum(1.05);
  m_Plot->GetXAxis()->SetTitle("Input image intensity");
  m_Plot->GetXAxis()->SetBehavior(vtkAxis::FIXED);
  m_Plot->GetYAxis()->SetTitle("Threshold function");

  // Set the background to white
  m_BackgroundColor.fill(1.0);

  // Customize the render window
  this->m_RenderWindow->SetMultiSamples(0);
  this->m_RenderWindow->SetLineSmoothing(1);
  this->m_RenderWindow->SetPolygonSmoothing(1);
}

ThresholdSettingsRenderer::~ThresholdSettingsRenderer()
{
  // Create the histogram assembly
  delete m_HistogramAssembly;
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
    m_Model->EvaluateThresholdFunction(NUM_POINTS,
                                       m_DataX->GetPointer(0),
                                       m_DataY->GetPointer(0));

    ScalarImageWrapperBase *layer = m_Model->GetActiveScalarLayer(PREPROCESS_THRESHOLD);
    const ScalarImageHistogram *hist = layer->GetHistogram(0);
    m_HistogramAssembly->PlotWithFixedLimits(hist, 0.0, 1.0);

    m_Plot->GetXAxis()->SetRange(
          layer->GetImageMinNative() - hist->GetBinWidth(),
          layer->GetImageMaxNative() + hist->GetBinWidth());

    m_PlotTable->Modified();
    m_Chart->RecalculateBounds();
    }
}


void ThresholdSettingsRenderer::OnUpdate()
{
  // Update if any events took place
  this->UpdatePlotValues();

}

void ThresholdSettingsRenderer::OnDevicePixelRatioChange(int old_ratio, int new_ratio)
{
  this->UpdateChartDevicePixelRatio(m_Chart, old_ratio, new_ratio);
}



