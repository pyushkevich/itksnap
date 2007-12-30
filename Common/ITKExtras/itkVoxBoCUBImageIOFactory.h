/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: itkVoxBoCUBImageIOFactory.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:12 $
  Version:   $Revision: 1.2 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  
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
#ifndef __itkVoxBoCUBImageIOFactory_h
#define __itkVoxBoCUBImageIOFactory_h

#include "itkObjectFactoryBase.h"
#include "itkImageIOBase.h"

namespace itk
{
/** \class VoxBoCUBImageIOFactory
 * \brief Create instances of VoxBoCUBImageIO objects using an object factory.
 */
class ITK_EXPORT VoxBoCUBImageIOFactory : public ObjectFactoryBase
{
public:  
  /** Standard class typedefs. */
  typedef VoxBoCUBImageIOFactory   Self;
  typedef ObjectFactoryBase  Superclass;
  typedef SmartPointer<Self>  Pointer;
  typedef SmartPointer<const Self>  ConstPointer;
  
  /** Class methods used to interface with the registered factories. */
  virtual const char* GetITKSourceVersion(void) const;
  virtual const char* GetDescription(void) const;
  
  /** Method for class instantiation. */
  itkFactorylessNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(VoxBoCUBImageIOFactory, ObjectFactoryBase);

  /** Register one factory of this type  */
  static void RegisterOneFactory(void)
  {
    VoxBoCUBImageIOFactory::Pointer VoxBoCUBFactory = VoxBoCUBImageIOFactory::New();
    ObjectFactoryBase::RegisterFactory(VoxBoCUBFactory);
  }

protected:
  VoxBoCUBImageIOFactory();
  ~VoxBoCUBImageIOFactory();

private:
  VoxBoCUBImageIOFactory(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

};
  
  
} // end namespace itk

#endif
