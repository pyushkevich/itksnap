/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: PreprocessingImageIOWizardLogic.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __PreprocessingImageIOWizardLogic_h_
#define __PreprocessingImageIOWizardLogic_h_

#include "SNAPCommonUI.h"
#include "RestrictedImageIOWizardLogic.h"

/**
 * \class PreprocessingImageIOWizardLogic
 * \brief A concrete instantiation of a wizard for loading and saving 
 * grey images
 */
class PreprocessingImageIOWizardLogic : 
  public RestrictedImageIOWizardLogic<float> 
{
public:
  typedef RestrictedImageIOWizardLogic<float> Superclass;
  typedef Superclass::FileFormat FileFormat;

  /** Not all file types support saving and loading floating 
   * point images. This method returns the allowed types */
  virtual bool CanLoadFileFormat(FileFormat type) const
  {
    return Superclass::CanLoadFileFormat(type) && (
      type == GuidedImageIOBase::FORMAT_MHA || 
      type == GuidedImageIOBase::FORMAT_ANALYZE || 
      type == GuidedImageIOBase::FORMAT_RAW);
  }

  /** Not all file types support saving and loading floating 
   * point images. This method returns the allowed types */
  virtual bool CanSaveFileFormat(FileFormat type) const
  {
    return Superclass::CanSaveFileFormat(type) && CanLoadFileFormat(type);
  }

};

#endif

