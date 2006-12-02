/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: IRISVectorTypes.txx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:09 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/

// Convert vector to integer vector
template <class T, unsigned int VSize>
inline iris_vector_fixed<int,VSize> 
to_int(const vnl_vector_fixed<T,VSize> &x)
{
  iris_vector_fixed<int,VSize> z;

  for(unsigned int i=0;i<VSize;i++)
    z(i) = static_cast<int>(x(i));

  return z;
}

// Convert vector to unsigned integer vector
template <class T, unsigned int VSize>
inline iris_vector_fixed<unsigned int,VSize> 
to_unsigned_int(const vnl_vector_fixed<T,VSize> &x)
{
  iris_vector_fixed<unsigned int,VSize> z;

  for(unsigned int i=0;i<VSize;i++)
    z(i) = static_cast<unsigned int>(x(i));

  return z;
}

// Convert vector to long vector
template <class T, unsigned int VSize>
inline iris_vector_fixed<long,VSize> 
to_long(const vnl_vector_fixed<T,VSize> &x)
{
  iris_vector_fixed<long,VSize> z;

  for(unsigned int i=0;i<VSize;i++)
    z(i) = static_cast<long>(x(i));

  return z;
}

// Convert vector to unsigned long vector
template <class T, unsigned int VSize>
inline iris_vector_fixed<unsigned long,VSize> 
to_unsigned_long(const vnl_vector_fixed<T,VSize> &x)
{
  iris_vector_fixed<unsigned long,VSize> z;

  for(unsigned int i=0;i<VSize;i++)
    z(i) = static_cast<unsigned long>(x(i));

  return z;
}

// Convert vector to float vector
template <class T, unsigned int VSize>
inline iris_vector_fixed<float,VSize> 
to_float(const vnl_vector_fixed<T,VSize> &x)
{
  iris_vector_fixed<float,VSize> z;

  for(unsigned int i=0;i<VSize;i++)
    z(i) = static_cast<float>(x(i));

  return z;
}
// Convert vector to double vector
template <class T, unsigned int VSize>
inline iris_vector_fixed<double,VSize> 
to_double(const vnl_vector_fixed<T,VSize> &x)
{
  iris_vector_fixed<double,VSize> z;

  for(unsigned int i=0;i<VSize;i++)
    z(i) = static_cast<double>(x(i));

  return z;
}

/**
 * Given two vectors, get a vector that contains the smaller elements of the
 * two.  Used for bounding box computations
 */
template <class T, unsigned int VSize> 
inline iris_vector_fixed<T,VSize> 
vector_min(const vnl_vector_fixed<T,VSize> &x,
           const vnl_vector_fixed<T,VSize> &y)
{
  iris_vector_fixed<T,VSize> z;

  for(unsigned int i=0;i<VSize;i++)
    z(i) = x(i) < y(i) ? x(i) : y(i);

  return z;
}

/**
 * Given two vectors, get a vector that contains the larger elements of the
 * two.  Used for bounding box computations
 */
template <class T, unsigned int VSize> 
inline iris_vector_fixed<T,VSize> 
vector_max(const vnl_vector_fixed<T,VSize> &x,
           const vnl_vector_fixed<T,VSize> &y)
{
  iris_vector_fixed<T,VSize> z;

  for(unsigned int i=0;i<VSize;i++)
    z(i) = x(i) > y(i) ? x(i) : y(i);

  return z;
}

/**
 * Multiply the corresponding elements of two vectors and return
 * a vector containing the results 
 */
template <class T, unsigned int VSize> 
inline iris_vector_fixed<T,VSize> 
vector_multiply(const vnl_vector_fixed<T,VSize> &x,
                const vnl_vector_fixed<T,VSize> &y)
{
  iris_vector_fixed<T,VSize> z;

  for(unsigned int i=0;i<VSize;i++)
    z(i) = x(i) * y(i);

  return z;
}

/**
 * Multiply two inhomogeneous vectors, returning the result in the
 * type of the first vector
 */
template <class T1, class T2, unsigned int VSize> 
inline iris_vector_fixed<T1,VSize> 
vector_multiply_mixed(const vnl_vector_fixed<T1,VSize> &x,
                      const vnl_vector_fixed<T2,VSize> &y)
{
  iris_vector_fixed<T1,VSize> z;
  for(unsigned int i=0;i<VSize;i++)
    z(i) = x(i) * y(i);
  return z;
}

/**
 * Compute x*y+z for three inhomogeneous vectors, returning result in the 
 * same type as x
 */
template <class T1, class T2, class T3, unsigned int VSize> 
inline iris_vector_fixed<T1,VSize> 
vector_multiply_add_mixed(const vnl_vector_fixed<T1,VSize> &x,
                          const vnl_vector_fixed<T2,VSize> &y,
                          const vnl_vector_fixed<T3,VSize> &z)
{
  iris_vector_fixed<T1,VSize> r;
  for(unsigned int i=0;i<VSize;i++)
    r(i) = x(i) * y(i) + z(i);
  return r; 
}
