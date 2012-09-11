/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: RegionInteractionMode.cxx,v $
  Language:  C++
  Date:      $Date: 2008/02/10 23:55:22 $
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
#include "RegionInteractionMode.h"

#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "IRISSliceWindow.h"
#include "UserInterfaceBase.h"
#include "IRISVectorTypesToITKConversion.h"
#include "SNAPAppearanceSettings.h"

#include <assert.h>
#include <cmath>

// The click detection radius (delta)
const unsigned int RegionInteractionMode::m_PixelDelta = 4;

RegionInteractionMode
::RegionInteractionMode(GenericSliceWindow *parent)
:GenericSliceWindow::EventHandler(parent)
{
  // Initialize the edges
  for(unsigned int dir=0;dir<2;dir++) 
  {
    for(unsigned int i=0;i<2;i++) 
    {
      m_EdgeHighlighted[dir][i] = false;
    }
  }
}

void 
RegionInteractionMode
::GetEdgeVertices(unsigned int direction,unsigned int index,
                  Vector2f &x0,Vector2f &x1, 
                  const Vector3f corner[2])
{
  x0(direction) = corner[0](direction);
  x1(direction) = corner[1](direction);
  x0(1-direction) = x1(1-direction) = corner[index](1-direction);
}

float
RegionInteractionMode
::GetEdgeDistance(unsigned int direction,
                  unsigned int index,
                  const Vector2f &x,
                  const Vector3f corner[2])
{
  // Compute the vertices of the edge
  Vector2f x0,x1;
  GetEdgeVertices(direction,index,x0,x1,corner);
  
  // Compute the squared distance between the vertices
  float l2 = (x1-x0).squared_magnitude();
  float l = sqrt(l2);
  
  // Compute the projection of x onto x1-x0
  float p = dot_product(x-x0,x1-x0) / sqrt(l2);
  float p2 = p*p;
  
  // Compute the squared distance to the line of the edge
  float q2 = (x-x0).squared_magnitude() - p2;

  // Compute the total distance
  float d = sqrt(q2 + (p < 0 ? p2 : 0) + (p > l ? (p-l)*(p-l) : 0));

  // Return this distance
  return d;
}

int RegionInteractionMode
::OnMousePress(const FLTKEvent &event)
{
  // Flag indicating whether we respond to this event or not
  m_IsAnyEdgeHighlighted = false;
  
  // Convert the event location into slice u,v coordinates
  Vector3f xSlice = m_Parent->MapWindowToSlice(event.XSpace.extract(2));    
  Vector2f uvSlice(xSlice(0),xSlice(1));
  
  // Record the system's corners at the time of drag start
  GetSystemROICorners(m_CornerDragStart);
    
  // Repeat for vertical and horizontal edges
  for(unsigned int dir=0;dir<2;dir++) 
  {
    // Variables used to find the closest edge that's within delta
    int iClosest = -1;
    float dToClosest = m_PixelDelta;

    // Search for the closest edge
    for(unsigned int i=0;i<2;i++) 
    {
      float d = GetEdgeDistance(dir,i,uvSlice,m_CornerDragStart);
      if(d < dToClosest) 
      {
        dToClosest = d;
        iClosest = i;
      }
    }

    // Highlight the selected edge
    if(iClosest >= 0) 
      {
      m_EdgeHighlighted[dir][iClosest] = true;
      m_IsAnyEdgeHighlighted = true;
      }      
  }

  // If nothing was highlighted, then return and let the next handler process
  // the event
  if(!m_IsAnyEdgeHighlighted)
    return 0;

  // Event has been handled
  return 1;
}

void RegionInteractionMode
::GetSystemROICorners(Vector3f corner[2])
{
  // Get the region of interest in image coordinates  
  GlobalState::RegionType roi = m_GlobalState->GetSegmentationROI();

  // Get the lower-valued corner
  Vector3l ul(roi.GetIndex().GetIndex());
  
  // Get the higher valued corner
  Vector3ul sz(roi.GetSize().GetSize());

  // Remap to slice coordinates
  corner[0] = m_Parent->MapImageToSlice(to_float(ul));
  corner[1] = m_Parent->MapImageToSlice(to_float(ul+to_long(sz)));
}

void RegionInteractionMode
::UpdateCorners(const FLTKEvent &event, const FLTKEvent &pressEvent)
{
  // Compute the corners in slice coordinates
  Vector3f corner[2];
  GetSystemROICorners(corner);

  // Convert the location of the events into slice u,v coordinates
  Vector3f uvSliceNow = 
    m_Parent->MapWindowToSlice(event.XSpace.extract(2));
  Vector3f uvSlicePress = 
    m_Parent->MapWindowToSlice(pressEvent.XSpace.extract(2));

  // Get the current bounds and extents of the region of interest 
  Vector3f xCornerImage[2] = 
  {
    m_Parent->MapSliceToImage(corner[0]),
    m_Parent->MapSliceToImage(corner[1])
  };

  // TODO: For dragging entire region, the clamps should be just image extents
  // Compute the clamps for each of the corners
  Vector3f clamp[2][2] = 
  {
    {
      Vector3f(0.0f,0.0f,0.0f),
      xCornerImage[1] - Vector3f(1.0f,1.0f,1.0f)
    },
    {
      xCornerImage[0] + Vector3f(1.0f,1.0f,1.0f),
      to_float( m_Driver->GetCurrentImageData()->GetVolumeExtents())
    }
  };

  // For each highlighted edge, update the coordinates of the affected vertex
  // by clamping to the maximum range
  for (unsigned int dir=0;dir<2;dir++)
    {
    for (unsigned int i=0;i<2;i++)
      {
      if (m_EdgeHighlighted[dir][i])
        {
        // Horizontal edge affects the y of the vertex and vice versa
        corner[i](1-dir) =
          m_CornerDragStart[i](1-dir) + uvSliceNow(1-dir) - uvSlicePress(1-dir);

        // Map the affected vertex to image space
        Vector3f vImage = m_Parent->MapSliceToImage(corner[i]);

        // Clamp the affected vertex in image space
        Vector3f vImageClamped = vImage.clamp(clamp[i][0],clamp[i][1]);

        // Map the affected vertex back into slice space
        corner[i] = m_Parent->MapImageToSlice(vImageClamped);
        }
      }
    }

  // Update the region of interest in the system
  Vector3i xImageLower = to_int(m_Parent->MapSliceToImage(corner[0]));
  Vector3i xImageUpper = to_int(m_Parent->MapSliceToImage(corner[1]));

  // Create a region based on the corners
  GlobalState::RegionType roiCorner(
    to_itkIndex(xImageLower),to_itkSize(xImageUpper-xImageLower));

  // Get the system's region of interest
  GlobalState::RegionType roiSystem = m_GlobalState->GetSegmentationROI();

  // The slice z-direction index and size in the ROI should retain the system's
  // previous value because we are only manipulating the slice in 2D
  unsigned int idx = m_Parent->m_ImageAxes[2];
  roiCorner.SetIndex(idx,roiSystem.GetIndex(idx));
  roiCorner.SetSize(idx,roiSystem.GetSize(idx));

  // Update the system's ROI
  m_GlobalState->SetSegmentationROI(roiCorner);

  // Cause a system redraw
  m_ParentUI->RedrawWindows();
}

int RegionInteractionMode
::OnMouseDrag(const FLTKEvent &event, const FLTKEvent &pressEvent)
{
  // Only do something if there is a highlight
  if(m_IsAnyEdgeHighlighted)
    {
    // Update the corners in response to the dragging
    UpdateCorners(event,pressEvent);
    
    // Event has been handled
    return 1;
    }
  
  return 0;
}

int RegionInteractionMode
::OnMouseRelease(const FLTKEvent &event, const FLTKEvent &pressEvent)
{
  // Only do something if there is a highlight
  if(m_IsAnyEdgeHighlighted)
    {
    // Update the corners in response to the dragging
    UpdateCorners(event,pressEvent);
    
    // Clear highlights of the connected edges
    for(unsigned int i=0;i<2;i++)
      m_EdgeHighlighted[0][i] = m_EdgeHighlighted[1][i] = false;

    // Clear the summary highlight flag
    m_IsAnyEdgeHighlighted = false;
    
    // Event has been handled
    return 1;
    }
  
  // Event has not been handled
  return 0;
}

void
RegionInteractionMode
::OnDraw()
{
  // The region of interest should be in effect
  assert(m_GlobalState->GetIsValidROI());

  // Compute the corners in slice coordinates
  Vector3f corner[2];
  GetSystemROICorners(corner);

  // Check that the current slice is actually within the bounding box
  // int slice = m_Parent->m_SliceIndex;
  int dim = m_Parent->m_ImageAxes[2];
  int slice = m_Driver->GetCursorPosition()[dim];
  int bbMin = m_GlobalState->GetSegmentationROI().GetIndex(dim);
  int bbMax = bbMin + m_GlobalState->GetSegmentationROI().GetSize(dim);

  // And if so, return without painting anything
  if(bbMin > slice || bbMax <= slice)
    return;

  // Get the line color, thickness and dash spacing
  const SNAPAppearanceSettings::Element &elt = 
    m_ParentUI->GetAppearanceSettings()->GetUIElement(
    SNAPAppearanceSettings::ROI_BOX);

  // Set line properties
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

  // Apply the line properties
  SNAPAppearanceSettings::ApplyUIElementLineSettings(elt);

  // Start drawing the lines
  glBegin(GL_LINES);
  
  // Draw each of the edges
  for(unsigned int dir=0;dir<2;dir++)
  {
    for(unsigned int i=0;i<2;i++)
    {
    // Select color according to edge state
    glColor3dv( m_EdgeHighlighted[dir][i] ? 
      elt.ActiveColor.data_block() : elt.NormalColor.data_block() );

    // Compute the vertices of the edge
    Vector2f x0,x1;
    GetEdgeVertices(dir,i,x0,x1,corner);

    // Draw the line
    glVertex2f(x0[0],x0[1]);
    glVertex2f(x1[0],x1[1]);
    }
  }

  glEnd();
  glPopAttrib();
}


