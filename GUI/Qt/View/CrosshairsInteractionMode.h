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

#ifndef CROSSHAIRSINTERACTIONMODE_H
#define CROSSHAIRSINTERACTIONMODE_H

#include <SliceWindowInteractionDelegateWidget.h>
#include <SNAPCommon.h>

class GenericSliceModel;
class OrthogonalSliceCursorNavigationModel;

class CrosshairsInteractionMode : public SliceWindowInteractionDelegateWidget
{
  Q_OBJECT

public:
  explicit CrosshairsInteractionMode(QWidget *parent, QWidget *canvasWidget);
  ~ CrosshairsInteractionMode();

  void SetModel(OrthogonalSliceCursorNavigationModel *model);

  virtual void mousePressEvent(QMouseEvent *ev) override;
  virtual void mouseMoveEvent(QMouseEvent *) override;
  virtual void mouseReleaseEvent(QMouseEvent *) override;
  virtual void wheelEvent(QWheelEvent *) override;

  virtual void enterEvent(QEnterEvent *) override;
  virtual void leaveEvent(QEvent *) override;

  virtual bool gestureEvent(QGestureEvent *) override;

  // Handle keystrokes
  virtual void keyPressEvent(QKeyEvent *) override;

  void SetMouseButtonBehaviorToCrosshairsMode();
  void SetMouseButtonBehaviorToZoomPanMode();

  // Set the widget to which the wheel events should be forwarded
  void SetWheelEventTargetWidget(QWidget *w);

signals:

public slots:

protected:
  OrthogonalSliceCursorNavigationModel *m_Model;

  // The behavior of buttons when envoking zoom/pan/cursor actions
  Qt::MouseButton m_BtnCursor, m_BtnZoom, m_BtnPan;

  // Widget to which wheel events are forwarded
  QWidget *m_WheelEventTarget;

  // Internal used to emulate right/middle button
  Qt::MouseButton GetButtonForEvent(QMouseEvent *ev);
  Qt::MouseButton m_LastPressEmulatedButton;
};

#endif // CROSSHAIRSINTERACTIONMODE_H
