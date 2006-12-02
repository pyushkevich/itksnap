/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: ThresholdSettings.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:15 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "ThresholdSettings.h"
#include "GreyImageWrapper.h"

bool 
ThresholdSettings
::operator == (const ThresholdSettings &other) const
{
  return memcmp(this,&other,sizeof(ThresholdSettings)) == 0;
}

ThresholdSettings
ThresholdSettings
::MakeDefaultSettings(GreyImageWrapper *wrapper)
{
  // Use the min and the max of the wrapper
  int iMin = wrapper->GetImageMin();
  int iMax = wrapper->GetImageMax();

  // If the image is constant, return default settings
  if(iMin == iMax) return MakeDefaultSettingsWithoutImage();
  
  // Generate the default settings
  ThresholdSettings settings;
  settings.m_LowerThresholdEnabled = true;
  settings.m_UpperThresholdEnabled = true;  
  settings.m_LowerThreshold = iMin + (iMax-iMin) / 3.0;
  settings.m_UpperThreshold = iMin + 2.0 * (iMax-iMin) / 3.0;
  settings.m_Smoothness = 3.0;

  return settings;
}

ThresholdSettings
ThresholdSettings
::MakeDefaultSettingsWithoutImage()
{
  ThresholdSettings settings;
  settings.m_LowerThresholdEnabled = true;
  settings.m_UpperThresholdEnabled = true;  
  settings.m_LowerThreshold = 40.0;
  settings.m_UpperThreshold = 80.0; 
  settings.m_Smoothness = 3.0;

  return settings;
}

ThresholdSettings
::ThresholdSettings()
{
  m_LowerThresholdEnabled = true;
  m_UpperThresholdEnabled = true;  
  m_LowerThreshold = 0.0;
  m_UpperThreshold = 1.0; 
  m_Smoothness = 1.0;
}

bool
ThresholdSettings
::IsValid() const
{
  if(m_Smoothness < 0.0) return false;
  if(m_LowerThresholdEnabled && m_UpperThresholdEnabled && 
      m_UpperThreshold <= m_LowerThreshold)
    return false;
  return true;
}

