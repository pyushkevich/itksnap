/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: ThumbnailInteractionMode.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:27 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __ThumbnailInteractionMode_h_
#define __ThumbnailInteractionMode_h_

#include "GenericSliceWindow.h"
#include "GlobalState.h"

/**
 * \class ThumbnailInteractionMode
 * \brief UI interaction mode that takes care of the zoom thumbnail shown in the
 * bottom left corner of the window
 *
 * \see GenericSliceWindow
 */
class ThumbnailInteractionMode : public GenericSliceWindow::EventHandler {
public:
  ThumbnailInteractionMode(GenericSliceWindow *parent);

  void OnDraw(); 
  int OnMousePress(const FLTKEvent &event);
  int OnMouseRelease(const FLTKEvent &event, 
                     const FLTKEvent &irisNotUsed(pressEvent));
  int OnMouseDrag(const FLTKEvent &event, 
                  const FLTKEvent &irisNotUsed(pressEvent));

private:
  bool m_PanFlag;
  Vector2f m_StartViewPosition;
};

#endif // __ThumbnailInteractionMode_h_
