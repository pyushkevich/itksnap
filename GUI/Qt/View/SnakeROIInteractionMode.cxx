#include "SnakeROIInteractionMode.h"
#include "SnakeROIModel.h"
#include "SnakeROIRenderer.h"

SnakeROIInteractionMode::SnakeROIInteractionMode(QWidget *parent, QWidget *canvasWidget)
  : SliceWindowInteractionDelegateWidget(parent, canvasWidget)
{
}

SnakeROIInteractionMode::~SnakeROIInteractionMode()
{

}

void SnakeROIInteractionMode::SetModel(SnakeROIModel *model)
{
  m_Model = model;
  SetParentModel(model->GetParent());
}

void SnakeROIInteractionMode::mousePressEvent(QMouseEvent *ev)
{
  if(ev->button() == Qt::LeftButton)
    if(m_Model->ProcessPushEvent(m_XSlice[0], m_XSlice[1]))
      ev->accept();
}

void SnakeROIInteractionMode::mouseMoveEvent(QMouseEvent *ev)
{
  ev->ignore();
  if(this->m_LeftStatus == PRESS_ACCEPTED)
    {
    if(m_Model->ProcessDragEvent(
         m_XSlice[0], m_XSlice[1],
         m_LastPressXSlice[0], m_LastPressXSlice[1], false))
      ev->accept();
    }
  else if(this->isHovering())
    {
    if(m_Model->ProcessMoveEvent(m_XSlice[0], m_XSlice[1]))
      ev->accept();
    }
}

void SnakeROIInteractionMode::mouseReleaseEvent(QMouseEvent *ev)
{
  ev->ignore();
  if(this->m_LeftStatus == PRESS_ACCEPTED)
    {
    if(m_Model->ProcessDragEvent(
         m_XSlice[0], m_XSlice[1],
         m_LastPressXSlice[0], m_LastPressXSlice[1], true))
      ev->accept();
    }
  else if(this->isHovering())
    {
    if(m_Model->ProcessMoveEvent(m_XSlice[0], m_XSlice[1]))
      ev->accept();
    }
}

void
SnakeROIInteractionMode::enterEvent(QEnterEvent *)
{
  m_Model->ProcessEnterEvent();
  this->setMouseMotionTracking(true);
}

void SnakeROIInteractionMode::leaveEvent(QEvent *)
{
  if(!this->isDragging())
    m_Model->ProcessLeaveEvent();

  this->setMouseMotionTracking(false);
}




