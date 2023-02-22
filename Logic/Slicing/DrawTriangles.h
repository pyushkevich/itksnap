/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: DrawTriangles.h,v $
  Language:  C++
  Date:      $Date: 2023/02/20 $
  Copyright (c) 2023 Paul A. Yushkevich

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
#ifndef _DRAW_TRIANGLES_H_
#define _DRAW_TRIANGLES_H_

/*
 * This code is a modification of DrawTriangles.cc from Andras Kelemen and
 * Martin Styner, circa 2000, adapted to ITK-SNAP RLE images
 */

namespace draw_triangles
{
template <class T> inline T SGN(T a) { return (((a)<0) ? -1 : ((a)>0)); }
template <class T> inline T ABS(T a) { return (((a)<0) ? -(a) : (a)); }
using Point3 = double[3];

template <class TImage, class TUpdateFunctor>
void
digline(int x1, int y1,  int x2,   int y2,
        TImage *image, int *dim, const TUpdateFunctor &fn_update, char direction, int slice)
{
  int d, x, y, ax, ay, sx, sy, dx, dy;

  dx = x2-x1;  ax = ABS(dx)<<1;  sx = SGN(dx);
  dy = y2-y1;  ay = ABS(dy)<<1;  sy = SGN(dy);

  int offset = 0;
  int size1,size2;
  if (direction == 'Z')
    {
    offset = dim[0]*dim[1]*slice;
    size1 = dim[0];
    size2 = 1;
    }
  else if (direction == 'Y')
    {
    offset = dim[0]*slice;
    size1 = dim[0]*dim[1];
    size2 = 1;
    }
  else if (direction == 'X')
    {
    offset = slice;
    size1 = dim[0]*dim[1];
    size2 = dim[0];
    }


  x = x1;
  y = y1;
  if (ax>ay)
    {                /* x dominant */
    d = ay-(ax>>1);
    for (;;)
      {
      // Update the pixel at the specified offset
      fn_update(image, offset+size1*y+size2*x);
      // image[offset+size1*y+size2*x] = (char) (color);
      if (x==x2) return;
      if (d>=0)
        {
        y += sy;
        d -= ax;
        }
      x += sx;
      d += ay;
      }
    }
  else {                      /* y dominant */
    d = ax-(ay>>1);
    for (;;)
      {
      // Update the pixel at the specified offset
      fn_update(image, offset+size1*y+size2*x);
      // image[offset+size1*y+size2*x] = (char) (color);
      if (y==y2) return;
      if (d>=0)
        {
        x += sx;
        d -= ay;
        }
      y += sy;
      d += ax;
      }
    }
}


}

/**
 * Scan converts a manifold object described by a set of triangles.
 * The coordinates of vertex_table are in units of voxels with image(0,0,0)
 * as an origin, vertex_table is a table of num_triangles*3 Pointers to 3 doubles (a point)
 */
template <class TImage, class TUpdateFunctor>
void DrawBinaryTrianglesSheetFilled(
    TImage *image, int *dim, double **vertex_table, int num_triangles, const TUpdateFunctor &fn_update)
{
  using namespace draw_triangles;

  int i,k,p_p;
  int maxz, minz, maxy,miny, maxx, minx;
  Point3 p0, p1, p2;
  //  int color = 255;

  maxz = 0;
  minz = dim[2];

  // Draw twice, once in z and then in y, because triangles parrallel to scan
  // plane won't be drawn otherwise

  // determine maxz
  for(i = 0; i < num_triangles*3; i++)
    {
    if(vertex_table[i][2]>maxz) maxz = (int) vertex_table[i][2];
    if(vertex_table[i][2]<minz) minz = (int) vertex_table[i][2];
    }
  for(p_p=minz;p_p<=maxz;p_p++)
    { // Zcoor

    for(k=0;k<num_triangles;k++)
      {

      p0[0]=vertex_table[k*3+0][0];
      p0[1]=vertex_table[k*3+0][1];
      p0[2]=vertex_table[k*3+0][2];
      p1[0]=vertex_table[k*3+1][0];
      p1[1]=vertex_table[k*3+1][1];
      p1[2]=vertex_table[k*3+1][2];
      p2[0]=vertex_table[k*3+2][0];
      p2[1]=vertex_table[k*3+2][1];
      p2[2]=vertex_table[k*3+2][2];

      double x1,y1,x2,y2;

      if((SGN(p0[2]-(float)p_p)!=SGN(p1[2]-(float)p_p)) ||
         (SGN(p1[2]-(float)p_p)!=SGN(p2[2]-(float)p_p)) ||
         (SGN(p0[2]-(float)p_p)!=SGN(p2[2]-(float)p_p)))
        {
        if((SGN(p0[2]-(float)p_p)!=SGN(p1[2]-(float)p_p)) &&
           (SGN(p1[2]-(float)p_p)!=SGN(p2[2]-(float)p_p)))
          {
          x1 = (p_p-p0[2])*(p1[0]-p0[0])/(p1[2]-p0[2])+p0[0];
          y1 = (p_p-p0[2])*(p1[1]-p0[1])/(p1[2]-p0[2])+p0[1];
          x2 = (p_p-p1[2])*(p2[0]-p1[0])/(p2[2]-p1[2])+p1[0];
          y2 = (p_p-p1[2])*(p2[1]-p1[1])/(p2[2]-p1[2])+p1[1];
          digline((int)x1, (int)y1, (int)x2, (int)y2,
                  image,dim,fn_update,'Z',p_p);
          }
        if((SGN(p1[2]-(float)p_p)!=SGN(p2[2]-(float)p_p)) &&
           (SGN(p0[2]-(float)p_p)!=SGN(p2[2]-(float)p_p)))
          {
          x1 = (p_p-p0[2])*(p2[0]-p0[0])/(p2[2]-p0[2])+p0[0];
          y1 = (p_p-p0[2])*(p2[1]-p0[1])/(p2[2]-p0[2])+p0[1];
          x2 = (p_p-p1[2])*(p2[0]-p1[0])/(p2[2]-p1[2])+p1[0];
          y2 = (p_p-p1[2])*(p2[1]-p1[1])/(p2[2]-p1[2])+p1[1];
          digline((int)x1, (int)y1, (int)x2,(int)y2,
                  image,dim,fn_update,'Z',p_p);
          }
        if((SGN(p0[2]-(float)p_p)!=SGN(p1[2]-(float)p_p)) &&
           (SGN(p0[2]-(float)p_p)!=SGN(p2[2]-(float)p_p)))
          {
          x1 = (p_p-p0[2])*(p1[0]-p0[0])/(p1[2]-p0[2])+p0[0];
          y1 = (p_p-p0[2])*(p1[1]-p0[1])/(p1[2]-p0[2])+p0[1];
          x2 = (p_p-p0[2])*(p2[0]-p0[0])/(p2[2]-p0[2])+p0[0];
          y2 = (p_p-p0[2])*(p2[1]-p0[1])/(p2[2]-p0[2])+p0[1];
          digline((int)x1, (int)y1, (int)x2, (int)y2,
                  image,dim,fn_update,'Z',p_p);
          }
        }
      }
    }

  maxy = 0;
  miny = dim[1];
  // determine maxy
  for(i = 0; i < num_triangles*3; i++)
    {
    if(vertex_table[i][1]>maxy) maxy = (int) vertex_table[i][1];
    if(vertex_table[i][1]<miny) miny = (int) vertex_table[i][1];
    }
  for(p_p=miny;p_p<=maxy;p_p++)
    { // Ycoor

    for(k=0;k<num_triangles;k++) {

      p0[0]=vertex_table[k*3+0][0];
      p0[1]=vertex_table[k*3+0][1];
      p0[2]=vertex_table[k*3+0][2];
      p1[0]=vertex_table[k*3+1][0];
      p1[1]=vertex_table[k*3+1][1];
      p1[2]=vertex_table[k*3+1][2];
      p2[0]=vertex_table[k*3+2][0];
      p2[1]=vertex_table[k*3+2][1];
      p2[2]=vertex_table[k*3+2][2];

      double x1,y1,x2,y2;
      if((SGN(p0[1]-(float)p_p)!=SGN(p1[1]-(float)p_p)) ||
         (SGN(p1[1]-(float)p_p)!=SGN(p2[1]-(float)p_p)) ||
         (SGN(p0[1]-(float)p_p)!=SGN(p2[1]-(float)p_p)))
        {
        if((SGN(p0[1]-(float)p_p)!=SGN(p1[1]-(float)p_p)) &&
           (SGN(p1[1]-(float)p_p)!=SGN(p2[1]-(float)p_p)))
          {
          x1 = (p_p-p0[1])*(p1[0]-p0[0])/(p1[1]-p0[1])+p0[0];
          y1 = (p_p-p0[1])*(p1[2]-p0[2])/(p1[1]-p0[1])+p0[2];
          x2 = (p_p-p1[1])*(p2[0]-p1[0])/(p2[1]-p1[1])+p1[0];
          y2 = (p_p-p1[1])*(p2[2]-p1[2])/(p2[1]-p1[1])+p1[2];
          digline((int)x1, (int)y1, (int)x2, (int)y2,
                  image,dim,fn_update,'Y',p_p);
          }
        if((SGN(p1[1]-(float)p_p)!=SGN(p2[1]-(float)p_p)) &&
           (SGN(p0[1]-(float)p_p)!=SGN(p2[1]-(float)p_p)))
          {
          x1 = (p_p-p0[1])*(p2[0]-p0[0])/(p2[1]-p0[1])+p0[0];
          y1 = (p_p-p0[1])*(p2[2]-p0[2])/(p2[1]-p0[1])+p0[2];
          x2 = (p_p-p1[1])*(p2[0]-p1[0])/(p2[1]-p1[1])+p1[0];
          y2 = (p_p-p1[1])*(p2[2]-p1[2])/(p2[1]-p1[1])+p1[2];
          digline((int)x1, (int)y1, (int)x2, (int)y2,
                  image,dim,fn_update,'Y',p_p);
          }
        if((SGN(p0[1]-(float)p_p)!=SGN(p1[1]-(float)p_p)) &&
           (SGN(p0[1]-(float)p_p)!=SGN(p2[1]-(float)p_p)))
          {
          x1 = (p_p-p0[1])*(p1[0]-p0[0])/(p1[1]-p0[1])+p0[0];
          y1 = (p_p-p0[1])*(p1[2]-p0[2])/(p1[1]-p0[1])+p0[2];
          x2 = (p_p-p0[1])*(p2[0]-p0[0])/(p2[1]-p0[1])+p0[0];
          y2 = (p_p-p0[1])*(p2[2]-p0[2])/(p2[1]-p0[1])+p0[2];
          digline((int)x1, (int)y1, (int)x2, (int)y2,
                  image,dim,fn_update,'Y',p_p);
          }
        }
      }
    }

  maxx = 0;
  minx = dim[0];
  // determine maxx
  for(i = 0; i < num_triangles*3; i++)
    {
    if(vertex_table[i][0]>maxx) maxx = (int) vertex_table[i][0];
    if(vertex_table[i][0]<minx) minx = (int) vertex_table[i][0];
    }
  for(p_p=minx;p_p<=maxx;p_p++)
    { // Xcoor

    for(k=0;k<num_triangles;k++) {

      p0[0]=vertex_table[k*3+0][0];
      p0[1]=vertex_table[k*3+0][1];
      p0[2]=vertex_table[k*3+0][2];
      p1[0]=vertex_table[k*3+1][0];
      p1[1]=vertex_table[k*3+1][1];
      p1[2]=vertex_table[k*3+1][2];
      p2[0]=vertex_table[k*3+2][0];
      p2[1]=vertex_table[k*3+2][1];
      p2[2]=vertex_table[k*3+2][2];

      double x1,y1,x2,y2;
      if((SGN(p0[0]-(float)p_p)!=SGN(p1[0]-(float)p_p)) ||
         (SGN(p1[0]-(float)p_p)!=SGN(p2[0]-(float)p_p)) ||
         (SGN(p0[0]-(float)p_p)!=SGN(p2[0]-(float)p_p)))
        {
        if((SGN(p0[0]-(float)p_p)!=SGN(p1[0]-(float)p_p)) &&
           (SGN(p1[0]-(float)p_p)!=SGN(p2[0]-(float)p_p)))
          {
          x1 = (p_p-p0[0])*(p1[1]-p0[1])/(p1[0]-p0[0])+p0[1];
          y1 = (p_p-p0[0])*(p1[2]-p0[2])/(p1[0]-p0[0])+p0[2];
          x2 = (p_p-p1[0])*(p2[1]-p1[1])/(p2[0]-p1[0])+p1[1];
          y2 = (p_p-p1[0])*(p2[2]-p1[2])/(p2[0]-p1[0])+p1[2];
          digline((int)x1, (int)y1, (int)x2, (int)y2,
                  image,dim,fn_update,'X',p_p);
          }
        if((SGN(p1[0]-(float)p_p)!=SGN(p2[0]-(float)p_p)) &&
           (SGN(p0[0]-(float)p_p)!=SGN(p2[0]-(float)p_p)))
          {
          x1 = (p_p-p0[0])*(p2[1]-p0[1])/(p2[0]-p0[0])+p0[1];
          y1 = (p_p-p0[0])*(p2[2]-p0[2])/(p2[0]-p0[0])+p0[2];
          x2 = (p_p-p1[0])*(p2[1]-p1[1])/(p2[0]-p1[0])+p1[1];
          y2 = (p_p-p1[0])*(p2[2]-p1[2])/(p2[0]-p1[0])+p1[2];
          digline((int)x1, (int)y1, (int)x2, (int)y2,
                  image,dim,fn_update,'X',p_p);
          }
        if((SGN(p0[0]-(float)p_p)!=SGN(p1[0]-(float)p_p)) &&
           (SGN(p0[0]-(float)p_p)!=SGN(p2[0]-(float)p_p)))
          {
          x1 = (p_p-p0[0])*(p1[1]-p0[1])/(p1[0]-p0[0])+p0[1];
          y1 = (p_p-p0[0])*(p1[2]-p0[2])/(p1[0]-p0[0])+p0[2];
          x2 = (p_p-p0[0])*(p2[1]-p0[1])/(p2[0]-p0[0])+p0[1];
          y2 = (p_p-p0[0])*(p2[2]-p0[2])/(p2[0]-p0[0])+p0[2];
          digline((int)x1, (int)y1, (int)x2, (int)y2,
                  image,dim,fn_update,'X',p_p);
          }
        }
      }
    }
}


#ifdef _NOT_USED_

//void drawBinaryTrianglesFilled(unsigned char *image, int *dim, 
//			       double ** vertex_table,
//			       int num_triangles);
void drawBinaryTrianglesFilled(unsigned char *image, int *dim, 
			       double ** vertex_table,
			       int num_triangles, int segColor);
//Scan converts a closed object whose surface is described by triangles, the vertex_table
// always groups 3 points to triangles, each point is an array of 3 doubles

// Updated by jwsuh 04/06/2011
// Insert the segColor


void drawBinaryPolygonsFilled(unsigned char *image, int *dim,
			      double** vertex_polygon_table,
			      int num_polygons, int *num_points_per_polygon);
//Scan converts a closed object described by a set of polygons
// the vertex_polygon_table stores all points of the num_polygons polygons
// in  num_points_per_polygon[i] the number of points of the i-th polygon has to be stored

void drawBinaryPolygonsFilled(unsigned char *image, int *dim,
			      double** vertex_polygon_table,
			      int num_polygons, int *num_points_per_polygon,
			      double ** &vertex_triangle_table, int &numTriangles);
//Scan converts a closed object described by a set of polygons
// the vertex_polygon_table stores all points of the num_polygons polygons
// in  num_points_per_polygon[i] the number of points of the i-th polygon has
// to be stored. Returns in vertex_triangle_table the triangle table



//void drawBinaryTrianglesSheetFilled(unsigned char *image, int *dim, 
//				    double ** vertex_table,
//				    int num_triangles);
void drawBinaryTrianglesSheetFilled(unsigned char *image, int *dim, 
				    double ** vertex_table,
				    int num_triangles, int segColor);
//Scan converts a manifold object described by a set of triangles
// the coordinates of vertex_table are in units of voxels with 
// image(0,0,0) as an origin
// vertex_table is a table of num_triangles*3 Pointers to 3 doubles (a point)

// Updated by jwsuh 04/06/2011
// Insert the segColor

     
void drawBinaryPolygonsSheetFilled(unsigned char *image, int *dim,
				   double** vertex_polygon_table,
				   int num_polygons, int *num_points_per_polygon);

//Scan converts a manifold object described by a set of polygons
// the vertex_polygon_table stores all points of the num_polygons polygons
// in  num_points_per_polygon[i] the number of points of the i-th polygon has to be stored


void 
drawBinaryPolygonsSheetFilled(unsigned char *image, int *dim,
			      double** vertex_polygon_table,
			      int num_polygons, int *num_points_per_polygon,
			      double ** &vertex_triangle_table, int &numTriangles);
//Scan converts a manifold object described by a set of polygons
// the vertex_polygon_table stores all points of the num_polygons polygons
// in  num_points_per_polygon[i] the number of points of the i-th polygon has
// to be stored. Returns in vertex_triangle_table the triangle table

#endif // not used

#endif
