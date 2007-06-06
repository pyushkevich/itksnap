/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: RGBImageIOWizardLogic.h,v $
  Language:  C++
  Date:      $Date: 2007/06/06 22:27:22 $
  Version:   $Revision: 1.1 $
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
  public ImageIOWizardLogic<RGBType> {};

class RestrictedRGBImageIOWizardLogic : 
  public RestrictedImageIOWizardLogic<RGBType> {};

#endif

