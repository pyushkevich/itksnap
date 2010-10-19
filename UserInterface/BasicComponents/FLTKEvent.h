/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: FLTKEvent.h,v $
  Language:  C++
  Date:      $Date: 2010/10/19 20:28:56 $
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
#ifndef __FLTKEvent_h_
#define __FLTKEvent_h_

#include <FL/Fl.H>
#include <FL/Enumerations.H>

#include "SNAPCommonUI.h"

class FLTKCanvas;

extern void fl_gettime(long *sec, long *usec);

struct FLTKEventTimeStamp
{
  long sec, usec;
  FLTKEventTimeStamp()
    { fl_gettime(&sec, &usec); }
  long ElapsedUSecFrom(const FLTKEventTimeStamp &earlier)
    { return 1000000 * (sec - earlier.sec) + (usec - earlier.usec); }
};

/**
 * \class FLTKEvent
 * \brief A wrapper around FLTK event info.
 *
 * \sa FLTKCanvas
 */
struct FLTKEvent {
    /** The FLTK id of the event */
    int Id;

    /** The cursor position of the event in window coordinates */
    Vector2i XCanvas;

    /** The cursor position of the event in object space coordinates */
    Vector3f XSpace;

    /** Time when the event was generated (value of the clock() function) */
    FLTKEventTimeStamp TimeStamp;
    
    /** The button that generated this event */
    int Button;

    /**
     * The simulated event mouse button.  If user presses CTRL-LEFT, this will
     * have the value FL_MIDDLE_MOUSE and when the user presses ALT-LEFT, this
     * will have the value FL_RIGHT_MOUSE.  Useful for Macs and people without
     * a middle mouse button 
     */
    int SoftButton;

    /** The state of the interface (ALT,CTRL,SHIFT) */
    int State;

    /** The key associated with the event */
    int Key;

    /**
     * Pointer to the FLTKCanvas that generated this event.  This is a safety
     * measure because the recipient of these events should already have such
     * a pointer to the specific subclass of FLTKCanvas
     */
    FLTKCanvas *Source;
};

#endif // __FLTKEvent_h_

