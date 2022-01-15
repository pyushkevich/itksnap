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

  // Set the pixel ratio
  m_DevicePixelRatio = 1;
}

void AbstractVTKRenderer::SetRenderWindow(vtkRenderWindow *rwin)
{
  // Store the render window pointer
  m_RenderWindow = rwin;

  // Add the renderer to the window
  m_RenderWindow->AddRenderer(m_Renderer);

  // Set the interaction style
  m_RenderWindow->GetInteractor()->SetInteractorStyle(m_InteractionStyle);
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
  return m_RenderWindow->GetInteractor();
}

void AbstractVTKRenderer::SetInteractionStyle(AbstractVTKRenderer::InteractionStyle style)
{
  m_InteractionStyle = nullptr;
  switch(style)
    {
    case AbstractVTKRenderer::NO_INTERACTION:
      m_InteractionStyle = nullptr;
      break;
    case AbstractVTKRenderer::TRACKBALL_CAMERA:
      m_InteractionStyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
      break;
    case AbstractVTKRenderer::TRACKBALL_ACTOR:
      m_InteractionStyle = vtkSmartPointer<vtkInteractorStyleTrackballActor>::New();
      break;
    case AbstractVTKRenderer::PICKER:
      m_InteractionStyle = vtkSmartPointer<vtkInteractorStyleTrackballActor>::New();
      break;
    }

  if(m_RenderWindow)
    m_RenderWindow->GetInteractor()->SetInteractorStyle(m_InteractionStyle);
}

void AbstractVTKRenderer::SyncronizeCamera(Self *reference)
{
  // Make a copy of the camera
  m_Renderer->SetActiveCamera(reference->m_Renderer->GetActiveCamera());

  // Respond to modified events from the source interactor
  Rebroadcast(reference->GetRenderWindowInteractor(),
              vtkCommand::ModifiedEvent, ModelUpdateEvent());

  // And vice versa
  reference->Rebroadcast(this->GetRenderWindowInteractor(),
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

void AbstractVTKRenderer::OnWindowResize(int itkNotUsed(w), int itkNotUsed(h), int vppr)
{
  if(m_DevicePixelRatio != vppr)
    {
    int old_ratio = m_DevicePixelRatio;
    m_DevicePixelRatio = vppr;
    this->OnDevicePixelRatioChange(old_ratio, vppr);
    }
}

