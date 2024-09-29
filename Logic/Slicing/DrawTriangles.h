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


/**
 * Another routine for voxelization, adapted from
 * https://raw.githubusercontent.com/Forceflow/cuda_voxelizer/main/src/cpu_voxelizer.cpp
 */

#include "IRISVectorTypes.h"
#include <vnl/vnl_cross.h>

namespace cpu_voxelizer {

static constexpr float float_error = 0.000001;

// Mesh voxelization method
template <class TImage, class TUpdateFunctor>
void DrawBinaryTrianglesSheetFilled(
    TImage *image, int *dim, double **vertex_table, int num_triangles, const TUpdateFunctor &fn_update)
{
  unsigned int debug_n_triangles = 0;
  unsigned int debug_n_voxels_tested = 0;
  unsigned int debug_n_voxels_marked = 0;

  // Box dimensions
  Vector3d delta_p(1.0, 1.0, 1.0);

  // Critical point
  // Vector3d c(0.0, 0.0, 0.0);
  Vector3i grid_max(dim[0]-1, dim[1]-1, dim[2]-1);

  // Iterate over the triangles
  for(int i = 0; i < num_triangles; i++)
    {
    debug_n_triangles++;

    // Vertices
    Vector3d v0(vertex_table[i*3+0][0],vertex_table[i*3+0][1],vertex_table[i*3+0][2]);
    Vector3d v1(vertex_table[i*3+1][0],vertex_table[i*3+1][1],vertex_table[i*3+1][2]);
    Vector3d v2(vertex_table[i*3+2][0],vertex_table[i*3+2][1],vertex_table[i*3+2][2]);

    // Edge vectors
    Vector3d e0 = v1 - v0;
    Vector3d e1 = v2 - v1;
    Vector3d e2 = v0 - v2;

    // Normal vector
    Vector3d n = vnl_cross_3d(e0, e1).normalize();
    Vector3i t_bbox_grid_min, t_bbox_grid_max;

    // Precompute d1/d2 constants for plane intersection check
    double d1 = 0.0, d2 = 0.0;
    for(unsigned int k = 0; k < 3; k++)
      {
      // Compute extents for the voxel grid for this triangle
      t_bbox_grid_min[k] = std::clamp(std::min((int) v0[k], std::min((int) v1[k], (int) v2[k])), 0, grid_max[k]);
      t_bbox_grid_max[k] = std::clamp(std::max((int) v0[k], std::max((int) v1[k], (int) v2[k])), 0, grid_max[k]);

      // Compute critical point for plane intersection check
      if(n[k] > 0.0)
        {
        d1 -= n[k] * (v0[k] - 1.0);
        d2 -= n[k] * v0[k];
        }
      else
        {
        d1 -= n[k] * v0[k];
        d2 -= n[k] * (v0[k] - 1.0);
        }
      }

    // PREPARE PROJECTION TEST PROPERTIES
    // XY plane
    Vector2d n_xy_e0(-1.0 * e0[1], e0[0]);
    Vector2d n_xy_e1(-1.0 * e1[1], e1[0]);
    Vector2d n_xy_e2(-1.0 * e2[1], e2[0]);
    if (n[2] < 0.0)
      {
      n_xy_e0 = -n_xy_e0;
      n_xy_e1 = -n_xy_e1;
      n_xy_e2 = -n_xy_e2;
      }

    double d_xy_e0 = (-1.0 * dot_product(n_xy_e0, Vector2d(v0[0], v0[1]))) + std::max(0.0, n_xy_e0[0]) + std::max(0.0, n_xy_e0[1]);
    double d_xy_e1 = (-1.0 * dot_product(n_xy_e1, Vector2d(v1[0], v1[1]))) + std::max(0.0, n_xy_e1[0]) + std::max(0.0, n_xy_e1[1]);
    double d_xy_e2 = (-1.0 * dot_product(n_xy_e2, Vector2d(v2[0], v2[1]))) + std::max(0.0, n_xy_e2[0]) + std::max(0.0, n_xy_e2[1]);

    // YZ plane
    Vector2d n_yz_e0(-1.0 * e0[2], e0[1]);
    Vector2d n_yz_e1(-1.0 * e1[2], e1[1]);
    Vector2d n_yz_e2(-1.0 * e2[2], e2[1]);
    if (n[0] < 0.0)
      {
      n_yz_e0 = -n_yz_e0;
      n_yz_e1 = -n_yz_e1;
      n_yz_e2 = -n_yz_e2;
      }
    double d_yz_e0 = (-1.0 * dot_product(n_yz_e0, Vector2d(v0[1], v0[2]))) + std::max(0.0, n_yz_e0[0]) + std::max(0.0, n_yz_e0[1]);
    double d_yz_e1 = (-1.0 * dot_product(n_yz_e1, Vector2d(v1[1], v1[2]))) + std::max(0.0, n_yz_e1[0]) + std::max(0.0, n_yz_e1[1]);
    double d_yz_e2 = (-1.0 * dot_product(n_yz_e2, Vector2d(v2[1], v2[2]))) + std::max(0.0, n_yz_e2[0]) + std::max(0.0, n_yz_e2[1]);

    // ZX plane
    Vector2d n_zx_e0(-1.0 * e0[0], e0[2]);
    Vector2d n_zx_e1(-1.0 * e1[0], e1[2]);
    Vector2d n_zx_e2(-1.0 * e2[0], e2[2]);
    if (n[1] < 0.0)
      {
      n_zx_e0 = -n_zx_e0;
      n_zx_e1 = -n_zx_e1;
      n_zx_e2 = -n_zx_e2;
      }
    double d_xz_e0 = (-1.0 * dot_product(n_zx_e0, Vector2d(v0[2], v0[0]))) + std::max(0.0, n_zx_e0[0]) + std::max(0.0, n_zx_e0[1]);
    double d_xz_e1 = (-1.0 * dot_product(n_zx_e1, Vector2d(v1[2], v1[0]))) + std::max(0.0, n_zx_e1[0]) + std::max(0.0, n_zx_e1[1]);
    double d_xz_e2 = (-1.0 * dot_product(n_zx_e2, Vector2d(v2[2], v2[0]))) + std::max(0.0, n_zx_e2[0]) + std::max(0.0, n_zx_e2[1]);

    // test possible grid boxes for overlap
    for (int z = t_bbox_grid_min[2]; z <= t_bbox_grid_max[2]; z++)
      {
      unsigned int offset_z = z * image->GetOffsetTable()[2];
      for (int y = t_bbox_grid_min[1]; y <= t_bbox_grid_max[1]; y++)
        {
        unsigned int offset_y = offset_z + y * image->GetOffsetTable()[1];
        for (int x = t_bbox_grid_min[0]; x <= t_bbox_grid_max[0]; x++)
          {
          unsigned int offset_x = offset_y + x * image->GetOffsetTable()[0];
          debug_n_voxels_tested++;

          // TRIANGLE PLANE THROUGH BOX TEST
          Vector3d p(x,y,z);
          double nDOTp = dot_product(n, p);
          if (((nDOTp + d1) * (nDOTp + d2)) > 0.0) { continue; }

          // PROJECTION TESTS
          // XY
          Vector2d p_xy(p[0], p[1]);
          if ((dot_product(n_xy_e0, p_xy) + d_xy_e0) < 0.0) { continue; }
          if ((dot_product(n_xy_e1, p_xy) + d_xy_e1) < 0.0) { continue; }
          if ((dot_product(n_xy_e2, p_xy) + d_xy_e2) < 0.0) { continue; }

          // YZ
          Vector2d p_yz(p[1], p[2]);
          if ((dot_product(n_yz_e0, p_yz) + d_yz_e0) < 0.0) { continue; }
          if ((dot_product(n_yz_e1, p_yz) + d_yz_e1) < 0.0) { continue; }
          if ((dot_product(n_yz_e2, p_yz) + d_yz_e2) < 0.0) { continue; }

          // XZ
          Vector2d p_zx(p[2], p[0]);
          if ((dot_product(n_zx_e0, p_zx) + d_xz_e0) < 0.0) { continue; }
          if ((dot_product(n_zx_e1, p_zx) + d_xz_e1) < 0.0) { continue; }
          if ((dot_product(n_zx_e2, p_zx) + d_xz_e2) < 0.0) { continue; }

          // Got to this point - voxel will be marked
          debug_n_voxels_marked += 1;

          // Mark the voxel
          fn_update(image, offset_x);
        }
      }
    }

    }
}

} // namespace

#ifdef deadcode


      // test possible grid boxes for overlap
      for (int z = t_bbox_grid.min.z; z <= t_bbox_grid.max.z; z++) {
        for (int y = t_bbox_grid.min.y; y <= t_bbox_grid.max.y; y++) {
          for (int x = t_bbox_grid.min.x; x <= t_bbox_grid.max.x; x++) {
            // size_t location = x + (y*info.gridsize) + (z*info.gridsize*info.gridsize);
            // if (checkBit(voxel_table, location)){ continue; }
#ifdef _DEBUG
            debug_n_voxels_tested++;
#endif

            // TRIANGLE PLANE THROUGH BOX TEST
            glm::vec3 p(x * info.unit.x, y * info.unit.y, z * info.unit.z);
            float nDOTp = glm::dot(n, p);
            if (((nDOTp + d1) * (nDOTp + d2)) > 0.0f) { continue; }

            // PROJECTION TESTS
            // XY
            glm::vec2 p_xy(p.x, p.y);
            if ((glm::dot(n_xy_e0, p_xy) + d_xy_e0) < 0.0f) { continue; }
            if ((glm::dot(n_xy_e1, p_xy) + d_xy_e1) < 0.0f) { continue; }
            if ((glm::dot(n_xy_e2, p_xy) + d_xy_e2) < 0.0f) { continue; }

            // YZ
            glm::vec2 p_yz(p.y, p.z);
            if ((glm::dot(n_yz_e0, p_yz) + d_yz_e0) < 0.0f) { continue; }
            if ((glm::dot(n_yz_e1, p_yz) + d_yz_e1) < 0.0f) { continue; }
            if ((glm::dot(n_yz_e2, p_yz) + d_yz_e2) < 0.0f) { continue; }

            // XZ
            glm::vec2 p_zx(p.z, p.x);
            if ((glm::dot(n_zx_e0, p_zx) + d_xz_e0) < 0.0f) { continue; }
            if ((glm::dot(n_zx_e1, p_zx) + d_xz_e1) < 0.0f) { continue; }
            if ((glm::dot(n_zx_e2, p_zx) + d_xz_e2) < 0.0f) { continue; }
#ifdef _DEBUG
            debug_n_voxels_marked += 1;
#endif
            if (morton_order) {
              size_t location = mortonEncode_LUT(x, y, z);
              setBit(voxel_table, location);
            }
            else {
              size_t location = static_cast<size_t>(x) + (static_cast<size_t>(y)* static_cast<size_t>(info.gridsize.y)) + (static_cast<size_t>(z)* static_cast<size_t>(info.gridsize.y)* static_cast<size_t>(info.gridsize.z));
              //std:: cout << "Voxel found at " << x << " " << y << " " << z << std::endl;
              setBit(voxel_table, location);
            }
            continue;
          }
        }
      }
    }
    cpu_voxelization_timer.stop(); fprintf(stdout, "[Perf] CPU voxelization time: %.1f ms \n", cpu_voxelization_timer.elapsed_time_milliseconds);
#ifdef _DEBUG
    printf("[Debug] Processed %llu triangles on the CPU \n", debug_n_triangles);
    printf("[Debug] Tested %llu voxels for overlap on CPU \n", debug_n_voxels_tested);
    printf("[Debug] Marked %llu voxels as filled (includes duplicates!) on CPU \n", debug_n_voxels_marked);
#endif
  }

  // use Xor for voxels whose corresponding bits have to flipped
  void setBitXor(unsigned int* voxel_table, size_t index) {
    size_t int_location = index / size_t(32);
    unsigned int bit_pos = size_t(31) - (index % size_t(32)); // we count bit positions RtL, but array indices LtR
    unsigned int mask = 1 << bit_pos;
    #pragma omp critical
    {
      voxel_table[int_location] = (voxel_table[int_location] ^ mask);
    }
  }

  bool TopLeftEdge(glm::vec2 v0, glm::vec2 v1) {
    return ((v1.y < v0.y) || (v1.y == v0.y && v0.x > v1.x));
  }

  //check the triangle is counterclockwise or not
  bool checkCCW(glm::vec2 v0, glm::vec2 v1, glm::vec2 v2) {
    glm::vec2 e0 = v1 - v0;
    glm::vec2 e1 = v2 - v0;
    float result = e0.x * e1.y - e1.x * e0.y;
    if (result > 0)
      return true;
    else
      return false;
  }

  //find the x coordinate of the voxel
  float get_x_coordinate(glm::vec3 n, glm::vec3 v0, glm::vec2 point) {
    return (-(n.y * (point.x - v0.y) + n.z * (point.y - v0.z)) / n.x + v0.x);
  }


  //check the location with point and triangle
  int check_point_triangle(glm::vec2 v0, glm::vec2 v1, glm::vec2 v2, glm::vec2 point) {
    glm::vec2 PA = point - v0;
    glm::vec2 PB = point - v1;
    glm::vec2 PC = point - v2;

    float t1 = PA.x * PB.y - PA.y * PB.x;
    if (std::fabs(t1) < float_error && PA.x * PB.x <= 0 && PA.y * PB.y <= 0)
      return 1;

    float t2 = PB.x * PC.y - PB.y * PC.x;
    if (std::fabs(t2) < float_error && PB.x * PC.x <= 0 && PB.y * PC.y <= 0)
      return 2;

    float t3 = PC.x * PA.y - PC.y * PA.x;
    if (std::fabs(t3) < float_error && PC.x * PA.x <= 0 && PC.y * PA.y <= 0)
      return 3;

    if (t1 * t2 > 0 && t1 * t3 > 0)
      return 0;
    else
      return -1;
  }

  // Mesh voxelization method
  void cpu_voxelize_mesh_solid(voxinfo info, trimesh::TriMesh* themesh, unsigned int* voxel_table, bool morton_order) {
    Timer cpu_voxelization_timer; cpu_voxelization_timer.start();

    // PREPASS
    // Move all vertices to origin (can be done in parallel)
    trimesh::vec3 move_min = glm_to_trimesh<trimesh::vec3>(info.bbox.min);
#pragma omp parallel for
    for (int64_t i = 0; i < themesh->vertices.size(); i++) {
      if (i == 0) { printf("[Info] Using %d threads \n", omp_get_num_threads()); }
      themesh->vertices[i] = themesh->vertices[i] - move_min;
    }

#pragma omp parallel for
    for (int64_t i = 0; i < info.n_triangles; i++) {
      glm::vec3 v0 = trimesh_to_glm<trimesh::point>(themesh->vertices[themesh->faces[i][0]]);
      glm::vec3 v1 = trimesh_to_glm<trimesh::point>(themesh->vertices[themesh->faces[i][1]]);
      glm::vec3 v2 = trimesh_to_glm<trimesh::point>(themesh->vertices[themesh->faces[i][2]]);

      // Edge vectors
      glm::vec3 e0 = v1 - v0;
      glm::vec3 e1 = v2 - v1;
      glm::vec3 e2 = v0 - v2;
      // Normal vector pointing up from the triangle
      glm::vec3 n = glm::normalize(glm::cross(e0, e1));
      if (std::fabs(n.x) < float_error) {
        continue;
      }

      //Calculate the projection of three point into yoz plane
      glm::vec2 v0_yz = glm::vec2(v0.y, v0.z);
      glm::vec2 v1_yz = glm::vec2(v1.y, v1.z);
      glm::vec2 v2_yz = glm::vec2(v2.y, v2.z);

      //set the triangle counterclockwise
      if (!checkCCW(v0_yz, v1_yz, v2_yz))
      {
        glm::vec2 v3 = v1_yz;
        v1_yz = v2_yz;
        v2_yz = v3;
      }

      // COMPUTE TRIANGLE BBOX IN GRID
      // Triangle bounding box in world coordinates is min(v0,v1,v2) and max(v0,v1,v2)
      glm::vec2 bbox_max = glm::max(v0_yz, glm::max(v1_yz, v2_yz));
      glm::vec2 bbox_min = glm::min(v0_yz, glm::min(v1_yz, v2_yz));

      glm::vec2 bbox_max_grid = glm::vec2(floor(bbox_max.x / info.unit.y - 0.5), floor(bbox_max.y / info.unit.z - 0.5));
      glm::vec2 bbox_min_grid = glm::vec2(ceil(bbox_min.x / info.unit.y - 0.5), ceil(bbox_min.y / info.unit.z - 0.5));

      for (int y = bbox_min_grid.x; y <= bbox_max_grid.x; y++)
      {
        for (int z = bbox_min_grid.y; z <= bbox_max_grid.y; z++)
        {
          glm::vec2 point = glm::vec2((y + 0.5) * info.unit.y, (z + 0.5) * info.unit.z);
          int checknum = check_point_triangle(v0_yz, v1_yz, v2_yz, point);
          if ((checknum == 1 && TopLeftEdge(v0_yz, v1_yz)) || (checknum == 2 && TopLeftEdge(v1_yz, v2_yz)) || (checknum == 3 && TopLeftEdge(v2_yz, v0_yz)) || (checknum == 0))
          {
            unsigned int xmax = int(get_x_coordinate(n, v0, point) / info.unit.x - 0.5);
            for (int x = 0; x <= xmax; x++)
            {
              if (morton_order) {
                size_t location = mortonEncode_LUT(x, y, z);
                setBitXor(voxel_table, location);
              }
              else {
                size_t location = static_cast<size_t>(x) + (static_cast<size_t>(y) * static_cast<size_t>(info.gridsize.y)) + (static_cast<size_t>(z) * static_cast<size_t>(info.gridsize.y) * static_cast<size_t>(info.gridsize.z));
                setBitXor(voxel_table, location);
              }
              continue;
            }
          }
        }
      }

    }
    cpu_voxelization_timer.stop(); fprintf(stdout, "[Perf] CPU voxelization time: %.1f ms \n", cpu_voxelization_timer.elapsed_time_milliseconds);
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

#endif // deadcode

#endif // not used

#endif
