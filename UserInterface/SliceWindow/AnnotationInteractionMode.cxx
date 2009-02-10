/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: AnnotationInteractionMode.cxx,v $
  Language:  C++
  Date:      $Date: 2009/02/10 17:54:23 $
  Version:   $Revision: 1.11 $
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
#include <cmath>
#include <iomanip>

#ifndef PI
#define PI 3.14159265358979323846
#endif

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
::OnKeyDown(const FLTKEvent &event)
{
  // on mac there are two delete keys
  // one maps to backspace on other pc
  if(Fl::event_key() == FL_Delete || Fl::event_key() == FL_BackSpace)
    {
    m_Lines.clear();
    // Redraw
    m_Parent->GetCanvas()->redraw();
    // only return 1 when the keystroke is handled
    return 1;
    }
  // leave it for additional handling
  return 0;
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
    Vector3f xEvent = m_Parent->MapWindowToSlice(event.XSpace.extract(2));
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
  Vector3f xEvent = m_Parent->MapWindowToSlice(event.XSpace.extract(2));

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
  // Get the current annotation settings
  AnnotationSettings as = m_ParentUI->GetDriver()->GetGlobalState()->GetAnnotationSettings();
  const bool shownOnAllSlices = as.shownOnAllSlices;

  // Push the line state
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

  // set line and point drawing parameters
  glPointSize(4);
  glLineWidth(0.5);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Draw current line
  if(m_FlagDrawingLine)
    {
    glColor3d(1.,1.,0.);
    glBegin(GL_POINTS);
    glVertex2d(m_CurrentLine.first[0], m_CurrentLine.first[1]);
    glVertex2d(m_CurrentLine.second[0], m_CurrentLine.second[1]);
    glEnd();
    glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x9999);
    glBegin(GL_LINES);
    glVertex2d(m_CurrentLine.first[0], m_CurrentLine.first[1]);
    glVertex2d(m_CurrentLine.second[0], m_CurrentLine.second[1]);
    glEnd();
    glPopAttrib();

    // Compute the length of the drawing line
    Vector3f Pt1InAnatomy = m_Parent->MapSliceToAnatomy(m_CurrentLine.first);
    Vector3f Pt2InAnatomy = m_Parent->MapSliceToAnatomy(m_CurrentLine.second);
    double length = (Pt1InAnatomy[0] - Pt2InAnatomy[0]) * (Pt1InAnatomy[0] - Pt2InAnatomy[0])
                  + (Pt1InAnatomy[1] - Pt2InAnatomy[1]) * (Pt1InAnatomy[1] - Pt2InAnatomy[1])
                  + (Pt1InAnatomy[2] - Pt2InAnatomy[2]) * (Pt1InAnatomy[2] - Pt2InAnatomy[2]);
    length = sqrt(length);
    std::ostringstream oss_length;
    oss_length << std::setprecision(4) << length << " " << "mm";

    // Compute the offset of 5 screen pixels
    Vector3f v_offset = 
      m_Parent->MapWindowToSlice(Vector2f(5.f,5.f)) - m_Parent->MapWindowToSlice(Vector2f(0.f,0.f));
    Vector3f v_dims = 
      m_Parent->MapWindowToSlice(Vector2f(48.f,12.f)) - m_Parent->MapWindowToSlice(Vector2f(0.f,0.f));

    // Show the length of the drawing line
    gl_draw(oss_length.str().c_str(), 
      (float) (m_CurrentLine.second[0] + v_offset(0)), 
      (float) (m_CurrentLine.second[1] + v_offset(1)));

    // Compute and show the intersection angles of the drawing line with the other (visible) lines
    for(LineIntervalList::iterator it = m_Lines.begin(); it!=m_Lines.end(); it++)
      {
      if(shownOnAllSlices || it->first[2] == m_Parent->m_DisplayAxisPosition)
        {
        Vector3f vit = it->second - it->first;
        vit /= sqrt(vit[0]*vit[0] + vit[1]*vit[1]);
	   Vector3f vc = m_CurrentLine.second - m_CurrentLine.first;
        vc /= sqrt(vc[0]*vc[0] + vc[1]*vc[1]);

        // Compute the dot product and no need for the third components that are zeros
        double angle = 180.0 * acos(fabs(vc[0]*vit[0]+vc[1]*vit[1])) / PI;
        std::ostringstream oss_angle;
        oss_angle << std::setprecision(3) << angle << " " << "deg";

	   // Show the length of the drawing line
        gl_draw(oss_angle.str().c_str(), 
          (float) (0.5*(it->first[0] + it->second[0]) + v_offset(0)), 
          (float) (0.5*(it->first[1] + it->second[1]) + v_offset(1)));
        }
      }
    }

  // Draw each of the lines
  glColor3d(1.,0.,0.);
  glBegin(GL_POINTS);
  for(LineIntervalList::iterator it = m_Lines.begin(); it!=m_Lines.end(); it++)
    {
    if(shownOnAllSlices || it->first[2] == m_Parent->m_DisplayAxisPosition)
      glVertex2d(0.5*(it->first[0] + it->second[0]), 0.5*(it->first[1] + it->second[1]));
    }
  glEnd();
  glBegin(GL_LINES);
  for(LineIntervalList::iterator it = m_Lines.begin(); it!=m_Lines.end(); it++)
    {
    if(shownOnAllSlices || it->first[2] == m_Parent->m_DisplayAxisPosition)
      {
      glVertex2d(it->first[0], it->first[1]);
      glVertex2d(it->second[0], it->second[1]);
	 }
    }
  glEnd();

  glPopAttrib();
}

