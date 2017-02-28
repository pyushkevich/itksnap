/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ImageCoordinateTransform.cxx,v $
  Language:  C++
  Date:      $Date: 2008/04/15 21:42:29 $
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
#include "ImageCoordinateTransform.h"
#include "vnl/vnl_inverse.h"

#include <cmath>
#include <cstdlib>
#include <IRISVectorTypes.h>

Vector3ui IRISSNAPdummyVector;
iris_vector_fixed<double,3> irissnapdummyvector
  = to_double<unsigned int,3>(IRISSNAPdummyVector);


ImageCoordinateTransform
::ImageCoordinateTransform()
{
  // Transform is the identity
  m_Transform.fill(0.0f);
  m_Transform(0,0) = m_Transform(1,1) = m_Transform(2,2) = 1.0f;

  // The offset is zero
  m_Offset.fill(0.0f);

  ComputeSecondaryVectors();  
}

void 
ImageCoordinateTransform
::SetTransform(const Vector3i &map, const Vector3ui &size)
{
  unsigned int i;

  // Make sure it's a legal mapping
  for(i=0;i<3;i++)
    {
    assert(abs(map[i]) <= 3 && abs(map[i]) > 0);
    assert(abs(map[i]) != abs(map[(i+1) % 3]));
    }
  
  // Initialize the transform matrix
  m_Transform.fill(0);
  for(i=0;i<3;i++)
    {
    if(map[i] > 0)
      m_Transform(map[i]-1,i) = 1.0;
    else
      m_Transform(-1-map[i],i) = -1.0;
    }

  // Initialize the offset vector
  m_Offset = m_Transform * to_double(size);

  for(i=0;i<3;i++)
    {
    // Update the offset vector to make it right
    m_Offset[i] = m_Offset[i] < 0 ? - m_Offset[i] : 0;
    }

  ComputeSecondaryVectors();
}

void
ImageCoordinateTransform
::SetTransform(const ImageCoordinateTransform::Self *other)
{
  // A transform matrix
  m_Transform = other->m_Transform;

  // An offset vector
  m_Offset = other->m_Offset;

  // The operation abs(i) - 1 applied to m_Mapping
  m_AxesIndex = other->m_AxesIndex;

  // The operation sign(i) applied to m_Mapping
  m_AxesDirection = other->m_AxesDirection;
}

void
ImageCoordinateTransform
::ComputeSecondaryVectors()
{
  // For this calculation we need the transpose of the matrix
  MatrixType T = m_Transform.transpose();

  Vector3d map = T * Vector3d(0.0, 1.0, 2.0);
  m_AxesIndex[0] = (unsigned int) fabs(map[0]);
  m_AxesIndex[1] = (unsigned int) fabs(map[1]);
  m_AxesIndex[2] = (unsigned int) fabs(map[2]);

  m_AxesDirection = to_int(T * Vector3d(1.0));
}



void
ImageCoordinateTransform
::ComputeInverse(Self *result) const
{
  // Compute the new transform's details
  result->m_Transform = vnl_inverse(m_Transform);
  result->m_Offset = - result->m_Transform * m_Offset;

  // Compute additional quantities
  result->ComputeSecondaryVectors();
}

void ImageCoordinateTransform::ComputeProduct(const Self *t1, Self *result) const
{
  // Compute the new transform's details
  result->m_Transform = m_Transform * t1->m_Transform;
  result->m_Offset = m_Transform * t1->m_Offset + m_Offset;

  // Compute additional quantities
  result->ComputeSecondaryVectors();
}

Vector3d ImageCoordinateTransform::TransformPoint(const Vector3d &x) const
{
  return m_Transform * x + m_Offset;
}

Vector3d
ImageCoordinateTransform
::TransformVector(const Vector3d &x) const
{
  return m_Transform * x;
}

Vector3ui
ImageCoordinateTransform
::TransformSize(const Vector3ui &sz) const
{
  Vector3d szSigned = m_Transform * to_double(sz);
  return Vector3ui(
    (unsigned int)fabs(szSigned(0)),
    (unsigned int)fabs(szSigned(1)),
    (unsigned int)fabs(szSigned(2)));
}

Vector3ui 
ImageCoordinateTransform
::TransformVoxelIndex(const Vector3ui &x) const 
{
  // Convert to voxel center coordinates
  Vector3d xVoxelCenter = to_double(x) + Vector3d(0.5);

  // Make sure the mapping is legit
  Vector3d xMappedCenter = TransformPoint(xVoxelCenter);
  assert(xMappedCenter(0) >= 0.0 &&
         xMappedCenter(1) >= 0.0 &&
         xMappedCenter(2) >= 0.0);

  // Compute the result and scale down to integers
  return to_unsigned_int(xMappedCenter);
}


