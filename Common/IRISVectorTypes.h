/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IRISVectorTypes.h,v $
  Language:  C++
  Date:      $Date: 2008/11/01 11:32:00 $
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

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#ifndef __IRISVectorTypes_h_
#define __IRISVectorTypes_h_

// For the little vector operations
#include <vnl/vnl_vector_fixed.h>
#include <vnl/vnl_matrix_fixed.h>
#include <itkSize.h>
#include <itkIndex.h>

/**
 * \class iris_vector_fixed
 * \brief  An extension of the VNL vector with some special trivial 
 * extra functionality.
 */
template<class T, int VSize>
class iris_vector_fixed : public vnl_vector_fixed<T,VSize> {
public:
  typedef iris_vector_fixed<T,VSize> Self;
  typedef vnl_vector_fixed<T,VSize> Parent;

  typedef iris_vector_fixed<int,VSize> IntegerVectorType;
  typedef iris_vector_fixed<float,VSize> FloatVectorType;
  typedef iris_vector_fixed<double,VSize> DoubleVectorType;

  typedef itk::Size<VSize> ITKSizeType;
  typedef itk::Index<VSize> ITKIndexType;
  typedef itk::FixedArray<T, VSize> ITKFixedArray;

  // Initialize the n-vector to zeros
  iris_vector_fixed() : Parent()
  {
    this->fill(0);
  }

  // Copy constructor
  iris_vector_fixed(const Parent& rhs ) : Parent(rhs) {}

  // Construct an fixed-n-vector copy of rhs.
  iris_vector_fixed( const vnl_vector<T>& rhs )  : Parent(rhs) {}

  // Constructs n-vector with elements initialised to \a v
  explicit iris_vector_fixed(const T& v) : Parent(v) {}

  // Construct an fixed-n-vector initialized from \a datablck
  //  The data *must* have enough data. No checks performed.
  // explicit iris_vector_fixed(const T* data) : Parent(data) {}

  // Convenience constructor for 2-D vectors
  iris_vector_fixed(const T& x0,const T& x1) : Parent(x0,x1) {}

  // Convenience constructor for 3-D vectors
  iris_vector_fixed(const T& x0,const T& x1,const T& x2) : Parent(x0,x1,x2) {}

  // Initialize with an itk::Size
  iris_vector_fixed(const ITKSizeType &size)
  {
    for(int i = 0; i < VSize; i++)
      (*this)[i] = static_cast<T>(size[i]);
  }

  // Initialize with an itk::Index
  iris_vector_fixed(const ITKIndexType &idx)
  {
    for(int i = 0; i < VSize; i++)
      (*this)[i] = static_cast<T>(idx[i]);
  }

  // Initialize with an itk::FixedArray
  iris_vector_fixed(const ITKFixedArray &arr)
  {
    this->copy_in(arr.GetDataPointer());
  }

  // Assignment operator that takes an int
  Self &operator = (int value)
  { this->fill((T) value); return *this; }

  /**
   * Clamp the vector between a pair of vectors (the elements of this vector
   * that are smaller than the corresponding elements of lower are set to lower, 
   * and the same is done for upper).
   */
  Self clamp(const Self &lower, const Self &upper) 
  {
    Self y;
    for(unsigned int i=0;i<VSize;i++)
    {
      T a = this->get(i), l = lower(i), u = upper(i);
      assert(l <= u);      
      y(i) = a < l ? l : (a > u ? u : a);
    }
    return y;
  }
};

// Common 2D vector types
typedef iris_vector_fixed<int,2> Vector2i;
typedef iris_vector_fixed<unsigned int,2> Vector2ui;
typedef iris_vector_fixed<long,2> Vector2l;
typedef iris_vector_fixed<unsigned long,2> Vector2ul;
typedef iris_vector_fixed<float,2> Vector2f;
typedef iris_vector_fixed<double,2> Vector2d;
typedef iris_vector_fixed<bool,2> Vector2b;

// Common 3D vector types
typedef iris_vector_fixed<int,3> Vector3i;
typedef iris_vector_fixed<unsigned int,3> Vector3ui;
typedef iris_vector_fixed<long,3> Vector3l;
typedef iris_vector_fixed<unsigned long,3> Vector3ul;
typedef iris_vector_fixed<float,3> Vector3f;
typedef iris_vector_fixed<double,3> Vector3d;
typedef iris_vector_fixed<bool,3> Vector3b;

// A matrix definition
typedef vnl_matrix_fixed<double,3,3> Matrix3d;
typedef vnl_matrix_fixed<double,4,4> Matrix4d;

// An equivalent to MATLAB's linspace command
template<class T>
inline vnl_vector<T> linspace(T x0, T x1, unsigned int n)
{
  vnl_vector<T> v(n);
  double step = (x1 - x0) / (n - 1);
  for(int i = 0; i < n; i++)
    v[i] = (i == n-1) ? x1 : x0 + i * step;
  return v;
}

template<class T>
void
linspace(T *v, T x0, T x1, unsigned int n)
{
  double step = (x1 - x0) / (n - 1);
  for(int i = 0; i < n; i++)
    v[i] = (i == n-1) ? x1 : x0 + i * step;
}

#ifndef ITK_MANUAL_INSTANTIATION
#include "IRISVectorTypes.txx"
#endif

#endif // __IRISVectorTypes_h_
