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
#include <vtkRenderWindowInteractor.h>
#include <vtkWindowToImageFilter.h>

#include <QVBoxLayout>
// #include <QStackedLayout>
#include <QResizeEvent>
#include <QVTKInteractorAdapter.h>
#include <QVTKInteractor.h>
#include <QImage>
#include <QPainter>
#include <QCoreApplication>

/**
 * An extension of QVTKOpenGLNativeWidget that handles saving screenshots
 */
class QVTKOpenGLNativeWidgetWithScreenshot : public QVTKOpenGLNativeWidget
{
public:
  QVTKOpenGLNativeWidgetWithScreenshot(QtVTKRenderWindowBox *parent)
    : QVTKOpenGLNativeWidget(parent)
  {
    m_Parent = parent;
  }

  virtual void paintGL() override
  {
    if(m_NeedRender)
      {
      this->renderWindow()->Render();
      m_NeedRender = false;
      }

    QVTKOpenGLNativeWidget::paintGL();
    if(m_ScreenshotRequest.size())
      {
      QImage img = this->grabFramebuffer();
      img.save(m_ScreenshotRequest);
      m_ScreenshotRequest = QString();
      }
  }

  virtual bool event(QEvent* evt) override
  {
    // Touch events on the Mac touchpad seem to confuse VTK interactors. We let Qt handle them
    // as mouse events instead.
    if(evt->type() == QEvent::TouchBegin || evt->type() == QEvent::TouchEnd ||
       evt->type() == QEvent::TouchUpdate || evt->type() == QEvent::TouchCancel)
      {
      return false;
      }
    else
      {
      return QVTKOpenGLNativeWidget::event(evt);
      }
  }

  virtual void setScreenshotRequest(const QString &s)
  {
    m_ScreenshotRequest = s;
  }

  virtual void setNeedRender()
  {
    m_NeedRender = true;
  }

private:
  bool m_NeedRender = false;
  QtVTKRenderWindowBox *m_Parent;
  QString m_ScreenshotRequest;
};

/** QWidget that renders VTK output rendered offscren using OsMESA */
class QtVTKOffscreenMesaWidget : public QWidget
{
public:
  QtVTKOffscreenMesaWidget(QWidget *parent) : QWidget(parent)
    {
    m_InteractorAdapter = new QVTKInteractorAdapter(this);
    m_InteractorAdapter->SetDevicePixelRatio(this->devicePixelRatio());
    }

  void SetRenderWindow(vtkRenderWindow *rwin)
    {
    m_RenderWindow = rwin;
    m_ImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
    m_ImageFilter->SetInput(m_RenderWindow);
    }

  vtkRenderWindow *GetRenderWindow()
    {
    return m_RenderWindow;
    }

  void paintEvent(QPaintEvent *evt) override
    {
    m_RenderWindow->SetSize(this->width(), this->height());
    m_RenderWindow->Render();
    m_ImageFilter->Modified();
    m_ImageFilter->Update();
    vtkImageData *id = m_ImageFilter->GetOutput();

    QImage img((const unsigned char *) id->GetScalarPointer(), 
               id->GetDimensions()[0], id->GetDimensions()[1], 
               3 * id->GetDimensions()[0], QImage::Format_RGB888);

    QPainter p(this);
    p.drawImage(0, 0, img.mirrored());
    std::cout << "Paint completed" << std::endl;
    }

  void processEvent(QEvent *evt) 
    {
    if(m_RenderWindow)
      {
      m_InteractorAdapter->ProcessEvent(evt, m_RenderWindow->GetInteractor());
      this->update();
      }
    }

  void mousePressEvent(QMouseEvent *evt) override
    {
    processEvent(evt);
    }

  void mouseMoveEvent(QMouseEvent *evt) override
    {
    processEvent(evt);
    }

  void mouseReleaseEvent(QMouseEvent *evt) override
    {
    processEvent(evt);
    }

  void enterEvent(QEnterEvent *evt) override
    {
    processEvent(evt);
    }

  void leaveEvent(QEvent *evt) override
    {
    processEvent(evt);
    }

protected:

  vtkSmartPointer<vtkRenderWindow> m_RenderWindow;
  vtkSmartPointer<vtkWindowToImageFilter> m_ImageFilter;
  QVTKInteractorAdapter *m_InteractorAdapter;
 
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
  iw->setRenderWindow(vtkNew<vtkGenericOpenGLRenderWindow>());
  iw->renderWindow()->AddRenderer(renderer);
  iw->setObjectName("internalWidget");
  m_InternalWidget = iw;

#else
  auto *iw = new QtVTKOffscreenMesaWidget(this);
  vtkNew<vtkRenderWindow> rwin;
  rwin->AddRenderer(renderer);

  vtkNew<QVTKInteractor> inter;
  inter->SetRenderWindow(rwin);
  inter->Initialize();

  iw->SetRenderWindow(rwin);
  iw->setObjectName("internalWidget");
  m_InternalWidget = iw;
#endif

  // Create an internal layout without margins
  auto *lo = new QVBoxLayout(this);
  lo->setContentsMargins(0,0,0,0);
  lo->setSpacing(0);
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
    m_Renderer->SetRenderWindow(iw->renderWindow());
#else
    auto *iw = dynamic_cast<QtVTKOffscreenMesaWidget *>(m_InternalWidget);
    m_Renderer->SetRenderWindow(iw->GetRenderWindow());
#endif
    connectITK(m_Renderer, ModelUpdateEvent());
    }
}

vtkRenderWindow *QtVTKRenderWindowBox::GetRenderWindow()
{
#ifndef VTK_OPENGL_HAS_OSMESA
    auto *iw = dynamic_cast<QVTKOpenGLNativeWidgetWithScreenshot *>(m_InternalWidget);
    return iw->renderWindow();
#else
    auto *iw = dynamic_cast<QtVTKOffscreenMesaWidget *>(m_InternalWidget);
    return iw->GetRenderWindow();
#endif
}

QWidget *QtVTKRenderWindowBox::GetInternalWidget()
{
  return m_InternalWidget;
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

void QtVTKRenderWindowBox::enterEvent(QEnterEvent *)
{
  if(m_GrabFocusOnEntry)
    this->m_InternalWidget->setFocus();
}

void QtVTKRenderWindowBox::leaveEvent(QEvent *)
{
  if(m_GrabFocusOnEntry)
    this->m_InternalWidget->clearFocus();
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

void QtVTKRenderWindowBox::onModelUpdate(const EventBucket &b)
{
  m_Renderer->Update();

#ifndef VTK_OPENGL_HAS_OSMESA
  auto *iw = dynamic_cast<QVTKOpenGLNativeWidgetWithScreenshot *>(m_InternalWidget);
  iw->setNeedRender();
  iw->update();
#else
  this->update();
#endif
}


