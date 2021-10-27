#include "AbstractVTKRenderer.h"

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkGenericRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkInteractorStyleTrackballActor.h>
#include <vtkRenderer.h>
#include <vtkCommand.h>
#include <vtkObjectFactory.h>

class QtRenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
  static QtRenderWindowInteractor *New();

  vtkTypeMacro(QtRenderWindowInteractor, vtkRenderWindowInteractor);

  virtual void Initialize() override
  {
    this->Initialized = 1;
    this->Enable();
  }

protected:

  QtRenderWindowInteractor() {}
  virtual ~QtRenderWindowInteractor() {}

};

vtkStandardNewMacro(QtRenderWindowInteractor);



AbstractVTKRenderer::AbstractVTKRenderer()
{
  // Create a VTK renderer
  m_Renderer = vtkSmartPointer<vtkRenderer>::New();

  // Set up the interactor
  m_Interactor = vtkSmartPointer<QtRenderWindowInteractor>::New();
  m_Interactor->SetInteractorStyle(NULL);

  // Set the pixel ratio
  m_DevicePixelRatio = 1;
}

void AbstractVTKRenderer::SetRenderWindow(vtkRenderWindow *rwin)
{
  // Store the render window pointer
  m_RenderWindow = rwin;

  // Add the renderer to the window
  m_RenderWindow->AddRenderer(m_Renderer);

  // Add the interactor to the render window
  // TODO: fix this
  // m_Interactor->SetRenderWindow(m_RenderWindow);

}

vtkRenderer *AbstractVTKRenderer::GetRenderer()
{
  return m_Renderer;
}


vtkRenderWindow *AbstractVTKRenderer::GetRenderWindow()
{
  return m_RenderWindow;
}

vtkRenderWindowInteractor *AbstractVTKRenderer::GetRenderWindowInteractor()
{
  return m_Interactor;
}

void AbstractVTKRenderer::SetInteractionStyle(AbstractVTKRenderer::InteractionStyle style)
{
  vtkSmartPointer<vtkInteractorObserver> stylePtr = NULL;
  switch(style)
    {
    case AbstractVTKRenderer::NO_INTERACTION:
      stylePtr = NULL;
      break;
    case AbstractVTKRenderer::TRACKBALL_CAMERA:
      stylePtr = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
      break;
    case AbstractVTKRenderer::TRACKBALL_ACTOR:
      stylePtr = vtkSmartPointer<vtkInteractorStyleTrackballActor>::New();
      break;
    case AbstractVTKRenderer::PICKER:
      stylePtr = vtkSmartPointer<vtkInteractorStyleTrackballActor>::New();
      break;
    }
  m_Interactor->SetInteractorStyle(stylePtr);
}

void AbstractVTKRenderer::SyncronizeCamera(Self *reference)
{
  // Make a copy of the camera
  m_Renderer->SetActiveCamera(reference->m_Renderer->GetActiveCamera());

  // Respond to modified events from the source interactor
  Rebroadcast(reference->m_Interactor,
              vtkCommand::ModifiedEvent, ModelUpdateEvent());

  // And vice versa
  reference->Rebroadcast(m_Interactor,
                         vtkCommand::ModifiedEvent, ModelUpdateEvent());
}

void AbstractVTKRenderer::SetBackgroundColor(Vector3d color)
{
  m_Renderer->SetBackground(color[0], color[1], color[2]);
}

Vector3d AbstractVTKRenderer::GetBackgroundColor() const
{
  return Vector3d(m_Renderer->GetBackground());
}

