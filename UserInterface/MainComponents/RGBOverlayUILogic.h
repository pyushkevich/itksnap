/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: RGBOverlayUILogic.h,v $
  Language:  C++
  Date:      $Date: 2007/06/06 22:27:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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
