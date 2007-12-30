/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: RGBOverlayUILogic.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:17 $
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
#ifndef __RGBOverlayUILogic_h_
#define __RGBOverlayUILogic_h_

#include "RGBImageWrapper.h"
#include "RGBOverlayUI.h"

#include "itkSmartPointer.h"
#include "itkEventObject.h"
namespace itk{
  class Object;
  class Command;
};

/**
 * \class RGBOverlayUILogic
 * \brief Logic for the UI interaction for RGB Overlay Adjustment
 */
class RGBOverlayUILogic : public RGBOverlayUI {
public:
  // Event storage and propagation
  typedef itk::Object EventSystemType;
  typedef itk::SmartPointer<EventSystemType> EventSystemPointer;

  RGBOverlayUILogic();
  virtual ~RGBOverlayUILogic() {}

  /**
   * Display the dialog window (call after MakeWindow)
   */
  void DisplayWindow();

  /**
   * Return the event system associated with this object
   */
  irisGetMacro(EventSystem,EventSystemPointer);

  /**
   * RGB Overlay opacity update event object
   */
  itkEventMacro(OpacityUpdateEvent,itk::AnyEvent);

  // get the opacity value
  unsigned char GetOpacity () const;
  
  // get the opacity value
  void SetOpacity (unsigned char value);
  
  // Callbacks made from the user interface
  void OnClose();
  void OnRGBOverlayOpacityChange();
  
protected:
  
  // Event system for this class -> register events here
  EventSystemPointer m_EventSystem;

};

#endif // __RGBOverlayUILogic_h_
