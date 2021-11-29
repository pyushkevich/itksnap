#include "QtVTKRenderWindowBox.h"
#include "AbstractVTKRenderer.h"
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

#include <QVBoxLayout>
#include <QResizeEvent>

/**
 * An extension of QVTKOpenGLNativeWidget that handles saving screenshots
 */
class QVTKOpenGLNativeWidgetWithScreenshot : public QVTKOpenGLNativeWidget
{
public:
  QVTKOpenGLNativeWidgetWithScreenshot(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent) {}

  virtual void paintGL() override
  {
    QVTKOpenGLNativeWidget::paintGL();
    if(m_ScreenshotRequest.size())
      {
      QImage img = this->grabFramebuffer();
      img.save(m_ScreenshotRequest);
      m_ScreenshotRequest = QString();
      }
  }

  virtual void setScreenshotRequest(const QString &s)
  {
    m_ScreenshotRequest = s;
  }

private:
  QString m_ScreenshotRequest;
};


QtVTKRenderWindowBox::QtVTKRenderWindowBox(QWidget *parent) :
  QWidget(parent)
{
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

  // Create an internal GL rendering widget
#ifndef VTK_OPENGL_HAS_OSMESA
  auto *iw = new QVTKOpenGLNativeWidgetWithScreenshot(this);
  iw->SetRenderWindow(vtkNew<vtkGenericOpenGLRenderWindow>());
  iw->GetRenderWindow()->AddRenderer(renderer);
  m_InternalWidget = iw;
#else
#endif

  // Create an internal layout without margins
  auto *lo = new QVBoxLayout(this);
  lo->setContentsMargins(0,0,0,0);
  lo->addWidget(m_InternalWidget);
  this->setLayout(lo);
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
#ifndef VTK_OPENGL_HAS_OSMESA
    auto *iw = dynamic_cast<QVTKOpenGLNativeWidgetWithScreenshot *>(m_InternalWidget);
    m_Renderer->SetRenderWindow(iw->GetRenderWindow());
#else
#endif
    connectITK(m_Renderer, ModelUpdateEvent());
    }
}

vtkRenderWindow *QtVTKRenderWindowBox::GetRenderWindow()
{
#ifndef VTK_OPENGL_HAS_OSMESA
    auto *iw = dynamic_cast<QVTKOpenGLNativeWidgetWithScreenshot *>(m_InternalWidget);
    return iw->GetRenderWindow();
#else
#endif

}

void QtVTKRenderWindowBox::resizeEvent(QResizeEvent *evt)
{
  // Handle changes in VPPR
  if(m_Renderer)
    m_Renderer->OnWindowResize(evt->size().width(), evt->size().height(),
                               this->devicePixelRatio());
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
#ifndef VTK_OPENGL_HAS_OSMESA
    auto *iw = dynamic_cast<QVTKOpenGLNativeWidgetWithScreenshot *>(m_InternalWidget);
    iw->setScreenshotRequest(from_utf8(filename));
#else
#endif

  this->update();
  return true;
}

void QtVTKRenderWindowBox::onModelUpdate(const EventBucket &)
{
  m_Renderer->Update();
  this->update();
}


