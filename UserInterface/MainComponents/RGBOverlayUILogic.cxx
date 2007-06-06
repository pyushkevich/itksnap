/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: RGBOverlayUILogic.cxx,v $
  Language:  C++
  Date:      $Date: 2007/06/06 22:27:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "RGBOverlayUILogic.h"

RGBOverlayUILogic
::RGBOverlayUILogic()
{
  m_EventSystem = EventSystemType::New();
}

void 
RGBOverlayUILogic
::DisplayWindow()
{
  // Show the window
  m_WinRGBOverlay->show();
}

unsigned char
RGBOverlayUILogic
::GetOpacity() const
{
  return (unsigned char) m_InRGBOverlayOpacity->value();
}

void
RGBOverlayUILogic
::SetOpacity(unsigned char opacity)
{
  m_InRGBOverlayOpacity->Fl_Valuator::value(opacity);
}

void 
RGBOverlayUILogic
::OnClose()
{
  m_WinRGBOverlay->hide();  
}

void 
RGBOverlayUILogic
::OnRGBOverlayOpacityChange()
{
  // Fire the event
  GetEventSystem()->InvokeEvent(
    RGBOverlayUILogic::OpacityUpdateEvent());
}

