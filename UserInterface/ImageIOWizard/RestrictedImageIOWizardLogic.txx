/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: RestrictedImageIOWizardLogic.txx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/

#include "RestrictedImageIOWizardLogic.h"

template<class TPixel>
bool
RestrictedImageIOWizardLogic<TPixel>
::DisplayInputWizard(const char *file)
{
  // Make sure there is a grey image as a reference
  assert(m_GreyImage);

  // Get the size and spacing of the grey image
  SizeType requiredSize = m_GreyImage->GetBufferedRegion().GetSize();

  const double *requiredSpacing = m_GreyImage->GetSpacing().GetDataPointer();

  // Prepare the header page of the wizard UI
  this->m_InHeaderPageDimX->value(requiredSize[0]);
  this->m_InHeaderPageDimY->value(requiredSize[1]);
  this->m_InHeaderPageDimZ->value(requiredSize[2]);
  this->m_InHeaderPageSpacingX->value(requiredSpacing[0]);
  this->m_InHeaderPageSpacingY->value(requiredSpacing[1]);
  this->m_InHeaderPageSpacingZ->value(requiredSpacing[2]);

  // Disable the orientation page
  this->m_PageOrientation->deactivate();

  // Call the parent's method
  return Superclass::DisplayInputWizard(file);    
}

template<class TPixel>
bool
RestrictedImageIOWizardLogic<TPixel>
::CheckImageValidity()
{
  SizeType requiredSize = m_GreyImage->GetBufferedRegion().GetSize();
  SizeType loadedSize = this->m_Image->GetBufferedRegion().GetSize();

  // Check whether or not the image size matches the 'forced' image size
  if(!(requiredSize == loadedSize))
    {
    // Bark at the user
    fl_alert(
      "The size of the image you are attempting to load does not match "
      "the size of the 'grey' image already loaded.");

    return false;
    }
  else
    return true;
}
