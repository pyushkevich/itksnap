/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ThumbnailInteractionMode.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:29 $
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
