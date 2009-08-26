/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: OverlayUILogic.h,v $
  Language:  C++
  Date:      $Date: 2009/08/26 01:10:20 $
  Version:   $Revision: 1.3 $
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
#ifndef __OverlayUILogic_h_
#define __OverlayUILogic_h_

#include "OverlayUI.h"
#include "GenericImageData.h"

/**
 * \class OverlayUILogic
 * \brief Logic for the UI interaction for Overlay Adjustment
 */
class OverlayUILogic : public OverlayUI {
public:

  OverlayUILogic();
  virtual ~OverlayUILogic() {}

  /**
   * Display the dialog window (call after MakeWindow)
   */
  void DisplayWindow();

  // Callbacks made from the user interface
  void OnClose();

  typedef GenericImageData::WrapperList WrapperList;
  typedef GenericImageData::WrapperIterator WrapperIterator;
  void UpdateOverlayMenuSelection(WrapperList *overlays);

  void OnGreyOverlaySelectionChange();
  void OnGreyOverlayOpacityChange();
  void OnGreyOverlayColorMapChange();
  void OnRGBOverlaySelectionChange();
  void OnRGBOverlayOpacityChange();

protected:

  // Currently chosen image wrapper
  ImageWrapperBase *m_GreyOverlayWrapper;
  ImageWrapperBase *m_RGBOverlayWrapper;

};

#endif // __OverlayUILogic_h_
