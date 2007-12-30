/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: CrosshairsInteractionMode.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:28 $
  Version:   $Revision: 1.2 $
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
