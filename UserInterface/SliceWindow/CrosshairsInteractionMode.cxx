/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: CrosshairsInteractionMode.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/19 20:28:56 $
  Version:   $Revision: 1.13 $
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
#include "CrosshairsInteractionMode.h"
#include "SNAPOpenGL.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "SNAPAppearanceSettings.h"
#include "UserInterfaceBase.h"
#include "SliceWindowCoordinator.h"

CrosshairsInteractionMode
::CrosshairsInteractionMode(
  GenericSliceWindow *parent, Button cursor_button, Button zoom_button, Button pan_button) 
: GenericSliceWindow::EventHandler(parent), 
  m_BtnCursor(cursor_button), m_BtnZoom(zoom_button), m_BtnPan(pan_button)
{
  m_NeedToRepaintControls = false;
  m_LastViewposUpdateTime = 0;
  m_NeedUIUpdateOnRepaint = false;
}

int
CrosshairsInteractionMode
::OnMousePress(const FLTKEvent &event)
{
  if(m_BtnCursor == ANY || event.SoftButton == m_BtnCursor)
    {
    UpdateCrosshairs(event);
    m_Parent->m_GlobalState->SetUpdateSliceFlag(0);
    m_RepeatEvent = event;
    }
  else if(m_BtnZoom == ANY || event.SoftButton == m_BtnZoom)
    {
    m_StartViewZoom = m_Parent->GetViewZoom();
    }
  else if(m_BtnPan == ANY || event.SoftButton == m_BtnPan)
    {
    m_StartViewPosition = m_Parent->m_ViewPosition;
    }
  return 1;
}

int
CrosshairsInteractionMode
::OnMouseRelease(const FLTKEvent &event,const FLTKEvent &dragEvent)
{
  if(m_BtnCursor == ANY || dragEvent.SoftButton == m_BtnCursor)
    {
    UpdateCrosshairs(event);
    m_Parent->m_GlobalState->SetUpdateSliceFlag(1);
    m_RepeatEvent = event;
    return 1;
    }
  else if(m_BtnZoom == ANY || dragEvent.SoftButton == m_BtnZoom)
    return 1;
  else if(m_BtnPan == ANY || dragEvent.SoftButton == m_BtnPan)
    return 1;

  return 0;
}

int
CrosshairsInteractionMode
::OnMouseDrag(const FLTKEvent &event, 
              const FLTKEvent &dragEvent)
{
  if(m_BtnCursor == ANY || dragEvent.SoftButton == m_BtnCursor)
    {
    UpdateCrosshairs(event);
    m_Parent->m_GlobalState->SetUpdateSliceFlag(1);
    m_RepeatEvent = event;
    return 1;
    }
  else if(m_BtnZoom == ANY || dragEvent.SoftButton == m_BtnZoom)
    {
    // Under the right button, the tool causes us to zoom based on the vertical
    // motion
    float zoom = m_StartViewZoom 
      * pow(1.02f,(float)(event.XSpace(1) - dragEvent.XSpace(1)));


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
    return 1;
    } 
  else if (m_BtnPan == ANY || dragEvent.SoftButton == m_BtnPan)
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
    return 1;
    } 
  return 0;
}

int
CrosshairsInteractionMode
::OnMouseWheel(const FLTKEvent &irisNotUsed(event))
{
  // Must have the cursor inside the window to process key events
  if(!m_Parent->GetCanvas()->GetFocus()) return 0;

  // Get the amount of the scroll
  float scroll = (float) Fl::event_dy();
  
  // Get the cross-hairs position in image space
  Vector3ui xCrossImage = m_Driver->GetCursorPosition();

  // Map it into slice space
  Vector3f xCrossSlice = 
    m_Parent->MapImageToSlice(to_float(xCrossImage) + Vector3f(0.5f));

  // Advance by the scroll amount
  xCrossSlice[2] += scroll;

  // Map back into display space
  xCrossImage = to_unsigned_int(m_Parent->MapSliceToImage(xCrossSlice));

  // Clamp by the display size
  Vector3ui xSize = m_Driver->GetCurrentImageData()->GetVolumeExtents();
  Vector3ui xCrossClamped = xCrossImage.clamp(
    Vector3ui(0,0,0),xSize - Vector3ui(1,1,1));

  // Update the crosshairs position in the global state
  m_Driver->SetCursorPosition(xCrossClamped);

  // Cause a repaint
  m_NeedToRepaintControls = true;

  // Update the crosshairs position in the current image data
  m_ParentUI->OnCrosshairPositionUpdate(true);  
  m_ParentUI->RedrawWindows();  

  return 1;
}

void
CrosshairsInteractionMode
::UpdateCrosshairs(const Vector3f &xClick)
{
  // Compute the new cross-hairs position in image space
  Vector3f xCross = m_Parent->MapSliceToImage(xClick);

  // Round the cross-hairs position down to integer
  Vector3i xCrossInteger = to_int(xCross);
  
  // Make sure that the cross-hairs position is within bounds by clamping
  // it to image dimensions
  Vector3i xSize = to_int(m_Driver->GetCurrentImageData()->GetVolumeExtents());
  Vector3ui xCrossClamped = to_unsigned_int(
    xCrossInteger.clamp(Vector3i(0),xSize - Vector3i(1)));

  // Update the crosshairs position in the global state
  m_Driver->SetCursorPosition(xCrossClamped);

  // Cause a repaint
  m_NeedToRepaintControls = true;

  // Update the crosshairs position in the current image data
  m_ParentUI->OnCrosshairPositionUpdate(true);  
  m_ParentUI->RedrawWindows();  
}


void
CrosshairsInteractionMode
::TimeoutCallback(void *vp)
{
  CrosshairsInteractionMode *cim  = (CrosshairsInteractionMode *)vp;
  FLTKEvent ev;
  if(cim->GetCanvas()->IsDragging())
    {
    FLTKEvent ev = cim->m_RepeatEvent;
    ev.TimeStamp = FLTKEventTimeStamp();
    cim->UpdateCrosshairs(ev);
    }
}



void
CrosshairsInteractionMode
::UpdateCrosshairs(const FLTKEvent &event)
{
  // Compute the position in slice coordinates
  Vector3f xClick = m_Parent->MapWindowToSlice(event.XSpace.extract(2));

  // Check if the cursor is in one of the hot zones
  Vector3f z0 = m_Parent->MapWindowToSlice(
    Vector2f(0.1 * m_Parent->GetCanvas()->w(),0.1 * m_Parent->GetCanvas()->h()));
  Vector3f z1 = m_Parent->MapWindowToSlice(
    Vector2f(0.9 * m_Parent->GetCanvas()->w(),0.9 * m_Parent->GetCanvas()->h()));
  Vector3f y0 = m_Parent->MapWindowToSlice(
    Vector2f(0.0 * m_Parent->GetCanvas()->w(),0.0 * m_Parent->GetCanvas()->h()));
  Vector3f y1 = m_Parent->MapWindowToSlice(
    Vector2f(1.0 * (m_Parent->GetCanvas()->w()-1),1.0 * (m_Parent->GetCanvas()->h()-1)));
 
  bool hotzone = false;
  Vector2f newViewPos = m_Parent->m_ViewPosition;
  for(size_t i = 0; i < 2; i++)
    {
    // There are three discrete speeds
    int pixel_speeds[] = {5, 10, 20};
    int min_voxel_speeds[] = {1, 2, 4};

    double zmin = std::min(z0[i], z1[i]), zmax = std::max(z0[i], z1[i]);
    double ymin = std::min(y0[i], y1[i]), ymax = std::max(y0[i], y1[i]);

    if(xClick[i] < zmin)
      {
      // The speed is based on how close to the border we are
      double relspeed = (zmin - xClick[i]) / (zmin - ymin);
      int speedidx = std::min((int) (relspeed * 2.0), 2);
      int pixspeed = pixel_speeds[speedidx];
      
      // We want to move 5 screen pixels every 0.1 sec. How many voxels is that
      int nvox = std::max(min_voxel_speeds[speedidx], 
        (int) (0.5 + pixspeed / (m_Parent->m_ViewZoom * m_Parent->m_SliceSpacing[i])));

      
      // Move the cursor left by nvox voxel
      newViewPos[i] -= nvox * m_Parent->m_SliceSpacing[i];
      hotzone = true;
      }
    else if(xClick[i] > zmax)
      {
      // The speed is based on how close to the border we are
      double relspeed = (xClick[i] - zmax) / (ymax - zmax);
      int speedidx = std::min((int) (relspeed * 2.0), 2);
      int pixspeed = pixel_speeds[speedidx];
      
      // We want to move 5 screen pixels every 0.1 sec. How many voxels is that
      int nvox = std::max(min_voxel_speeds[speedidx], 
        (int) (0.5 + pixspeed / (m_Parent->m_ViewZoom * m_Parent->m_SliceSpacing[i])));      

      // Move the cursor left by 1 voxel
      newViewPos[i] += nvox * m_Parent->m_SliceSpacing[i];
      hotzone = true;
      }
    }

  // Hotzone is disabled when whole slice is visible
  if(m_Parent->m_ViewZoom <= m_Parent->m_OptimalZoom)
    hotzone = false;

  // Check apperarance settings
  if(!m_ParentUI->GetAppearanceSettings()->GetFlagAutoPan())
    hotzone = false;

  // Now we are ready to do something about the hotzome
  if(hotzone)
    {
    // If we were in the hotzone less that 100 ms ago, forget it
    static FLTKEventTimeStamp last_hotzone_action;
    FLTKEventTimeStamp tsnow;
    if(tsnow.ElapsedUSecFrom(last_hotzone_action) > 100000)
      {
      m_Parent->SetViewPosition(newViewPos);
      m_ParentUI->OnViewPositionsUpdate(true);
      last_hotzone_action = tsnow;
      }
  
    // Even if we were too soon for a hotzone, schedule the check in a few ms
    Fl::add_timeout(0.02, CrosshairsInteractionMode::TimeoutCallback, this);
    }
  
  xClick = m_Parent->MapWindowToSlice(event.XSpace.extract(2));

  UpdateCrosshairs(xClick);
}

int 
CrosshairsInteractionMode
::OnKeyDown(const FLTKEvent &event)
{
  // Must have the cursor inside the window to process key events
  if(!m_Parent->GetCanvas()->GetFocus()) return 0;

  // Vector encoding the movement in pixels
  Vector3f xMotion(0.0f);

  // Check the shift state
  float xStep = (event.State & FL_SHIFT) ? 5.0f : 1.0f;

  // Handle up, down, left, right, pg-up and pg-down events
  switch(Fl::event_key())
    {
    case FL_Right:     xMotion[0] += xStep; break;
    case FL_Left:      xMotion[0] -= xStep; break;
    case FL_Down:      xMotion[1] -= xStep; break;
    case FL_Up:        xMotion[1] += xStep; break;
    case FL_Page_Up:   xMotion[2] -= xStep; break;
    case FL_Page_Down: xMotion[2] += xStep; break;
    default: return 0;
    };

  // Get the crosshair coordinates, in slice space
  Vector3f xCross = 
    m_Parent->MapImageToSlice(
      to_float(m_Driver->GetCursorPosition()));

  // Add the motion vector
  UpdateCrosshairs(xCross + xMotion);

  // Handled!
  return 1;
}

void 
CrosshairsInteractionMode::
OnDraw()
{
  // Do the painting only if necessary
  if(m_NeedToRepaintControls)
    {
    // Update the image probe and scrollbar controls
    m_ParentUI->OnCrosshairPositionUpdate(false);
    
    // No need to call this until another update
    m_NeedToRepaintControls = false;
    }

  if(m_NeedUIUpdateOnRepaint)
    {
    m_ParentUI->OnZoomUpdate();
    m_NeedUIUpdateOnRepaint = false;
    }

  // Get the line color, thickness and dash spacing for the crosshairs
  SNAPAppearanceSettings::Element elt = 
    m_Parent->m_ThumbnailIsDrawing 
    ? m_ParentUI->GetAppearanceSettings()->GetUIElement(
      SNAPAppearanceSettings::CROSSHAIRS_THUMB)
    : m_ParentUI->GetAppearanceSettings()->GetUIElement(
      SNAPAppearanceSettings::CROSSHAIRS);

  // Exit if the crosshars are not drawn
  if(!elt.Visible) return;

  // Get the current cursor position
  Vector3ui xCursorInteger = m_Driver->GetCursorPosition();

  // Shift the cursor position by by 0.5 in order to have it appear
  // between voxels
  Vector3f xCursorImage = to_float(xCursorInteger) + Vector3f(0.5f);
  
  // Get the cursor position on the slice
  Vector3f xCursorSlice = m_Parent->MapImageToSlice(xCursorImage);

  // Upper and lober bounds to which the crosshairs are drawn
  Vector2i lower(0);
  Vector2i upper = m_Parent->m_SliceSize.extract(2);

  // Set line properties
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

  // Apply the line properties; thick line is only applied in zoom thumbnail (?)
  SNAPAppearanceSettings::ApplyUIElementLineSettings(elt);

  // Apply the color
  glColor3dv(elt.NormalColor.data_block());
  
  // Refit matrix so that the lines are centered on the current pixel
  glPushMatrix();
  glTranslated( xCursorSlice(0), xCursorSlice(1), 0.0 );

  // Paint the cross-hairs
  glBegin(GL_LINES);  
  glVertex2f(0, 0); glVertex2f(lower(0) - xCursorSlice(0), 0);
  glVertex2f(0, 0); glVertex2f(upper(0) - xCursorSlice(0), 0);
  glVertex2f(0, 0); glVertex2f(0, lower(1) - xCursorSlice(1));
  glVertex2f(0, 0); glVertex2f(0, upper(1) - xCursorSlice(1));
  glEnd();

  glPopMatrix();
  glPopAttrib();
}

