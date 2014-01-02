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
#include "itkDataObject.h"
#include "itkObjectFactory.h"

// Forward references
class ScalarImageWrapperBase;
class Registry;

/**
  This class encapsulates the thresholding settings used by the smooth 2-sided
  threshold preprocessing option. This settings object inherits from the
  itk::DataObject class. This makes it possible to pass the settings as an
  input to the data processing filter, which makes sure that the filter
  automatically recomputes in response to changes to the parameters.

  The settings are meant to be passed by reference, with a single object in
  the program owning the settings.
*/
class ThresholdSettings : public itk::DataObject
{
public:

  irisITKObjectMacro(ThresholdSettings, itk::DataObject)

  /** The mode for setting the threshold */
  enum ThresholdMode { TWO_SIDED=0, LOWER, UPPER };

  itkSetMacro(LowerThreshold, float)
  itkGetConstMacro(LowerThreshold, float)

  itkSetMacro(UpperThreshold, float)
  itkGetConstMacro(UpperThreshold, float)

  itkSetMacro(Smoothness, float)
  itkGetConstMacro(Smoothness, float)

  itkSetMacro(ThresholdMode, ThresholdMode)
  itkGetConstMacro(ThresholdMode, ThresholdMode)

  itkGetMacro(Initialized, bool)

  bool IsLowerThresholdEnabled() const
  {
    return m_ThresholdMode != UPPER;
  }

  bool IsUpperThresholdEnabled() const
  {
    return m_ThresholdMode != LOWER;
  }

  /** Compare two sets of settings */
  bool operator == (const ThresholdSettings &other) const;
  bool operator != (const ThresholdSettings &other) const;

  /** Check if the settings are valid */
  bool IsValid() const;

  /** Check validity relative to a loaded image */
  bool IsValidForImage(ScalarImageWrapperBase *wrapper);

  /**
   * Create a default instance of the settings based on an image wrapper
   */
  void InitializeToDefaultForImage(ScalarImageWrapperBase *wrapper);

  /**
    Read the settings from a registry, given the current grey image. This will
    check if the settings are valid and abort if they are not
    */
  void ReadFromRegistry(Registry &reg, ScalarImageWrapperBase *wrapper);

  /** Write the settings to a registry */
  void WriteToRegistry(Registry &registry);

protected:

  ThresholdSettings();
  virtual ~ThresholdSettings() { /*To avoid compiler warning.*/ }

private:
  float m_LowerThreshold;
  float m_UpperThreshold;
  float m_Smoothness;

  // Whether the settings have been initialized. When constructed, the settings
  // are set to an uninitialized state.
  bool m_Initialized;
  
  ThresholdMode m_ThresholdMode;
};

#endif // __ThresholdSettings_h_

