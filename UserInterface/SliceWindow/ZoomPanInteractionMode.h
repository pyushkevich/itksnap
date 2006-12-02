/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: ZoomPanInteractionMode.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:27 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __ZoomPanInteractionMode_h_
#define __ZoomPanInteractionMode_h_

#include "GenericSliceWindow.h"

/**
 * \class ZoomPanInteractionMode
 * \brief UI interaction mode that takes care of zooming and panning.
 *
 * \see GenericSliceWindow
 */
class ZoomPanInteractionMode : public GenericSliceWindow::EventHandler {
public:
  ZoomPanInteractionMode(GenericSliceWindow *parent);

  int OnMousePress(const FLTKEvent &event);
  int OnMouseRelease(const FLTKEvent &event, const FLTKEvent &pressEvent);    
  int OnMouseDrag(const FLTKEvent &event, const FLTKEvent &pressEvent);
  void OnDraw();
  
  // void OnDraw();
protected:
  /** The starting point for panning */
  Vector2f m_StartViewPosition;

  /** The starting zoom level */
  float m_StartViewZoom;

  /** Used to schedule UI repaint updates */
  bool m_NeedUIUpdateOnRepaint;
};

#endif // __ZoomPanInteractionMode_h_
