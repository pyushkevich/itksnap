/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ThresholdSettings.cxx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:15 $
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

