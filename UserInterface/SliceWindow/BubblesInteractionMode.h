/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: BubblesInteractionMode.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:26 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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
