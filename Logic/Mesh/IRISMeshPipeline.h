/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IRISMeshPipeline.h,v $
  Language:  C++
  Date:      $Date: 2009/01/23 20:09:38 $
  Version:   $Revision: 1.3 $
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
#ifndef __IRISMeshPipeline_h_
#define __IRISMeshPipeline_h_

#include "SNAPCommon.h"
#include "itkImageRegion.h"
#include "itkSmartPointer.h"
#include "MeshOptions.h"

// Forward reference to itk classes
namespace itk {
  template <class TPixel,unsigned int VDimension> class Image;
  template <class TInputImage, class TOutputImage> class RegionOfInterestImageFilter;
  template <class TInputImage, class TOutputImage> class BinaryThresholdImageFilter;
}

// Forward references
class VTKMeshPipeline;
class vtkPolyData;
class AllPurposeProgressAccumulator;

/**
 * \class IRISMeshPipeline
 * \brief A small pipeline used to convert a segmentation image to a mesh in IRIS.
 *
 * This pipeline preprocesses each label in the segmentation image by blurring it.
 */
class IRISMeshPipeline 
{
public:
  /** Input image type */
  typedef itk::Image<LabelType,3> InputImageType;
  typedef itk::SmartPointer<InputImageType> InputImagePointer;
  
  /** Set the input segmentation image */
  void SetImage(InputImageType *input);

  /** Compute the bounding boxes for different regions.  Prerequisite for 
   * calling ComputeMesh(). Returns the total number of voxels in all boxes */
  unsigned long ComputeBoundingBoxes();

  unsigned long GetVoxelsInBoundingBox(LabelType label) const;

  /** Set the mesh options for this filter */
  void SetMeshOptions(const MeshOptions &options);

  /** Can we compute a mesh for this label? */
  bool CanComputeMesh(LabelType label)
  {
    return m_Histogram[label] > 0l;
  }

  /** Compute a mesh for a particular color label.  Returns true if 
   * the color label is not present in the image */
  bool ComputeMesh(LabelType label, vtkPolyData *outData);
  
  /** Constructor, which builds the pipeline */
  IRISMeshPipeline();

  /** Deallocate the pipeline filters */
  ~IRISMeshPipeline();

  /** Get the progress accumulator from the VTK mesh pipeline */
  AllPurposeProgressAccumulator *GetProgressAccumulator();

private:
  // Type definitions for the various filters used by this object
  typedef itk::Image<float,3>                InternalImageType;
  typedef itk::SmartPointer<InternalImageType>       InternalImagePointer;
  
  typedef itk::RegionOfInterestImageFilter<
    InputImageType,InputImageType>                   ROIFilter;
  typedef itk::SmartPointer<ROIFilter>               ROIFilterPointer;
  
  typedef itk::BinaryThresholdImageFilter<
    InputImageType,InternalImageType>                ThresholdFilter;
  typedef itk::SmartPointer<ThresholdFilter>         ThresholdFilterPointer;
  
  // Current set of mesh options
  MeshOptions                 m_MeshOptions;

  // The input image
  InputImagePointer           m_InputImage;

  // The ROI extraction filter used for constructing a bounding box
  ROIFilterPointer            m_ROIFilter;

  // The thresholding filter used to map intensity in the bounding box to
  // standardized range
  ThresholdFilterPointer      m_ThrehsoldFilter;

  // Set of bounding boxes
  itk::ImageRegion<3>         m_BoundingBox[MAX_COLOR_LABELS];

  // Histogram of the image
  long                        m_Histogram[MAX_COLOR_LABELS];

  // The VTK pipeline
  VTKMeshPipeline *           m_VTKPipeline;
};

#endif
