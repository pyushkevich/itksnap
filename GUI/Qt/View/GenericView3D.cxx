#include "GenericView3D.h"
#include "Generic3DModel.h"
#include "Generic3DRenderer.h"

#include "vtkGenericRenderWindowInteractor.h"
#include <QEvent>
#include <QMouseEvent>
#include <vtkInteractorStyle.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkCommand.h>

GenericView3D::GenericView3D(QWidget *parent) :
    SNAPQGLWidget(parent)
{
  // Create the renderer
  m_Renderer = Generic3DRenderer::New();

  // Repaint ourselves when renderer updates
  connectITK(m_Renderer, ModelUpdateEvent());

  iren = vtkGenericRenderWindowInteractor::New();
  iren->SetRenderWindow(m_Renderer->GetRenderWindow());

  // context events
  m_Renderer->GetRenderWindow()->AddObserver(
        vtkCommand::WindowMakeCurrentEvent,
        this, &GenericView3D::RendererCallback);

  m_Renderer->GetRenderWindow()->AddObserver(
        vtkCommand::WindowIsCurrentEvent,
        this, &GenericView3D::RendererCallback);

  vtkSmartPointer<vtkInteractorStyleTrackballCamera> inter =
      vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
  iren->SetInteractorStyle(inter);
  iren->Initialize();

  m_Dragging = false;
}

GenericView3D::~GenericView3D()
{
  iren->Delete();

}

void GenericView3D::RendererCallback(
    vtkObject *src, unsigned long event, void *data)
{
  if(event == vtkCommand::WindowMakeCurrentEvent)
    {
    this->makeCurrent();
    }
  else if(event == vtkCommand::WindowIsCurrentEvent)
    {
    bool *result = static_cast<bool *>(data);
    *result = QGLContext::currentContext() == this->context();
    }
}

void GenericView3D::SetModel(Generic3DModel *model)
{
  m_Model = model;
  m_Renderer->SetModel(model);

  // Listen to updates on the model
  connectITK(m_Model, ModelUpdateEvent());
}

AbstractRenderer * GenericView3D::GetRenderer() const
{
  return m_Renderer;
}

void GenericView3D::onModelUpdate(const EventBucket &bucket)
{
  m_Model->Update();
  m_Renderer->Update();
  this->repaint();
}

bool GenericView3D::event(QEvent *ev)
{
  QMouseEvent *emouse = dynamic_cast<QMouseEvent *>(ev);
  if(emouse)
    {
    iren->SetEventInformationFlipY(emouse->posF().x(), emouse->posF().y());
    if(ev->type() == QEvent::MouseButtonPress)
      {
      m_Dragging = true;
      m_DragButton = emouse->button();
      switch(emouse->button())
        {
        case Qt::LeftButton:
          iren->LeftButtonPressEvent();
          this->repaint();
          return true;
        case Qt::RightButton:
          iren->RightButtonPressEvent();
          this->repaint();
          return true;
        }
      }
    else if(ev->type() == QEvent::MouseButtonRelease)
      {
      m_Dragging = false;
      switch(emouse->button())
        {
        case Qt::LeftButton:
          iren->LeftButtonReleaseEvent();
          this->repaint();
          return true;
        case Qt::RightButton:
          iren->RightButtonReleaseEvent();
          this->repaint();
          return true;
        }
      }
    else if(ev->type() == QEvent::MouseMove && m_Dragging)
      {
      iren->MouseMoveEvent();
      this->repaint();
      return true;
      }
    }

  return SNAPQGLWidget::event(ev);
}

void GenericView3D::resizeGL(int w, int h)
{
  SNAPQGLWidget::resizeGL(w,h);
  iren->SetSize(w, h);
}
