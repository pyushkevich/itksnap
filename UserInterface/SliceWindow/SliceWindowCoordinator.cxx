/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SliceWindowCoordinator.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/12 17:57:11 $
  Version:   $Revision: 1.5 $
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
#include "SliceWindowCoordinator.h"

#include "GlobalState.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "OpenGLSliceTexture.h"
#include "UserInterfaceBase.h"

SliceWindowCoordinator
::SliceWindowCoordinator()
{
  m_Window[0] = m_Window[1] = m_Window[2] = NULL;
  m_Canvas[0] = m_Canvas[1] = m_Canvas[2] = NULL;
  m_LinkedZoom = false;
  m_WindowsRegistered = false;
}

SliceWindowCoordinator
::~SliceWindowCoordinator()
{
}

void
SliceWindowCoordinator
::RegisterWindows(GenericSliceWindow *windows[3])
{
  for(unsigned int i=0;i<3;i++)
    {
    m_Window[i] = windows[i];
    m_Canvas[i] = m_Window[i]->GetCanvas();
    m_Window[i]->SetManagedZoom(m_LinkedZoom);
    }
  m_WindowsRegistered = true;
}

void 
SliceWindowCoordinator
::SetLinkedZoom(bool flag)
{
  m_LinkedZoom = flag;

  if(m_WindowsRegistered)
    {
    // Tell the windows whether they are managed or not
    for(unsigned int i=0;i<3;i++)
      m_Window[i]->SetManagedZoom(m_LinkedZoom);
    
    // Set the common zoom
    if(m_LinkedZoom)
      SetCommonZoomToSmallestWindowZoom();
    }
}

void
SliceWindowCoordinator
::ResetViewToFitInAllWindows()
{
  // Only if initialized
  assert(m_WindowsRegistered);
  
  // Set each zoom to optimal value for that window
  for(unsigned int i=0;i<3;i++)
    {
    m_Window[i]->ResetViewToFit();
    }

  // If linked, zoom back to smallest zoom
  if(m_LinkedZoom)
    SetCommonZoomToSmallestWindowZoom();
}

void 
SliceWindowCoordinator
::SetZoomFactorAllWindows(float factor)
{
  // First, fit all windows
  ResetViewToFitInAllWindows();

  // Now scale the zoom in each window
  for(unsigned int i=0;i<3;i++)
    {
    m_Window[i]->SetViewZoom(m_Window[i]->GetViewZoom() * factor);
    }
}

void
SliceWindowCoordinator
::SetZoomLevelAllWindows(float level)
{
  // Now scale the zoom in each window
  for(unsigned int i=0;i<3;i++)
    {
    m_Window[i]->SetViewZoom(level);
    }
}



void
SliceWindowCoordinator
::ResetViewToFitInOneWindow(unsigned int window)
{
  // Only if initialized
  assert(m_WindowsRegistered);

  // Reset zoom to fit in the current window
  m_Window[window]->ResetViewToFit();

  if(m_LinkedZoom)
    SetZoomLevelAllWindows(m_Window[window]->GetViewZoom());
}

void
SliceWindowCoordinator
::OnZoomUpdateInWindow(unsigned int irisNotUsed(window), float zoom)
{
  // Only if initialized
  assert(m_WindowsRegistered);
  
  if(m_LinkedZoom)
    {
    for(unsigned int i=0;i<3;i++) 
      {
      m_Window[i]->SetViewZoom(zoom);
      }
    }
}

void
SliceWindowCoordinator
::OnWindowResize()
{
  if(m_LinkedZoom)
    SetCommonZoomToSmallestWindowZoom();
}

void
SliceWindowCoordinator
::SetCommonZoomToSmallestWindowZoom()
{
  // Compute the minimum zoom
  float minZoom = m_Window[0]->GetViewZoom();
  for(unsigned int i=1;i<3;i++)
    {
    if(minZoom > m_Window[i]->GetViewZoom())
      minZoom = m_Window[i]->GetViewZoom();
    }

  // Assign the minimum zoom
  for(unsigned int j=0;j<3;j++)
    {
    m_Window[j]->SetViewZoom(minZoom);
    }
}


float
SliceWindowCoordinator
::GetCommonZoomLevel()
{
  assert(m_LinkedZoom && m_WindowsRegistered);
  return m_Window[0]->GetViewZoom();
}

float
SliceWindowCoordinator
::GetCommonOptimalFitZoomLevel()
{
  assert(m_LinkedZoom && m_WindowsRegistered);
  return m_Window[0]->GetOptimalZoom();
}

float
SliceWindowCoordinator
::ClampZoom(unsigned int window,float zoom)
{
  assert(m_WindowsRegistered);

  float maxZoom = 0.0f;
  float minZoom = 0.0f;

  for(unsigned int i=0;i<3;i++)
    {
    if(m_LinkedZoom || window == i)
      {
      // Maximum zoom is constrained by the requirement that at least four
      // pixels are visible in at least one dimensions
      float zMax1 = 0.25 * m_Canvas[i]->w() / m_Window[i]->GetSliceSpacing()[0];
      float zMax2 = 0.25 * m_Canvas[i]->h() / m_Window[i]->GetSliceSpacing()[1];
      float zMax = zMax1 > zMax2 ? zMax1 : zMax2;
      maxZoom = (maxZoom == 0.0f || maxZoom < zMax) ? zMax : maxZoom;

      // Minimum zoom is just 0.25 of the optimal zoom
      float zMin = 0.25 * m_Window[i]->GetOptimalZoom();
      minZoom = (minZoom == 0.0f || minZoom > zMin) ? zMin : minZoom;
      }
    }

  // Apply the clamp
  if(zoom < minZoom)
    return minZoom;
  if(zoom > maxZoom)
    return maxZoom;
  return zoom;
}
