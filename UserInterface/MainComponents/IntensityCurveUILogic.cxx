/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: IntensityCurveUILogic.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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


