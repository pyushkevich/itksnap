/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: InteractionModeClient.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:21 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "InteractionModeClient.h"
#include <algorithm>
#include <list>

void                            
InteractionModeClient
::PushInteractionMode(InteractionMode *mode)
{
  m_Interactors.push_front(mode);
}

InteractionMode *
InteractionModeClient
::PopInteractionMode() 
{
  InteractionMode *lastMode = m_Interactors.front();
  m_Interactors.pop_front();
  return lastMode;
}

void 
InteractionModeClient
::ClearInteractionStack() 
{
  m_Interactors.clear();
} 

void InteractionModeClient
::SetSingleInteractionMode(InteractionMode *mode)
{
  this->ClearInteractionStack();
  this->PushInteractionMode(mode);
}

unsigned int 
InteractionModeClient
::GetInteractionModeCount()
{
  return m_Interactors.size();
}

InteractionMode *
InteractionModeClient
::GetTopInteractionMode()
{
  return m_Interactors.front();
}

bool
InteractionModeClient
::IsInteractionModeAdded(InteractionMode *target)
{
  return 
    std::find(m_Interactors.begin(), m_Interactors.end(), target) !=
      m_Interactors.end();
}

void 
InteractionModeClient
::FireInteractionDrawEvent()
{
  // Propagate the drawing event through the stack
  typedef std::list<InteractionMode *>::iterator ModeIterator; 
  for (ModeIterator it = m_Interactors.begin(); it!=m_Interactors.end();it++)
    {
    InteractionMode *mode = *it;
    mode->OnDraw();
    }
}

