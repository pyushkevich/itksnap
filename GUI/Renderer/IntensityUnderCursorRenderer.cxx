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
    int n = wrapper->GetNumberOfComponents();

    if(n != m_CurveX->GetNumberOfTuples())
      {
      m_CurveX->SetNumberOfValues(n);
      m_CurveY->SetNumberOfValues(n);

      for(int i = 0; i < n; i++)
        m_CurveX->SetValue(i, i+1);

      if(n < 10)
        {
        m_CurvePlot->GetXAxis()->SetCustomTickPositions(m_CurveX);
        }
      else
        {
        m_CurvePlot->GetXAxis()->SetCustomTickPositions(NULL);
        }

      // Limit the maximum width
      }

    // Set the selected bar
    AbstractMultiChannelDisplayMappingPolicy *dp =
        dynamic_cast<AbstractMultiChannelDisplayMappingPolicy *>(wrapper->GetDisplayMapping());
    if(dp && dp->GetDisplayMode().IsSingleComponent())
      {
      m_Selection->SetNumberOfValues(1);
      m_Selection->SetValue(0, dp->GetDisplayMode().SelectedComponent);
      }
    else
      {
      m_Selection->SetNumberOfValues(0);
      }

    // Get the data with voxel intensities
    wrapper->GetVoxelMappedToNative(wrapper->GetSliceIndex(), m_CurveY->GetPointer(0));

    // Set the x range
    m_CurvePlot->GetXAxis()->SetMinimumLimit(0.4);
    m_CurvePlot->GetXAxis()->SetMinimum(0.4);
    m_CurvePlot->GetXAxis()->SetMaximumLimit(wrapper->GetNumberOfComponents() + 0.6);
    m_CurvePlot->GetXAxis()->SetMaximum(wrapper->GetNumberOfComponents() + 0.6);

    // Get the intensity range
    Vector2d y_range = dp ? dp->GetCurveMinMaxNative() :
                            Vector2d(wrapper->GetImageMinNative(),
                                     wrapper->GetImageMaxNative());

    // Set the y range
    m_CurvePlot->GetYAxis()->SetMinimumLimit(y_range[0]);
    m_CurvePlot->GetYAxis()->SetMinimum(y_range[0]);
    m_CurvePlot->GetYAxis()->SetMaximumLimit(y_range[1]);
    m_CurvePlot->GetYAxis()->SetMaximum(y_range[1]);

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
  m_CurvePlot->GetXAxis()->SetTitle("Component");
  m_CurvePlot->GetYAxis()->SetBehavior(vtkAxis::FIXED);
  m_CurvePlot->GetYAxis()->SetTitle("Intensity");
}

IntensityUnderCursorRenderer::~IntensityUnderCursorRenderer()
{

}
