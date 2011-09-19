/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Filename.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
  Copyright (c) 2011 Paul A. Yushkevich

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

=========================================================================*/

#ifndef INTERACTIONMODECLIENT_H
#define INTERACTIONMODECLIENT_H

#include "InteractionMode.h"
#include <list>

/**
 * \class InteractionModeClient
 * \brief An abstract class that can pass on Qt events to
 * interaction modes
 */
class InteractionModeClient
{
public:
  /**
   * Push an interaction mode onto the stack of modes.  Mode becomes first to
   * receive events.  The events that it does not receive are passed on to the
   * next mode on the stack.
   */
  void PushInteractionMode(InteractionMode *mode);

  /**
   * Pop the last interaction mode off the stack
   */
  InteractionMode *PopInteractionMode();

  /**
   * Get the top interaction mode on the stack
   */
  InteractionMode *GetTopInteractionMode();

  /**
   * See if the interaction mode is in the stack
   */
  bool IsInteractionModeAdded(InteractionMode *target);

  /**
   * Remove all interaction modes
   */
  void ClearInteractionStack();

  /**
   * Set the interaction stack to consist of just one interaction mode. This is
   * equivalent to calling ClearInteractionStack() followed by PushInteractionMode()
   */
  void SetSingleInteractionMode(InteractionMode *mode);

  /**
   * Get the number of interaction modes on the stack
   */
  unsigned int GetInteractionModeCount();

  // Virtual destructor
  virtual ~InteractionModeClient() {}

protected:

  // The stack of interaction modes
  std::list<InteractionMode *> m_Interactors;
};
#endif // INTERACTIONMODECLIENT_H
