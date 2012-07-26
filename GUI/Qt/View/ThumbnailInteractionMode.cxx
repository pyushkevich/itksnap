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

#include "OrthogonalSliceCursorNavigationModel.h"
#include "ThumbnailInteractionMode.h"
#include <QMouseEvent>

ThumbnailInteractionMode::ThumbnailInteractionMode(GenericSliceView *parent) :
    SliceWindowInteractionDelegateWidget(parent)
{

}

void ThumbnailInteractionMode
::SetModel(OrthogonalSliceCursorNavigationModel *model)
{
  m_Model = model;
  m_PanFlag = false;
  this->SetParentModel(model->GetParent());
}


void ThumbnailInteractionMode::mousePressEvent(QMouseEvent *ev)
{
  // Press position in screen pixels
  Vector2i x(ev->pos().x(), this->height() - ev->pos().y());

  // Only react to left mouse button presses
  if(ev->button() == Qt::LeftButton && m_Model->CheckThumbnail(x))
    {
    m_PanFlag = true;
    m_Model->BeginPan();
    ev->accept();
    }
}

void ThumbnailInteractionMode::mouseMoveEvent(QMouseEvent *ev)
{
  // Press position in screen pixels
  Vector2i x(ev->pos().x(), this->height() - ev->pos().y());

  ev->ignore();
  if(m_PanFlag)
    {
    Vector2i dx(ev->pos().x() - m_LastPressPos.x(),
                - (ev->pos().y() - m_LastPressPos.y()));
    m_Model->ProcessThumbnailPanGesture(-dx);
    ev->accept();
    }
  else if(!isDragging() && m_Model->CheckThumbnail(x))
    {
    // When the view is responding to hover events, we want to consume
    // mouse hovering that occurs over the thumbnail. Otherwise the user
    // would not see the effect of the hovering
    ev->accept();
    }
}

void ThumbnailInteractionMode::mouseReleaseEvent(QMouseEvent *ev)
{
  this->mouseMoveEvent(ev);
  if(m_PanFlag)
    m_Model->EndPan();
  m_PanFlag = false;
}


