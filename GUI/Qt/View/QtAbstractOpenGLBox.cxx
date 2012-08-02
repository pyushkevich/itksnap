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

#include "QtAbstractOpenGLBox.h"
#include <QMouseEvent>
#include <QStackedLayout>
#include <QtInteractionDelegateWidget.h>
#include "LatentITKEventNotifier.h"
#include <AbstractRenderer.h>

QtAbstractOpenGLBox::QtAbstractOpenGLBox(QWidget *parent) :
    QGLWidget(parent)
{
  m_NeedResizeOnNextRepaint = false;
  m_GrabFocusOnEntry = false;
}

Vector3d QtAbstractOpenGLBox::GetEventWorldCoordinates(QMouseEvent *ev, bool flipY)
{
  // Make this window the current context
  this->makeCurrent();

  // Convert the event coordinates into the model view coordinates
  double modelMatrix[16], projMatrix[16];
  GLint viewport[4];
  glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
  glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
  glGetIntegerv(GL_VIEWPORT,viewport);

  int x = ev->x();
  int y = (flipY) ? this->height() - 1 - ev->y() : ev->y();

  // Unproject to get the coordinate of the event
  Vector3d xProjection;
  gluUnProject(x, y, 0,
               modelMatrix,projMatrix,viewport,
               &xProjection[0], &xProjection[1], &xProjection[2]);
  return xProjection;
}

void QtAbstractOpenGLBox::AttachSingleDelegate(QtInteractionDelegateWidget *delegate)
{
  // Install the delegate
  // QStackedLayout *l = new QStackedLayout();
  // l->addWidget(delegate);
  // this->setLayout(l);

  // Delegate handles all of our events
  this->installEventFilter(delegate);
}


void
QtAbstractOpenGLBox
::connectITK(itk::Object *src, const itk::EventObject &ev, const char *slot)
{
  LatentITKEventNotifier::connect(src, ev, this, slot);
}

void QtAbstractOpenGLBox::paintGL()
{
  // Update the renderer. This will cause the renderer to update itself
  // based on any events that it has received upstream.
  GetRenderer()->Update();

  // Qt bug workaround
  if(m_NeedResizeOnNextRepaint)
    {
    GetRenderer()->resizeGL(this->size().width(), this->size().height());
    m_NeedResizeOnNextRepaint = false;
    }

  // Do the actual painting
  GetRenderer()->paintGL();
}

void QtAbstractOpenGLBox::resizeGL(int w, int h)
{
  GetRenderer()->Update();
  GetRenderer()->resizeGL(w, h);
}

void QtAbstractOpenGLBox::initializeGL()
{
  GetRenderer()->Update();
  GetRenderer()->initializeGL();
}

void QtAbstractOpenGLBox::resizeEvent(QResizeEvent *)
{
  // This is a workaround for a Qt bug. It didn't take long to find bugs
  // in Qt. How sad.
  m_NeedResizeOnNextRepaint = true;

  // Set geometry of all child widgets (which are interactors)
  QList<QWidget *> kids = this->findChildren<QWidget *>();
  for(int i = 0; i < kids.size(); i++)
    kids.at(i)->setGeometry(this->geometry());
}


void QtAbstractOpenGLBox::enterEvent(QEvent *)
{
  if(m_GrabFocusOnEntry)
    this->setFocus();
}

void QtAbstractOpenGLBox::leaveEvent(QEvent *)
{
  if(m_GrabFocusOnEntry)
    this->clearFocus();
}









