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
#include "ImageWrapperBase.h"
#include "Registry.h"

bool 
ThresholdSettings
::operator == (const ThresholdSettings &other) const
{
  return(
        this->m_LowerThreshold == other.m_LowerThreshold &&
        this->m_UpperThreshold == other.m_UpperThreshold &&
        this->m_Smoothness == other.m_Smoothness &&
        this->m_ThresholdMode == other.m_ThresholdMode &&
        this->m_Initialized == other.m_Initialized);
}

bool
ThresholdSettings
::operator !=(const ThresholdSettings &other) const
{
  return !((*this) == other);
}

void
ThresholdSettings
::InitializeToDefaultForImage(ScalarImageWrapperBase *wrapper)
{
  // Use the min and the max of the wrapper
  double iMin = wrapper->GetImageMinAsDouble();
  double iMax = wrapper->GetImageMaxAsDouble();

  // If the image is constant, return default settings
  if(iMin < iMax)
    {
    m_ThresholdMode = TWO_SIDED;
    m_LowerThreshold = iMin + (iMax-iMin) / 3.0;
    m_UpperThreshold = iMin + 2.0 * (iMax-iMin) / 3.0;
    m_Smoothness = 3.0;
    }
  else
    {
    m_ThresholdMode = LOWER;
    m_LowerThreshold = iMin;
    m_UpperThreshold = iMax;
    m_Smoothness = 3.0;
    }

  m_Initialized = true;
}

ThresholdSettings
::ThresholdSettings()
{
  m_ThresholdMode = TWO_SIDED;
  m_LowerThreshold = 0.0;
  m_UpperThreshold = 1.0; 
  m_Smoothness = 1.0;
  m_Initialized = false;
}

bool
ThresholdSettings
::IsValid() const
{
  if(m_Smoothness < 0.0) return false;
  if(m_ThresholdMode == TWO_SIDED && m_UpperThreshold <= m_LowerThreshold)
    return false;
  return true;
}

bool
ThresholdSettings
::IsValidForImage(ScalarImageWrapperBase *wrapper)
{
  // Use the min and the max of the wrapper
  double iMin = wrapper->GetImageMinAsDouble();
  double iMax = wrapper->GetImageMaxAsDouble();

  // Check threshold irregularities
  if(m_ThresholdMode == TWO_SIDED || m_ThresholdMode == LOWER)
    if(m_LowerThreshold < iMin) return false;

  if(m_ThresholdMode == TWO_SIDED || m_ThresholdMode == UPPER)
    if(m_UpperThreshold > iMax) return false;

  if(m_ThresholdMode == TWO_SIDED)
    if(m_LowerThreshold >= m_UpperThreshold) return false;

  // Check smoothness
  if(m_Smoothness < 0.0) return false;

  return true;
}

void
ThresholdSettings
::ReadFromRegistry(Registry &registry, ScalarImageWrapperBase *wrapper)
{
  // Use the min and the max of the wrapper to check validity
  double iMin = wrapper->GetImageMinAsDouble();
  double iMax = wrapper->GetImageMaxAsDouble();

  // Try reading the settings from the registry
  float lt = registry["LowerThreshold"][m_LowerThreshold];
  float ut = registry["UpperThreshold"][m_UpperThreshold];
  float sm = registry["Smoothness"][m_Smoothness];

  // Check that the settings are valid before proceding
  if(iMin <= lt && lt <= ut && ut <= iMax && sm >= 0.0)
    {
    // Use the threshold values we read
    m_LowerThreshold = lt;
    m_UpperThreshold = ut;

    // Read the smoothness information
    m_Smoothness = sm;

    // For backward compatibility (and out of laziness to set up an enum mapping)
    // we store the threshold limits as a pair of bools, rather than as the enum
    bool lower_on =
        registry["LowerThresholdEnabled"][IsLowerThresholdEnabled()];

    bool upper_on =
        registry["UpperThresholdEnabled"][IsUpperThresholdEnabled()];

    m_ThresholdMode = upper_on ?
          (lower_on ? TWO_SIDED : UPPER) :
          (lower_on ? LOWER : TWO_SIDED);

    // Set as having been initialized
    m_Initialized = true;
    }
}

void
ThresholdSettings
::WriteToRegistry(Registry &registry)
{
  assert(m_Initialized);

  registry["LowerThreshold"] << this->GetLowerThreshold();
  registry["UpperThreshold"] << this->GetUpperThreshold();
  registry["Smoothness"] << this->GetSmoothness();
  registry["LowerThresholdEnabled"] << this->IsLowerThresholdEnabled();
  registry["UpperThresholdEnabled"] << this->IsUpperThresholdEnabled();
}

