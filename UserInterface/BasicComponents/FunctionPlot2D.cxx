/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: FunctionPlot2D.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:21 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "FunctionPlot2D.h"

#include "FL/gl.h"
  
FunctionPlot2D
::FunctionPlot2D()
{
  m_Settings = FunctionPlot2DSettings::GetDefaultSettings();
}

void
FunctionPlot2D
::SetDataPoints(float *xPoints,float *yPoints,unsigned int nPoints)
{
  // Intialize the array
  m_Points.clear();
  m_Points.reserve(nPoints);
  
  // Fill the array
  for(unsigned int i=0;i<nPoints;i++)
    {
    m_Points.push_back(Vector2f(xPoints[i],yPoints[i]));    
    }
}

void
FunctionPlot2D
::Draw()
{
  // Push the color/blending attributes
  glPushAttrib(GL_COLOR_BUFFER_BIT);

  // Draw the background rectangle
  glColor3fv(m_Settings.GetBackgroundColor().data_block());
  glBegin(GL_QUADS);
  glVertex2f(0,0);
  glVertex2f(0,1);
  glVertex2f(1,1);
  glVertex2f(1,0);
  glEnd();

  // Draw the frame around the plot if requested
  if(m_Settings.GetShowFrame())
    {
    glColor3fv(m_Settings.GetFrameColor().data_block());
    glBegin(GL_LINE_LOOP);
    glVertex2f(0,0);
    glVertex2f(0,1);
    glVertex2f(1,1);
    glVertex2f(1,0);
    glEnd();
    }

  // These are the extents of the plot region
  Vector2f xMin = m_Settings.GetPlotRangeMin();
  Vector2f xMax = m_Settings.GetPlotRangeMax();

  // Make sure these points create a rectangle
  assert(xMin(0) < xMax(0) && xMin(1) < xMax(1));
  
  // Change the view matrix to the passed in coordinates
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glScalef(1.0f / (xMax(0) - xMin(0)), 1.0f / (xMax(1) - xMin(1)), 1.0f);
  glTranslatef(-xMin(0),-xMin(1),0.0f);

  // Do we draw the axes?
  if(m_Settings.GetShowAxes())
    {

    // First draw the ticks, or it may look ugly
    if(m_Settings.GetShowMajorTicks())
      {
      
      }

    if(m_Settings.GetShowMinorTicks())
      {

      }

    // Now draw the axes
    Vector2f xAxes = m_Settings.GetAxesPosition();

    glColor3fv(m_Settings.GetAxesColor().data_block());
    glBegin(GL_LINES);
    glVertex2f(xAxes(0),xMin(1));
    glVertex2f(xAxes(0),xMax(1));
    glVertex2f(xMin(0), xAxes(1));
    glVertex2f(xMax(0), xAxes(1));
    glEnd();

    }
  
  // Allow line smoothing
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glLineWidth(2.0);

  // Finally, plot the function
  glColor3fv(m_Settings.GetPlotColor().data_block());
  glBegin(GL_LINE_STRIP);

  // Plot all the points
  for(PointVector::iterator it=m_Points.begin();it!=m_Points.end();++it)
    {
    glVertex2fv(it->data_block());
    }

  glEnd();

  // Restore the model matrix
  glPopMatrix();  

  // Restore the attributes
  glPopAttrib();
}

FunctionPlot2DSettings
FunctionPlot2DSettings
::GetDefaultSettings()
{
  FunctionPlot2DSettings settings;

  settings.m_PlotRangeMin = Vector2f(-1.0f);
  settings.m_PlotRangeMax = Vector2f(1.0f);
  settings.m_AxesPosition = Vector2f(0.0f);
  settings.m_MajorTickSpacing = Vector2f(0.5f);
  settings.m_MinorTickSpacing = Vector2f(0.1f);

  settings.m_ShowAxes = true;
  settings.m_ShowFrame = true;
  settings.m_ShowMajorTicks = true;
  settings.m_ShowMinorTicks = true;
  
  settings.m_AxesColor = Vector3f(0.0f);
  settings.m_FrameColor = Vector3f(0.0f);
  settings.m_PlotColor = Vector3f(1.0f,0.0f,0.0f);
  settings.m_BackgroundColor = Vector3f(1.0f);

  return settings;
}


