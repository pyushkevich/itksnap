/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: RGBOverlayUILogic.cxx,v $
  Language:  C++
  Date:      $Date: 2009/06/16 04:55:45 $
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
#include "RGBOverlayUILogic.h"
#include <sstream>

RGBOverlayUILogic
::RGBOverlayUILogic()
{
  m_ImageWrapper = NULL;
}

void 
RGBOverlayUILogic
::DisplayWindow()
{
  // Show the window
  m_WinRGBOverlay->show();
}

void 
RGBOverlayUILogic
::OnClose()
{
  m_WinRGBOverlay->hide();  
}

void
RGBOverlayUILogic
::UpdateOverlayMenuSelection(
WrapperList *greyOverlays, WrapperList *RGBOverlays)
{
  // clear the menu
  m_InOverlaySelection->clear();
  m_ImageWrapper = NULL;

  // add grey overlays
  WrapperIterator it = greyOverlays->begin();
  
  int count = 1;
  while (it != greyOverlays->end())
    {
    string label = "Grey Overlay ";
    std::stringstream itoa;
    itoa << count;
    label += itoa.str();
    m_InOverlaySelection->add(label.c_str(), NULL, NULL, *it);
    ++count;
    ++it;
    }
  // add RGB overlays
  it = RGBOverlays->begin();
  count = 1;
  while (it != RGBOverlays->end())
    {
    string label = "RGB Overlay ";
    std::stringstream itoa;
    itoa << count;
    label += itoa.str();
    m_InOverlaySelection->add(label.c_str(), NULL, NULL, *it);
    ++count;
    ++it;
    }
  // redraw
  m_WinRGBOverlay->redraw();
}

void
RGBOverlayUILogic
::OnOverlaySelectionChange()
{
  m_ImageWrapper = static_cast<ImageWrapperBase *>(m_InOverlaySelection->mvalue()->user_data());
  m_InRGBOverlayOpacity->value(m_ImageWrapper->GetAlpha());
  // redraw
  m_WinRGBOverlay->redraw();
}

void
RGBOverlayUILogic
::OnRGBOverlayOpacityChange()
{
  if (m_ImageWrapper)
    {
     m_ImageWrapper->SetAlpha(m_InRGBOverlayOpacity->value());
    }
}

