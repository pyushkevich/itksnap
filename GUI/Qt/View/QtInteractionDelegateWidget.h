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

#ifndef QTINTERACTIONDELEGATEWIDGET_H
#define QTINTERACTIONDELEGATEWIDGET_H

#include <QWidget>
#include <SNAPCommon.h>
#include <QMouseEvent>
#include <SNAPComponent.h>

class QtAbstractOpenGLBox;
class QGestureEvent;
class GenericSliceModel;

class QtInteractionDelegateWidget : public SNAPComponent
{
  Q_OBJECT

public:
  explicit QtInteractionDelegateWidget(QWidget *parent = 0);

signals:

public slots:

protected:

  // Event handler
  virtual bool event(QEvent *);

  // Handler for event filtering
  virtual bool eventFilter(QObject *, QEvent *ev);

  // Children may override this method to provide additional event processing,
  // but should call QtInteractionDelegateWidget::preprocessEvent in there.
  virtual void preprocessEvent(QEvent *);

  virtual void postprocessEvent(QEvent *);

  // Get the world coordinates of the Qt event. The default implementation uses
  // the current GL viewport, projection and model matrices of the parent GL
  // widget.
  virtual Vector3d GetEventWorldCoordinates(QMouseEvent *ev, bool flipY);

  // Gesture event handler
  virtual bool gestureEvent(QGestureEvent *ev)
    { return false; }

  /**
   * Returns true when at least one mouse button is down and the corresponding
   * press event was accepted by the widget
   */
  virtual bool isDragging();

  /**
   * Returns true if none of the mouse buttons are pressed.
   */
  virtual bool isHovering();

  // Return the number of pixels moved since last press
  double GetNumberOfPixelsMoved(QMouseEvent *ev);

  // Get a pointer to the parent GL widget
  QtAbstractOpenGLBox *GetParentGLWidget() const;

  // Information about the mouse press event
  QPoint m_LastPressPos, m_LastPressGlobalPos;
  Qt::MouseButton m_LastPressButton;

  // Spatial coordinates of the last press event, current event
  Vector3d m_LastPressXSpace, m_XSpace;

  // Status for a mouse button
  enum ButtonStatus {
    NOT_PRESSED = 0,      // The button is not pressed
    PRESS_ACCEPTED,       // The button is pressed and press was accepted (dragging)
    PRESS_IGNORED         // The button is pressed and press was ignored
  };

  // Whether we are between a press and a release for a particular button
  ButtonStatus m_LeftStatus, m_RightStatus, m_MiddleStatus;

  // Whether we are currently filtering an event from another widget
  bool m_Filtering;
};

#endif // QTINTERACTIONDELEGATEWIDGET_H
