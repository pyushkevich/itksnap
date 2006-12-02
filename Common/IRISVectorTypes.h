/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: IRISVectorTypes.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:09 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __IRISVectorTypes_h_
#define __IRISVectorTypes_h_

// For the little vector operations
#include <vnl/vnl_vector_fixed.h>

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

  // Construct an uninitialized n-vector
  iris_vector_fixed() : Parent() {}

  // Copy constructor
  iris_vector_fixed(const Parent& rhs ) : Parent(rhs) {}

  // Construct an fixed-n-vector copy of rhs.
  iris_vector_fixed( const vnl_vector<T>& rhs )  : Parent(rhs) {}

  // Constructs n-vector with elements initialised to \a v
  explicit iris_vector_fixed(const T& v) : Parent(v) {}

  // Construct an fixed-n-vector initialized from \a datablck
  //  The data *must* have enough data. No checks performed.
  explicit iris_vector_fixed(const T* data) : Parent(data) {}

  // Convenience constructor for 2-D vectors
  iris_vector_fixed(const T& x0,const T& x1) : Parent(x0,x1) {}

  // Convenience constructor for 3-D vectors
  iris_vector_fixed(const T& x0,const T& x1,const T& x2) : Parent(x0,x1,x2) {}

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

// Common 3D vector types
typedef iris_vector_fixed<int,3> Vector3i;
typedef iris_vector_fixed<unsigned int,3> Vector3ui;
typedef iris_vector_fixed<long,3> Vector3l;
typedef iris_vector_fixed<unsigned long,3> Vector3ul;
typedef iris_vector_fixed<float,3> Vector3f;
typedef iris_vector_fixed<double,3> Vector3d;

#ifndef ITK_MANUAL_INSTANTIATION
#include "IRISVectorTypes.txx"
#endif

#endif // __IRISVectorTypes_h_
