/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: InteractionMode.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:21 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __InteractionMode_h_
#define __InteractionMode_h_

#include <FL/Fl.H>

#include "SNAPCommonUI.h"
#include "FLTKEvent.h"

// Forward reference to the canvas object
class FLTKCanvas;

/**
 * \class InteractionMode
 * \brief This class defines a UI interaction mode.  
 *
 * It is used to define the behavior of a tool on a canvas (FLTKCanvas).   
 * This class removes the need to write huge handle() methods
 */
class InteractionMode
{
public:

  /** Constructor takes the pointer to the target canvas object */
  InteractionMode(FLTKCanvas *canvas)
    { this->m_Canvas = canvas; }

  /** Destructor */
  virtual ~InteractionMode(void) {} 

  /**
   * Called when mouse button is pressed.
   */
  virtual int OnMousePress(const FLTKEvent &irisNotUsed(event)) 
    { return 0; }

  /**
   * Called when mouse is pressed.  The event generated when the mouse
   * was pressed is also passed in for reference.
   */
  virtual int OnMouseRelease(const FLTKEvent &irisNotUsed(event),
                             const FLTKEvent &irisNotUsed(pressEvent)) 
    { return 0; }

  /**
   * Called when the mouse is dragged.  The event generated when the mouse
   * was pressed is also passed in for reference.
   */
  virtual int OnMouseDrag(const FLTKEvent &irisNotUsed(event),
                          const FLTKEvent &irisNotUsed(pressEvent)) 
    { return 0; }

  /**
   * Called when mouse enters the canvas.  Return 1 to track motion events.
   */
  virtual int OnMouseEnter(const FLTKEvent &irisNotUsed(event)) 
    { return 0; }

  /**
   * Called when mouse exits the canvas.  
   */
  virtual int OnMouseLeave(const FLTKEvent &irisNotUsed(event)) 
    { return 0; }

  /**
   * Called when mouse moves in the canvas
   */
  virtual int OnMouseMotion(const FLTKEvent &irisNotUsed(event)) 
    { return 0; }

  /**
   * Called when mouse moves in the canvas
   */
  virtual int OnMouseWheel(const FLTKEvent &irisNotUsed(event)) 
    { return 0; }

  /**
   * Called when a key on the keyboard is pressed.
   */
  virtual int OnKeyDown(const FLTKEvent &irisNotUsed(event)) 
    { return 0; }

  /**
   * Called when a key on the keyboard is released.
   */
  virtual int OnKeyUp(const FLTKEvent &irisNotUsed(event)) 
    { return 0; }

  /**
   * Called for FLTK short-cut events
   */
  virtual int OnShortcut(const FLTKEvent &irisNotUsed(event)) 
    { return 0; }

  /**
   * Called for all other FLTK events
   */
  virtual int OnOtherEvent(const FLTKEvent &irisNotUsed(event)) 
    { return 0; }

  /**
   * Can be called when the canvas is redrawing itself. 
   * This is not really an event but a convenience method.
   */
  virtual void OnDraw() 
    { return; }

  /** Get the pointer to the client canvas */
  FLTKCanvas *GetCanvas() const
    { return m_Canvas; }

protected:

  // The target canvas
  FLTKCanvas *m_Canvas;
};

#endif // __InteractionMode_h_
