/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: EdgePreprocessingSettings.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:15 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "EdgePreprocessingSettings.h"

bool 
EdgePreprocessingSettings
::operator == (const EdgePreprocessingSettings &other) const
{
  return memcmp(this,&other,sizeof(EdgePreprocessingSettings)) == 0;
}

EdgePreprocessingSettings::
EdgePreprocessingSettings()
{
  SetGaussianBlurScale(1.0f);
  SetRemappingSteepness(0.1f);
  SetRemappingExponent(2.0f);
}
EdgePreprocessingSettings::
~EdgePreprocessingSettings() { /*Needs to be defined to avoid compiler warning about needing a virtual destructor*/};

EdgePreprocessingSettings
EdgePreprocessingSettings
::MakeDefaultSettings()
{
  EdgePreprocessingSettings settings;
  return settings;
}
