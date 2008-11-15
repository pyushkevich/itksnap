/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: RGBImageIOWizardLogic.h,v $
  Language:  C++
  Date:      $Date: 2008/11/15 12:20:38 $
  Version:   $Revision: 1.2 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __RGBImageIOWizardLogic_h_
#define __RGBImageIOWizardLogic_h_

#include "SNAPCommonUI.h"
#include "RestrictedImageIOWizardLogic.h"

/**
 * \class RGBImageIOWizardLogic
 * \brief A concrete instantiation of a wizard for loading and saving 
 * RGB images
 */
class RGBImageIOWizardLogic : 
  public ImageIOWizardLogic<RGBType> 
{
  bool IsNativeFormatSupported() const
    { return false; };    
};

class RestrictedRGBImageIOWizardLogic : 
  public RestrictedImageIOWizardLogic<RGBType> 
{
  bool IsNativeFormatSupported() const
    { return false; };      
};

#endif

