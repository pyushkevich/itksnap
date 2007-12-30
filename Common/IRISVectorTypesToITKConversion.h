/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IRISVectorTypesToITKConversion.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:12 $
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
