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

#ifndef QTABSTRACTOPENGLBOX_H
#define QTABSTRACTOPENGLBOX_H

#include <QOpenGLWidget>
#include <SNAPCommon.h>
#include <SNAPEvents.h>

class QMouseEvent;
class EventBucket;
class QtInteractionDelegateWidget;
class AbstractRenderer;

namespace itk { class Object; }

class QtAbstractOpenGLBox : public QOpenGLWidget
{
  Q_OBJECT

public:
  explicit QtAbstractOpenGLBox(QWidget *parent = 0);

  // Whether to grab keyboard focus when the mouse enters this widget
  irisGetSetMacro(GrabFocusOnEntry, bool)

  /**
    Use this function when the GL widget is only associated with a single
    interaction mode.
    */
  void AttachSingleDelegate(QtInteractionDelegateWidget *delegate);

  /**
    The child class must override this method to return its pointer to the
    renderer object. Every instance of this object must have a renderer.
    */
  virtual AbstractRenderer* GetRenderer() const = 0;

  /**
   Take a screenshot, save to a PNG file
   */
  virtual bool SaveScreenshot(std::string filename);

public slots:

  // Default slot for model updates
  virtual void onModelUpdate(const EventBucket &bucket) {}

protected:

  /** Register to receive ITK events from object src. Events will be cached in
    an event bucket and delivered once execution returns to the UI loop */
  void connectITK(itk::Object *src, const itk::EventObject &ev,
                  const char *slot = SLOT(onModelUpdate(const EventBucket &)));

  // OpenGL painter methods
  virtual void paintGL();
  virtual void resizeGL(int w, int h);
  virtual void initializeGL();

  // Resize event
  // virtual void resizeEvent(QResizeEvent *);
  virtual void enterEvent(QEvent *);
  virtual void leaveEvent(QEvent *);

  // Whether we need to call GL resize next time a paint command occurs
  bool m_NeedResizeOnNextRepaint;

  // Whether this widget grabs keyboard focus when the mouse enters it
  bool m_GrabFocusOnEntry;

  // Whether a screenshot has been requested (non-empty string)
  QString m_ScreenshotRequest;

signals:

public slots:

};



#endif // QTABSTRACTOPENGLBOX_H
