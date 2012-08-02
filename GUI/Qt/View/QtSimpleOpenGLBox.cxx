#include "QtSimpleOpenGLBox.h"
#include "AbstractRenderer.h"

QtSimpleOpenGLBox::QtSimpleOpenGLBox(QWidget *parent)
  : QtAbstractOpenGLBox(parent)
{
  m_Renderer = NULL;
}

void QtSimpleOpenGLBox::onModelUpdate(const EventBucket &bucket)
{
  this->update();
}

void QtSimpleOpenGLBox::SetRenderer(AbstractRenderer *renderer)
{
  m_Renderer = renderer;
  connectITK(m_Renderer, ModelUpdateEvent());
}
