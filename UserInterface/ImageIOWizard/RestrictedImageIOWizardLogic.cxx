/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: RestrictedImageIOWizardLogic.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/14 16:21:04 $
  Version:   $Revision: 1.3 $
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
#include "FL/fl_ask.H"

RestrictedImageIOWizardLogic
::RestrictedImageIOWizardLogic()
{
  m_NumberOfComponentsRestriction = 0;
  m_MainImage = 0;
}

bool
RestrictedImageIOWizardLogic
::DisplayInputWizard(const char *file, const char *type)
{
  // If there is no main image specified, the restriction is ignored
  if(m_MainImage)
    {
    // Get the size and spacing of the main image
    SizeType requiredSize = m_MainImage->GetBufferedRegion().GetSize();

    const double *requiredSpacing = m_MainImage->GetSpacing().GetDataPointer();

    // Prepare the header page of the wizard UI
    this->m_InHeaderPageDimX->value(requiredSize[0]);
    this->m_InHeaderPageDimY->value(requiredSize[1]);
    this->m_InHeaderPageDimZ->value(requiredSize[2]);
    this->m_InHeaderPageSpacingX->value(requiredSpacing[0]);
    this->m_InHeaderPageSpacingY->value(requiredSpacing[1]);
    this->m_InHeaderPageSpacingZ->value(requiredSpacing[2]);
    }

  // Call the parent's method
  return Superclass::DisplayInputWizard(file, type);
}

bool
RestrictedImageIOWizardLogic
::CheckImageValidity()
{
  // Get the native image pointer
  ImageBaseType *native = this->m_GuidedIO.GetNativeImage();

  // If the main image is specified, compare size, origin, etc to it
  if(m_MainImage)
    {
    // TODO: Move this code to IRISApplications and call from here and from
    // command-line loading.
    SizeType requiredSize = m_MainImage->GetBufferedRegion().GetSize();
    SizeType loadedSize = native->GetBufferedRegion().GetSize();

    // Check whether or not the image size matches the 'forced' image size
    if(!(requiredSize == loadedSize))
      {
      // Bark at the user
      fl_alert(
        "The size of the image you are attempting to load does not match "
        "the size of the main image already loaded.");

      return false;
      }

    // Check if there is a discrepancy in the header fields. This will not
    // preclude the user from loading the image, but it will generate a 
    // warning, hopefully leading users to adopt more flexible file formats
    bool match_spacing = true, match_origin = true, match_direction = true;
    for(unsigned int i = 0; i < 3; i++)
      {
      if(m_MainImage->GetSpacing()[i] != native->GetSpacing()[i])
        match_spacing = false;

      if(m_MainImage->GetOrigin()[i] != native->GetOrigin()[i])
        match_origin = false;

      for(size_t j = 0; j < 3; j++)
        {
        double diff = fabs(m_MainImage->GetDirection()(i,j) - native->GetDirection()(i,j));
        if(diff > 1.0e-4)
          match_direction = false;
        }
      }

    if(!match_spacing || !match_origin || !match_direction)
      {
      // Come up with a warning message
      std::string object, verb;
      if(!match_spacing && !match_origin && !match_direction)
        { object = "spacing, origin and orientation"; }
      else if (!match_spacing && !match_origin)
        { object = "spacing and origin"; }
      else if (!match_spacing && !match_direction)
        { object = "spacing and orientation"; }
      else if (!match_origin && !match_direction)
        { object = "origin and orientation";}
      else if (!match_spacing)
        { object = "spacing"; }
      else if (!match_direction)
        { object = "orientation";}
      else if (!match_origin)
        { object = "origin"; }

      // Create an alert box
      fl_choice(
        "There is a mismatch between the header of the image that you are\n"
        "loading and the header of the main image currently open in SNAP.\n\n"
        "The images have different %s. \n\n"
        "SNAP will ignore the header information in the image you are loading.\n",
        "Ok", NULL, NULL,
        object.c_str());

      // Make the header of the image match that of the main image
      native->SetOrigin(m_MainImage->GetOrigin());
      native->SetSpacing(m_MainImage->GetSpacing());
      native->SetDirection(m_MainImage->GetDirection());
      }
    }

  if(m_NumberOfComponentsRestriction > 0)
    {
    if(native->GetNumberOfComponentsPerPixel() != m_NumberOfComponentsRestriction)
      {
      fl_alert(
        "The number of components per pixel (%d) in the image you are \n"
        "attempting to load does not match the required number (%d).", 
        (int) native->GetNumberOfComponentsPerPixel(),
        (int) m_NumberOfComponentsRestriction); 
      return false;
      }
    }
  return true;
}
