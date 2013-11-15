/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPSegmentationROISettings.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:13 $
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
#ifndef __SNAPSegmentationROISettings_h_
#define __SNAPSegmentationROISettings_h_

// #include "itkImageRegion.h"
#include "SNAPCommon.h"
#include "itkImageRegion.h"
#include "IRISVectorTypesToITKConversion.h"

/** 
 * Settings describing the region of interest selected for the SnAP 
 * segmentation and the resampling associated with it */
class SNAPSegmentationROISettings
{
public:
  // List of available interpolation methods
  enum InterpolationMethod {
    NEAREST_NEIGHBOR, TRILINEAR, TRICUBIC, SINC_WINDOW_05
  };

  SNAPSegmentationROISettings()
    {
    m_InterpolationMethod = NEAREST_NEIGHBOR;
    }

  virtual ~SNAPSegmentationROISettings() {}

  // Get the region of interest, in the main IRIS image
  void SetROI(const itk::ImageRegion<3> &roi);
  
  // Set the region of interest, in the main IRIS image
  irisGetMacro(ROI,itk::ImageRegion<3>)

  // Get whether or not resampling is desired for the region
  bool IsResampling() const;

  // Specify the resampling to be applied to the ROI (new size)
  irisSetMacro(ResampleDimensions,Vector3ui)
  
  // Get the size of the ROI after resampling
  irisGetMacro(ResampleDimensions,Vector3ui)

  // Get the interpolation method used
  irisSetMacro(InterpolationMethod,InterpolationMethod)
  
  // Set the interpolation method used
  irisGetMacro(InterpolationMethod,InterpolationMethod)

  bool operator == (const SNAPSegmentationROISettings &other) const;
  bool operator != (const SNAPSegmentationROISettings &other) const;


private:
  // The region of interest, in the main IRIS image
  itk::ImageRegion<3> m_ROI;
  
  // The scaling factor for each dimension
  Vector3ui m_ResampleDimensions;
  
  // The interpolation method used
  InterpolationMethod m_InterpolationMethod;
};


#endif // __SNAPSegmentationROISettings_h_
