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

  virtual void Initialize() { this->Initialized = 1; this->Enable(); }
  // virtual void Start() { }
  // virtual void TerminateApp() { }


protected:

  QtRenderWindowInteractor() {}
  virtual ~QtRenderWindowInteractor() {}

};

vtkStandardNewMacro(QtRenderWindowInteractor);



AbstractVTKRenderer::AbstractVTKRenderer()
{
  // Create a VTK renderer
  m_Renderer = vtkSmartPointer<vtkRenderer>::New();

  // Set up a render window that uses GL commands to paint
  m_RenderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();

  // Add the renderer to the window
  m_RenderWindow->AddRenderer(m_Renderer);

  // Set up the interactor
  m_Interactor = vtkSmartPointer<QtRenderWindowInteractor>::New();
  m_Interactor->SetRenderWindow(m_RenderWindow);
  m_Interactor->SetInteractorStyle(NULL);

  // Set the pixel ratio
  m_DevicePixelRatio = 1;
}

void AbstractVTKRenderer::paintGL()
{
  // Update the scene
  this->Update();

  // Make sure the interactor is enabled
  if(!m_Interactor->GetInitialized())
    {
    m_Interactor->Initialize();
    m_Interactor->Enable();
    }

  // Clear the screen
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Do the rendering but only when interactor is enabled (from QVTKWidget2)
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

void AbstractVTKRenderer::resizeGL(int w, int h, int device_pixel_ratio)
{
  // Pass the size to VTK
  m_RenderWindow->SetSize(w, h);
  m_Interactor->UpdateSize(w, h);

  if(m_DevicePixelRatio != device_pixel_ratio)
    {
    int old_ratio = m_DevicePixelRatio;
    m_DevicePixelRatio = device_pixel_ratio;
    this->OnDevicePixelRatioChange(old_ratio, device_pixel_ratio);
    }
}
