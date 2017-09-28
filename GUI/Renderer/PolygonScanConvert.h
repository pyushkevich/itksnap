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
#include "itkImageRegionIteratorWithIndex.h"
#include <vtkPolygon.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>

template<class TImage, class TVertex, class TVertexIterator>
class PolygonScanConvert
{
public:
  static void RasterizeFilled(TVertexIterator first, unsigned int n, TImage *image)
  {
    // Create a vtkPolygol
    vtkSmartPointer<vtkPolygon> polygon = vtkPolygon::New();


    // typedef itk::PolyLineParametricPath<TImage::ImageDimension> InputPolylineType;
    // typedef typename InputPolylineType::VertexType VertexType;
    // SmartPtr<InputPolylineType> path = InputPolylineType::New();

    // Get the image region
    // itk::ImageRegion<2> region = image->GetBufferedRegion();

    // Add points to the path
    for (unsigned int i = 0; i < n; ++i, ++first)
      polygon->GetPoints()->InsertNextPoint((*first)[0], (*first)[1], 0.0);

    // Get the raw point data
    double *point_data =
        static_cast<double*>(polygon->GetPoints()->GetData()->GetVoidPointer(0));
    int np = polygon->GetPoints()->GetNumberOfPoints();

    // Get the normal and bounds
    double normal[3];
    polygon->ComputeNormal(np, point_data, normal);

    double bounds[6];
    polygon->GetPoints()->GetBounds(bounds);

    // Go through the image and test each point
    typedef itk::ImageRegionIteratorWithIndex<TImage> IteratorType;
    IteratorType it(image, image->GetBufferedRegion());
    for(; !it.IsAtEnd(); ++it)
      {
      double x[3];
      x[0] = it.GetIndex()[0] + 0.5;
      x[1] = it.GetIndex()[1] + 0.5;
      x[2] = 0.0;

      if(polygon->PointInPolygon(x, np, point_data, bounds, normal) == 1)
        it.Set(1);
      else
        it.Set(0);
      }
  }
};



#endif

