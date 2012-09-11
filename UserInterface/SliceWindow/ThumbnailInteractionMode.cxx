/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ThumbnailInteractionMode.cxx,v $
  Language:  C++
  Date:      $Date: 2009/06/16 19:05:37 $
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
#include "ThumbnailInteractionMode.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "UserInterfaceBase.h"

ThumbnailInteractionMode
::ThumbnailInteractionMode(GenericSliceWindow *parent) 
: GenericSliceWindow::EventHandler(parent)
{
  m_PanFlag = false;  
}

int
ThumbnailInteractionMode
::OnMousePress(const FLTKEvent &event)
{
  // Clear the pan flag
  m_PanFlag = false;

  // Only react to left mouse button presses
  if(event.SoftButton == FL_LEFT_MOUSE)
    {
    // Check if the event is inside of the thumbnail boundaries
    Vector2i xThumb = m_Parent->m_ThumbnailPosition;
    Vector2i sThumb = m_Parent->m_ThumbnailSize;
    if(m_Parent->IsThumbnailOn() &&
      event.XCanvas[0] > xThumb[0] && 
      event.XCanvas[0] < xThumb[0] + sThumb[0] &&
      event.XCanvas[1] > xThumb[1] && 
      event.XCanvas[1] < xThumb[1] + sThumb[1])
      {
      // Set the panning flag
      m_PanFlag = true;

      // Store the starting view position
      m_StartViewPosition = m_Parent->m_ViewPosition;

      // Return success
      return 1;
      }
    }

  // Pass the event on
  return 0;
}

int
ThumbnailInteractionMode
::OnMouseRelease(const FLTKEvent &event, 
                 const FLTKEvent &pressEvent)
{
  // Call the drag code
  return OnMouseDrag(event, pressEvent);
}

int
ThumbnailInteractionMode
::OnMouseDrag(const FLTKEvent &event, 
              const FLTKEvent &pressEvent)
{
  // Must be in pan mode
  if(m_PanFlag && event.SoftButton == FL_LEFT_MOUSE)
    {
    // Get the number of pixels that the thumbnail was moved by
    Vector2i xMoved = pressEvent.XCanvas - event.XCanvas;

    // Figure out how this movement translates to space units
    Vector2f xOffset(
      xMoved[0] / m_Parent->m_ThumbnailZoom,
      xMoved[1] / m_Parent->m_ThumbnailZoom);

    // Add to the position
    m_Parent->m_ViewPosition = m_StartViewPosition - xOffset;
    m_Parent->m_ParentUI->OnViewPositionsUpdate();

    // Tell parent to repaint
    m_Parent->GetCanvas()->redraw();

    // The event's been handled
    return 1;
    }
  else return 0;
}

void 
ThumbnailInteractionMode::
OnDraw()
{
}

