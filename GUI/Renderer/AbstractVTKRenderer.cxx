#include "AbstractVTKRenderer.h"

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>

AbstractVTKRenderer::AbstractVTKRenderer()
{
  // Create a VTK renderer
  m_Renderer = vtkSmartPointer<vtkRenderer>::New();

  // Set up a render window that uses GL commands to paint
  m_RenderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();

  // Add the renderer to the window
  m_RenderWindow->AddRenderer(m_Renderer);
}

void AbstractVTKRenderer::paintGL()
{
  // Update the scene
  this->Update();

  // Do the actual rendering!
  m_RenderWindow->Render();
}

void AbstractVTKRenderer::initializeGL()
{
  // Is this what we should be calling?
  m_RenderWindow->OpenGLInit();
}

vtkRenderWindow *AbstractVTKRenderer::GetRenderWindow()
{
  return m_RenderWindow;
}

void AbstractVTKRenderer::resizeGL(int w, int h)
{
  // Pass the size to VTK
  m_RenderWindow->SetSize(w, h);
}
