/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: PolygonScanConvert.cxx,v $
  Language:  C++
  Date:      $Date: 2009/01/23 20:09:38 $
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
#include "PolygonScanConvert.h"
#include "SNAPCommon.h"
#include "itkImage.h"

#include <iostream>

// Typecast for the callback functions
#ifdef WIN32
typedef void (CALLBACK *TessCallback)();
#elif defined (__APPL__)
typedef GLvoid (*TessCallback)(...);
#else
typedef void (*TessCallback)();
#endif

void 
PolygonScanConvertBase
::RasterizeFilled(double *vArray, unsigned int nVertices, 
  unsigned int width, unsigned int height, GLenum glType, void *buffer)
{
  // Push the GL attributes to preserve everything
  glPushAttrib(GL_ALL_ATTRIB_BITS);

  // Tesselate the polygon and save the tesselation as a display list
  GLint dl = glGenLists(1);
  glNewList(dl, GL_COMPILE);

  // Set the background to black
  glClearColor(0,0,0,1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Paint in white
  glColor3d(1.0, 1.0, 1.0);

  // Start the tesselation
  GLUtesselator *tess = gluNewTess();
  gluTessCallback(tess,(GLenum) GLU_TESS_VERTEX, (TessCallback) &glVertex3dv);
  gluTessCallback(tess,(GLenum) GLU_TESS_BEGIN, (TessCallback) &glBegin); 
  gluTessCallback(tess,(GLenum) GLU_TESS_END, (TessCallback) &glEnd);
  gluTessCallback(tess,(GLenum) GLU_TESS_ERROR, 
    (TessCallback) &PolygonScanConvertBase::ErrorCallback);     
  gluTessCallback(tess,(GLenum) GLU_TESS_COMBINE, 
    (TessCallback) &PolygonScanConvertBase::CombineCallback);
  gluTessProperty(tess,(GLenum) GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);  
  gluTessNormal(tess,0.0,0.0,1.0);

  gluTessBeginPolygon(tess,NULL);
  gluTessBeginContour(tess);

  // Add the vertices
  for(unsigned int i=0; i < nVertices; i++)
    { gluTessVertex(tess, vArray + 3*i, vArray + 3*i); }
    
  // End the tesselation
  gluTessEndContour(tess);
  gluTessEndPolygon(tess);

  // End the display list
  glEndList();

  // Draw polygon into back buffer - back buffer should get redrawn
  // anyway before it gets swapped to the screen.
  glDrawBuffer(GL_BACK);
  glReadBuffer(GL_BACK);

  // We will perform a tiled drawing, because the backbuffer may be smaller
  // than the size of the image. First get the viewport size, i.e., tile size
  GLint xViewport[4]; glGetIntegerv(GL_VIEWPORT, xViewport);
  unsigned int wTile = (unsigned int) xViewport[2];
  unsigned int hTile = (unsigned int) xViewport[3];

  // Figure out the number of tiles in x and y dimension
  unsigned int nTilesX = (unsigned int) ceil( width * 1.0 / wTile );
  unsigned int nTilesY = (unsigned int) ceil( height * 1.0 / hTile );

  // Draw and retrieve each tile
  for(unsigned int iTileX = 0; iTileX < nTilesX; iTileX++)
    {
    for(unsigned int iTileY = 0; iTileY < nTilesY; iTileY++)
      {
      // Get the corner of the tile
      unsigned int xTile = iTileX * wTile, yTile = iTileY * hTile;

      // Set the projection matrix
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadIdentity();
      irisOrtho2D(xTile, xTile + wTile, yTile, yTile + hTile);

      // Set the model view matrix
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();

      // Draw the triangles
      glCallList(dl);

      // Figure out the size of the data chunk to copy
      unsigned int wCopy = width - xTile < wTile ? width - xTile : wTile;
      unsigned int hCopy = height - yTile < hTile ? height - yTile : hTile;
      
      // Set up the copy so that the strides are correct
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glPixelStorei(GL_PACK_ROW_LENGTH, width);
      glPixelStorei(GL_PACK_SKIP_PIXELS, xTile);
      glPixelStorei(GL_PACK_SKIP_ROWS, yTile);

      // Copy the pixels to the buffer
      glReadPixels(0, 0, wCopy, hCopy, GL_RED, glType, buffer);

      // Restore the GL state
      glPopMatrix();
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      }
    }

  // Get rid of the display list
  glDeleteLists(dl,1);

  // Delete the tesselator
  gluDeleteTess(tess);

  // Restore the GL state
  glPopAttrib();
}

void PolygonScanConvertBase::ErrorCallback(GLenum errorCode)
{ 
  std::cerr << "Tesselation Error: " << gluErrorString(errorCode) << std::endl; 
}

void PolygonScanConvertBase::CombineCallback(GLdouble coords[3], 
                GLdouble **vertex_data,  
                GLfloat *weight, 
                GLdouble **dataOut) 
{
  GLdouble *vertex = new GLdouble[3];
  vertex[0] = coords[0]; vertex[1] = coords[1]; vertex[2] = coords[2];
  *dataOut = vertex;
}

