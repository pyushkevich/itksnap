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
#include "GlobalUIModel.h"
#include "DisplayLayoutModel.h"
#include <limits>

SliceWindowCoordinator
::SliceWindowCoordinator()
{
  // Set up the zoom model
  m_CommonZoomFactorModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetCommonZoomValueAndRange, &Self::SetCommonZoomValue,
        ZoomLevelUpdateEvent(), ZoomLevelUpdateEvent());

  m_LinkedZoomModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetLinkedZoomValue, &Self::SetLinkedZoomValue,
        ZoomLevelUpdateEvent(), ZoomLevelUpdateEvent());

  // Initialize to defaults
  m_SliceModel[0] = m_SliceModel[1] = m_SliceModel[2] = NULL;
  m_LinkedZoom = false;
  m_WindowsRegistered = false;
  m_ParentModel = NULL;
}

SliceWindowCoordinator
::~SliceWindowCoordinator()
{
}

void
SliceWindowCoordinator
::SetParentModel(GlobalUIModel *model)
{
  m_ParentModel = model;

  for(unsigned int i=0;i<3;i++)
    {
    m_SliceModel[i] = m_ParentModel->GetSliceModel(i);
    m_SliceModel[i]->SetManagedZoom(m_LinkedZoom);

    // Listen to updates to the viewport size. These require zoom factors to be
    // recomputed
    Rebroadcast(m_SliceModel[i], GenericSliceModel::ViewportResizeEvent(), ModelUpdateEvent());
    }

  m_WindowsRegistered = true;

  // Listen to image dimension change events
  Rebroadcast(m_ParentModel->GetDriver(), MainImageDimensionsChangeEvent(), ModelUpdateEvent());

  // Listen to changes in the layout of the slice view into cells. When
  // this change occurs, we have to modify the size of the slice views
  DisplayLayoutModel *dlm = m_ParentModel->GetDisplayLayoutModel();
  Rebroadcast(dlm, DisplayLayoutModel::LayerLayoutChangeEvent(), ModelUpdateEvent());

}

void SliceWindowCoordinator::OnUpdate()
{
  // Has a new main image been loaded
  if(this->m_EventBucket->HasEvent(MainImageDimensionsChangeEvent()))
    {
    // Update each of the slice models, allowing them to respond to the main image
    // dimensions change
    for(unsigned int i = 0; i < 3; i++)
      m_SliceModel[i]->Update();

    // Reset the view to fit (depending on linked zoom)
    if(m_ParentModel->GetDriver()->IsMainImageLoaded())
      this->ResetViewToFitInAllWindows();
    }

  if(this->m_EventBucket->HasEvent(GenericSliceModel::ViewportResizeEvent())
     || this->m_EventBucket->HasEvent(DisplayLayoutModel::LayerLayoutChangeEvent()))
    {
    // If we are maintaining linked zoom, then this class is going to manage the
    // recomputation of optimal zoom in each window and resetting of the zoom.
    if(m_LinkedZoom && AreSliceModelsInitialized())
      {
      // Is the current zoom same as the optimal zoom? If so, we will force a
      // reset after the optimal zooms have been computed
      double common_opt_zoom = ComputeSmallestOptimalZoomLevel();
      double common_zoom = GetCommonZoomLevel();
      bool rezoom = (common_zoom == common_opt_zoom);

      // Recompute the optimal zoom in each of the views
      for(unsigned int i = 0; i < 3; i++)
        m_SliceModel[i]->ComputeOptimalZoom();

      // Optionally, reset the view
      if(rezoom)
        this->ResetViewToFitInAllWindows();
      }

    // Update each of the slice models. This will cause them to recompute their
    // optimal zoom.
    }

}

double
SliceWindowCoordinator
::ComputeSmallestOptimalZoomLevel()
{
  assert(m_WindowsRegistered);

  // How this is computed depends on whether all four views are visible or not
  DisplayLayoutModel *dlm = m_ParentModel->GetDisplayLayoutModel();

  // Figure out what is the optimal zoom in each window
  bool foundVisible = false;
  double minoptzoom = 0;
  for(int i = 0; i < 3; i++)
    {
    if(dlm->GetViewPanelVisibilityModel(i)->GetValue())
      {
      double optzoom = m_SliceModel[i]->GetOptimalZoom();
      if(!foundVisible || minoptzoom > optzoom)
        {
        minoptzoom = optzoom;
        foundVisible = true;
        }
      }
    }

  // If nothing is visible, use the optimal zoom from the first window
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
    double optzoom = ComputeSmallestOptimalZoomLevel();
    if(optzoom > 0.0)
      SetZoomLevelAllWindows(optzoom);
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
::ZoomInOrOutInOneWindow(unsigned int window, float factor)
{
  // Only if initialized
  assert(m_WindowsRegistered);

  // Apply in the current window
  m_SliceModel[window]->ZoomInOrOut(factor);

  // Reset zoom to fit in the current window
  if(m_LinkedZoom)
    {
    SetZoomLevelAllWindows(m_SliceModel[window]->GetViewZoom());
    }
}

void SliceWindowCoordinator::CenterViewOnCursorInAllWindows()
{
  for(int i = 0; i < 3; i++)
    m_SliceModel[i]->CenterViewOnCursor();
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
  else return std::numeric_limits<float>::quiet_NaN();
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


/**
  Compute a round step size so that there are approximately
  n_steps steps between a and b
  */
double round_step_size(double a, double b, double nsteps)
{
  double delta = fabs(b-a);
  double k = pow(10, floor(log((delta) / nsteps) / log(10.0)));
  double s = floor(0.5 + delta / (nsteps * k)) * k;
  return s;
}

bool SliceWindowCoordinator::GetCommonZoomValueAndRange(
    double &zoom, NumericValueRange<double> *range)
{
  // Linked zoom required
  if(!GetLinkedZoom() || !m_ParentModel->GetDriver()->IsMainImageLoaded())
    return false;

  // Get the zoom
  zoom = (double) GetCommonZoomLevel();

  // Get the range
  if(range)
    {
    float fmin, fmax;
    GetZoomRange(0, fmin, fmax);

    range->Minimum = fmin;
    range->Maximum = fmax;

    // Compute a reasonable step value. This is tricky, because zoom is not
    // really a linear variable, at high zoom levels, you want steps to be
    // larger than at small zoom levels. So how about a step that's just on
    // the order of one hundredth of the current level?
    range->StepSize = CalculatePowerOfTenStepSize(0, zoom, 10);
    }

  return true;
}

void SliceWindowCoordinator::SetCommonZoomValue(double zoom)
{
  this->SetZoomLevelAllWindows((float) zoom);
}

bool SliceWindowCoordinator::GetLinkedZoomValue(bool &out_value)
{
  out_value = m_LinkedZoom;
  return true;
}

void SliceWindowCoordinator::SetLinkedZoomValue(bool value)
{
  if(m_LinkedZoom != value)
    {
    m_LinkedZoom = value;

    if(m_WindowsRegistered)
      {
      // Tell the windows whether they are managed or not
      for(unsigned int i=0;i<3;i++)
        m_SliceModel[i]->SetManagedZoom(m_LinkedZoom);

      // Set the common zoom if an image is loaded
      if(m_LinkedZoom && m_ParentModel->GetDriver()->IsMainImageLoaded())
        SetCommonZoomToSmallestWindowZoom();
      }

    // Fire the appropriate event
    InvokeEvent(LinkedZoomUpdateEvent());
    }
}

bool SliceWindowCoordinator::AreSliceModelsInitialized()
{
  if(!m_WindowsRegistered)
    return false;

  for(unsigned int i=0;i<3;i++)
    if(!m_SliceModel[i]->IsSliceInitialized())
      return false;

  return true;
}


