/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IntensityCurveUILogic.cxx,v $
  Language:  C++
  Date:      $Date: 2008/02/11 18:21:14 $
  Version:   $Revision: 1.4 $
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
#include "IntensityCurveUILogic.h"

IntensityCurveUILogic
::IntensityCurveUILogic()
{
  m_EventSystem = EventSystemType::New();
  m_Curve = NULL;
  m_ImageWrapper = NULL;
}

void 
IntensityCurveUILogic
::SetCurve(IntensityCurveInterface *curve) 
{
  m_Curve = curve;
  m_BoxCurve->SetCurve(curve);
}

void 
IntensityCurveUILogic
::SetImageWrapper(GreyImageWrapper *wrapper)
{
  // Give the image to the histogram
  m_ImageWrapper = wrapper;
  m_BoxCurve->ComputeHistogram(wrapper, 4);

  // Pull out the current histogram settings
  m_InHistogramMaxLevel->value(m_BoxCurve->GetHistogramMaxLevel() * 100.0f);
  m_InHistogramBinSize->value(m_BoxCurve->GetHistogramBinSize());
  m_ChkHistogramLog->value(m_BoxCurve->IsHistogramLog());
}

void 
IntensityCurveUILogic
::UpdateWindowAndLevel()
{
  // Need a curve and a wrapper
  assert(m_Curve && m_ImageWrapper);

  // This is the range of the curve in unit coordinates (0 to 1)
  float t0,x0,t1,x1;

  // Get the starting and ending control points
  m_Curve->GetControlPoint(0,t0,x0);
  m_Curve->GetControlPoint(m_Curve->GetControlPointCount()-1,t1,x1);

  // Get 'absolute' image intensity range, i.e., the largest and smallest
  // intensity in the whole image
  GreyType iAbsMin = m_ImageWrapper->GetImageMin();
  GreyType iAbsMax = m_ImageWrapper->GetImageMax();
  GreyType iAbsSpan = (iAbsMax - iAbsMin);

  // The the curve intensity range
  GreyType iMin = iAbsMin + static_cast<GreyType>(iAbsSpan * t0); 
  GreyType iMax = iAbsMin + static_cast<GreyType>(iAbsSpan * t1);

  // Compute the level and window in intensity units
  GreyType level = iMin;
  GreyType window = iMax - iMin;

  // Compute and constrain the level 
  m_InLevel->value(level);
  m_InLevel->minimum(iAbsMin);
  m_InLevel->maximum(iAbsMax - window);

  // Compute and constrain the window
  m_InWindow->value(window);
  m_InWindow->minimum(1);
  m_InWindow->maximum(iAbsMax - level);
}

void 
IntensityCurveUILogic
::OnWindowLevelChange()
{
  // Assure that input and output outside of the image range
  // is handled gracefully
  m_InLevel->value(m_InLevel->clamp(m_InLevel->value()));
  m_InWindow->value(m_InWindow->clamp(m_InWindow->value()));

  // Get 'absolute' image intensity range, i.e., the largest and smallest
  // intensity in the whole image
  GreyType iAbsMin = m_ImageWrapper->GetImageMin();
  GreyType iAbsMax = m_ImageWrapper->GetImageMax();

  // Get the new values of min and max
  int iMin = (int) m_InLevel->value();
  int iMax = iMin + (int)m_InWindow->value();

  // Min better be less than max
  assert(iMin < iMax);

  // Compute the unit coordinate values that correspond to min and max
  float factor = 1.0f / (iAbsMax - iAbsMin);
  float t0 = factor * (iMin - iAbsMin);
  float t1 = factor * (iMax - iAbsMin);

  // Update the curve boundary
  m_Curve->ScaleControlPointsToWindow(t0,t1);

  // Alert the box to redisplay curve
  m_BoxCurve->redraw();

  // Fire the reset event
  OnCurveChange();
}

void 
IntensityCurveUILogic
::DisplayWindow()
{
  // Should have both a curve and an image before displaying the interface
  assert(m_Curve);
  assert(m_ImageWrapper);

  // Update the window and level text boxes
  UpdateWindowAndLevel();

  // Connect to child object(s)
  m_BoxCurve->SetParent(this);  

  // Show the window
  m_WinCurve->show();
  m_BoxCurve->show();
}

void 
IntensityCurveUILogic
::OnCurveChange()
{
  // Update the values of the window and level
  UpdateWindowAndLevel();

  // Fire the event
  GetEventSystem()->InvokeEvent(
    IntensityCurveUILogic::CurveUpdateEvent());

  // Redraw the window
  m_BoxCurve->redraw();
}

void 
IntensityCurveUILogic
::OnClose()
{
  // Hide the curve window
  m_WinCurve->hide();  
}

void 
IntensityCurveUILogic
::OnReset()
{
  // Reinitialize the curve
  m_Curve->Initialize((int)m_InControlPoints->value());

  // Alert the box to redisplay curve
  m_BoxCurve->redraw();

  // Fire the reset event
  OnCurveChange();
}

void
IntensityCurveUILogic
::OnAutoFitWindow()
  {
  // Get the histogram
  const std::vector<unsigned int> &hist = m_BoxCurve->GetHistogram();

  // Integrate the histogram until reaching 1%
  GreyType imin = m_ImageWrapper->GetImageMin();
  GreyType ilow = imin;
  size_t accum = 0;
  size_t accum_goal = m_ImageWrapper->GetNumberOfVoxels() / 100;
  for(size_t i = 0; i < hist.size(); i++)
    {
    if(accum + hist[i] < accum_goal)
      {
      accum += hist[i];
      ilow += m_BoxCurve->GetHistogramBinSize();
      }
    else break;
    }

  // Same, but from above
  GreyType imax = m_ImageWrapper->GetImageMax();
  GreyType ihigh = imax;
  accum = 0;
  for(size_t i = hist.size() - 1; i >= 0; i--)
    {
    if(accum + hist[i] < accum_goal)
      {
      accum += hist[i];
      ihigh -= m_BoxCurve->GetHistogramBinSize();
      }
    else break;
    }

  // If for some reason the window is off, we set everything to max/min
  if(ilow >= ihigh)
    { ilow = imin; ihigh = imax; }

  // Compute and constrain the window
  GreyType iwin = ihigh - ilow;

  m_InWindow->maximum(imax - ilow);
  m_InWindow->value(iwin);

  m_InLevel->maximum(imax - iwin);
  m_InLevel->value(ilow);  

  this->OnWindowLevelChange();
  }

void 
IntensityCurveUILogic
::OnControlPointNumberChange()
{
  // Reinitialize the curve, using the new number of control points
  m_Curve->Initialize((int)m_InControlPoints->value());

  // Stretch the curve to the currently entered level and window
  OnWindowLevelChange();
}

void 
IntensityCurveUILogic
::OnUpdateHistogram()
{
  // Recompute the histogram and redisplay
  m_BoxCurve->SetHistogramBinSize((size_t) m_InHistogramBinSize->value());
  m_BoxCurve->SetHistogramMaxLevel(m_InHistogramMaxLevel->value() / 100.0f);
  m_BoxCurve->SetHistogramLog(m_ChkHistogramLog->value() ? true : false);
  m_BoxCurve->ComputeHistogram(m_ImageWrapper, 1);
  m_BoxCurve->redraw();

  // The histogram controls may have changed. Update them
  m_InHistogramMaxLevel->value(m_BoxCurve->GetHistogramMaxLevel() * 100.0);
  m_InHistogramBinSize->value(m_BoxCurve->GetHistogramBinSize());
}


