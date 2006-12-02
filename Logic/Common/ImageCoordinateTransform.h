/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: ImageCoordinateTransform.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:11 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __ImageCoordinateTransform_h_
#define __ImageCoordinateTransform_h_

#include "SNAPCommon.h"
#include <vnl/vnl_matrix_fixed.h>

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
class ImageCoordinateTransform 
{
public:

  /** Default constructor creates an identity transform */
  ImageCoordinateTransform();
  
  /**
   * Initialize the transform with new singed coordinate mappings 
   * (1-based signed indices)
   */
  void SetTransform(const Vector3i &map, const Vector3ui &size);

  /** Compute the inverse of this transform */
  ImageCoordinateTransform Inverse() const;

  /** Multiply by another transform */
  ImageCoordinateTransform Product(const ImageCoordinateTransform &t1) const;
                                                                      
  /** Apply transform to a vector */
  Vector3f TransformVector(const Vector3f &x) const;

  /** Apply transform to a point. */
  Vector3f TransformPoint(const Vector3f &x) const;

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

private:
  typedef vnl_matrix_fixed<float,3,3> MatrixType;

  // A transform matrix
  MatrixType m_Transform;
  
  // An offset vector
  Vector3f m_Offset;

  // The operation abs(i) - 1 applied to m_Mapping
  Vector3i m_AxesIndex;

  // The operation sign(i) applied to m_Mapping
  Vector3i m_AxesDirection;

  // Compute the internal vectors once the matrix and offset have been computed
  void ComputeSecondaryVectors();
};

#endif
