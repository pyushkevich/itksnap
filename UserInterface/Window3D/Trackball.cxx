/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Trackball.cxx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:29 $
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
#include "Trackball.h"
#include <stdlib.h>
#include <iostream>
#include <math.h>

using std::cerr;
using std::endl;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Trackball
::Trackball()
{
  m_TrackingMotion = GL_FALSE;
  this->Reset();
}


Trackball
::Trackball( const Trackball& M )
{
  *this = M;
}


Trackball
::~Trackball()
{
}


void 
Trackball
::Reset()
{
  int i,j;
  m_Angle = 0.0;
  for (i=0; i<4; i++) {
    for (j=0; j<4; j++) m_RotationMatrix[i][j] = 0.0;
    m_RotationMatrix[i][i] = 1.0;
  }
  m_Zoom = m_OldZoom = 1.0;
  m_PanX = m_OldPanX = 0.0;
  m_PanY = m_OldPanY = 0.0;
}

void 
Trackball
::StartPan( int x, int y )
{
  m_TrackingMotion = GL_TRUE;
  m_LastPosition[0] = (float)x;
  m_LastPosition[1] = (float)y;
}

void 
Trackball
::StopPan()
{
  m_TrackingMotion = GL_FALSE;
  m_OldPanX = m_PanX;
  m_OldPanY = m_PanY;
}

void 
Trackball
::TrackPan( int x, int y, int w, int h,
            float ratew, float rateh )
{
  if ( m_TrackingMotion )
    {
    float dx, dy;
    dx = ratew*(x - m_LastPosition[0])/w;
    dy = rateh*(y - m_LastPosition[1])/h;
    m_PanX = m_OldPanX + dx;
    m_PanY = m_OldPanY - dy;
    }
}

void 
Trackball
::StartZoom( int y )
{
  m_TrackingMotion = GL_TRUE;
  m_LastPosition[0] = (float)y;
}

void 
Trackball
::StopZoom()
{
  m_TrackingMotion = GL_FALSE;
  m_OldZoom = m_Zoom;
}

void 
Trackball
::TrackZoom( int y )
{
  if ( m_TrackingMotion ) {
    float dy;
    dy = y - m_LastPosition[0];
    //        m_Zoom = m_OldZoom - dy/h;
    //        m_Zoom = m_OldZoom - dy;
    m_Zoom = m_OldZoom * 
      (float) pow(1.01f,-dy);
    if ( m_Zoom < 0.0 ) m_Zoom = 0.0;
  }
}

void 
Trackball
::StartRot( int x, int y, int w, int h )
{
  m_TrackingMotion = GL_TRUE;
  PToV( x, y, w, h, m_LastPosition );
}

void 
Trackball
::StopRot()
{
  m_TrackingMotion = GL_FALSE;
  m_Angle = 0.0;
}

void 
Trackball
::TrackRot( int x, int y, int w, int h )
{
  if ( m_TrackingMotion ) {
    float curPos[3], dx, dy, dz;

    PToV(x, y, w, h, curPos);

    dx = curPos[0] - m_LastPosition[0];
    dy = curPos[1] - m_LastPosition[1];
    dz = curPos[2] - m_LastPosition[2];
    m_Angle = (float)(90.0 * sqrt(dx*dx + dy*dy + dz*dz));

    m_Axis[0] = m_LastPosition[1]*curPos[2] - m_LastPosition[2]*curPos[1];
    m_Axis[1] = m_LastPosition[2]*curPos[0] - m_LastPosition[0]*curPos[2];
    m_Axis[2] = m_LastPosition[0]*curPos[1] - m_LastPosition[1]*curPos[0];

    m_LastPosition[0] = curPos[0];
    m_LastPosition[1] = curPos[1];
    m_LastPosition[2] = curPos[2];

    /* Have OpenGL compute the new transformation (simple but slow). */
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();
    glRotatef(m_Angle, m_Axis[0], m_Axis[1], m_Axis[2]);
    glMultMatrixf((GLfloat *) m_RotationMatrix);
    glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) m_RotationMatrix);
    glPopMatrix();
  }
}

void
Trackball
::PToV(int x, int y, int width, int height, float v[3])
{
  float d, a;

  /* project x,y onto a hemi-sphere centered within width, height */
  v[0] = ((float)2.0*x - width) / width;
  v[1] = (height - (float)2.0*y) / height;
  d = (float)sqrt(v[0]*v[0] + v[1]*v[1]);
  v[2] = (float)cos((M_PI/2.0) * ((d < 1.0) ? d : 1.0));
  a = (float)(1.0 / sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]));
  v[0] *= a;
  v[1] *= a;
  v[2] *= a;
}

/*
 *$Log: Trackball.cxx,v $
 *Revision 1.2  2007/12/30 04:05:29  pyushkevich
 *GPL License
 *
 *Revision 1.1  2006/12/02 04:22:27  pyushkevich
 *Initial sf checkin
 *
 *Revision 1.1.1.1  2006/09/26 23:56:17  pauly2
 *Import
 *
 *Revision 1.6  2004/08/26 18:29:20  pauly
 *ENH: New user interface for configuring the UI options
 *
 *Revision 1.5  2003/10/02 14:55:53  pauly
 *ENH: Development during the September code freeze
 *
 *Revision 1.1  2003/09/11 13:51:15  pauly
 *FIX: Enabled loading of images with different orientations
 *ENH: Implemented image save and load operations
 *
 *Revision 1.3  2003/08/27 14:03:23  pauly
 *FIX: Made sure that -Wall option in gcc generates 0 warnings.
 *FIX: Removed 'comment within comment' problem in the cvs log.
 *
 *Revision 1.2  2003/08/27 04:57:47  pauly
 *FIX: A large number of bugs has been fixed for 1.4 release
 *
 *Revision 1.1  2003/07/12 04:46:51  pauly
 *Initial checkin of the SNAP application into the InsightApplications tree.
 *
 *Revision 1.2  2003/07/12 01:34:18  pauly
 *More final changes before ITK checkin
 *
 *Revision 1.1  2003/07/11 23:25:33  pauly
 **** empty log message ***
 *
 *Revision 1.1  2003/03/07 19:29:48  pauly
 *Initial checkin
 *
 *Revision 1.2  2002/12/16 16:40:19  pauly
 **** empty log message ***
 *
 *Revision 1.1.1.1  2002/12/10 01:35:36  pauly
 *Started the project repository
 *
 *
 *Revision 1.2  2002/03/08 14:06:32  moon
 *Added Header and Log tags to all files
 **/
