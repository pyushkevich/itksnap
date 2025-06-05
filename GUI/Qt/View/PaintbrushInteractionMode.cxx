#include "PaintbrushInteractionMode.h"
#include "PaintbrushRenderer.h"
#include "PaintbrushModel.h"
#include "QtWarningDialog.h"
#include "SliceViewPanel.h"

PaintbrushInteractionMode::PaintbrushInteractionMode(QWidget *parent, QWidget *canvasWidget)
  : SliceWindowInteractionDelegateWidget(parent, canvasWidget)
{
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
  SetParentModel(model->GetParent());
}

void
PaintbrushInteractionMode::mousePressEvent(QMouseEvent *ev)
{
  try
  {
    bool isleft = (ev->button() == Qt::LeftButton);
    bool isright = (ev->button() == Qt::RightButton);
    if (isleft || isright)
    {
      if (m_Model->ProcessPushEvent(m_XSlice, this->m_LastPressLayoutCell, isright))
        ev->accept();
    }
  }
  catch (IRISException &exc)
  {
    QtWarningDialog::show({ IRISWarning(exc.what()) });
  }
}

void
PaintbrushInteractionMode::mouseMoveEvent(QMouseEvent *ev)
{
  ev->ignore();
  try
  {
    if (this->isDragging())
    {
      if (m_Model->ProcessDragEvent(m_XSlice, m_LastPressXSlice, GetNumberOfPixelsMoved(ev), false))
      {
        ev->accept();
      }
    }
    else if (this->isHovering())
    {
      if (m_Model->ProcessMouseMoveEvent(m_XSlice))
        ev->accept();
    }
  }
  catch (IRISException &exc)
  {
    QtWarningDialog::show({ IRISWarning(exc.what()) });
  }
}

void
PaintbrushInteractionMode::mouseReleaseEvent(QMouseEvent *ev)
{
  try
  {
    if (m_Model->ProcessDragEvent(m_XSlice, m_LastPressXSlice, GetNumberOfPixelsMoved(ev), true))
    {
      ev->accept();
    }
  }
  catch (IRISException &exc)
  {
    QtWarningDialog::show({ IRISWarning(exc.what()) });
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

void
PaintbrushInteractionMode::keyPressEvent(QKeyEvent *ev)
{
  if (ev->key() == Qt::Key_Space)
  {
    try
    {
      m_Model->AcceptAtCursor();
    }
    catch (IRISException &exc)
    {
      QtWarningDialog::show({ IRISWarning(exc.what()) });
    }
  }
}


void
PaintbrushInteractionMode::onModelUpdate(const EventBucket &bucket)
{}
