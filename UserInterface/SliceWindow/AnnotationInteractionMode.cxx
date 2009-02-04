/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: AnnotationInteractionMode.cxx,v $
  Language:  C++
  Date:      $Date: 2009/02/04 00:25:28 $
  Version:   $Revision: 1.4 $
  Copyright (c) 2007 Paul A. Yushkevich
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "AnnotationInteractionMode.h"

#include "GlobalState.h"
#include "PolygonDrawing.h"
#include "UserInterfaceBase.h"
#include "IRISApplication.h"

AnnotationInteractionMode
::AnnotationInteractionMode(GenericSliceWindow *parent)
: GenericSliceWindow::EventHandler(parent)
{
  m_FlagDrawingLine = false;
}

AnnotationInteractionMode
::~AnnotationInteractionMode()
{
}

int
AnnotationInteractionMode
::OnMousePress(const FLTKEvent &event)
{
  if(Fl::event_button() == FL_RIGHT_MOUSE)
    {
    if(m_FlagDrawingLine)
      {
      // Commit the current line
	 m_Lines.push_back(m_CurrentLine);
      m_FlagDrawingLine = false;
	 }
    else
      // Erase the last line
      if(!m_Lines.empty())
        {
        m_Lines.pop_back();
        }
    }
  else if(Fl::event_button() == FL_LEFT_MOUSE)
    {
    // Record the location
    Vector3d xEvent = to_double(m_Parent->MapWindowToSlice(event.XSpace.extract(2)));
    m_CurrentLine.second = xEvent;
    }

  // Redraw
  m_Parent->GetCanvas()->redraw();

  // Even though no action may have been performed, we don't want other handlers
  // to get the left and right mouse button events
  return 1;
}

int
AnnotationInteractionMode
::OnMouseDrag(const FLTKEvent &event,
              const FLTKEvent &pressEvent)
{
  // Make sure it's left mouse button being pressed
  if(Fl::event_button() != FL_LEFT_MOUSE)
    return 1;

  // Record the location
  Vector3d xEvent = to_double(m_Parent->MapWindowToSlice(event.XSpace.extract(2)));

  // Record the location of the event
  if(m_FlagDrawingLine)
    {
    m_CurrentLine.second = xEvent;
    }
  else
    {
    m_CurrentLine.first = xEvent;
    m_CurrentLine.second = xEvent;
    m_FlagDrawingLine = true;
    }

  // redraw
  m_Parent->GetCanvas()->redraw();
  return 1;
}

void
AnnotationInteractionMode
::OnDraw()
{
  // Push the line state
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);  

  // set line and point drawing parameters
  glPointSize(4);
  glLineWidth(0.5);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor3d(1.,0.,0.);

  // Draw current line
  if(m_FlagDrawingLine)
    {
    glBegin(GL_POINTS);
    glVertex2d(m_CurrentLine.first[0], m_CurrentLine.first[1]);
    glVertex2d(m_CurrentLine.second[0], m_CurrentLine.second[1]);
    glEnd();
    glPushAttrib(GL_LINE_BIT);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x9999);
    glBegin(GL_LINES);
    glVertex2d(m_CurrentLine.first[0], m_CurrentLine.first[1]);
    glVertex2d(m_CurrentLine.second[0], m_CurrentLine.second[1]);
    glEnd();
    glPopAttrib();
    }
  // Draw each of the lines
  glBegin(GL_LINES);
  for(LineIntervalList::iterator it = m_Lines.begin(); it!=m_Lines.end(); it++)
    {
    glVertex2d(it->first[0], it->first[1]);
    glVertex2d(it->second[0], it->second[1]);
    }
  glEnd();

  glPopAttrib();
}

