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
#include "RegistrationInteractionMode.h"
#include "RegistrationRenderer.h"
#include "InteractiveRegistrationModel.h"
#include "GenericSliceView.h"

RegistrationInteractionMode::RegistrationInteractionMode(GenericSliceView *parent)
  : SliceWindowInteractionDelegateWidget(parent),
    m_Model(NULL)
{
  // Create the renderer
  m_Renderer = RegistrationRenderer::New();
  m_Renderer->SetParentRenderer(
        static_cast<GenericSliceRenderer *>(parent->GetRenderer()));
}

RegistrationInteractionMode::~RegistrationInteractionMode()
{

}

void RegistrationInteractionMode::SetModel(InteractiveRegistrationModel *model)
{
  m_Model = model;
  m_Renderer->SetModel(model);
  this->SetParentModel(model->GetParent());

  connectITK(m_Model, StateMachineChangeEvent());
  connectITK(m_Model, ModelUpdateEvent());
}


void RegistrationInteractionMode::onModelUpdate(const EventBucket &bucket)
{
  this->update();
}


void RegistrationInteractionMode::mousePressEvent(QMouseEvent *ev)
{
  if(m_Model->ProcessPushEvent(m_XSlice))
    ev->accept();
}

void RegistrationInteractionMode::mouseMoveEvent(QMouseEvent *ev)
{
  ev->ignore();
  if(this->IsMouseOverFullLayer())
    {
    if(this->isDragging())
      {
      if(m_Model->ProcessDragEvent(m_XSlice, m_LastPressXSlice))
        ev->accept();
      }
    else
      {
      if(m_Model->ProcessMouseMoveEvent(m_XSlice, this->m_HoverOverLayer->GetUniqueId()))
        ev->accept();
      }
    }
}

void RegistrationInteractionMode::mouseReleaseEvent(QMouseEvent *ev)
{
  if(this->isDragging())
    {
    if(m_Model->ProcessReleaseEvent(m_XSlice, m_LastPressXSlice))
      ev->accept();
    }
}




