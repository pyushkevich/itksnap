/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Filename.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
  Copyright (c) 2011 Paul A. Yushkevich

  This file is part of ITK-SNAP

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

=========================================================================*/

#include "CrosshairsInteractionMode.h"
#include "OrthogonalSliceCursorNavigationModel.h"
#include "CrosshairsRenderer.h"
#include "GenericImageData.h"
#include "IRISApplication.h"
#include <QPinchGesture>
#include <QPanGesture>
#include <QSwipeGesture>
#include <QApplication>
#include <QMessageBox>

CrosshairsInteractionMode::CrosshairsInteractionMode(QWidget *parent, QWidget *canvasWidget)
  : SliceWindowInteractionDelegateWidget(parent, canvasWidget)
{
  // Create the renderer
  m_WheelEventTarget = NULL;
  m_Model = NULL;

  SetMouseButtonBehaviorToCrosshairsMode();
  setAttribute(Qt::WA_AcceptTouchEvents, true);
}

CrosshairsInteractionMode::~CrosshairsInteractionMode()
{

}

void CrosshairsInteractionMode
::SetMouseButtonBehaviorToCrosshairsMode()
{
  m_BtnCursor = Qt::LeftButton;
  m_BtnZoom = Qt::RightButton;
  m_BtnPan = Qt::MiddleButton;
}

void CrosshairsInteractionMode
::SetMouseButtonBehaviorToZoomPanMode()
{
  m_BtnCursor = Qt::MiddleButton;
  m_BtnZoom = Qt::RightButton;
  m_BtnPan = Qt::LeftButton;
}

Qt::MouseButton
CrosshairsInteractionMode
::GetButtonForEvent(QMouseEvent *ev)
{
  if(ev->button() == Qt::RightButton)
    return Qt::RightButton;
  if(ev->button() == Qt::MiddleButton)
    return Qt::MiddleButton;
  else if(ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ControlModifier)
    return Qt::RightButton;
  else if(ev->button() == Qt::LeftButton && ev->modifiers() == Qt::AltModifier)
    return Qt::MiddleButton;
  else if(ev->button() == Qt::LeftButton)
    return Qt::LeftButton;
  else return Qt::NoButton;
}

void CrosshairsInteractionMode
::SetModel(OrthogonalSliceCursorNavigationModel *model)
{
  m_Model = model;
  SetParentModel(model->GetParent());
}

void CrosshairsInteractionMode::mousePressEvent(QMouseEvent *ev)
{
  // Get the button after emulation
  Qt::MouseButton btn = this->GetButtonForEvent(ev);

  // We have to be careful to check that the event landed in the
  // region corresponding to an image view and not to a thumbnail
  if(this->IsMouseOverFullLayer())
    {
    // Use model to envoke event
    if(btn == m_BtnCursor)
      {
      m_Model->UpdateCursor(Vector2d(m_XSpace[0], m_XSpace[1]));
      }
    else if(btn == m_BtnZoom)
      {
      m_Model->BeginZoom();
      }
    else if(btn == m_BtnPan)
      {
      m_Model->BeginPan();
      }

    // Eat this event
    ev->accept();
    }

  m_LastPressEmulatedButton = btn;
}

void CrosshairsInteractionMode::mouseMoveEvent(QMouseEvent *ev)
{
  if(isDragging())
    {
    Vector3d dx = m_XSpace - m_LastPressXSpace;

    if(m_LastPressEmulatedButton == m_BtnCursor)
      {
      m_Model->UpdateCursor(Vector2d(m_XSpace[0], m_XSpace[1]));
      }
    else if(m_LastPressEmulatedButton == m_BtnZoom)
      {
      double scaleFactor = pow(1.02, dx(1));
      m_Model->ProcessZoomGesture(scaleFactor);
      }
    else if(m_LastPressEmulatedButton == m_BtnPan)
      {
      m_Model->ProcessPanGesture(Vector2d(dx(0), dx(1)));
      }

    // Eat this event
    ev->accept();
    }
  else
    {
    ev->ignore();
    }
}


void CrosshairsInteractionMode::mouseReleaseEvent(QMouseEvent *ev)
{
  // Get the button after emulation
  Qt::MouseButton btn = this->GetButtonForEvent(ev);

  if(isDragging())
    {
      if(btn == m_BtnCursor)
      {
      m_Model->UpdateCursor(Vector2d(m_XSpace[0], m_XSpace[1]));
      }
    else if(btn == m_BtnZoom)
      {
      m_Model->EndZoom();
      }
    else if(btn == m_BtnPan)
      {
      m_Model->EndPan();
      }

    // Eat this event
    ev->accept();
    }
  else
    {
    ev->ignore();
    }
}

bool CrosshairsInteractionMode::gestureEvent(QGestureEvent *ev)
{
  // Get the pinch gesture if there is one
  if(QPinchGesture *pinch =
      static_cast<QPinchGesture *>(ev->gesture(Qt::PinchGesture)))
    {
    if(pinch->state() == Qt::GestureStarted)
      {
      m_Model->BeginZoom();
      }
    else if(pinch->state() == Qt::GestureUpdated)
      {
      m_Model->ProcessZoomGesture(pinch->totalScaleFactor());
      }
    else if(pinch->state() == Qt::GestureFinished)
      {
      m_Model->ProcessZoomGesture(pinch->totalScaleFactor());
      m_Model->EndZoom();
      }
    ev->accept();
    return true;
    }

  else return false;
}

void
CrosshairsInteractionMode::keyPressEvent(QKeyEvent *ev)
{
  Vector3i dx(0, 0, 0);
  switch (ev->key())
  {
    case Qt::Key_Up:
      dx = Vector3i(0, 1, 0);
      break;
    case Qt::Key_Down:
      dx = Vector3i(0, -1, 0);
      break;
    case Qt::Key_Left:
      dx = Vector3i(-1, 0, 0);
      break;
    case Qt::Key_Right:
      dx = Vector3i(1, 0, 0);
      break;
    case Qt::Key_PageUp:
      dx = Vector3i(0, 0, 1);
      break;
    case Qt::Key_PageDown:
      dx = Vector3i(0, 0, -1);
      break;
    case Qt::Key_G:
      if ((ev->modifiers() & Qt::AltModifier) && (ev->modifiers() & Qt::ShiftModifier))
      {
        QMessageBox::StandardButton reply = QMessageBox::question(
          this,
          tr("Crash simulation"),
          tr("This key combination simulates a crash in ITK-SNAP. Do you want ITK-SNAP to crash now?"),
          QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes)
        {
          throw std::runtime_error("Simulated uncaught exception");
        }
      }

      break;
    default:
      SliceWindowInteractionDelegateWidget::keyPressEvent(ev);
      return;
  }

  if (ev->modifiers() & Qt::ShiftModifier)
    dx *= 5;

  m_Model->ProcessKeyNavigation(dx);
  ev->accept();
}

void
CrosshairsInteractionMode::enterEvent(QEnterEvent *)
{
  // Respond to standard gestures
  grabGesture(Qt::PinchGesture);

  // Grab keyboard focus
  this->m_CanvasWidget->setFocus();

  // this->m_ParentView->grabGesture(Qt::PanGesture);
  // this->m_ParentView->grabGesture(Qt::SwipeGesture);
}

void CrosshairsInteractionMode::leaveEvent(QEvent *)
{
  this->m_CanvasWidget->clearFocus();
  // Stop responding to gestures
  // this->ungrabGesture(Qt::PinchGesture);
}

void CrosshairsInteractionMode::wheelEvent(QWheelEvent *event)
{
  // Get the event position and global position
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  QPointF epos = event->position(), egpos = event->globalPosition();
#else
  QPointF epos = event->pos(), egpos = event->globalPos();
#endif

  // We want to scroll 1 line at a time!
  int scrollLines = QApplication::wheelScrollLines();
  QApplication::setWheelScrollLines(1);

  // Special case - when the user uses shift, we scroll in component/time, not in Z!
  if(event->modifiers() == Qt::ShiftModifier)
    {
    bool isThumb;

    // The global UI model
    IRISApplication *app = m_Model->GetParent()->GetDriver();

    // Current layer
    ImageWrapperBase *layer =
        m_Model->GetParent()->GetContextLayerAtPosition(
          epos.x(),
          m_Model->GetParent()->GetSizeReporter()->GetLogicalViewportSize()[1] - epos.y(),
        isThumb);

    // Check if image is 4D
    int n_tp = (int) app->GetNumberOfTimePoints();

    // Figure out the amount of component/timepoint shift
    static double delta_accum = 0.0;
    int comp_change = 0;

    delta_accum += event->angleDelta().x() + event->angleDelta().y();
    if(delta_accum <= -120.0 || delta_accum >= 120.0)
      {
      comp_change = (int) (delta_accum / 120.0);
      delta_accum = 0.0;
      }

    if(layer && layer->GetNumberOfComponents() > 1)
      {
      AbstractMultiChannelDisplayMappingPolicy *dpolicy =
          static_cast<AbstractMultiChannelDisplayMappingPolicy *>(layer->GetDisplayMapping());

      // Get the current display mode
      MultiChannelDisplayMode mode = dpolicy->GetDisplayMode();

      // Mode must be single component
      if(mode.IsSingleComponent())
        {
        if(comp_change != 0)
          {
          mode.SelectedComponent += comp_change;
          if(mode.SelectedComponent < 0)
            mode.SelectedComponent = 0;
          else if(mode.SelectedComponent >= (int) layer->GetNumberOfComponents())
            mode.SelectedComponent = layer->GetNumberOfComponents()-1;
          }
        dpolicy->SetDisplayMode(mode);
        }
      event->accept();
      }

    else if(n_tp > 1)
      {
      int tp = (int) app->GetCursorTimePoint();
      tp += comp_change;
      tp = (tp < 0) ? 0 : tp;
      tp = (tp >= n_tp) ? n_tp - 1 : tp;
      app->SetCursorTimePoint(tp);
      event->accept();
      }
    }

  else if(m_WheelEventTarget)
    {
    QWheelEvent evnew(
          epos, egpos,
          event->pixelDelta(), event->angleDelta(),
          event->buttons(), event->modifiers(),
          event->phase(), event->inverted());

    QCoreApplication::sendEvent(m_WheelEventTarget, &evnew);
    event->accept();
    }

  /*
  int numDegrees = event->delta() / 8;
  int numSteps = numDegrees / 15;
  std::cout << "Wheel event: " << event->delta() << std::endl;

  // Scroll
  m_Model->ProcessScrollGesture(numSteps);
  */

  QApplication::setWheelScrollLines(scrollLines);
}

void CrosshairsInteractionMode::SetWheelEventTargetWidget(QWidget *w)
{
  m_WheelEventTarget = w;
}
