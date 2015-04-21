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
#include <QtAbstractOpenGLBox.h>
#include <QMouseEvent>
#include <QGestureEvent>
#include "GenericSliceModel.h"
#include "SNAPOpenGL.h"

QtInteractionDelegateWidget::QtInteractionDelegateWidget(QWidget *parent) :
  SNAPComponent(parent)
{
  m_LeftDown = false;
  m_MiddleDown = false;
  m_RightDown = false;
  m_Filtering = false;

  // The widget is hidden
  this->hide();
}

bool QtInteractionDelegateWidget::event(QEvent *ev)
{
  bool result;

  // If the event was sent to the widget itself, ignore it. The delegate
  // only should receive events through the sender
  if(!m_Filtering)
    return false;

  // Generate data from this event
  preprocessEvent(ev);

  // Deal with gesture events
  if(ev->type() == QEvent::Gesture)
    result = gestureEvent(static_cast<QGestureEvent*>(ev));
  else
    result = QWidget::event(ev);

  // Set the dragging information
  postprocessEvent(ev);

  return result;
}

QtAbstractOpenGLBox * QtInteractionDelegateWidget::GetParentGLWidget() const
{
  // Search up until a parent widget is found
  for(QObject *p = parent(); p != NULL; p = p->parent())
    {
    QtAbstractOpenGLBox *pgl = dynamic_cast<QtAbstractOpenGLBox *>(p);
    if(pgl)
      return pgl;
    }

  return NULL;
}

bool QtInteractionDelegateWidget::eventFilter(QObject *obj, QEvent *ev)
{
  ev->setAccepted(false);
  m_Filtering = true;
  this->event(ev);
  m_Filtering = false;

  if(ev->isAccepted())
    return true;
  else return QWidget::eventFilter(obj, ev);
}

void QtInteractionDelegateWidget::preprocessEvent(QEvent *ev)
{
  // Deal with mouse events
  if(ev->type() == QEvent::MouseButtonPress ||
     ev->type() == QEvent::MouseButtonRelease ||
     ev->type() == QEvent::MouseMove ||
     ev->type() == QEvent::MouseButtonDblClick)
    {
    // Compute the spatial location of the event
    QMouseEvent *emouse = static_cast<QMouseEvent *>(ev);
    m_XSpace = this->GetEventWorldCoordinates(emouse, true);
    }
}

void QtInteractionDelegateWidget::postprocessEvent(QEvent *ev)
{
  QMouseEvent *emouse = static_cast<QMouseEvent *>(ev);

  if(ev->isAccepted() && ev->type() == QEvent::MouseButtonPress)
    {
    m_LastPressPos = emouse->pos();
    m_LastPressGlobalPos = emouse->globalPos();
    m_LastPressButton = emouse->button();
    m_LastPressXSpace = m_XSpace;

    // Store what buttons are up or down
    if(emouse->button() == Qt::LeftButton)
      m_LeftDown = true;
    if(emouse->button() == Qt::RightButton)
      m_RightDown = true;
    if(emouse->button() == Qt::MiddleButton)
      m_MiddleDown = true;
    }

  else if (ev->type() == QEvent::MouseButtonRelease)
    {
    // Store what buttons are up or down
    if(emouse->button() == Qt::LeftButton)
      m_LeftDown = false;
    if(emouse->button() == Qt::RightButton)
      m_RightDown = false;
    if(emouse->button() == Qt::MiddleButton)
      m_MiddleDown = false;
    }
}

Vector3d
QtInteractionDelegateWidget
::GetEventWorldCoordinates(QMouseEvent *ev, bool flipY)
{
  // Make the parent window the current context
  QtAbstractOpenGLBox *parent = this->GetParentGLWidget();
  parent->makeCurrent();

  // Convert the event coordinates into the model view coordinates
  double modelMatrix[16], projMatrix[16];
  GLint viewport[4];
  glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
  glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
  glGetIntegerv(GL_VIEWPORT,viewport);

  // For retina displays, these are the 'logical' coordinates of the event
  int lx = ev->x();
  int ly = (flipY) ? parent->height() - 1 - ev->y() : ev->y();

  // Scale to actual pixels for the unproject call
  double px = lx * this->devicePixelRatio();
  double py = ly * this->devicePixelRatio();

  // Unproject to get the coordinate of the event
  Vector3d xProjection;
  gluUnProject(px, py, 0,
               modelMatrix,projMatrix,viewport,
               &xProjection[0], &xProjection[1], &xProjection[2]);
  return xProjection;
}

double QtInteractionDelegateWidget::GetNumberOfPixelsMoved(QMouseEvent *ev)
{
  QPoint delta = ev->pos() - m_LastPressPos;
  return std::sqrt((double)(delta.x() * delta.x() + delta.y() * delta.y()));
}
