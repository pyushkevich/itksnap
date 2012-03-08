#include "ColorMapBox.h"
#include "ColorMapRenderer.h"
#include "ColorMapModel.h"
#include "ColorMapInspector.h"
#include <QPalette>
#include <QWidget>

ColorMapBox::ColorMapBox(QWidget *parent)
  : SNAPQGLWidget(parent)
{
  m_Model = NULL;
  m_Renderer = ColorMapRenderer::New();
  m_Delegate = new ColorMapInteractionDelegate();

  // attach delegate
  AttachSingleDelegate(m_Delegate);
}

ColorMapBox::~ColorMapBox()
{
}

void ColorMapBox::SetModel(ColorMapModel *model)
{
  m_Model = model;
  m_Renderer->SetModel(model);
  m_Delegate->SetModel(model);

  connectITK(m_Model, ModelUpdateEvent());
}

void ColorMapBox::paintGL()
{
  m_Renderer->paintGL();
}

void ColorMapBox::resizeGL(int w, int h)
{
  m_Renderer->resizeGL(w, h);
}

void ColorMapBox::onModelUpdate(const EventBucket &bucket)
{
  this->update();
}




void ColorMapInteractionDelegate::mousePressEvent(QMouseEvent *ev)
{
  m_Model->ProcessMousePressEvent(m_XSpace);
}

void ColorMapInteractionDelegate::mouseReleaseEvent(QMouseEvent *)
{
  m_Model->ProcessMouseReleaseEvent(m_XSpace);
}

void ColorMapInteractionDelegate::mouseMoveEvent(QMouseEvent *)
{
  m_Model->ProcessMouseDragEvent(m_XSpace);
}

void ColorMapInteractionDelegate::mouseDoubleClickEvent(QMouseEvent *)
{
  m_InspectorWidget->PromptUserForColor();
}

ColorMapInteractionDelegate::ColorMapInteractionDelegate(QWidget *parent)
  : QtInteractionDelegateWidget(parent)
{
  m_Model = NULL;
}


