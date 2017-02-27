#include "QtVTKRenderWindowBox.h"
#include "AbstractVTKRenderer.h"
#include "QtVTKInteractionDelegateWidget.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "QtReporterDelegates.h"

#if QT_VERSION >= 0x050000
  #include <QOpenGLContext>
#else
  #include <QGLContext>
  #define QOpenGLContext QGLContext
#endif

QtVTKRenderWindowBox::QtVTKRenderWindowBox(QWidget *parent) :
  QtSimpleOpenGLBox(parent)
{
  m_InteractionDelegate = new QtVTKInteractionDelegateWidget(this);
  this->AttachSingleDelegate(m_InteractionDelegate);
}

void QtVTKRenderWindowBox::SetRenderer(AbstractRenderer *renderer)
{
  // Cast to the VTK type
  AbstractVTKRenderer *renvtk = dynamic_cast<AbstractVTKRenderer *>(renderer);
  if(renvtk)
    {
    // Hook up the interaction delegate
    m_InteractionDelegate->SetVTKInteractor(renvtk->GetRenderWindowInteractor());

    // Disable rendering directly in the interactor
    renvtk->GetRenderWindowInteractor()->EnableRenderOff();

    // Disable buffer swapping in render window
    renvtk->GetRenderWindow()->SwapBuffersOn();

    // Create a size reporter
    renvtk->GetRenderWindowInteractor()->AddObserver(
          vtkCommand::RenderEvent,
          this, &QtVTKRenderWindowBox::RendererCallback);
    }

  // Call parent method
  QtSimpleOpenGLBox::SetRenderer(renderer);
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



