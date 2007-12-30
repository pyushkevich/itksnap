/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: RecursiveInteractionMode.cxx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:16 $
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
#include "RecursiveInteractionMode.h"
#include <algorithm>

int 
RecursiveInteractionMode
::OnMousePress(const FLTKEvent &event) 
{
  for(ModeIterator m = m_Interactors.begin(); m != m_Interactors.end(); m++)
    if((*m)->OnMousePress(event)) return 1;
  return 0;
}

int RecursiveInteractionMode
::OnMouseRelease(const FLTKEvent &event, const FLTKEvent &pressEvent) 
{
  for(ModeIterator m = m_Interactors.begin(); m != m_Interactors.end(); m++)
    if((*m)->OnMouseRelease(event, pressEvent)) return 1;
  return 0;
}

int 
RecursiveInteractionMode
::OnMouseDrag(const FLTKEvent &event, const FLTKEvent &pressEvent) 
{
  for(ModeIterator m = m_Interactors.begin(); m != m_Interactors.end(); m++)
    if((*m)->OnMouseDrag(event, pressEvent)) return 1;
  return 0;
}

int 
RecursiveInteractionMode
::OnMouseEnter(const FLTKEvent &event)
{
  for(ModeIterator m = m_Interactors.begin(); m != m_Interactors.end(); m++)
    if((*m)->OnMouseEnter(event)) return 1;
  return 0;
}

int 
RecursiveInteractionMode
::OnMouseLeave(const FLTKEvent &event) 
{
  for(ModeIterator m = m_Interactors.begin(); m != m_Interactors.end(); m++)
    if((*m)->OnMouseLeave(event)) return 1;
  return 0;
}

int 
RecursiveInteractionMode
::OnMouseMotion(const FLTKEvent &event) 
{
  for(ModeIterator m = m_Interactors.begin(); m != m_Interactors.end(); m++)
    if((*m)->OnMouseMotion(event)) return 1;
  return 0;
}

int 
RecursiveInteractionMode
::OnMouseWheel(const FLTKEvent &event) 
{
  for(ModeIterator m = m_Interactors.begin(); m != m_Interactors.end(); m++)
    if((*m)->OnMouseWheel(event)) return 1;
  return 0;
}

int 
RecursiveInteractionMode
::OnKeyDown(const FLTKEvent &event) 
{
  for(ModeIterator m = m_Interactors.begin(); m != m_Interactors.end(); m++)
    if((*m)->OnKeyDown(event)) return 1;
  return 0;
}

int 
RecursiveInteractionMode
::OnKeyUp(const FLTKEvent &event) 
{
  for(ModeIterator m = m_Interactors.begin(); m != m_Interactors.end(); m++)
    if((*m)->OnKeyUp(event)) return 1;
  return 0;
}

int 
RecursiveInteractionMode
::OnShortcut(const FLTKEvent &event) 
{
  for(ModeIterator m = m_Interactors.begin(); m != m_Interactors.end(); m++)
    if((*m)->OnShortcut(event)) return 1;
  return 0;
}

int 
RecursiveInteractionMode
::OnOtherEvent(const FLTKEvent &event) 
{
  for(ModeIterator m = m_Interactors.begin(); m != m_Interactors.end(); m++)
    if((*m)->OnOtherEvent(event)) return 1;
  return 0;
}

void 
RecursiveInteractionMode
::OnDraw() 
{
  for(ModeIterator m = m_Interactors.begin(); m != m_Interactors.end(); m++)
    (*m)->OnDraw();
}


