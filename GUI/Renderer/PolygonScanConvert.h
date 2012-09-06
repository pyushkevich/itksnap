/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: PolygonScanConvert.h,v $
  Language:  C++
  Date:      $Date: 2009/01/23 20:09:38 $
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
#ifndef __PolygonScanConvert_h_
#define __PolygonScanConvert_h_

#include "SNAPOpenGL.h"
#include "itkImage.h"

class PolygonScanConvertBase
{
public:
  // Private version of the method, takes a flattened array of coordinates
  // and is not templated
  static void RasterizeFilled(
    double *vArray, unsigned int nVertices, 
    unsigned int width, unsigned int height, 
    GLenum glType, void *buffer);

  // Callbacks for tesselation
  static void ErrorCallback(GLenum errorCode);
  static void CombineCallback(
    GLdouble coords[3], GLdouble **vertex_data,  
    GLfloat *weight, GLdouble **dataOut);
};

template<class TPixel, GLenum VGlPixelType, class TVertexIterator>
class PolygonScanConvert : public PolygonScanConvertBase
{
public:
  typedef itk::Image<TPixel, 2> ImageType;

  /** 
   * This method uses OpenGL to scan-convert a polygonal curve. The input is a pair 
   * of iterators (begin, end) to a list/array of double arrays or vectors, i.e., 
   * objects for which indices [0] and [1] are supported.
   */
  static void RasterizeFilled(
    TVertexIterator first, unsigned int n, ImageType *image)
    {
    double *vArray = new double[3 * (n + 1)], *vPointer = vArray;
    for (unsigned int i = 0; i < n; ++i, ++first)
      {
      *vPointer++ = (double) (*first)[0];
      *vPointer++ = (double) (*first)[1];
      *vPointer++ = 0.0;
      }
  
    // Set up the image properties
    unsigned int width  = image->GetBufferedRegion().GetSize()[0];
    unsigned int height = image->GetBufferedRegion().GetSize()[1];
    PolygonScanConvertBase::RasterizeFilled(
      vArray, n, width, height, VGlPixelType, image->GetBufferPointer());
    delete vArray;
    }

private:


};

#endif

