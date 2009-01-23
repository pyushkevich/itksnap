/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: AnnotationInteractionMode.cxx,v $
  Language:  C++
  Date:      $Date: 2009/01/23 21:48:59 $
  Version:   $Revision: 1.2 $
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
::OnMouseRelease(const FLTKEvent &event, 
                 const FLTKEvent &irisNotUsed(pressEvent))
{
  // Record the location
  Vector3d xEvent = to_double(m_Parent->MapWindowToSlice(event.XSpace.extract(2)));

  // Record the location of the event
  if(m_FlagDrawingLine)
    {
    LineIntervalType lit;
    lit.first = m_LineStart;
    lit.second = xEvent;
    m_Lines.push_back(lit);
    m_FlagDrawingLine = false;
    }
  else
    {
    m_LineStart = xEvent;
    m_FlagDrawingLine = true;
    }

  // Even though no action may have been performed, we don't want other handlers
  // to get the left and right mouse button events
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

  // Draw each of the lines
  glColor3d(1.,1.,1.);
  glBegin(GL_LINES);
  for(LineIntervalList::iterator it = m_Lines.begin(); it!=m_Lines.end(); it++)
    {
    glVertex2d(it->first[0], it->first[1]);
    glVertex2d(it->second[0], it->second[1]);
    }
  glEnd();

  glPopAttrib();
}

