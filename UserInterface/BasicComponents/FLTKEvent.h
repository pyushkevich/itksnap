/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: FLTKEvent.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:21 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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
    long int TimeStamp;
    
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

