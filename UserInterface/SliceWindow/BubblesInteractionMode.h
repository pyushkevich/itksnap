/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: BubblesInteractionMode.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:27 $
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
#ifndef __BubblesInteractionMode_h_
#define __BubblesInteractionMode_h_

#include "GenericSliceWindow.h"

/**
 * \class BubblesInteractionMode
 * \brief UI interaction mode that takes care of bubble placement 
 * in segmentation.
 *
 * The interaction mode for display (and one day, manipulation) of 
 * intialization bubbles for the segmentation.  Presently, this class 
 * only has a draw method, but if we ever want to me able to click and
 * drag bubbles, this is the place to do so
 *
 * \see GenericSliceWindow
 */
class BubblesInteractionMode : public GenericSliceWindow::EventHandler {
public:
  BubblesInteractionMode(GenericSliceWindow *parent);
  void OnDraw();
};

#endif // __BubblesInteractionMode_h_
