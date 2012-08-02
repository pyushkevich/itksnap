#include "AbstractVTKSceneRenderer.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkContextView.h"

AbstractVTKSceneRenderer::AbstractVTKSceneRenderer()
  : AbstractRenderer()
{
  // Create a VTK renderer
  m_Renderer = vtkSmartPointer<vtkRenderer>::New();

  // Set up a render window that uses GL commands to paint
  m_RenderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
  m_RenderWindow->SwapBuffersOff();
  m_RenderWindow->AddRenderer(m_Renderer);
  m_RenderWindow->SetMultiSamples(0);

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
  this->Update();

  // Do the actual rendering!
  m_RenderWindow->PushState();
  m_RenderWindow->OpenGLInit();
  m_RenderWindow->Render();
  m_RenderWindow->PopState();
}

void AbstractVTKSceneRenderer::initializeGL()
{
  // Nothing to do here?
}

void AbstractVTKSceneRenderer::resizeGL(int w, int h)
{
  // Pass the size to VTK
  m_RenderWindow->SetSize(w, h);
}
