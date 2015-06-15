#include "QtVTKRenderWindowBox.h"
#include "AbstractVTKRenderer.h"
#include "QtVTKInteractionDelegateWidget.h"
#include "vtkRenderWindow.h"
#include "vtkCommand.h"
#include "QtReporterDelegates.h"

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

    // Create a size reporter
    }

  // Hook up context-related events (is this needed?)
  renvtk->GetRenderWindow()->AddObserver(
        vtkCommand::WindowMakeCurrentEvent,
        this, &QtVTKRenderWindowBox::RendererCallback);

  renvtk->GetRenderWindow()->AddObserver(
        vtkCommand::WindowIsCurrentEvent,
        this, &QtVTKRenderWindowBox::RendererCallback);

  // Call parent method
  QtSimpleOpenGLBox::SetRenderer(renderer);
}

#include <QOpenGLContext>

void
QtVTKRenderWindowBox
::RendererCallback(
    vtkObject *src, unsigned long event, void *data)
{
  if(event == vtkCommand::WindowMakeCurrentEvent)
    {
    this->makeCurrent();
    }
  else if(event == vtkCommand::WindowIsCurrentEvent)
    {
    bool *result = static_cast<bool *>(data);
    *result = QOpenGLContext::currentContext() == this->context();
    }
}



