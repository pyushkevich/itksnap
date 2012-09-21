/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IntensityCurveBox.cxx,v $
  Language:  C++
  Date:      $Date: 2010/06/03 19:25:32 $
  Version:   $Revision: 1.8 $
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
#include "IntensityCurveBox.h"
#include "LayerInspectorUILogic.h"
#include "GreyImageWrapper.h"
#include "itkImageRegionConstIterator.h"

#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>

#include <FL/Fl.H>
#include "SNAPOpenGL.h"
#include <FL/fl_draw.H>

using namespace std;
unsigned int IntensityCurveBox::CURVE_RESOLUTION = 64;

IntensityCurveBox
::IntensityCurveBox(int x, int y, int w, int h, const char *label)
: FLTKCanvas(x,y,w,h,label)
{
  // Start with the blank curve
  m_Curve = NULL;
  m_Parent = NULL;

  // Initialize the histogram parameters
  m_HistogramBinSize = 1;
  m_HistogramMaxLevel = 1.0;
  m_HistogramLog = false;

  // Set up the interactor
  m_Interactor = new IntensityCurveInteraction(this);
  PushInteractionMode(m_Interactor);
}

IntensityCurveBox
::~IntensityCurveBox()
{
  delete m_Interactor;
}

void 
IntensityCurveBox
::draw() 
{       
  // The curve should have been initialized
  assert(m_Curve);

  if (!valid()) 
    {
    // Set up the basic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-0.025,1.025,-0.05,1.05);
    glViewport(0,0,w(),h());

    // Establish the model view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    GLdouble modelMatrix[16], projMatrix[16];
    GLint viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
    glGetIntegerv(GL_VIEWPORT,viewport);
    }

  // Clear the viewport
  glClearColor(.95,.95,.95,1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

  // Push the related attributes
  glPushAttrib(GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);

  // Disable lighting
  glDisable(GL_LIGHTING);

  // Draw the plot area. The intensities outside of the window and level
  // are going to be drawn in darker shade of gray
  float t0, t1, xDummy;
  m_Curve->GetControlPoint(0, t0, xDummy);
  m_Curve->GetControlPoint(m_Curve->GetControlPointCount() - 1, t1, xDummy);
  
  // Scale the display so that leftmost point to plot maps to 0, rightmost to 1
  float z0 = std::min(t0, 0.0f);
  float z1 = std::max(t1, 1.0f);
  glPushMatrix();
  glTranslated(-z0 / (z1-z0), 0, 0);
  glScaled(1.0 / (z1-z0), 1, 1);

  // Draw the quads
  glBegin(GL_QUADS);

  // Outer quad
  glColor3d(0.9, 0.9, 0.9);
  glVertex2d(z0,0);
  glVertex2d(z0,1);
  glVertex2d(z1,1);
  glVertex2d(z1,0);

  float q0 = std::max(t0, 0.0f);
  float q1 = std::min(t1, 1.0f);
  if(q1 > q0)
    {
    // Inner quad
    glColor3d(1.0, 1.0, 1.0);
    glVertex2d(q0, 0.0); 
    glVertex2d(q0, 1.0); 
    glVertex2d(q1, 1.0); 
    glVertex2d(q1, 0.0);
    }

  glEnd();

  // Draw a histogram if it exists
  if(m_Histogram.size())
    {
    // Compute the heights of all the histogram bars
    float xWidth = 1.0f / m_Histogram.size();
    unsigned int xPixelsPerBin = w() / m_Histogram.size();
    vector<double> xHeight(m_Histogram.size(), 0.0);

    // Start by computing the maximum (cutoff) height
    float xHeightMax = m_HistogramMax * m_HistogramMaxLevel;
    if(m_HistogramLog)
      xHeightMax = log(xHeightMax) / log(10.0);

    // Continue by computing each bin's height
    unsigned int i;
    for(i=0;i < m_Histogram.size();i++)
      {
      // Process the histogram height based on options
      xHeight[i] = m_Histogram[i];
      if(m_HistogramLog)
        xHeight[i] = (xHeight[i] > 0) ? log(xHeight[i]) / log(10.0) : 0;
      if(xHeight[i] > xHeightMax)
        xHeight[i] = xHeightMax / 0.9f;
      }

    // Draw the horizontal lines at various powers of 10 if in log mode
    if(m_HistogramLog)
      {
      glColor3d(0.75, 0.75, 0.75);
      glBegin(GL_LINES);
      for(double d = 1.0; d < xHeightMax; d+=1.0)
        { glVertex2f(0,d); glVertex2f(1,d); }
      glEnd();
      }
      
    // Paint the filled-in histogram bars. We fill in with black color if the 
    // histogram width is less than 4 pixels
    if(xPixelsPerBin < 4) 
      glColor3f(0.0f, 0.0f, 0.0f); 
    else 
      glColor3f(0.8f, 0.8f, 1.0f);

    // Paint the bars as quads
    glBegin(GL_QUADS);
    for(i=0;i < m_Histogram.size();i++)
      {
      // Compute the physical height of the bin
      float xBin = xWidth * i;
      float hBin = xHeight[i] * 0.9f / xHeightMax;

      // Paint the bar
      glVertex2f(xBin,0);
      glVertex2f(xBin,hBin);
      glVertex2f(xBin+xWidth,hBin);
      glVertex2f(xBin+xWidth,0);
      }
    glEnd();

    // Draw lines around the quads, but only if the bins are thick
    if(xPixelsPerBin >= 4)
      {
      // Draw the vertical lines between the histogram bars
      glBegin(GL_LINE_STRIP);
      glColor3d(0.0, 0.0, 0.0);
      for(i = 0; i < m_Histogram.size(); i++)
        {
        // Compute the physical height of the bin
        float xBin = xWidth * i;
        float hBin = xHeight[i] * 0.9f / xHeightMax;

        // Draw around the bar, starting at the lower left corner
        glVertex2f(xBin, 0.0f);
        glVertex2f(xBin, hBin);
        glVertex2f(xBin + xWidth, hBin);
        glVertex2f(xBin + xWidth, 0.0f);
        }
      glEnd();
      }
    }

  // Draw the box around the plot area
  glColor3d(0,0,0);
  glBegin(GL_LINE_LOOP);
  glVertex2d(z0,0);
  glVertex2d(z0,1);
  glVertex2d(z1,1);
  glVertex2d(z1,0);
  glEnd();

  // Set up the smooth line drawing style
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glLineWidth(2.0);

  // Draw the curve using linear segments
  glColor3d(1.0,0.0,0.0);
  glBegin(GL_LINE_STRIP);

  float t = z0;
  float tStep = (z1-z0) / (CURVE_RESOLUTION);
  for (unsigned int i=0;i<=CURVE_RESOLUTION;i++) 
    {
    glVertex2f(t,m_Curve->Evaluate(t));
    t+=tStep;
    }

  glEnd();

  // Draw the handles
  glLineWidth(1.0);
  for (unsigned int c=0;c<m_Curve->GetControlPointCount();c++) 
    {
    // Get the next control point
    float t,x;
    m_Curve->GetControlPoint(c,t,x);

    // Draw a quad around the control point
    if(c == (unsigned int)m_Interactor->GetMovingControlPoint())
      {
      glColor3d(1,1,0);
      gl_draw_circle_with_border(t, x, 5.0, z1-z0, false);
      }
    else
      {
      glColor3d(1,0,0);
      gl_draw_circle_with_border(t, x, 4.0, z1-z0, false);
      }
    }

  glPopMatrix();

  // Pop the attributes
  glPopAttrib();

  // Done
  glFlush();
}

void
IntensityCurveBox
::gl_draw_circle_with_border(double x, double y, double r, double bw, bool select)
{
  static std::vector<double> cx, cy;
  if(cx.size() == 0)
    {
    for(double a = 0; a < 2 * vnl_math::pi - 1.0e-6; a += vnl_math::pi / 20)
      {
      cx.push_back(cos(a));
      cy.push_back(sin(a));
      }
    }

  glPushMatrix();
  glTranslated(x, y, 0.0);
  glScaled(1.2 * bw / this->w(), 1.2 / this->h(), 1.0);

  glBegin(GL_TRIANGLE_FAN);
  glVertex2d(0, 0);
  for(size_t i = 0; i < cx.size(); i++)
    glVertex2d(r * cx[i], r * cy[i]);
  glVertex2d(r, 0);
  glEnd();

  if(select)
    glColor3ub(0xff,0x00,0xff);
  else
    glColor3ub(0x00,0x00,0x00);
  
  glBegin(GL_LINE_LOOP);
  for(size_t i = 0; i < cx.size(); i++)
    glVertex2d(r * cx[i], r * cy[i]);
  glEnd();

  glPopMatrix();
}

int 
IntensityCurveBox
::GetControlPointInVicinity(float x, float y, int pixelRadius) 
{
  float rx = pixelRadius * 1.0f / w();
  float ry = pixelRadius * 1.0f / h();
  float fx = 1.0f / (rx * rx);
  float fy = 1.0f / (ry * ry);

  float minDistance = 1.0f;
  int nearestPoint = -1;

  for (unsigned int c=0;c<m_Curve->GetControlPointCount();c++) {

    // Get the next control point
    float cx,cy;
    m_Curve->GetControlPoint(c,cx,cy);

    // Check the distance to the control point
    float d = (cx - x) * (cx - x) * fx + (cy - y) * (cy - y) * fy;
    if (minDistance >= d) {
      minDistance = d;
      nearestPoint = c;
    }
  }

  // Negative: return -1
  return nearestPoint;
}

void 
IntensityCurveBox
::ComputeHistogram(GreyImageWrapper *source, unsigned int iMinPixelsPerBin)
{
  // Need a wrapper
  assert(source);
  assert(this->w() > 0);

  // Get 'absolute' image intensity range, i.e., the largest and smallest
  // intensity in the whole image
  GreyType iAbsMin = source->GetImageMin();
  GreyType iAbsMax = source->GetImageMax();
  unsigned int nFrequencies = (iAbsMax - iAbsMin) + 1;

  // Compute the freqencies of the intensities
  unsigned int *frequency = new unsigned int[nFrequencies];
  memset(frequency,0,sizeof(unsigned int) * nFrequencies);
  GreyImageWrapper::ConstIterator it = source->GetImageConstIterator();
  for(it.GoToBegin();!it.IsAtEnd();++it)
    {
    frequency[it.Value()-iAbsMin]++;
    }

  // Determine the bin size: no bin should be less than a single pixel wide
  if(nFrequencies * iMinPixelsPerBin > m_HistogramBinSize * this->w()) 
    m_HistogramBinSize = 
    (unsigned int) ceil(nFrequencies * iMinPixelsPerBin * 1.0 / this->w());
  unsigned int nBins = (unsigned int) ceil(nFrequencies * 1.0 / m_HistogramBinSize);

  // Allocate an array of bins
  m_Histogram.resize(nBins);
  fill(m_Histogram.begin(), m_Histogram.end(), 0);

  // Reset the max-frequency
  m_HistogramMax = 0;

  // Put the frequencies into the bins
  for(unsigned int i=0;i<nFrequencies;i++)
    {
    unsigned int iBin = i / m_HistogramBinSize;
    m_Histogram[iBin] += frequency[i];

    // Compute the maximum frequency
    if(m_HistogramMax < m_Histogram[iBin])
      m_HistogramMax  = m_Histogram[iBin]; 
    }

  // Clear the frequencies
  delete frequency;
}

IntensityCurveInteraction
::IntensityCurveInteraction(IntensityCurveBox *parent) 
: InteractionMode(parent)
{
  m_Parent = parent;
  m_MovingControlPoint = 0;
}

Vector3f
IntensityCurveBox
::GetEventCurveCoordinates(const FLTKEvent &e)
{
  // Draw the plot area. The intensities outside of the window and level
  // are going to be drawn in darker shade of gray
  float t0, t1, xDummy;
  m_Curve->GetControlPoint(0, t0, xDummy);
  m_Curve->GetControlPoint(m_Curve->GetControlPointCount() - 1, t1, xDummy);
  float z0 = std::min(t0, 0.0f);
  float z1 = std::max(t1, 1.0f);
  
  // Scale the display so that leftmost point to plot maps to 0, rightmost to 1
  return Vector3f(e.XSpace[0] * (z1 - z0) + z0, e.XSpace[1], e.XSpace[2]);
}

int 
IntensityCurveInteraction
::OnMousePress(const FLTKEvent &event)
{
  // Check the control point affected by the event
  Vector3f xCurve = m_Parent->GetEventCurveCoordinates(event);
  m_MovingControlPoint = 
    m_Parent->GetControlPointInVicinity(xCurve[0],xCurve[1],5);

  SetCursor(m_MovingControlPoint);

  // Clear the dragged flag
  m_FlagDraggedControlPoint = false;

  return 1;
}

int 
IntensityCurveInteraction
::OnMouseRelease(const FLTKEvent &event,
  const FLTKEvent &irisNotUsed(dragEvent))
{
  Vector3f xCurve = m_Parent->GetEventCurveCoordinates(event);
  if (m_MovingControlPoint >= 0) 
    {
    if(m_FlagDraggedControlPoint)
      {
      // Update the control point
      if (UpdateControl(xCurve))
        {
        // Repaint parent
        m_Parent->redraw();

        // Fire the update event (should this be done on drag?)
        m_Parent->GetParent()->OnCurveChange();
        m_Parent->GetParent()->OnControlPointUpdate();
        }
      }
    else
      {
      m_Parent->GetParent()->OnControlPointUpdate();
      m_Parent->redraw();
      }

    // Set the cursor back to normal
    SetCursor(-1);
  }

  return 1;
}

int 
IntensityCurveInteraction
::OnMouseDrag(const FLTKEvent &event,
  const FLTKEvent &irisNotUsed(dragEvent))
{
  Vector3f xCurve = m_Parent->GetEventCurveCoordinates(event);
  if (m_MovingControlPoint >= 0) 
    {
    // Update the moving control point
    if (UpdateControl(xCurve)) 
      {
      // Repaint parent
      m_Parent->redraw();

      // Fire the update event (should this be done on drag?)
      m_Parent->GetParent()->OnCurveChange();
      m_Parent->GetParent()->OnControlPointUpdate();
      }

    // Set the dragged flag
    m_FlagDraggedControlPoint = true;
    }

  return 1;
}

int 
IntensityCurveInteraction
::OnMouseEnter(const FLTKEvent &irisNotUsed(event))
{
  return 1;
}

int 
IntensityCurveInteraction
::OnMouseLeave(const FLTKEvent &irisNotUsed(event))
{
  return 1;
}

int 
IntensityCurveInteraction
::OnMouseMotion(const FLTKEvent &event)
{
  Vector3f xCurve = m_Parent->GetEventCurveCoordinates(event);
  int cp = 
    m_Parent->GetControlPointInVicinity(xCurve[0],xCurve[1],5);

  SetCursor(cp);

  return 1;
}

void
IntensityCurveInteraction
::SetCursor(int cp)
{
  if (cp < 0)
    {
    fl_cursor(FL_CURSOR_DEFAULT);
    }
  else if (cp == 0 ||
    cp == (int)(m_Parent->GetCurve()->GetControlPointCount()-1))
    {
    fl_cursor(FL_CURSOR_WE);
    }
  else
    {
    fl_cursor(FL_CURSOR_MOVE);
    }
}

int
IntensityCurveInteraction
::GetMovingControlPoint() const
{
  return m_MovingControlPoint;
}

void
IntensityCurveInteraction
::SetMovingControlPoint(int cp)
{
  m_MovingControlPoint = cp;
}


bool
IntensityCurveBox
::UpdateControlPoint(size_t i, float t, float x)
{
  // Must be in range
  assert(i < m_Curve->GetControlPointCount());
  
  // Get the current values
  float told, xold;
  m_Curve->GetControlPoint(i, told, xold);

  // The control value should be in range
  // if(t < 0.0 || t > 1.0)
  //  return false;

  // First and last control points are treated specially because they
  // provide windowing style behavior
  int last = m_Curve->GetControlPointCount()-1;
  if (i == 0 || i == (size_t) last) 
    {
    // Get the current domain
    float tMin,tMax,x;        
    m_Curve->GetControlPoint(0,   tMin, x);
    m_Curve->GetControlPoint(last,tMax, x);

    // Check if the new domain is valid
    float epsilon = 0.02;
    if (i == 0 && t < tMax - epsilon) 
      tMin = t;
    else if (i == (size_t) last && t > tMin + epsilon) 
      tMax = t;
    else 
      // One of the conditions failed; the window has size <= 0
      return false;

    // Change the domain of the curve
    m_Curve->ScaleControlPointsToWindow(tMin, tMax);
    } 
  else
    {
    // Check whether the X coordinate is in range
    if (x < 0.0 || x > 1.0)
      return false;

    // Update the control point
    m_Curve->UpdateControlPoint(i, t, x);

    // Check the curve for monotonicity
    if(!m_Curve->IsMonotonic()) 
      {
      m_Curve->UpdateControlPoint(i, told, xold);
      return false;
      }
    }
  return true;
}

bool
IntensityCurveInteraction
::UpdateControl(const Vector3f &p)
{
  return m_Parent->UpdateControlPoint(
    m_MovingControlPoint, p[0], p[1]);
}

