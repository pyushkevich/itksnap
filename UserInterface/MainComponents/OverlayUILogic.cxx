/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: OverlayUILogic.cxx,v $
  Language:  C++
  Date:      $Date: 2009/06/16 05:57:00 $
  Version:   $Revision: 1.1 $
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
#include "OverlayUILogic.h"
#include <sstream>

OverlayUILogic
::OverlayUILogic()
{
  m_GreyOverlayWrapper = NULL;
  m_RGBOverlayWrapper = NULL;
}

void 
OverlayUILogic
::DisplayWindow()
{
  // Show the window
  m_WinOverlay->show();
}

void 
OverlayUILogic
::OnClose()
{
  m_WinOverlay->hide();  
}

void
OverlayUILogic
::UpdateOverlayMenuSelection(
WrapperList *greyOverlays, WrapperList *RGBOverlays)
{
  // clear the menu
  m_InGreyOverlaySelection->clear();
  m_GreyOverlayWrapper = NULL;
  m_InRGBOverlaySelection->clear();
  m_RGBOverlayWrapper = NULL;

  // add grey overlays
  WrapperIterator it = greyOverlays->begin();
  
  int count = 1;
  while (it != greyOverlays->end())
    {
    string label = "Grey Overlay ";
    std::stringstream itoa;
    itoa << count;
    label += itoa.str();
    m_InGreyOverlaySelection->add(label.c_str(), NULL, NULL, *it);
    ++count;
    ++it;
    }
  if (greyOverlays->size() > 0)
    m_InGreyOverlay->activate();
  else
    m_InGreyOverlay->deactivate();

  // add RGB overlays
  it = RGBOverlays->begin();
  count = 1;
  while (it != RGBOverlays->end())
    {
    string label = "RGB Overlay ";
    std::stringstream itoa;
    itoa << count;
    label += itoa.str();
    m_InRGBOverlaySelection->add(label.c_str(), NULL, NULL, *it);
    ++count;
    ++it;
    }
  if (RGBOverlays->size() > 0)
    m_InRGBOverlay->activate();
  else
    m_InRGBOverlay->deactivate();

  // redraw
  m_WinOverlay->redraw();
}


void
OverlayUILogic
::OnGreyOverlaySelectionChange()
{
  m_GreyOverlayWrapper = static_cast<ImageWrapperBase *>(m_InGreyOverlaySelection->mvalue()->user_data());
  m_InGreyOverlayOpacity->value(m_GreyOverlayWrapper->GetAlpha());
  // redraw
  m_WinOverlay->redraw();
}

void
OverlayUILogic
::OnGreyOverlayOpacityChange()
{
  if (m_GreyOverlayWrapper)
    {
     m_GreyOverlayWrapper->SetAlpha(m_InGreyOverlayOpacity->value());
    }
}

void
OverlayUILogic
::OnRGBOverlaySelectionChange()
{
  m_RGBOverlayWrapper = static_cast<ImageWrapperBase *>(m_InRGBOverlaySelection->mvalue()->user_data());
  m_InRGBOverlayOpacity->value(m_RGBOverlayWrapper->GetAlpha());
  // redraw
  m_WinOverlay->redraw();
}

void
OverlayUILogic
::OnRGBOverlayOpacityChange()
{
  if (m_RGBOverlayWrapper)
    {
     m_RGBOverlayWrapper->SetAlpha(m_InRGBOverlayOpacity->value());
    }
}

