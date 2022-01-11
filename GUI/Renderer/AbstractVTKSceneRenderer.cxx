#include "AbstractVTKSceneRenderer.h"

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkContextActor.h>
#include <vtkChart.h>
#include <vtkChartXY.h>
#include <vtkTextProperty.h>
#include <vtkTooltipItem.h>
#include <vtkPlot.h>
#include <vtkPen.h>
#include <vtkAxis.h>
#include <vtkRenderer.h>
#include <vtkContextInteractorStyle.h>
#include <vtkContextScene.h>
#include <vtkRenderWindowInteractor.h>


AbstractVTKSceneRenderer::AbstractVTKSceneRenderer()
  : AbstractVTKRenderer()
{
  // Set up the context actor/scene
  m_ContextActor = vtkSmartPointer<vtkContextActor>::New();
  m_Renderer->AddViewProp(m_ContextActor);

  // This seems to be a bug in VTK that we have to assign the renderer to the scene
  // TODO: check if this is necessary in VTK9, in VTK8 this causes scene->GetViewWidth() to return 0
  m_ContextActor->GetScene()->SetRenderer(m_Renderer);
}

void AbstractVTKSceneRenderer::SetRenderWindow(vtkRenderWindow *rwin)
{
  // Call parent method
  Superclass::SetRenderWindow(rwin);

  // Configure interaction
  vtkNew<vtkContextInteractorStyle> style;
  style->SetScene(m_ContextActor->GetScene());
  rwin->GetInteractor()->SetInteractorStyle(style);
}

vtkContextScene *AbstractVTKSceneRenderer::GetScene()
{
  return m_ContextActor->GetScene();
}

void AbstractVTKSceneRenderer::UpdateChartDevicePixelRatio(
    vtkChart *chart, int old_ratio, int new_ratio)
{
  /*
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
    */
}

