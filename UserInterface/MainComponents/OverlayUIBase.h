/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: OverlayUIBase.h,v $
  Language:  C++
  Date:      $Date: 2009/06/16 06:47:52 $
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
#ifndef __OverlayUIBase_h_
#define __OverlayUIBase_h_

/**
 * \class OverlayUIBase
 * \brief Base class for intensity curve FLTK interface.
 */
class OverlayUIBase {
public:
    virtual ~OverlayUIBase() {}
  // Callbacks made from the user interface
  virtual void OnClose() = 0;
  virtual void OnGreyOverlaySelectionChange() = 0;
  virtual void OnGreyOverlayOpacityChange() = 0;
  virtual void OnGreyOverlayColorMapChange() = 0;
  virtual void OnRGBOverlaySelectionChange() = 0;
  virtual void OnRGBOverlayOpacityChange() = 0;
};

#endif // __OverlayUIBase_h_
