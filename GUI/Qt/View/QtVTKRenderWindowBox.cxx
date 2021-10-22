#include "QtVTKRenderWindowBox.h"
#include "AbstractVTKRenderer.h"
#include "QtVTKInteractionDelegateWidget.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "QtReporterDelegates.h"
#include "LatentITKEventNotifier.h"
#include "SNAPQtCommon.h"

#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkSphereSource.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkProperty.h>

#if QT_VERSION >= 0x050000
  #include <QOpenGLContext>
  #include <QSurfaceFormat>
#else
  #include <QGLContext>
  #define QOpenGLContext QGLContext
#endif

QtVTKRenderWindowBox::QtVTKRenderWindowBox(QWidget *parent) :
  QVTKOpenGLNativeWidget(parent)
{
  //this->AttachSingleDelegate(m_InteractionDelegate);

  // Create a sphere
  sphereSource = vtkSmartPointer<vtkSphereSource>::New();
  sphereSource->SetCenter(0.0, 0.0, 0.0);
  sphereSource->SetRadius(5.0);
  sphereSource->SetPhiResolution(100);
  sphereSource->SetThetaResolution(100);

  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(sphereSource->GetOutputPort());

  actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  renderer = vtkSmartPointer<vtkRenderer>::New();
  renderer->AddActor(actor);
  renderer->ResetCamera();
  renderer->SetBackground(0.2,0.2,0.0);

  // Create a render window
  this->SetRenderWindow(vtkNew<vtkGenericOpenGLRenderWindow>());
  this->GetRenderWindow()->AddRenderer(renderer);

  // this->setFormat(QVTKOpenGLNativeWidget::defaultFormat());
  // m_InteractionDelegate = new QtVTKInteractionDelegateWidget(this);
  // m_InteractionDelegate->SetVTKInteractor(this->GetRenderWindow()->GetInteractor());
  // this->installEventFilter(m_InteractionDelegate);
}

void
QtVTKRenderWindowBox
::connectITK(itk::Object *src, const itk::EventObject &ev, const char *slot)
{
  LatentITKEventNotifier::connect(src, ev, this, slot);
}

void QtVTKRenderWindowBox::SetRenderer(AbstractVTKRenderer *renderer)
{
  m_Renderer = renderer;
  if(m_Renderer)
    {
    m_Renderer->SetRenderWindow(this->GetRenderWindow());
    connectITK(m_Renderer, ModelUpdateEvent());
    }
}

void QtVTKRenderWindowBox::initializeGL()
{
  // TODO: update model?
  QVTKOpenGLNativeWidget::initializeGL();
}

void QtVTKRenderWindowBox::resizeGL(int w, int h)
{
  // TODO: update model?
  QVTKOpenGLNativeWidget::resizeGL(w, h);
}

void QtVTKRenderWindowBox::paintGL()
{
  if(m_Renderer)
    m_Renderer->Update();

  QVTKOpenGLNativeWidget::paintGL();
}

void
QtVTKRenderWindowBox
::RendererCallback(
    vtkObject *src, unsigned long event, void *)
{
  if(event == vtkCommand::RenderEvent)
    {
    this->update();
    }
}

void QtVTKRenderWindowBox::enterEvent(QEvent *)
{
  if(m_GrabFocusOnEntry)
    this->setFocus();
}

void QtVTKRenderWindowBox::leaveEvent(QEvent *)
{
  if(m_GrabFocusOnEntry)
    this->clearFocus();
}

bool QtVTKRenderWindowBox::SaveScreenshot(std::string filename)
{
  m_ScreenshotRequest = from_utf8(filename);
  this->repaint();
  return true;
}

void QtVTKRenderWindowBox::onModelUpdate(const EventBucket &)
{
  m_Renderer->Update();
  this->GetRenderWindow()->Render();
  this->update();
}


