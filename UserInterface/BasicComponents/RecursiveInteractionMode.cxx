/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: RecursiveInteractionMode.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:21 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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


