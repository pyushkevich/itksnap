#include "SliceWindowInteractionDelegateWidget.h"
#include "GenericSliceModel.h"

SliceWindowInteractionDelegateWidget
::SliceWindowInteractionDelegateWidget(QWidget *parent)
  : QtInteractionDelegateWidget(parent)
{
  m_ParentModel = NULL;
}

void SliceWindowInteractionDelegateWidget::preprocessEvent(QEvent *ev)
{
  // Do the parent's processing
  QtInteractionDelegateWidget::preprocessEvent(ev);

  // Deal with mouse events
  if(ev->type() == QEvent::MouseButtonPress ||
     ev->type() == QEvent::MouseButtonRelease ||
     ev->type() == QEvent::MouseMove ||
     ev->type() == QEvent::MouseButtonDblClick)
    {
    // Compute the spatial location of the event
    m_XSlice = to_double(m_ParentModel->MapWindowToSlice(
                           to_float(Vector2d(m_XSpace.extract(2)))));

    // If a mouse press, back up this info for drag tracking
    if(ev->type() == QEvent::MouseButtonPress)
      {
      m_LastPressXSlice = m_XSlice;
      }
    }
}
