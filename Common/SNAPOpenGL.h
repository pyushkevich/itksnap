/*=========================================================================
                                                                                
  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPOpenGL.h,v $
  Language:  C++
  Date:      $Date: 2007/12/31 13:12:04 $
  Version:   $Revision: 1.3 $
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
#ifndef __SNAPOpenGL_h_
#define __SNAPOpenGL_h_


#include "IRISVectorTypes.h"

// Include OpenGL headers according to the platform
// #include <QtOpenGL/QtOpenGL>

#if defined(WIN32)
#include <windows.h>
#endif

// Include GLU, for some reason taken out of Qt 4.8
#if defined (__APPLE__)
  #include <OpenGL/glu.h> 
#else
 #include <GL/glu.h>
#endif

// Inline functions for use with vector classes
inline void glVertex( const Vector3f &x ) { glVertex3fv(x.data_block()); }
inline void glVertex( const Vector3d &x ) { glVertex3dv(x.data_block()); }

inline void glVertex( const Vector3i &x )
{
#if defined(__APPLE__)
    const GLint CastConvertTemp[3]={static_cast<GLint>(x[0]),static_cast<GLint>(x[1]),static_cast<GLint>(x[2])};
    glVertex3iv(CastConvertTemp);//NOTE:  On Mac GLint is a long integer, so we must create a long integer vector to send to glVertex3iv.
    //A more elegant solution could be made if partial template specialization were allowed.
    //Perhaps Vector3i could be defined in terms of GLint instead of int
#else
    glVertex3iv(x.data_block());
#endif
}
inline void glVertex( const Vector2f &x ) { glVertex2fv(x.data_block()); }
inline void glVertex( const Vector2d &x ) { glVertex2dv(x.data_block()); }

inline void glVertex( const Vector2i &x )
{
#if defined(__APPLE__)
    const GLint CastConvertTemp[2]={static_cast<GLint>(x[0]),static_cast<GLint>(x[1])};
    glVertex2iv(CastConvertTemp);//NOTE:  On Mac GLint is a long integer, so we must create a long integer vector to send to glVertex3iv.
    //A more elegant solution could be made if partial template specialization were allowed.
    //Perhaps Vector2i could be defined in terms of GLint instead of int
#else
    glVertex2iv(x.data_block());
#endif
}

inline void glTranslate( const Vector3f &x ) { glTranslatef(x[0],x[1],x[2]); }
inline void glTranslate( const Vector3d &x ) { glTranslated(x[0],x[1],x[2]); }

inline void glScale( const Vector3f &x ) { glScalef(x[0],x[1],x[2]); }
inline void glScale( const Vector3d &x ) { glScaled(x[0],x[1],x[2]); }

void
gl_draw_circle_with_border(double x, double y, double r,
                           int vp_width, int vp_height,
                           Vector3ui color);

Vector3d adjust_color_luminance(const Vector3d &color, double factor);

// GLU functions are deprecated on Apple so we need our own versions
void irisOrtho2D(double x, double w, double y, double h);

#endif // __SNAPOpenGL_h_
