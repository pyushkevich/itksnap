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
#include <MainImageWindow.h>
#include <LayerInspectorDialog.h>
#include <GenericImageData.h>
#include <QMenu>
#include <SNAPQtCommon.h>
#include <GlobalUIModel.h>
#include <LayerSelectionModel.h>
#include "QtReporterDelegates.h"

GenericSliceView::GenericSliceView(QWidget *parent) :
  QtAbstractOpenGLBox(parent)
{
  m_Model = NULL;
  m_Renderer = GenericSliceRenderer::New();

  m_ViewportReporter = QtViewportReporter::New();
  m_ViewportReporter->SetClientWidget(this);

  // We need to grab keyboard focus
  this->SetGrabFocusOnEntry(true);
}

void GenericSliceView::SetModel(GenericSliceModel *model)
{
  // Set the model
  m_Model = model;

  // Pass the viewport reporter to the model
  m_Model->SetSizeReporter(m_ViewportReporter);

  // Pass the model to the renderer
  m_Renderer->SetModel(m_Model);

  // Listen to the update events on the model. In response, simply repaint
  connectITK(m_Model, ModelUpdateEvent());
  connectITK(m_Model, SliceModelGeometryChangeEvent());
  connectITK(m_Renderer, AppearanceUpdateEvent());
}


void GenericSliceView::onModelUpdate(const EventBucket &b)
{
  // Make sure the model is up to date
  m_Model->Update();

  // Ask this widget to repaint
  this->update();
}

