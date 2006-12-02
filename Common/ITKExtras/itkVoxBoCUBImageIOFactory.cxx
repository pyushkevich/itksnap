/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkVoxBoCUBImageIOFactory.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:10 $
  Version:   $Revision: 1.1 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "itkVoxBoCUBImageIOFactory.h"
#include "itkCreateObjectFunction.h"
#include "itkVoxBoCUBImageIO.h"
#include "itkVersion.h"

  
namespace itk
{

VoxBoCUBImageIOFactory::VoxBoCUBImageIOFactory()
{
  this->RegisterOverride("itkImageIOBase",
                         "itkVoxBoCUBImageIO",
                         "VoxBo CUB Image IO",
                         1,
                         CreateObjectFunction<VoxBoCUBImageIO>::New());
}
  
VoxBoCUBImageIOFactory::~VoxBoCUBImageIOFactory()
{
}

const char* 
VoxBoCUBImageIOFactory::GetITKSourceVersion(void) const
{
  return ITK_SOURCE_VERSION;
}

const char* 
VoxBoCUBImageIOFactory::GetDescription() const
{
  return "VoxBo CUB ImageIO Factory, allows the loading of VoxBoCUB images into Insight";
}

} // end namespace itk

