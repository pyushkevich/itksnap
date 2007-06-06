/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: RGBOverlayUIBase.h,v $
  Language:  C++
  Date:      $Date: 2007/06/06 22:27:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __RGBOverlayUIBase_h_
#define __RGBOverlayUIBase_h_

/**
 * \class RGBOverlayUIBase
 * \brief Base class for intensity curve FLTK interface.
 */
class RGBOverlayUIBase {
public:
    virtual ~RGBOverlayUIBase() {}
  // Callbacks made from the user interface
  virtual void OnClose() = 0;
  virtual void OnRGBOverlayOpacityChange() = 0;
};

#endif // __RGBOverlayUIBase_h_
