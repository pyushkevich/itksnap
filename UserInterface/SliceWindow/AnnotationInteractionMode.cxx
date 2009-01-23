/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: AnnotationInteractionMode.cxx,v $
  Language:  C++
  Date:      $Date: 2009/01/23 21:22:02 $
  Version:   $Revision: 1.1 $
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
  // Draw each of the lines
  glBegin(GL_LINES);
  for(LineIntervalList::iterator it = m_Lines.begin(); it!=m_Lines.end(); it++)
    {
    glVertex3d(it->first[0], it->first[1], it->first[2]);
    glVertex3d(it->second[0], it->second[1], it->second[2]);
    }
  glEnd();
}

