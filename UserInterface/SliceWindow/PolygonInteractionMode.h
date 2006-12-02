/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: PolygonInteractionMode.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:27 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __PolygonInteractionMode_h_
#define __PolygonInteractionMode_h_

#include "GenericSliceWindow.h"
#include "PolygonDrawing.h"

/**
 * \class PolygonInteractionMode
 * \brief UI interaction mode that takes care of polygon drawing and editing.
 *
 * \see GenericSliceWindow
 */
class PolygonInteractionMode : public GenericSliceWindow::EventHandler 
{
public:
  PolygonInteractionMode(GenericSliceWindow *parent);
  virtual ~PolygonInteractionMode();

  int OnMousePress(const FLTKEvent &event)
  {
    return OnEitherEvent(event,event);
  }

  int OnKeyDown(const FLTKEvent &event)
  {
    return OnEitherEvent(event,event);
  }

  int OnMouseRelease(const FLTKEvent &event, const FLTKEvent &pressEvent)
  {
    return OnEitherEvent(event,pressEvent);
  }

  int OnMouseDrag(const FLTKEvent &event, const FLTKEvent &pressEvent)
  {
    return OnEitherEvent(event,pressEvent);
  }

  int OnShortcut(const FLTKEvent &event)
  {
    return OnEitherEvent(event,event);
  }

  void OnDraw();  
private:

  /**
   * The polygon handling object
   */
  PolygonDrawing *m_Drawing;

  /** Handler that gets envoked regardless of the event */
  int OnEitherEvent(const FLTKEvent &event, const FLTKEvent &pressEvent);

  /** Get a vector that represents the size of the pixel on the screen */
  Vector2f GetPixelSizeVector();

  // The window handler needs to access our privates
  friend class IRISSliceWindow;
};



#endif // __PolygonInteractionMode_h_
