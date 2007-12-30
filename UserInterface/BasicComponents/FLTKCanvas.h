/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: FLTKCanvas.h,v $
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
#ifndef __FLTKCanvas_h_
#define __FLTKCanvas_h_

#include <FL/Fl_Gl_Window.H>
#include "FLTKEvent.h"
#include "InteractionMode.h"
#include "InteractionModeClient.h"

#include <list>

/**
 * \class FLTKCanvas
 * \brief An extension of Fl_Gl_Window with advanced interaction handling.
 * 
 * This is an extension of the Fl_Gl_Window from FLTK that allows
 * the concept of interaction modes to be used.  InteractionModes 
 * are event handlers that are associated with different ways of user
 * interaction with the window.  This is used with toolbars, where different
 * toolbar buttons change the behavior of the interaction.  
 * 
 * Multiple interaction modes can be used simultaneously in a stack.  The top mode
 * in the stack is the first to receive events, and the event is propagated through
 * the stack until it has been handled properly.
 */
class FLTKCanvas : public Fl_Gl_Window, public InteractionModeClient 
{
public:

  /** Constructor sets up some basics, sets interaction mode stack to be empty */
  FLTKCanvas(int x, int y, int w, int h, const char *label);
  virtual ~FLTKCanvas() {}

  /** Handle events */
  virtual int handle(int eventID);

  /** Default drawing handler */
  virtual void draw();

  /** Are we dragging ? */
  irisIsMacro(Dragging);

  /** Are we flipping? */
  irisSetMacro(FlipYCoordinate,bool);

  /** Check if keyboard focus is grabbed when the mouse enters the window */
  irisGetMacro(GrabFocusOnEntry,bool);

  /** Set whether keyboard focus is grabbed when the mouse enters the window */
  irisSetMacro(GrabFocusOnEntry,bool);

  /** Check if the window has keyboard focus */
  irisGetMacro(Focus,bool);

  /** Save the window content to a PNG file */
  void SaveAsPNG(const char *filename);

private:

  // The event at the start of a drag operation (if there is one going on)
  FLTKEvent m_DragStartEvent;

  // Are we dragging or not
  bool m_Dragging;

  // Should we flip the Y coordinates of the events (top = 0, bottom = h)
  bool m_FlipYCoordinate;

  /** Whether the keyboard focus is grabbed when the mouse enters the window */
  bool m_GrabFocusOnEntry;

  /** Whether the window has keyboard focus */
  bool m_Focus;

  /** PNG filename to save on the next draw command */
  const char *m_DumpPNG;
};

#endif // __FLTKCanvas_h_

