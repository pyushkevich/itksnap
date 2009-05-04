/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ZoomPanInteractionMode.cxx,v $
  Language:  C++
  Date:      $Date: 2009/05/04 20:15:57 $
  Version:   $Revision: 1.3 $
  Copyright (c) 2007 Paul A. Yushkevich
  
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

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#include "ZoomPanInteractionMode.h"
#include "UserInterfaceBase.h"
#include "SliceWindowCoordinator.h"

#include <cmath>
#include <iostream>

ZoomPanInteractionMode
::ZoomPanInteractionMode(GenericSliceWindow *parent)
: GenericSliceWindow::EventHandler(parent)
{
  m_NeedUIUpdateOnRepaint = false;
}

int
ZoomPanInteractionMode
::OnMousePress(const FLTKEvent &event)
{
  if(event.SoftButton == FL_LEFT_MOUSE || event.SoftButton == FL_RIGHT_MOUSE)
    {
    // Record the current zoom and view position
    m_StartViewZoom = m_Parent->GetViewZoom();
    m_StartViewPosition = m_Parent->m_ViewPosition;

    // Done
    return 1;
    }
  else return 0;
}

int 
ZoomPanInteractionMode
::OnMouseDrag(const FLTKEvent &event,const FLTKEvent &dragEvent)
{
  if (dragEvent.SoftButton == FL_LEFT_MOUSE)
    {
    // Compute the start and end point in slice coordinates
    Vector3f xStart = m_Parent->MapWindowToSlice(dragEvent.XSpace.extract(2));
    Vector3f xEnd = m_Parent->MapWindowToSlice(event.XSpace.extract(2));
    Vector2f xOffset(xEnd[0] - xStart[0],xEnd[1] - xStart[1]);

    // Remove the scaling by spacing
    xOffset(0) *= m_Parent->m_SliceSpacing(0);
    xOffset(1) *= m_Parent->m_SliceSpacing(1);

    // Under the left button, the tool changes the view_pos by the
    // distance traversed
    m_Parent->SetViewPosition(m_StartViewPosition - xOffset);
    m_ParentUI->OnViewPositionsUpdate();
    } 
  else if (dragEvent.SoftButton == FL_RIGHT_MOUSE)
    {
    // Under the right button, the tool causes us to zoom based on the vertical
    // motion
    float zoom = 
    m_StartViewZoom * 
    pow(1.02f,(float)(event.XSpace(1) - dragEvent.XSpace(1)));

    // Clamp the zoom factor to reasonable limits
    zoom = m_ParentUI->GetSliceCoordinator()->ClampZoom(m_Parent->m_Id,zoom);

    // Make sure the zoom factor is an integer fraction
    zoom = m_Parent->GetOptimalZoom() * 
           ((int)(zoom / m_Parent->GetOptimalZoom() * 100)) / 100.0f;

    // Set the zoom factor using the window coordinator
    m_Parent->SetViewZoom(zoom);
    m_ParentUI->GetSliceCoordinator()->OnZoomUpdateInWindow(m_Parent->m_Id,zoom);

    // Schedule an update of the zoom percentage display in the parent
    m_NeedUIUpdateOnRepaint = true;
    } 
  else return 0;

  // Redraw the screen
  m_Parent->GetCanvas()->redraw();

  // Nothing to do here.
  return 1;
}

int 
ZoomPanInteractionMode
::OnMouseRelease(const FLTKEvent &event,
                 const FLTKEvent &irisNotUsed(dragEvent))
{
  // Eat the left and right buttons
  if(event.SoftButton == FL_LEFT_MOUSE || event.SoftButton == FL_RIGHT_MOUSE)
    return 1;
  else return 0;
}

void
ZoomPanInteractionMode
::OnDraw()
{
  if(m_NeedUIUpdateOnRepaint)
    {
    m_ParentUI->OnZoomUpdate();
    m_NeedUIUpdateOnRepaint = false;
    }   
}

