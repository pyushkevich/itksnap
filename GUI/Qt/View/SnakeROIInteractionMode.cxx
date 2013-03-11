#include "SnakeROIInteractionMode.h"
#include "GenericSliceView.h"
#include "SnakeROIModel.h"
#include "SnakeROIRenderer.h"

SnakeROIInteractionMode::SnakeROIInteractionMode(GenericSliceView *parent)
  : SliceWindowInteractionDelegateWidget(parent)
{
  // Create the renderer
  m_Renderer = SnakeROIRenderer::New();
  m_Renderer->SetParentRenderer(
        static_cast<GenericSliceRenderer *>(parent->GetRenderer()));
}

SnakeROIInteractionMode::~SnakeROIInteractionMode()
{

}

void SnakeROIInteractionMode::SetModel(SnakeROIModel *model)
{
  m_Model = model;
  m_Renderer->SetModel(model);
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
  if(m_LeftDown)
    {
    if(m_Model->ProcessDragEvent(
         m_XSlice[0], m_XSlice[1],
         m_LastPressXSlice[0], m_LastPressXSlice[1], false))
      {
      ev->accept();
      }
    }
  else if(!isDragging())
    {
    if(m_Model->ProcessMoveEvent(m_XSlice[0], m_XSlice[1]))
      {
      ev->accept();
      }
    }
}

void SnakeROIInteractionMode::mouseReleaseEvent(QMouseEvent *ev)
{
  if(m_Model->ProcessDragEvent(
       m_XSlice[0], m_XSlice[1],
       m_LastPressXSlice[0], m_LastPressXSlice[1], true))
    ev->accept();
}

#include <SliceViewPanel.h>

void SnakeROIInteractionMode::enterEvent(QEvent *)
{
  m_Model->ProcessEnterEvent();

  // TODO: this is hideous!
  SliceViewPanel *panel = dynamic_cast<SliceViewPanel *>(m_ParentView->parent());
  panel->SetMouseMotionTracking(true);
}

void SnakeROIInteractionMode::leaveEvent(QEvent *)
{
  if(!this->isDragging())
    m_Model->ProcessLeaveEvent();

  SliceViewPanel *panel = dynamic_cast<SliceViewPanel *>(m_ParentView->parent());
  panel->SetMouseMotionTracking(false);
}




