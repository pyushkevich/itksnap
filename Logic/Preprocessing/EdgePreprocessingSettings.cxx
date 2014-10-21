/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: EdgePreprocessingSettings.cxx,v $
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
#include "EdgePreprocessingSettings.h"
#include "ScalarImageWrapper.h"
#include "Registry.h"

bool 
EdgePreprocessingSettings
::operator == (const EdgePreprocessingSettings &other) const
{
  return (m_GaussianBlurScale == other.m_GaussianBlurScale &&
          m_RemappingSteepness == other.m_RemappingSteepness &&
          m_RemappingExponent == other.m_RemappingExponent);
}

EdgePreprocessingSettings::
EdgePreprocessingSettings():
        m_GaussianBlurScale(1.0f), 
        m_RemappingSteepness(0.04f),  
        m_RemappingExponent(3.0f) 
{
  this->InitializeToDefaults();
}

void
EdgePreprocessingSettings
::InitializeToDefaults()
{
  // These seem to be pretty reliable settings
  SetGaussianBlurScale(1.0f);
  SetRemappingSteepness(0.04f);
  SetRemappingExponent(3.0f);
}

void
EdgePreprocessingSettings
::ReadFromRegistry(Registry &registry)
{
  m_GaussianBlurScale = registry["GaussianBlurScale"][m_GaussianBlurScale];
  m_RemappingSteepness = registry["RemappingSteepness"][m_RemappingSteepness];
  m_RemappingExponent = registry["RemappingExponent"][m_RemappingExponent];
}

void EdgePreprocessingSettings
::WriteToRegistry(Registry &registry)
{
  registry["GaussianBlurScale"] << m_GaussianBlurScale;
  registry["RemappingSteepness"] << m_RemappingSteepness;
  registry["RemappingExponent"] << m_RemappingExponent;
}
