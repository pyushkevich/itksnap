/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: FunctionPlot2DBox.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:21 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "FunctionPlot2DBox.h"
#include "SNAPOpenGL.h"

FunctionPlot2DBox
::FunctionPlot2DBox(int x,int y,int w,int h,const char *label)
  : FLTKCanvas(x,y,w,h,label)
{
}


void 
FunctionPlot2DBox
::draw()
{
  // The standard 'valid' business
  if(!valid())
    {
    // Set up the basic projection with a small margin
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-0.05,1.05,-0.05,1.05);
    glViewport(0,0,w(),h());

    // Establish the model view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();    
    }
    
  // Clear the viewport
  glClearColor(0.906f,0.906f,0.906f,1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

  // Push the related attributes
  glPushAttrib(GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);

  // Disable lighting
  glDisable(GL_LIGHTING);

  // Call the plotting routine
  m_Plotter.Draw();
  
  // Pop the attributes
  glPopAttrib();

  // Done
  glFlush();
}
