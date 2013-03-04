#include "AbstractVTKSceneRenderer.h"

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkContextView.h>

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

void AbstractVTKSceneRenderer::paintGL()
{
  // Set renderer background
  m_ContextView->GetRenderer()->SetBackground(m_BackgroundColor.data_block());

  // Update the scene
  AbstractVTKRenderer::paintGL();
}
