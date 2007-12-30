/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ColorMapBox.cxx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:15 $
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
#include "ColorMapBox.h"
#include "SNAPOpenGL.h"

ColorMapBox
::ColorMapBox(int x,int y,int w,int h,const char *label)
  : FLTKCanvas(x, y, w, h, label)
{
  m_RangeStart = 0.0;
  m_RangeEnd = 0.0;
}

void
ColorMapBox
::draw()
{
  // The standard 'valid' business
  if(!valid())
    {
    // Set up the basic projection with a small margin
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-0.005,1.005,-0.05,1.05);
    glViewport(0,0,w(),h());

    // Establish the model view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();    
    }
    
  // Clear the viewport
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

  // Push the related attributes
  glPushAttrib(GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);

  // Disable lighting
  glDisable(GL_LIGHTING);

  // Draw the color map
  glBegin(GL_QUAD_STRIP);

  // Get the colors
  unsigned int n = 256;
  for(unsigned int i = 0; i <= n; i++)
    {
    double u = i * 1.0 / n;
    double t = m_RangeStart + u * (m_RangeEnd - m_RangeStart);
    SpeedColorMap::OutputType xColor = m_ColorMap(t);
    
    glColor3ub(xColor[0], xColor[1], xColor[2]);    
    glVertex2d(u, 0.0); glVertex2d(u, 1.0);
    }
  
  glEnd();

  // Pop the attributes
  glPopAttrib();

  // Done
  glFlush();
}
