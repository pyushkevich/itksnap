#include "PaintbrushInteractionMode.h"
#include "PaintbrushRenderer.h"
#include "GenericSliceView.h"
#include "PaintbrushModel.h"
#include "SliceViewPanel.h"

PaintbrushInteractionMode::PaintbrushInteractionMode(QWidget *parent, QWidget *canvasWidget)
  : SliceWindowInteractionDelegateWidget(parent, canvasWidget)
{
  m_Renderer = PaintbrushRenderer::New();
  m_Model = NULL;
}

PaintbrushInteractionMode::~PaintbrushInteractionMode()
{
}

void
PaintbrushInteractionMode
::SetModel(PaintbrushModel *model)
{
  m_Model = model;
  m_Renderer->SetModel(model);
  SetParentModel(model->GetParent());
}

void PaintbrushInteractionMode::mousePressEvent(QMouseEvent *ev)
{
  bool isleft = (ev->button() == Qt::LeftButton);
  bool isright = (ev->button() == Qt::RightButton);
  if(isleft || isright)
    {
    if(m_Model->ProcessPushEvent(m_XSlice,this->m_LastPressLayoutCell, isright))
      ev->accept();
    }
}

void PaintbrushInteractionMode::mouseMoveEvent(QMouseEvent *ev)
{
  ev->ignore();
  if(this->isDragging())
    {
    if(m_Model->ProcessDragEvent(
         m_XSlice, m_LastPressXSlice,
         GetNumberOfPixelsMoved(ev), false))
      {
      ev->accept();
      }
    }
  else if(this->isHovering())
    {
    if(m_Model->ProcessMouseMoveEvent(m_XSlice))
      ev->accept();
    }
}

void PaintbrushInteractionMode::mouseReleaseEvent(QMouseEvent *ev)
{
  if(m_Model->ProcessDragEvent(
       m_XSlice, m_LastPressXSlice,
       GetNumberOfPixelsMoved(ev), true))
    {
    ev->accept();
    }
}

void PaintbrushInteractionMode::enterEvent(QEnterEvent *)
{
  this->setMouseMotionTracking(true);
}

void PaintbrushInteractionMode::leaveEvent(QEvent *)
{
  this->setMouseMotionTracking(false);

  // This fixes a crash when you press quit in paintbrush mode
  if(isClientVisible())
    m_Model->ProcessMouseLeaveEvent();
}

void PaintbrushInteractionMode::keyPressEvent(QKeyEvent *ev)
{
  if(ev->key() == Qt::Key_Space)
    m_Model->AcceptAtCursor();
}


void PaintbrushInteractionMode::onModelUpdate(const EventBucket &bucket)
{
}
