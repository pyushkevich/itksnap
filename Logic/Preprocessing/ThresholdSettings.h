/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ThresholdSettings.h,v $
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
#ifndef __ThresholdSettings_h_
#define __ThresholdSettings_h_

#include "SNAPCommon.h"

// Forward references
class GreyImageWrapper;

/**
 * This class encapsulates the thresholding settings used by the program.
 * for In/Out snake initialization
 */
class ThresholdSettings
{
public:
    virtual ~ThresholdSettings() { /*To avoid compiler warning.*/ }
  irisGetMacro(LowerThreshold, float);
  irisSetMacro(LowerThreshold, float);

  irisGetMacro(UpperThreshold, float);
  irisSetMacro(UpperThreshold, float);

  irisGetMacro(Smoothness,float);
  irisSetMacro(Smoothness,float);

  irisIsMacro(LowerThresholdEnabled);
  irisSetMacro(LowerThresholdEnabled,bool);

  irisIsMacro(UpperThresholdEnabled);
  irisSetMacro(UpperThresholdEnabled,bool);

  /** Compare two sets of settings */
  bool operator == (const ThresholdSettings &other) const;

  /** Check if the settings are valid */
  bool IsValid() const;

  /**
   * Create a default instance of the settings based on an image wrapper
   */
  static ThresholdSettings MakeDefaultSettings(GreyImageWrapper *wrapper);

  /** This will create a slightly less useful settings object with thresholds
   * at 40 and 100.  Before using them, make sure that the image is in range
   * of 40 and 100 */
  static ThresholdSettings MakeDefaultSettingsWithoutImage();

  // Constructor: creates dummy settings
  ThresholdSettings();

private:
  float m_LowerThreshold;
  float m_UpperThreshold;
  float m_Smoothness;
  
  bool m_UpperThresholdEnabled;
  bool m_LowerThresholdEnabled;
};

#endif // __ThresholdSettings_h_

