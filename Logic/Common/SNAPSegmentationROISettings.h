/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: SNAPSegmentationROISettings.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:11 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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
    m_ResampleFlag = false;
    m_VoxelScale.fill(1.0);
    m_InterpolationMethod = NEAREST_NEIGHBOR;
    }

  virtual ~SNAPSegmentationROISettings() {}

  // Get the region of interest, in the main IRIS image
  irisSetMacro(ROI,itk::ImageRegion<3>);
  
  // Set the region of interest, in the main IRIS image
  irisGetMacro(ROI,itk::ImageRegion<3>);

  // Get whether or not resampling is desired for the region
  irisSetMacro(ResampleFlag,bool);
  
  // Set whether or not resampling is desired for the region
  irisGetMacro(ResampleFlag,bool);

  // Get the scaling factor for each dimension
  irisSetMacro(VoxelScale,Vector3d);
  
  // Set the scaling factor for each dimension
  irisGetMacro(VoxelScale,Vector3d);

  // Get the interpolation method used
  irisSetMacro(InterpolationMethod,InterpolationMethod);
  
  // Set the interpolation method used
  irisGetMacro(InterpolationMethod,InterpolationMethod);

  // Map image voxel to an ROI voxel, if the result is outside of the region
  // this will return false, and not change the index
  bool TransformImageVoxelToROIVoxel(const Vector3ui &vImage, Vector3ui &vROI);

  // Map ROI voxel into an image voxel. The result will be inside the image
  void TransformROIVoxelToImageVoxel(const Vector3ui &vROI, Vector3ui &vImage);

private:
  // The region of interest, in the main IRIS image
  itk::ImageRegion<3> m_ROI;
  
  // Whether or not resampling is desired for the region
  bool m_ResampleFlag;
  
  // The scaling factor for each dimension
  Vector3d m_VoxelScale;
  
  // The interpolation method used
  InterpolationMethod m_InterpolationMethod;
};


#endif // __SNAPSegmentationROISettings_h_
