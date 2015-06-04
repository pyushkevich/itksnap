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
#include "GenericSliceModel.h"
#include "IRISApplication.h"
#include "GlobalState.h"
#include <QMouseEvent>
#include <MainImageWindow.h>
#include <LayerInspectorDialog.h>
#include <GenericImageData.h>
#include <QMenu>
#include <SNAPQtCommon.h>
#include "GenericSliceView.h"
#include "SliceViewPanel.h"


ThumbnailInteractionMode::ThumbnailInteractionMode(GenericSliceView *parent) :
    SliceWindowInteractionDelegateWidget(parent)
{
  connect(this, SIGNAL(customContextMenuRequested(QPoint)),
          this, SLOT(onContextMenuRequested(QPoint)));
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
  Vector2i x(ev->pos().x(),
             m_ParentModel->GetSizeReporter()->GetLogicalViewportSize()[1] - ev->pos().y());

  // Only react to left mouse button presses
  if(ev->button() == Qt::LeftButton && m_Model->CheckZoomThumbnail(x))
    {
    m_PanFlag = true;
    m_Model->BeginPan();
    ev->accept();
    }

  // Check for layer thumbnail hit.
  else if(this->m_ThumbnailLayer && ev->button() == Qt::LeftButton)
    {
    if(!m_ThumbnailLayer->IsSticky())
      m_Model->GetParent()->GetDriver()->GetGlobalState()->SetSelectedLayerId(
            m_ThumbnailLayer->GetUniqueId());
    ev->accept();
    }
}

void ThumbnailInteractionMode::mouseMoveEvent(QMouseEvent *ev)
{
  // Press position in screen pixels
  Vector2i x(ev->pos().x(),
             m_ParentModel->GetSizeReporter()->GetLogicalViewportSize()[1] - ev->pos().y());

  // If we are hovering over the thumbnail, it nice to indicate that to the user by
  // highlighting the thumbnail.
  if(m_ThumbnailLayer)
    {
    m_Model->GetParent()->SetHoveredThumbnailLayerId(m_ThumbnailLayer->GetUniqueId());
    }
  else
    {
    m_Model->GetParent()->SetHoveredThumbnailLayerId(-1ul);
    }

  ev->ignore();
  if(m_PanFlag)
    {
    Vector2i dx(ev->pos().x() - m_LastPressPos.x(),
                - (ev->pos().y() - m_LastPressPos.y()));
    m_Model->ProcessThumbnailPanGesture(-dx);
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

void ThumbnailInteractionMode::contextMenuEvent(QContextMenuEvent *ev)
{
  if(this->m_ThumbnailLayer)
    {
    emit customContextMenuRequested(ev->pos());
    ev->accept();
    }
}

void ThumbnailInteractionMode::onContextMenuRequested(const QPoint &pt)
{
  if(this->m_ThumbnailLayer)
    {
    // Instead of creating a separate context menu here, we use a context menu
    // from the corresponding row in the LayerInspector.
    MainImageWindow *winmain = findParentWidget<MainImageWindow>(this);
    LayerInspectorDialog *inspector = winmain->GetLayerInspector();

    // Get the menu
    QMenu *menu = inspector->GetLayerContextMenu(this->m_ThumbnailLayer);

    // Show the menu
    if(menu)
      menu->popup(QCursor::pos());
    }
}

void ThumbnailInteractionMode::enterEvent(QEvent *)
{
  // TODO: this is hideous!
  SliceViewPanel *panel = dynamic_cast<SliceViewPanel *>(m_ParentView->parent());
  panel->SetMouseMotionTracking(true);
}

void ThumbnailInteractionMode::leaveEvent(QEvent *)
{
  SliceViewPanel *panel = dynamic_cast<SliceViewPanel *>(m_ParentView->parent());
  panel->SetMouseMotionTracking(false);
  m_Model->GetParent()->SetHoveredThumbnailLayerId(-1ul);
}



