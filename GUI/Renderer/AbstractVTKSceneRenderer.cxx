#include "AbstractVTKSceneRenderer.h"

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkContextView.h>
#include <vtkChart.h>
#include <vtkChartXY.h>
#include <vtkTextProperty.h>
#include <vtkTooltipItem.h>
#include <vtkPlot.h>
#include <vtkPen.h>
#include <vtkAxis.h>
#include <vtkContextInteractorStyle.h>
#include <vtkRenderWindowInteractor.h>


AbstractVTKSceneRenderer::AbstractVTKSceneRenderer()
  : AbstractVTKRenderer()
{
  // Set up the context view
  m_ContextView = vtkSmartPointer<vtkContextView>::New();

  // Set the background to black
  m_BackgroundColor.fill(0.0);
}

void AbstractVTKSceneRenderer::SetRenderWindow(vtkRenderWindow *rwin)
{
  // Call parent method
  AbstractVTKRenderer::SetRenderWindow(rwin);

  // Initialize some properties of the renderer
  rwin->SwapBuffersOff();
  rwin->SetMultiSamples(0);

  // Assign to context view
  m_ContextView->SetRenderWindow(m_RenderWindow);
  m_ContextView->SetInteractor(m_RenderWindow->GetInteractor());
  vtkNew<vtkContextInteractorStyle> style;
  style->SetScene(m_ContextView->GetScene());
  m_ContextView->GetInteractor()->SetInteractorStyle(style);

  std::cout << "INTERACTOR on " << m_ContextView << " SET TO " << m_RenderWindow->GetInteractor() << std::endl;
}

void AbstractVTKSceneRenderer::UpdateChartDevicePixelRatio(
    vtkChart *chart, int old_ratio, int new_ratio)
{
  // Axis titles
  const int axis_arr[] = {vtkAxis::LEFT, vtkAxis::RIGHT, vtkAxis::BOTTOM, vtkAxis::TOP};
  for(int i = 0; i < 4; i++)
    {
    if(chart->GetAxis(axis_arr[i]))
      {
      vtkTextProperty *propl = chart->GetAxis(axis_arr[i])->GetLabelProperties();
      propl->SetFontSize(new_ratio * propl->GetFontSize() / old_ratio);

      vtkTextProperty *propt = chart->GetAxis(axis_arr[i])->GetTitleProperties();
      propt->SetFontSize(new_ratio * propt->GetFontSize() / old_ratio);
      }
    }

  // Main chart title
  vtkTextProperty *prop = chart->GetTitleProperties();
  prop->SetFontSize(new_ratio * prop->GetFontSize() / old_ratio);

  // Tooltip
  vtkChartXY *chart_xy = dynamic_cast<vtkChartXY *>(chart);
  if(chart_xy)
    {
    vtkTextProperty *prop_tt = chart_xy->GetTooltip()->GetTextProperties();
    prop_tt->SetFontSize(new_ratio * prop_tt->GetFontSize() / old_ratio);
    }
}

void AbstractVTKSceneRenderer::paintGL()
{
  // Set renderer background
  m_ContextView->GetRenderer()->SetBackground(m_BackgroundColor.data_block());

  // Update the scene
  AbstractVTKRenderer::paintGL();
}
