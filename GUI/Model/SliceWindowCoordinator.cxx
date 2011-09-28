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
#include "IRISApplication.h"
#include "IRISImageData.h"

SliceWindowCoordinator
::SliceWindowCoordinator()
{
  // Set up the zoom model
  m_CommonZoomModel = CommonZoomFactorModel::New();
  m_CommonZoomModel->SetParentModel(this);

  // Initialize to defaults
  m_SliceModel[0] = m_SliceModel[1] = m_SliceModel[2] = NULL;
  m_LinkedZoom = false;
  m_WindowsRegistered = false;
}

SliceWindowCoordinator
::~SliceWindowCoordinator()
{
}

void
SliceWindowCoordinator
::RegisterSliceModels(GenericSliceModel *windows[])
{
  for(unsigned int i=0;i<3;i++)
    {
    m_SliceModel[i] = windows[i];
    m_SliceModel[i]->SetManagedZoom(m_LinkedZoom);
    }
  m_WindowsRegistered = true;
}

void 
SliceWindowCoordinator
::SetLinkedZoom(bool flag)
{
  if(m_LinkedZoom != flag)
    {
    m_LinkedZoom = flag;

    if(m_WindowsRegistered)
      {
      // Tell the windows whether they are managed or not
      for(unsigned int i=0;i<3;i++)
        m_SliceModel[i]->SetManagedZoom(m_LinkedZoom);

      // Set the common zoom
      if(m_LinkedZoom)
        SetCommonZoomToSmallestWindowZoom();
      }

    // Fire the appropriate event
    InvokeEvent(LinkedZoomUpdateEvent());
    }
}

double
SliceWindowCoordinator
::ComputeSmallestOptimalZoomLevel()
{
  assert(m_WindowsRegistered);

  // Figure out what is the optimal zoom in each window
  double minoptzoom = 0;
  for(int i = 0; i < 3; i++)
    {
    double optzoom = m_SliceModel[i]->GetOptimalZoom();
    if(i == 0 || minoptzoom > optzoom)
      minoptzoom = optzoom;
    }

  return minoptzoom;
}

void
SliceWindowCoordinator
::ResetViewToFitInAllWindows()
{
  // Only if initialized
  assert(m_WindowsRegistered);

  // Reset the view in all windows (center slices)
  for(unsigned int i=0;i<3;i++)
    {
    m_SliceModel[i]->ResetViewToFit();
    }

  // If linked zoom, use the smallest optimal zoom level
  if(m_LinkedZoom)
    {
    SetZoomLevelAllWindows(ComputeSmallestOptimalZoomLevel());
    }
}

void SliceWindowCoordinator
::SetZoomPercentageInAllWindows(float x)
{
  // x screen pixels = smallest voxel dimension
  // zf = x / (smallest voxel dimension)
  SetZoomLevelAllWindows(x / m_SliceModel[0]->GetSliceSpacing().min_value());
}

void 
SliceWindowCoordinator
::SetZoomFactorAllWindows(float factor)
{
  // Only if initialized
  assert(m_WindowsRegistered);

  // If linked zoom, use the smallest optimal zoom level
  if(m_LinkedZoom)
    {
    SetZoomLevelAllWindows(ComputeSmallestOptimalZoomLevel() * factor);
    }
  else
    {
    for(unsigned int i=0;i<3;i++)
      {
      m_SliceModel[i]->SetViewZoom(
            m_SliceModel[i]->GetOptimalZoom() * factor);
      }
    }
}

void
SliceWindowCoordinator
::SetZoomLevelAllWindows(float level)
{
  // Now scale the zoom in each window
  for(unsigned int i=0;i<3;i++)
    {
    m_SliceModel[i]->SetViewZoom(level);
    }

  // Invoke event
  if(m_LinkedZoom)
    {
    InvokeEvent(ZoomLevelUpdateEvent());
    }
}



void
SliceWindowCoordinator
::ResetViewToFitInOneWindow(unsigned int window)
{
  // Only if initialized
  assert(m_WindowsRegistered);

  // Reset zoom to fit in the current window
  if(m_LinkedZoom)
    {
    SetZoomLevelAllWindows(m_SliceModel[window]->GetOptimalZoom());
    }

  m_SliceModel[window]->ResetViewToFit();
}

void
SliceWindowCoordinator
::OnZoomUpdateInWindow(unsigned int irisNotUsed(window), float zoom)
{
  // Only if initialized
  assert(m_WindowsRegistered);
  
  if(m_LinkedZoom)
    {
    SetZoomLevelAllWindows(zoom);
    }
}

void
SliceWindowCoordinator
::OnWindowResize()
{
  if(m_LinkedZoom)
    {
    SetCommonZoomToSmallestWindowZoom();
    }
}

void
SliceWindowCoordinator
::SetCommonZoomToSmallestWindowZoom()
{
  // Compute the minimum zoom
  float minZoom = 0;
  for(unsigned int i=0; i<3; i++)
    {
    if(i == 0 || minZoom > m_SliceModel[i]->GetViewZoom())
      minZoom = m_SliceModel[i]->GetViewZoom();
    }

  // Assign the minimum zoom
  SetZoomLevelAllWindows(minZoom);
}


float
SliceWindowCoordinator
::GetCommonZoomLevel()
{
  if(m_LinkedZoom && m_WindowsRegistered)
    return m_SliceModel[0]->GetViewZoom();
  else return NAN;
}

float
SliceWindowCoordinator
::GetCommonOptimalFitZoomLevel()
{
  assert(m_LinkedZoom && m_WindowsRegistered);
  return m_SliceModel[0]->GetOptimalZoom();
}

void
SliceWindowCoordinator
::GetZoomRange(unsigned int window, float &minZoom, float &maxZoom)
{
  assert(m_WindowsRegistered);

  maxZoom = 0.0f;
  minZoom = 0.0f;

  for(unsigned int i=0;i<3;i++)
    {
    if(m_LinkedZoom || window == i)
      {
      double w = (double) m_SliceModel[i]->GetSize()[0];
      double h = (double) m_SliceModel[i]->GetSize()[1];

      // Maximum zoom is constrained by the requirement that at least four
      // pixels are visible in at least one dimensions
      float zMax1 = 0.25 * w / m_SliceModel[i]->GetSliceSpacing()[0];
      float zMax2 = 0.25 * h / m_SliceModel[i]->GetSliceSpacing()[1];
      float zMax = zMax1 > zMax2 ? zMax1 : zMax2;
      maxZoom = (maxZoom == 0.0f || maxZoom < zMax) ? zMax : maxZoom;

      // Minimum zoom is just 0.25 of the optimal zoom
      float zMin = 0.25 * m_SliceModel[i]->GetOptimalZoom();
      minZoom = (minZoom == 0.0f || minZoom > zMin) ? zMin : minZoom;
      }
    }
}


float
SliceWindowCoordinator
::ClampZoom(unsigned int window,float zoom)
{
  assert(m_WindowsRegistered);

  float minZoom, maxZoom;
  GetZoomRange(window, minZoom, maxZoom);

  // Apply the clamp
  if(zoom < minZoom)
    return minZoom;
  if(zoom > maxZoom)
    return maxZoom;
  return zoom;
}

double CommonZoomFactorModel::GetValue() const
{
  return (double) m_Parent->GetCommonZoomLevel();
}

void CommonZoomFactorModel::SetValue(double value)
{
  m_Parent->SetZoomLevelAllWindows((float) value);
}

/**
  Compute a round step size so that there are approximately
  n_steps steps between a and b
  */
double round_step_size(double a, double b, double nsteps)
{
  double k = pow(10, floor(log((b-a) / nsteps) / log(10.0)));
  double s = round((b-a) / (nsteps * k)) * k;
  return s;
}

NumericValueRange<double>
CommonZoomFactorModel::GetRange() const
{
  float fmin, fmax;
  m_Parent->GetZoomRange(0, fmin, fmax);

  // Compute a reasonable step value
  return NumericValueRange<double>((double) fmin, (double) fmax,
                                   round_step_size(fmax, fmin, 100.0));
}

void CommonZoomFactorModel
::SetParentModel(SliceWindowCoordinator *coord)
{
  m_Parent = coord;
  Rebroadcast(m_Parent, ZoomLevelUpdateEvent(), ValueChangedEvent());
}
