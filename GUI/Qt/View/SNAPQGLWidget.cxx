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

#include "SNAPQGLWidget.h"
#include <QMouseEvent>
#include <QStackedLayout>
#include <QtInteractionDelegateWidget.h>
#include "LatentITKEventNotifier.h"

SNAPQGLWidget::SNAPQGLWidget(QWidget *parent) :
    QGLWidget(parent)
{
  m_Dragging = false;

}

Vector3d SNAPQGLWidget::GetEventWorldCoordinates(QMouseEvent *ev, bool flipY)
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

bool SNAPQGLWidget::event(QEvent *ev)
{
  // Before dealing with interactors, take care of the dragging stuff
  if(ev->type() == QEvent::MouseButtonPress)
    {
    m_Dragging = true;
    m_DragStart = static_cast<QMouseEvent *>(ev)->pos();
    }
  else if(ev->type() == QEvent::MouseButtonRelease)
    {
    m_Dragging = false;
    }

  return QGLWidget::event(ev);
}

void SNAPQGLWidget::AttachSingleDelegate(QtInteractionDelegateWidget *delegate)
{
  QStackedLayout *l = new QStackedLayout();
  l->addWidget(delegate);
  this->setLayout(l);
}


void
SNAPQGLWidget
::connectITK(itk::Object *src, const itk::EventObject &ev, const char *slot)
{
  LatentITKEventNotifier::connect(src, ev, this, slot);
}


