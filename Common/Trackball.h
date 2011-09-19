/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Trackball.h,v $
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
#ifndef __Trackball_h_
#define __Trackball_h_

#include <SNAPOpenGL.h>

/**
 * \class Trackball
 * \brief Virtual trackball for the 3D window
 *
 * \sa Window3D
 */
class Trackball  
{
private:
  GLboolean m_TrackingMotion;
  float m_Angle;
  float m_Axis[3];
  float m_LastPosition[3];
  GLfloat m_RotationMatrix[4][4];
  GLfloat m_Zoom, m_OldZoom;
  GLfloat m_PanX, m_PanY;
  GLfloat m_OldPanX, m_OldPanY;
  
  void PToV( int x, int y, int width, int height, float v[3] );

public:
  Trackball();
  Trackball( const Trackball& T );
  ~Trackball();

  void Reset();
  void StartPan( int x, int y );
  void StopPan();
  void TrackPan( int x, int y, int w, int h, float ratew, float rateh );
  void StartZoom( int y );
  void StopZoom();
  void TrackZoom( int y );
  void StartRot( int x, int y, int w, int h );
  void StopRot();
  void TrackRot( int x, int y, int w, int h );
  inline GLfloat *GetRot() { return( (GLfloat*) m_RotationMatrix ); };
  inline GLfloat GetZoom() { return( m_Zoom ); };
  inline GLfloat GetPanX() { return( m_PanX ); };
  inline GLfloat GetPanY() { return( m_PanY ); };
};

#endif // __Trackball_h_

/*
 *$Log: Trackball.h,v $
 *Revision 1.2  2007/12/30 04:05:29  pyushkevich
 *GPL License
 *
 *Revision 1.1  2006/12/02 04:22:27  pyushkevich
 *Initial sf checkin
 *
 *Revision 1.1.1.1  2006/09/26 23:56:17  pauly2
 *Import
 *
 *Revision 1.5  2004/08/26 18:29:20  pauly
 *ENH: New user interface for configuring the UI options
 *
 *Revision 1.4  2003/10/02 14:55:53  pauly
 *ENH: Development during the September code freeze
 *
 *Revision 1.1  2003/09/11 13:51:15  pauly
 *FIX: Enabled loading of images with different orientations
 *ENH: Implemented image save and load operations
 *
 *Revision 1.3  2003/08/27 14:03:24  pauly
 *FIX: Made sure that -Wall option in gcc generates 0 warnings.
 *FIX: Removed 'comment within comment' problem in the cvs log.
 *
 *Revision 1.2  2003/08/27 04:57:47  pauly
 *FIX: A large number of bugs has been fixed for 1.4 release
 *
 *Revision 1.1  2003/07/12 04:46:51  pauly
 *Initial checkin of the SNAP application into the InsightApplications tree.
 *
 *Revision 1.3  2003/07/12 01:34:18  pauly
 *More final changes before ITK checkin
 *
 *Revision 1.2  2003/07/11 23:25:33  pauly
 **** empty log message ***
 *
 *Revision 1.1  2003/03/07 19:29:48  pauly
 *Initial checkin
 *
 *Revision 1.1.1.1  2002/12/10 01:35:36  pauly
 *Started the project repository
 *
 *
 *Revision 1.2  2002/03/08 14:06:32  moon
 *Added Header and Log tags to all files
 **/
