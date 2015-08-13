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
#include "GenericSliceView.h"
#include "OrthogonalSliceCursorNavigationModel.h"
#include "CrosshairsRenderer.h"
#include <QPinchGesture>
#include <QPanGesture>
#include <QSwipeGesture>
#include <QApplication>

CrosshairsInteractionMode::CrosshairsInteractionMode(GenericSliceView *parent) :
    SliceWindowInteractionDelegateWidget(parent)
{
  // Create the renderer
  m_Renderer = CrosshairsRenderer::New();
  m_Renderer->SetParentRenderer(
          static_cast<GenericSliceRenderer *>(parent->GetRenderer()));


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
  if(ev->button() == Qt::MidButton)
    return Qt::MidButton;
  else if(ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ControlModifier)
    return Qt::RightButton;
  else if(ev->button() == Qt::LeftButton && ev->modifiers() == Qt::AltModifier)
    return Qt::MidButton;
  else if(ev->button() == Qt::LeftButton)
    return Qt::LeftButton;
  else return Qt::NoButton;
}

void CrosshairsInteractionMode
::SetModel(OrthogonalSliceCursorNavigationModel *model)
{
  m_Model = model;
  m_Renderer->SetModel(model);
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
      m_Model->UpdateCursor(Vector2f(m_XSpace[0], m_XSpace[1]));
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
      m_Model->UpdateCursor(Vector2f(m_XSpace[0], m_XSpace[1]));
      }
    else if(m_LastPressEmulatedButton == m_BtnZoom)
      {
      double scaleFactor = pow(1.02, dx(1));
      m_Model->ProcessZoomGesture(scaleFactor);
      }
    else if(m_LastPressEmulatedButton == m_BtnPan)
      {
      m_Model->ProcessPanGesture(Vector2f(dx(0), dx(1)));
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
      m_Model->UpdateCursor(Vector2f(m_XSpace[0], m_XSpace[1]));
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
      m_Model->ProcessZoomGesture(pinch->scaleFactor());
      }
    else if(pinch->state() == Qt::GestureFinished)
      {
      m_Model->ProcessZoomGesture(pinch->scaleFactor());
      m_Model->EndZoom();
      }
    ev->accept();
    return true;
    }

  /*

  // Get the pan event
  if(QPanGesture *pan =
      static_cast<QPanGesture *>(ev->gesture(Qt::PanGesture)))
    {
    if(pan->state() == Qt::GestureStarted)
      {
      m_Model->BeginPan();
      }
    else if(pan->state() == Qt::GestureUpdated)
      {
      m_Model->ProcessPanGesture(Vector2f(pan->delta().x(), pan->delta().y()));
      }
    else if(pan->state() == Qt::GestureFinished)
      {
      m_Model->ProcessPanGesture(Vector2f(pan->delta().x(), pan->delta().y()));
      m_Model->EndPan();
      }
    ev->accept();
    return true;
    }

  // Get the swipe event
  if(QSwipeGesture *swipe =
      static_cast<QSwipeGesture *>(ev->gesture(Qt::SwipeGesture)))
    {
    if(swipe->verticalDirection() == QSwipeGesture::Down)
      {
      m_Model->ProcessScrollGesture(1);
      }
    else if(swipe->verticalDirection() == QSwipeGesture::Up)
      {
      m_Model->ProcessScrollGesture(-1);
      }
    ev->accept();
    return true;
    }
  return false;

  */
  else return false;
}

void CrosshairsInteractionMode::keyPressEvent(QKeyEvent *ev)
{
  Vector3i dx(0,0,0);
  switch(ev->key())
    {
    case Qt::Key_Up:       dx = Vector3i( 0, 1, 0); break;
    case Qt::Key_Down:     dx = Vector3i( 0,-1, 0); break;
    case Qt::Key_Left:     dx = Vector3i(-1, 0, 0); break;
    case Qt::Key_Right:    dx = Vector3i( 1, 0, 0); break;
    case Qt::Key_PageUp:   dx = Vector3i( 0, 0, 1); break;
    case Qt::Key_PageDown: dx = Vector3i( 0, 0,-1); break;
    default:
      SliceWindowInteractionDelegateWidget::keyPressEvent(ev);
      return;
    }

  if(ev->modifiers() & Qt::ShiftModifier)
    dx *= 5;

  m_Model->ProcessKeyNavigation(dx);
  ev->accept();
}

void CrosshairsInteractionMode::enterEvent(QEvent *)
{
  // Respond to standard gestures
  this->m_ParentView->grabGesture(Qt::PinchGesture);
  // this->m_ParentView->grabGesture(Qt::PanGesture);
  // this->m_ParentView->grabGesture(Qt::SwipeGesture);
}

void CrosshairsInteractionMode::leaveEvent(QEvent *)
{
  // Stop responding to gestures
  // this->ungrabGesture(Qt::PinchGesture);
}

void CrosshairsInteractionMode::wheelEvent(QWheelEvent *event)
{
  // We want to scroll 1 line at a time!
  int scrollLines = QApplication::wheelScrollLines();
  QApplication::setWheelScrollLines(1);

  // Special case - when the user uses shift, we scroll in time, not in Z!
  if(event->modifiers() == Qt::ShiftModifier)
    {
    bool isThumb;
    ImageWrapperBase *layer =
        m_Model->GetParent()->GetContextLayerAtPosition(
          event->pos().x(),
          m_Model->GetParent()->GetSizeReporter()->GetLogicalViewportSize()[1] - event->pos().y(),
        isThumb);

    if(layer && layer->GetNumberOfComponents() > 1)
      {
      AbstractMultiChannelDisplayMappingPolicy *dpolicy =
          static_cast<AbstractMultiChannelDisplayMappingPolicy *>(layer->GetDisplayMapping());

      // Get the current display mode
      MultiChannelDisplayMode mode = dpolicy->GetDisplayMode();

      // Mode must be single component
      if(!mode.UseRGB && mode.SelectedScalarRep == SCALAR_REP_COMPONENT)
        {
        static double delta_accum = 0.0;
        delta_accum += event->angleDelta().x() + event->angleDelta().y();

        if(delta_accum <= -120.0 || delta_accum >= 120.0)
          {
          mode.SelectedComponent += (int) (delta_accum / 120.0);
          delta_accum = 0.0;
          }

        if(mode.SelectedComponent < 0)
          mode.SelectedComponent = 0;
        else if(mode.SelectedComponent >= layer->GetNumberOfComponents())
          mode.SelectedComponent = layer->GetNumberOfComponents()-1;
        dpolicy->SetDisplayMode(mode);
        }
      event->accept();
      }
    }

  else if(m_WheelEventTarget)
    {
    QWheelEvent evnew(
          event->pos(), event->globalPos(), event->delta(),
          event->buttons(), event->modifiers(),
          event->orientation());
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
