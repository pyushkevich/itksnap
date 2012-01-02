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

#ifndef SNAPQGLWIDGET_H
#define SNAPQGLWIDGET_H

#include <QGLWidget>
#include <SNAPCommon.h>
#include <SNAPEvents.h>

class QMouseEvent;
class EventBucket;
class QtInteractionDelegateWidget;

class SNAPQGLWidget : public QGLWidget
{
  Q_OBJECT

public:
  explicit SNAPQGLWidget(QWidget *parent = 0);

  // Mouse events
  virtual bool event(QEvent *);

  /** Map the coordinates of a mouse event into 3d spatial coordinates
    * based on the current GL transform. The Y coordinate can be optionally
    * flipped
    */
  Vector3d GetEventWorldCoordinates(QMouseEvent *ev, bool flipY);

  /**
    Use this function when the GL widget is only associated with a single
    interaction mode.
    */
  void AttachSingleDelegate(QtInteractionDelegateWidget *delegate);

public slots:

  // Default slot for model updates
  virtual void onModelUpdate(const EventBucket &bucket) {}

protected:

  /** Register to receive ITK events from object src. Events will be cached in
    an event bucket and delivered once execution returns to the UI loop */
  void connectITK(itk::Object *src, const itk::EventObject &ev,
                  const char *slot = SLOT(onModelUpdate(const EventBucket &)));


  // Whether the user is dragging the mouse currently. More specifically
  // this is on between mouse press and mouse release events
  bool m_Dragging;

  // The cursor location where the dragging started. This is the coordinate
  // of the last mouse press event.
  QPoint m_DragStart;

signals:

public slots:

};

#endif // SNAPQGLWIDGET_H
