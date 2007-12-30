/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: RestrictedImageIOWizardLogic.txx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:17 $
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
