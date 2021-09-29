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
  this->setFormat(QVTKOpenGLNativeWidget::defaultFormat());
  // m_InteractionDelegate = new QtVTKInteractionDelegateWidget(this);
  // this->AttachSingleDelegate(m_InteractionDelegate);
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
    this->SetRenderWindow(m_Renderer->GetRenderWindow());
    }
}

void QtVTKRenderWindowBox::initializeGL()
{
  this->setFormat(QVTKOpenGLNativeWidget::defaultFormat());
  if(m_Renderer)
    m_Renderer->initializeGL();
}

void QtVTKRenderWindowBox::paintGL()
{
  if(m_Renderer)
    m_Renderer->paintGL();
}

void QtVTKRenderWindowBox::resizeGL(int w, int h)
{
  if(m_Renderer)
    m_Renderer->resizeGL(w, h, this->devicePixelRatio());
}

void
QtVTKRenderWindowBox
::RendererCallback(
    vtkObject *src, unsigned long event, void *data)
{
  if(event == vtkCommand::RenderEvent)
    {
    this->update();
    }
}



