/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: CrosshairsInteractionMode.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:26 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __CrosshairsInteractionMode_h_
#define __CrosshairsInteractionMode_h_

#include "GenericSliceWindow.h"
#include "GlobalState.h"

/**
 * \class CrosshairsInteractionMode
 * \brief UI interaction mode that takes care of crosshair positioning.
 *
 * \see GenericSliceWindow
 */
class CrosshairsInteractionMode : public GenericSliceWindow::EventHandler {
public:
  CrosshairsInteractionMode(GenericSliceWindow *parent);

  void OnDraw(); 
  int OnMousePress(const FLTKEvent &event);
  int OnMouseWheel(const FLTKEvent &event);
  int OnMouseRelease(const FLTKEvent &event, 
                     const FLTKEvent &irisNotUsed(pressEvent));
  int OnMouseDrag(const FLTKEvent &event, 
                  const FLTKEvent &irisNotUsed(pressEvent));
  int OnKeyDown(const FLTKEvent &event);

private:
  void UpdateCrosshairs(const FLTKEvent &event);
  void UpdateCrosshairs(const Vector3f &xCross);

  // Whether or not we need to repaint the controls that depend on
  // the current slice position
  bool m_NeedToRepaintControls;

};

#endif // __CrosshairsInteractionMode_h_
