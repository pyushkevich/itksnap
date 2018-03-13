#include "IntensityUnderCursorRenderer.h"

#include <vtkChartXY.h>
#include <vtkPlot.h>
#include <vtkDoubleArray.h>
#include <vtkTable.h>
#include <vtkContextView.h>
#include <vtkContextScene.h>
#include <vtkAxis.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkChartLegend.h>
#include "ImageInfoModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"

void IntensityUnderCursorRenderer::SetModel(ImageInfoModel *model)
{
  m_Model = model;

  // Crosshair changes and layer changes are captured by this
  Rebroadcast(m_Model, ModelUpdateEvent(), ModelUpdateEvent());
}

void IntensityUnderCursorRenderer::OnUpdate()
{
  m_Model->Update();
  if(m_Model && m_Model->GetLayer() && !m_Model->GetLayer()->IsScalar())
    this->UpdatePlotValues();
}

void IntensityUnderCursorRenderer::UpdatePlotValues()
{
  // There must be a currently selected layer
  VectorImageWrapperBase *wrapper =
      dynamic_cast<VectorImageWrapperBase *>(m_Model->GetLayer());
  if(wrapper)
    {
    if(wrapper->GetNumberOfComponents() != m_CurveX->GetNumberOfTuples())
      {
      m_CurveX->SetNumberOfValues(wrapper->GetNumberOfComponents());
      m_CurveY->SetNumberOfValues(wrapper->GetNumberOfComponents());
      for(int i = 0; i <  wrapper->GetNumberOfComponents(); i++)
        m_CurveX->SetValue(i, i+1);
      }

    // Get the data with voxel intensities
    wrapper->GetVoxelMappedToNative(wrapper->GetSliceIndex(), m_CurveY->GetPointer(0));

    // Set the y range
    m_CurvePlot->GetYAxis()->SetRange(wrapper->GetImageMinNative(), wrapper->GetImageMaxNative());

    // Set the x range
    m_CurvePlot->GetXAxis()->SetMinimumLimit(0.4);
    m_CurvePlot->GetXAxis()->SetMinimum(0.4);
    m_CurvePlot->GetXAxis()->SetMaximumLimit(wrapper->GetNumberOfComponents() + 0.6);
    m_CurvePlot->GetXAxis()->SetMaximum(wrapper->GetNumberOfComponents() + 0.6);
    m_CurvePlot->GetXAxis()->SetCustomTickPositions(m_CurveX);

    // Set the y range
    m_CurvePlot->GetYAxis()->SetMinimumLimit(wrapper->GetImageMinNative());
    m_CurvePlot->GetYAxis()->SetMinimum(wrapper->GetImageMinNative());
    m_CurvePlot->GetYAxis()->SetMaximumLimit(wrapper->GetImageMaxNative());
    m_CurvePlot->GetYAxis()->SetMaximum(wrapper->GetImageMaxNative());
    }
  else
    {
    m_CurveX->Reset();
    m_CurveY->Reset();
    }

  // Mark plot table as modified
  m_PlotTable->Modified();
  m_Chart->RecalculateBounds();
}

void IntensityUnderCursorRenderer::paintGL()
{
  if(m_Model && m_Model->GetLayer() && !m_Model->GetLayer()->IsScalar())
    Superclass::paintGL();
}

void IntensityUnderCursorRenderer::OnDevicePixelRatioChange(int old_ratio, int new_ratio)
{
  this->UpdateChartDevicePixelRatio(m_Chart, old_ratio, new_ratio);
}

IntensityUnderCursorRenderer::IntensityUnderCursorRenderer()
{
  this->m_RenderWindow->SetMultiSamples(0);
  this->m_RenderWindow->SetLineSmoothing(1);
  this->m_RenderWindow->SetPolygonSmoothing(1);

  m_Model = NULL;

  // Set up the scene for rendering
  m_Chart = vtkSmartPointer<vtkChartXY>::New();

  // Configure chart behavior
  m_Chart->ForceAxesToBoundsOff();
  m_Chart->SetShowLegend(false);
  m_Chart->GetLegend()->SetDragEnabled(false);

  // Add the chart to the renderer
  m_ContextView->GetScene()->AddItem(m_Chart);

  // Set up the data
  m_CurveX = vtkSmartPointer<vtkDoubleArray>::New();
  m_CurveX->SetName("Component");
  m_CurveY = vtkSmartPointer<vtkDoubleArray>::New();
  m_CurveY->SetName("Intensity");

  // Set up the table
  m_PlotTable = vtkSmartPointer<vtkTable>::New();
  m_PlotTable->AddColumn(m_CurveX);
  m_PlotTable->AddColumn(m_CurveY);

  // Set up the curve plot
  m_CurvePlot = m_Chart->AddPlot(vtkChart::BAR);
  m_CurvePlot->SetInputData(m_PlotTable, 0, 1);

  m_CurvePlot->GetXAxis()->SetBehavior(vtkAxis::FIXED);
  m_CurvePlot->GetXAxis()->SetTitle("Component");
  m_CurvePlot->GetYAxis()->SetBehavior(vtkAxis::FIXED);
  m_CurvePlot->GetYAxis()->SetTitle("Intensity");
}

IntensityUnderCursorRenderer::~IntensityUnderCursorRenderer()
{

}
