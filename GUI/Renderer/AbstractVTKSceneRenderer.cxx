#include "AbstractVTKSceneRenderer.h"

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkContextView.h>
#include <vtkChart.h>
#include <vtkTextProperty.h>

AbstractVTKSceneRenderer::AbstractVTKSceneRenderer()
  : AbstractVTKRenderer()
{
  // Initialize some properties of the renderer
  this->m_RenderWindow->SwapBuffersOff();
  this->m_RenderWindow->SetMultiSamples(0);

  // Set up the context view
  m_ContextView = vtkSmartPointer<vtkContextView>::New();
  m_ContextView->SetRenderWindow(m_RenderWindow);

  // Set the background to black
  m_BackgroundColor.fill(0.0);
}

#include <vtkPlot.h>
#include <vtkPen.h>
#include <vtkAxis.h>
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
}

void AbstractVTKSceneRenderer::paintGL()
{
  // Set renderer background
  m_ContextView->GetRenderer()->SetBackground(m_BackgroundColor.data_block());

  // Update the scene
  AbstractVTKRenderer::paintGL();
}
