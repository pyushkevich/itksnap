/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ImageCoordinateTransform.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:13 $
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
#ifndef __ImageCoordinateTransform_h_
#define __ImageCoordinateTransform_h_

#include "SNAPCommon.h"
#include <vnl/vnl_matrix_fixed.h>
#include "itkObjectFactory.h"
#include "itkSmartPointer.h"

/**
 * \class ImageCoordinateTransform
 *
 * This transform captures the coordinate renaming and change of sense
 * that occurs between the image coordinates, the anatomical coordinates 
 * and the image coordinates.
 *
 * This transform can be visualized as a coordinate frame attached to any
 * of the eight corners of an image volume.  This transform is a combination
 * of rotations by angles dividible by 90 degrees, multiplucations by -1 or 1
 * and translation by the image size
 *
 * In order to define one of these transforms, we need to pass in a coordinate 
 * mapping vector (example: (-1,3,-2) means x maps to -x, y to z and z to -y),
 * as well as the size of the image volume.
 */
class ImageCoordinateTransform : public itk::Object
{
public:

  /** Standard class typedefs. */
  typedef ImageCoordinateTransform       Self;
  typedef itk::Object                    Superclass;
  typedef itk::SmartPointer<Self>        Pointer;
  typedef itk::SmartPointer<const Self>  ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(ImageCoordinateTransform, itk::Object)

  /** define the New method */
  itkNewMacro(Self)

  /**
   * Initialize the transform with new singed coordinate mappings 
   * (1-based signed indices)
   */
  void SetTransform(const Vector3i &map, const Vector3ui &size);

  /**
   * Set to an existing transform
   */
  void SetTransform(const Self *other);

  /** Compute the inverse of this transform */
  void ComputeInverse(Self *result) const;

  /** Multiply by another transform */
  void ComputeProduct(const Self *t1, Self *result) const;
                                                                      
  /** Apply transform to a vector */
  Vector3d TransformVector(const Vector3d &x) const;

  /** Apply transform to a point. */
  Vector3d TransformPoint(const Vector3d &x) const;

  /** Apply to an integer voxel index */
  Vector3ui TransformVoxelIndex(const Vector3ui &xVoxel) const;

  /** Apply to a size vector */
  Vector3ui TransformSize(const Vector3ui &xSize) const;

  /** Get the index of a particular coordinate */
  unsigned int GetCoordinateIndexZeroBased(unsigned int c) const
  {
    return m_AxesIndex[c];
  }

  /** Get the orientation of a particular coordinate (returns 1 or -1) */
  int GetCoordinateOrientation(unsigned int c) const
  {
    return m_AxesDirection[c];
  }

protected:

  /** Default constructor creates an identity transform */
  ImageCoordinateTransform();
  ~ImageCoordinateTransform() {}

  typedef vnl_matrix_fixed<double,3,3> MatrixType;

  // A transform matrix
  MatrixType m_Transform;
  
  // An offset vector
  Vector3d m_Offset;

  // The operation abs(i) - 1 applied to m_Mapping
  Vector3i m_AxesIndex;

  // The operation sign(i) applied to m_Mapping
  Vector3i m_AxesDirection;

  // Compute the internal vectors once the matrix and offset have been computed
  void ComputeSecondaryVectors();
};

#endif
