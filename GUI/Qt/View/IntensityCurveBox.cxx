#include "IntensityCurveBox.h"
#include "IntensityCurveRenderer.h"
#include "IntensityCurveModel.h"
#include <QPalette>
#include <QWidget>


IntensityCurveBox::IntensityCurveBox(QWidget *parent)
  : SNAPQGLWidget(parent)
{
  m_Model = NULL;
  m_Renderer = IntensityCurveRenderer::New();
  m_Delegate = new IntensityCurveInteractionDelegate();

  // attach delegate
  AttachSingleDelegate(m_Delegate);
}

IntensityCurveBox::~IntensityCurveBox()
{
}

void IntensityCurveBox::SetModel(IntensityCurveModel *model)
{
  m_Model = model;
  m_Renderer->SetModel(model);
  m_Delegate->SetModel(model);

  connectITK(m_Model, ModelUpdateEvent());
}

void IntensityCurveBox::paintGL()
{
  QWidget *p = parentWidget();
  QColor bkg = p->palette().color(p->backgroundRole());
  int rgb[] = {bkg.red(), bkg.green(), bkg.blue()};

  m_Renderer->paintGL(rgb);
}

void IntensityCurveBox::resizeGL(int w, int h)
{
  m_Renderer->resizeGL(w, h);
}

void IntensityCurveBox::onModelUpdate(const EventBucket &bucket)
{
  this->update();
}




void IntensityCurveInteractionDelegate::mousePressEvent(QMouseEvent *ev)
{
  m_Model->ProcessMousePressEvent(m_XSpace);
}

void IntensityCurveInteractionDelegate::mouseReleaseEvent(QMouseEvent *)
{
  m_Model->ProcessMouseDragEvent(m_XSpace);
}

void IntensityCurveInteractionDelegate::mouseMoveEvent(QMouseEvent *)
{
  m_Model->ProcessMouseReleaseEvent(m_XSpace);
}

IntensityCurveInteractionDelegate::IntensityCurveInteractionDelegate(QWidget *parent)
  : QtInteractionDelegateWidget(parent)
{
  m_Model = NULL;
}



