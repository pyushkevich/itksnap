/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: IRISVectorTypesToITKConversion.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:09 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __IRISVectorTypesToITKConversion_h_
#define __IRISVectorTypesToITKConversion_h_

#include "IRISVectorTypes.h"

#include "itkSize.h"
#include "itkIndex.h"

/**
 * Convert a VectorNi to itk::Size
 */
template <class T, unsigned int VSize> 
inline itk::Size<VSize> 
to_itkSize(const vnl_vector_fixed<T,VSize> &x)
{
  itk::Size<VSize> z;

  for(unsigned int i=0;i<VSize;i++)
    z[i] = static_cast<unsigned long>(x(i));

  return z;
}

/**
 * Convert a VectorNi to itk::Size
 */
template <class T, unsigned int VSize> 
inline itk::Index<VSize> 
to_itkIndex(const vnl_vector_fixed<T,VSize> &x)
{
  itk::Index<VSize> z;

  for(unsigned int i=0;i<VSize;i++)
    z[i] = static_cast<unsigned long>(x(i));

  return z;
}

#endif // __IRISVectorTypesToITKConversion_h_
