#include "QtSimpleOpenGLBox.h"
#include "AbstractRenderer.h"

QtSimpleOpenGLBox::QtSimpleOpenGLBox(QWidget *parent)
  : QtAbstractOpenGLBox(parent)
{
  m_Renderer = NULL;
}

void QtSimpleOpenGLBox::onModelUpdate(const EventBucket &bucket)
{
  if(this->isVisible())
    {
    // TODO: this seems redundant, Update is already called in the paint method
    // of the renderer, and update() just causes paint to be called.
    if(m_Renderer)
      m_Renderer->Update();

    this->update();
    }
}

void QtSimpleOpenGLBox::SetRenderer(AbstractRenderer *renderer)
{
  m_Renderer = renderer;
  connectITK(m_Renderer, ModelUpdateEvent());
}
