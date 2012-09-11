/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: PolygonInteractionMode.cxx,v $
  Language:  C++
  Date:      $Date: 2010/05/27 07:29:36 $
  Version:   $Revision: 1.8 $
  Copyright (c) 2007 Paul A. Yushkevich
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "PolygonInteractionMode.h"

#include "GlobalState.h"
#include "PolygonDrawing.h"
#include "UserInterfaceBase.h"
#include "IRISException.h"
#include "IRISApplication.h"

PolygonInteractionMode
::PolygonInteractionMode(GenericSliceWindow *parent)
: GenericSliceWindow::EventHandler(parent)
{
  m_Drawing = new PolygonDrawing(parent);
}

PolygonInteractionMode
::~PolygonInteractionMode()
{
  delete m_Drawing;
}

int
PolygonInteractionMode
::OnEitherEvent(const FLTKEvent &event, 
                const FLTKEvent &irisNotUsed(pressEvent))
{

  // We'll need these shorthands
  int id = m_Parent->m_Id;

#ifdef DRAWING_LOCK
  if (!m_GlobalState->GetDrawingLock(id)) break;
#endif /* DRAWING_LOCK */

  // Compute the dimension of a pixel on the screen
  Vector2f pixelSize = GetPixelSizeVector();

  // Masquerade keypress events as mouse-clicks at the cursor position
  int rc;
  if(event.Key == ' ') 
    {
    // Map the cursor position into slice coordinates
    Vector3f xEvent = m_Parent->MapImageToSlice(
      to_float(m_Driver->GetCursorPosition()));

    // Get the event state based on shift-ctrl
    int fakeButton = (event.State & FL_SHIFT) ? FL_RIGHT_MOUSE : FL_LEFT_MOUSE;

    // Handle the event
    rc = m_Drawing->Handle(FL_PUSH, fakeButton, xEvent(0), xEvent(1),
                      pixelSize(0),pixelSize(1));
    }
  else
    {
    // Map the event into slice coordinates 
    Vector3f xEvent = m_Parent->MapWindowToSlice(event.XSpace.extract(2));

    // Handle the event
    rc = m_Drawing->Handle(event.Id,event.SoftButton,xEvent(0),xEvent(1),
                      pixelSize(0),pixelSize(1));
    }

#ifdef DRAWING_LOCK
  m_GlobalState->ReleaseDrawingLock(id);
#endif /* DRAWING_LOCK */
  
  // Update the display
  m_Parent->GetCanvas()->redraw();
  
  // Let the parent UI know that the polygon state has changed
  m_ParentUI->OnPolygonStateUpdate(id);

  // Even though no action may have been performed, we don't want other handlers
  // to get the left and right mouse button events
  return rc;
}

Vector2f 
PolygonInteractionMode
::GetPixelSizeVector()
{
  Vector3f x = 
    m_Parent->MapWindowToSlice(Vector2f(1.0f)) - 
    m_Parent->MapWindowToSlice(Vector2f(0.0f));

  return Vector2f(x[0],x[1]);
}

                          
void
PolygonInteractionMode
::OnDraw()
{
  // Compute the dimension of a pixel on the screen
  Vector2f pixelSize = GetPixelSizeVector();

  // Call the poly's draw method
  m_Drawing->Draw(pixelSize(0),pixelSize(1));
}

