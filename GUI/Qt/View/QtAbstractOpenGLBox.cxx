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
#include <QWindow>
#include <QtInteractionDelegateWidget.h>
#include "LatentITKEventNotifier.h"
#include "SNAPOpenGL.h"
#include <AbstractRenderer.h>
#include "SNAPQtCommon.h"

#include <vtkSmartPointer.h>
#include <vtkImageData.h>

#include "GLToPNG.h"

QtAbstractOpenGLBox::QtAbstractOpenGLBox(QWidget *parent) :
    QOpenGLWidget(parent)
{
  m_NeedResizeOnNextRepaint = false;
  m_GrabFocusOnEntry = false;
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

bool QtAbstractOpenGLBox::SaveScreenshot(std::string filename)
{
  m_ScreenshotRequest = from_utf8(filename);
  this->repaint();
  return true;
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

  // Get width and height in pixels
  int wp = (int) this->size().width() * this->devicePixelRatio();
  int hp = (int) this->size().height() * this->devicePixelRatio();

  // Qt bug workaround
  if(m_NeedResizeOnNextRepaint)
    {
    GetRenderer()->resizeGL(wp, hp, this->devicePixelRatio());
    m_NeedResizeOnNextRepaint = false;
    }

  // Do the actual painting
  GetRenderer()->paintGL();

  // If screenshot set, handle it
  if(m_ScreenshotRequest.length())
    {
    try
      {
      vtkSmartPointer<vtkImageData> image = GLToVTKImageData(
            (unsigned int) GL_RGB, 0, 0, wp, hp);
      std::string fn = to_utf8(m_ScreenshotRequest);
      VTKImageDataToPNG(image, fn.c_str());
      }
    catch(...)
      {
      // TODO: should we notify user of failures?
      }
    m_ScreenshotRequest = QString();
    }
}

void QtAbstractOpenGLBox::resizeGL(int w, int h)
{
  int wp = (int) this->size().width() * this->devicePixelRatio();
  int hp = (int) this->size().height() * this->devicePixelRatio();

  GetRenderer()->Update();
  GetRenderer()->resizeGL(wp, hp, this->devicePixelRatio());
}

void QtAbstractOpenGLBox::initializeGL()
{
  GetRenderer()->Update();
  GetRenderer()->initializeGL();
}

/*
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
*/

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









