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

#include "QtInteractionDelegateWidget.h"
#include <SNAPQGLWidget.h>
#include <QMouseEvent>

QtInteractionDelegateWidget::QtInteractionDelegateWidget(QWidget *parent) :
  QWidget(parent)
{
}

bool QtInteractionDelegateWidget::event(QEvent *ev)
{
  // Deal with mouse events
  if(ev->type() == QEvent::MouseButtonPress ||
     ev->type() == QEvent::MouseButtonRelease ||
     ev->type() == QEvent::MouseMove ||
     ev->type() == QEvent::MouseButtonDblClick)
    {
    // Compute the spatial location of the event
    QMouseEvent *emouse = static_cast<QMouseEvent *>(ev);
    m_XSpace = GetParentGLWidget()->GetEventWorldCoordinates(emouse, true);

    // If a mouse press, back up this info for drag tracking
    if(ev->type() == QEvent::MouseButtonPress)
      {
      m_LastPressPos = emouse->pos();
      m_LastPressGlobalPos = emouse->globalPos();
      m_LastPressButton = emouse->button();
      m_LastPressXSpace = m_XSpace;
      }
    }

  // Deal with gesture events
  else if(ev->type() == QEvent::Gesture)
    {
    return gestureEvent(static_cast<QGestureEvent*>(ev));
    }

  // Call parent's event method
  return QWidget::event(ev);
  }

SNAPQGLWidget * QtInteractionDelegateWidget::GetParentGLWidget() const
{
  if(!this->parent())
    return NULL;

  SNAPQGLWidget *p = dynamic_cast<SNAPQGLWidget *>(this->parent());
  assert(p);

  return p;
}

bool QtInteractionDelegateWidget::eventFilter(QObject *obj, QEvent *ev)
{
  ev->setAccepted(false);
  this->event(ev);
  if(ev->isAccepted())
    return true;
  else return QWidget::eventFilter(obj, ev);
}
