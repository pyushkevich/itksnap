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

#include "GenericSliceView.h"
#include "CrosshairsInteractionMode.h"
#include "LatentITKEventNotifier.h"

GenericSliceView::GenericSliceView(QWidget *parent) :
  SNAPQGLWidget(parent)
{
  m_Model = NULL;
  m_Renderer = GenericSliceRenderer::New();
  m_NeedResizeOnNextRepaint = false;
}

void GenericSliceView::SetModel(GenericSliceModel *model)
{
  // Set the model
  this->m_Model = model;

  // Pass the model to the renderer
  m_Renderer->SetModel(model);

  // Listen to the update events on the model. In response, simply repaint
  LatentITKEventNotifier::connect(
        m_Model, ModelUpdateEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));

  LatentITKEventNotifier::connect(
        m_Model, SliceModelGeometryChangeEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));

  LatentITKEventNotifier::connect(
        m_Renderer, AppearanceUpdateEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));

  // Add listeners to events supported by the model
  /*
  AddListener(m_Model, SliceModelImageDimensionsChangeEvent(),
              this, &GenericSliceView::OnModelUpdate);

  AddListener(m_Model, SliceModelGeometryChangeEvent(),
              this, &GenericSliceView::OnModelUpdate);
              */

  // Tell model about our current size
  m_Model->onViewResize(this->size().width(), this->size().height());
}

void GenericSliceView::paintGL()
{
  // Update the renderer. This will cause the renderer to update itself
  // based on any events that it has received upstream.
  m_Renderer->Update();

  if(m_NeedResizeOnNextRepaint)
    {
    m_Renderer->resizeGL(this->size().width(), this->size().height());
    m_NeedResizeOnNextRepaint = false;
    }
  m_Renderer->paintGL();
}

void GenericSliceView::resizeGL(int w, int h)
{
  m_Renderer->Update();
  m_Renderer->resizeGL(w, h);
}

void GenericSliceView::initializeGL()
{
  m_Renderer->Update();
  m_Renderer->initializeGL();
}

void GenericSliceView::resizeEvent(QResizeEvent *ev)
{
  // Notify the model of the repaint
  m_Model->onViewResize(ev->size().width(), ev->size().height());

  // This is a workaround for a Qt bug. It didn't take long to find bugs
  // in Qt. How sad.
  m_NeedResizeOnNextRepaint = true;

  // Set geometry of all child widgets (which are interactors)
  QList<QWidget *> kids = this->findChildren<QWidget *>();
  for(int i = 0; i < kids.size(); i++)
    kids.at(i)->setGeometry(this->geometry());
}

void GenericSliceView::onModelUpdate(const EventBucket &b)
{
  // Make sure the model is up to date
  m_Model->Update();

  // Ask this widget to repaint
  this->update();
}

void GenericSliceView::enterEvent(QEvent *)
{
  this->setFocus();
}

void GenericSliceView::leaveEvent(QEvent *)
{
  this->clearFocus();
}

