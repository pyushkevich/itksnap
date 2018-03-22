/*=========================================================================

  Program:   ITK-SNAP
  Module:    AffineTransformWrapper.h
  Language:  C++
  Copyright (c) 2018 Paul A. Yushkevich

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
#ifndef __AffineTransformWrapper_h_
#define __AffineTransformWrapper_h_

#include <itkObject.h>
#include "SNAPCommon.h"

namespace itk {
  template <class TScalar, unsigned int V1, unsigned int V2> class Transform;
  template <class TScalar, unsigned int V1, unsigned int V2> class MatrixOffsetTransformBase;
  template <class TScalar, unsigned int V1, unsigned int V2> class Matrix;
  template <class TScalar, unsigned int V1> class Vector;
}

class Registry;

/**
 * @brief A convenience wrapper around ITK transforms
 *
 * This class supports mapping between different transform classes (ITK,VNL)
 * and saving/loading from files.
 */
class AffineTransformHelper
{
public:

  // Typedefs
  typedef itk::Transform<double, 3, 3>                        ITKTransformBase;
  typedef itk::MatrixOffsetTransformBase<double, 3, 3>        ITKTransformMOTB;
  typedef itk::Matrix<double, 3, 3>                                  ITKMatrix;
  typedef itk::Vector<double, 3>                                     ITKVector;
  typedef vnl_matrix_fixed<double, 4, 4>                                 Mat44;

  // Get the matrix and offset from a transform
  static void GetMatrixAndOffset(const ITKTransformBase *t, ITKMatrix &mat, ITKVector &off);

  // Check if the transform is identity
  static bool IsIdentity(const ITKTransformBase *t, double tol = 1e-5);

  // Save as an ITK transform file
  static void WriteAsITKTransform(const ITKTransformBase *t, const char *file);

  // Save as a Convert3D matrix
  static void WriteAsRASMatrix(const ITKTransformBase *t, const char *file);

  // Read from ITK transform file
  static SmartPtr<ITKTransformMOTB> ReadAsITKTransform(const char *file);

  // Read from ITK transform file
  static SmartPtr<ITKTransformMOTB> ReadAsRASMatrix(const char *file);

  // Read transform from registry
  static SmartPtr<ITKTransformBase> ReadFromRegistry(Registry *reg);

  // Write transform to registry
  static void WriteToRegistry(Registry *reg, const ITKTransformBase *t);

  // Get the RAS matrix
  static Mat44 GetRASMatrix(const ITKTransformBase *t);


protected:

  static SmartPtr<const ITKTransformMOTB> CastToMOTB(const ITKTransformBase *t);


};

#endif // __AffineTransformWrapper_h_
