/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: EdgePreprocessingSettings.h,v $
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
#ifndef __EdgePreprocessingSettings_h_
#define __EdgePreprocessingSettings_h_

#include "SNAPCommon.h"

// Forward references
class GreyImageWrapper;

/**
 * This class encapsulates the thresholding settings used by the program.
 * for In/Out snake initialization
 */
class EdgePreprocessingSettings
{
public:
  irisGetMacro(GaussianBlurScale,float);
  irisSetMacro(GaussianBlurScale,float);

  irisGetMacro(RemappingSteepness,float);
  irisSetMacro(RemappingSteepness,float);

  irisGetMacro(RemappingExponent,float);
  irisSetMacro(RemappingExponent,float);
  
  /** Compare two sets of settings */
  bool operator == (const EdgePreprocessingSettings &other) const;

  /**
   * Create a default instance of the settings based on an image wrapper
   */
  static EdgePreprocessingSettings MakeDefaultSettings();

  // Constructor
  EdgePreprocessingSettings();
  // Destructor
  virtual ~EdgePreprocessingSettings();

private:
  float m_GaussianBlurScale;
  float m_RemappingSteepness;
  float m_RemappingExponent;
};

#endif // __EdgePreprocessingSettings_h_

