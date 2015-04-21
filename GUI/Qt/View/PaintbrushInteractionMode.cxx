#include "PaintbrushInteractionMode.h"
#include "PaintbrushRenderer.h"
#include "GenericSliceView.h"
#include "PaintbrushModel.h"
#include "SliceViewPanel.h"

PaintbrushInteractionMode::PaintbrushInteractionMode(GenericSliceView *parent)
  : SliceWindowInteractionDelegateWidget(parent)
{
  m_Renderer = PaintbrushRenderer::New();
  m_Renderer->SetParentRenderer(
        static_cast<GenericSliceRenderer *>(parent->GetRenderer()));

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
    if(m_Model->ProcessPushEvent(to_float(m_XSlice),this->m_LastPressLayoutCell, isright))
      ev->accept();
    }
}

void PaintbrushInteractionMode::mouseMoveEvent(QMouseEvent *ev)
{
  ev->ignore();

  if(isDragging())
    {
    if(m_Model->ProcessDragEvent(
         to_float(m_XSlice), to_float(m_LastPressXSlice),
         GetNumberOfPixelsMoved(ev), false))
      {
      ev->accept();
      }
    }
  else
    {
    if(m_Model->ProcessMouseMoveEvent(to_float(m_XSlice)))
      ev->accept();
    }
}

void PaintbrushInteractionMode::mouseReleaseEvent(QMouseEvent *ev)
{
  if(m_Model->ProcessDragEvent(
       to_float(m_XSlice), to_float(m_LastPressXSlice),
       GetNumberOfPixelsMoved(ev), true))
    {
    ev->accept();
    }
}

void PaintbrushInteractionMode::enterEvent(QEvent *)
{
  // TODO: this is hideous!
  SliceViewPanel *panel = dynamic_cast<SliceViewPanel *>(m_ParentView->parent());
  panel->SetMouseMotionTracking(true);
}

void PaintbrushInteractionMode::leaveEvent(QEvent *)
{
  SliceViewPanel *panel = dynamic_cast<SliceViewPanel *>(m_ParentView->parent());
  panel->SetMouseMotionTracking(false);

  // This fixes a crash when you press quit in paintbrush mode
  if(panel->isVisible())
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
