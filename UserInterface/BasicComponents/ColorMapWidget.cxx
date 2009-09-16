/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ColorMapWidget.cxx,v $
  Language:  C++
  Date:      $Date: 2009/09/16 20:03:13 $
  Version:   $Revision: 1.7 $
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
#include "ColorMapWidget.h"
#include "SNAPOpenGL.h"
#include "LayerInspectorUILogic.h"

ColorMapWidget
::ColorMapWidget(int x,int y,int w,int h,const char *label)
  : FLTKCanvas(x, y, w, h, label)
{
  m_TextureId = 0xffffffff;
  m_Interactor = new ColorMapInteraction(this);
  this->PushInteractionMode(m_Interactor);

  m_Parent = NULL;
  m_SelectedCMPoint = -1;
  m_SelectedSide = BOTH;
}

ColorMapWidget
::~ColorMapWidget()
{
  delete m_Interactor;
}

void
ColorMapWidget
::gl_draw_circle_with_border(double x, double y, double r, bool select)
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
  glScaled(1.2 / this->w(), 1.2 / this->h(), 1.0);

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

bool
ColorMapWidget
::SetSelection(int pt, Side side)
{
  bool changed = (m_SelectedCMPoint != pt) || (m_SelectedSide != side);
  m_SelectedCMPoint = pt;
  m_SelectedSide = side;
  if(changed)
    {
    this->redraw();
    m_Parent->OnColorMapSelectedPointUpdate();
    }
  return changed;
}


void ColorMapWidget
::SetSelectedCMPoint(int pt)
{
  this->SetSelection(pt, m_SelectedSide);
}

void ColorMapWidget
::SetSelectedSide(Side side)
{
  this->SetSelection(m_SelectedCMPoint, side);
}

void
ColorMapWidget
::draw()
{
  // The standard 'valid' business
  if(!valid())
    {
    // Set up the basic projection with a small margin
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-0.1,1.1,-0.1,1.1);
    glViewport(0,0,w(),h());

    // Establish the model view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();    
    }
    
  // Clear the viewport
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

  // Push the related attributes
  glPushAttrib(GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_TEXTURE_BIT);

  // Disable lighting
  glEnable(GL_TEXTURE_2D);

  // Generate the texture for the background
  const GLsizei boxw = 16;
  if(m_TextureId == 0xffffffff)
    {
    // Create a checkerboard    
    unsigned char *boxarr = new unsigned char[boxw * boxw], *p = boxarr;
    for(GLsizei i = 0; i < boxw; i++) 
      for(GLsizei j = 0; j < boxw; j++)
        if((i < boxw / 2 && j < boxw / 2) || (i > boxw / 2 && j > boxw / 2))
          *p++ = 0xef;
        else
          *p++ = 0xff;

    glGenTextures(1, &m_TextureId);
    glBindTexture(GL_TEXTURE_2D, m_TextureId);

    // Properties for the texture
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    // Turn off modulo-4 rounding in GL
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, boxw, boxw, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, boxarr);
    delete boxarr;
    }

  // Draw the background pattern
  glBindTexture(GL_TEXTURE_2D, m_TextureId);

  glColor3ub(0xff,0xff,0xff);

  glBegin(GL_QUADS);
  double tx = this->w() * 1.0 / boxw;
  double ty = this->h() * 1.0 / boxw;
  glTexCoord2d(0.0, 0.0); glVertex2d(-0.1, -0.1);
  glTexCoord2d(tx, 0.0); glVertex2d(1.1, -0.1);
  glTexCoord2d(tx, ty); glVertex2d(1.1, 1.1);
  glTexCoord2d(0.0, ty); glVertex2d(-0.1, 1.1);
  glEnd();

  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);

  // Turn on alpha blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  // Get the starting and ending color values
  ColorMap::RGBAType v0 = m_ColorMap.MapIndexToRGBA(-0.1);
  ColorMap::RGBAType v1 = m_ColorMap.MapIndexToRGBA(1.1);

  // Draw the actual texture
  glBegin(GL_QUADS);
  
  // Draw the color before 0.0
  glColor4ub(v0[0], v0[1], v0[2], 0x00); glVertex2d(-0.1,0.0);
  glColor4ub(v0[0], v0[1], v0[2], 0xff); glVertex2d(-0.1,1.0);

  /*
  // Draw each of the points
  for(size_t i = 0; i < m_ColorMap.GetNumberOfCMPoints(); i++)
    {
    ColorMap::CMPoint p = m_ColorMap.GetCMPoint(i);
    glColor4ub(p.m_RGBA[0][0], p.m_RGBA[0][1], p.m_RGBA[0][2], 0xff); 
    glVertex2d(p.m_Index, 1.0);
    glColor4ub(p.m_RGBA[0][0], p.m_RGBA[0][1], p.m_RGBA[0][2], 0x00); 
    glVertex2d(p.m_Index, 0.0);
    glColor4ub(p.m_RGBA[1][0], p.m_RGBA[1][1], p.m_RGBA[1][2], 0x00); 
    glVertex2d(p.m_Index, 0.0);
    glColor4ub(p.m_RGBA[1][0], p.m_RGBA[1][1], p.m_RGBA[1][2], 0xff); 
    glVertex2d(p.m_Index, 1.0);
    }
  */
  for(size_t i = 0; i < 512; i++)
    {
    double t = i / 511.0;
    ColorMap::RGBAType rgba = m_ColorMap.MapIndexToRGBA(t);

    glColor4ub(rgba[0], rgba[1], rgba[2], 0xff); 
    glVertex2d(t, 1.0);
    glColor4ub(rgba[0], rgba[1], rgba[2], 0x00); 
    glVertex2d(t, 0.0);
    glColor4ub(rgba[0], rgba[1], rgba[2], 0x00); 
    glVertex2d(t, 0.0);
    glColor4ub(rgba[0], rgba[1], rgba[2], 0xff); 
    glVertex2d(t, 1.0);

    }

  // Draw the color after 1.0
  glColor4ub(v1[0], v1[1], v1[2], 0xff); glVertex2d(1.1,1.0);
  glColor4ub(v1[0], v1[1], v1[2], 0x00); glVertex2d(1.1,0.0);
  
  // Done drawing
  glEnd();

  // Draw the gridlines
  glBegin(GL_LINES);
  glColor4ub(0x80, 0x80, 0x80, 0xff);

  // Horizontal gridlines
  glVertex2d(-0.1, 0.0); glVertex2d( 1.1, 0.0);
  glVertex2d(-0.1, 0.5); glVertex2d( 1.1, 0.5);
  glVertex2d(-0.1, 1.0); glVertex2d( 1.1, 1.0);

  // Vertical gridlines
  glVertex2d(0.0, -0.1); glVertex2d(0.0, 1.1);
  glVertex2d(0.25, -0.1); glVertex2d(0.25, 1.1);
  glVertex2d(0.5, -0.1); glVertex2d(0.5, 1.1);
  glVertex2d(0.75, -0.1); glVertex2d(0.75, 1.1);
  glVertex2d(1.0, -0.1); glVertex2d(1.0, 1.1);

  glEnd();

  // Draw the Alpha curve
  glEnable(GL_LINE_SMOOTH);
  glLineWidth(3.0);

  glBegin(GL_LINE_STRIP);
  glColor4ub(0x00,0x00,0x00,0xff);
  glVertex2d(-0.1, v0[3] / 255.0);
  for(size_t i = 0; i < m_ColorMap.GetNumberOfCMPoints(); i++)
    {
    ColorMap::CMPoint p = m_ColorMap.GetCMPoint(i);
    glVertex2d(p.m_Index, p.m_RGBA[0][3] / 255.0);
    glVertex2d(p.m_Index, p.m_RGBA[1][3] / 255.0);
    }
  glVertex2d(1.1, v1[3] / 255.0);
  glEnd();

  // Draw circles around the points
  glLineWidth(1.0);
  for(size_t i = 0; i < m_ColorMap.GetNumberOfCMPoints(); i++)
    {
    ColorMap::CMPoint p = m_ColorMap.GetCMPoint(i);

    glColor3ub(p.m_RGBA[0][0],p.m_RGBA[0][1],p.m_RGBA[0][2]);
    bool select = ((int)i == m_SelectedCMPoint) && (m_SelectedSide != RIGHT);
    gl_draw_circle_with_border(p.m_Index, p.m_RGBA[0][3] / 255.0, 5.0, select);

    if(p.m_RGBA[0][3] != p.m_RGBA[1][3])
      {
      glColor3ub(p.m_RGBA[1][0],p.m_RGBA[1][1],p.m_RGBA[1][2]);
      bool select = ((int)i == m_SelectedCMPoint) && (m_SelectedSide != LEFT);
      gl_draw_circle_with_border(p.m_Index, p.m_RGBA[1][3] / 255.0, 5.0, select);
      }
    }


  // Pop the attributes
  glPopAttrib();

  // Done
  glFlush();
}


int 
ColorMapInteraction
::OnMousePress(const FLTKEvent &e)
{
  // Reference to the color map  
  ColorMap &cm = m_Parent->m_ColorMap;
  
  // Check if the press occurs near a control point
  for(size_t i = 0; i < cm.GetNumberOfCMPoints(); i++)
    {
    ColorMap::CMPoint p = cm.GetCMPoint(i);
    double dx = fabs(e.XSpace[0] - p.m_Index);
    double dy0 = fabs(e.XSpace[1] - p.m_RGBA[0][3] / 255.0);
    double dy1 = fabs(e.XSpace[1] - p.m_RGBA[1][3] / 255.0);

    if(dx / 1.2 < 5.0 / m_Parent->w())
      {
      if(dy0 / 1.2 < 5.0 / m_Parent->h())
        {
        // We return 0 when the selected point changes to avoid dragging
        if(p.m_Type == ColorMap::CONTINUOUS)
          m_Parent->SetSelection(i, ColorMapWidget::BOTH);
        else
          m_Parent->SetSelection(i, ColorMapWidget::LEFT);
        return 1;
        }
      else if (dy1 / 1.2 < 5.0 / m_Parent->h())
        {
        m_Parent->SetSelection(i, ColorMapWidget::RIGHT);
        return 1;
        }
      }
    }

  // No selection has been made, so we insert a new point
  if(e.XSpace[0] > 0.0 && e.XSpace[0] < 1.0)
    {
    size_t sel = cm.InsertInterpolatedCMPoint(e.XSpace[0]);
    m_Parent->SetSelection(sel, ColorMapWidget::BOTH);
    return 1;
    }

  return 0;
}

int 
ColorMapInteraction
::OnMouseDrag(const FLTKEvent &e, const FLTKEvent &pe)
{
  // Reference to the color map  
  ColorMap &cm = m_Parent->m_ColorMap;
  int isel = m_Parent->m_SelectedCMPoint;

  // Nothing happens if zero is selected
  if(isel < 0) return 0;

  ColorMap::CMPoint psel = cm.GetCMPoint(isel);

  // Get the new alpha and index
  double j = e.XSpace[0];
  double a = e.XSpace[1] * 255;

  // Clip the new index
  if(isel == 0 || isel == (int)cm.GetNumberOfCMPoints()-1)
    {
    // The first and last point can not be moved left or right
    j = psel.m_Index;
    }
  else
    {
    // Other points are constrained by neighbors
    ColorMap::CMPoint p0 = cm.GetCMPoint(isel-1);
    ColorMap::CMPoint p1 = cm.GetCMPoint(isel+1);
    if(j < p0.m_Index) j = p0.m_Index;
    if(j > p1.m_Index) j = p1.m_Index;
    }

  // Update the index of the point
  psel.m_Index = j;

  // Clip the new alpha
  if(a < 0) a = 0;
  if(a > 255) a = 255;

  // Assign the alpha
  if(m_Parent->m_SelectedSide != ColorMapWidget::RIGHT)
    psel.m_RGBA[0][3] = (unsigned char) a;
  if(m_Parent->m_SelectedSide != ColorMapWidget::LEFT)
    psel.m_RGBA[1][3] = (unsigned char) a;

  // Redraw
  cm.UpdateCMPoint(isel, psel);
  m_Parent->redraw();

  // Fire the update event
  m_Parent->GetParent()->OnColorMapChange();

  return 1;
}


int 
ColorMapInteraction
::OnMouseRelease(const FLTKEvent &e, const FLTKEvent &pe)
{
  return 1;
  // return 0; 
  // this->OnMouseDrag(e,pe);
}
