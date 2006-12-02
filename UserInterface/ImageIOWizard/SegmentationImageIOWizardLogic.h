/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: SegmentationImageIOWizardLogic.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __SegmentationImageIOWizardLogic_h_
#define __SegmentationImageIOWizardLogic_h_

#include "SNAPCommonUI.h"
#include "RestrictedImageIOWizardLogic.h"

/**
 * \class SegmentationImageIOWizardLogic
 * \brief A concrete instantiation of a wizard for loading and saving 
 * grey images
 */
class SegmentationImageIOWizardLogic : 
  public RestrictedImageIOWizardLogic<LabelType> {};

#endif

