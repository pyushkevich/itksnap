/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: MultiLabelMeshPipeline.h,v $
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
#ifndef __MultiLabelMeshPipeline_h_
#define __MultiLabelMeshPipeline_h_

#include "SNAPCommon.h"
#include "itkImageRegion.h"
#include "itkSmartPointer.h"
#include "vtkSmartPointer.h"
#include "itksys/MD5.h"
#include "itkObjectFactory.h"

// Forward reference to itk classes
namespace itk {
  template <class TPixel,unsigned int VDimension> class Image;
  template <class TInputImage, class TOutputImage> class RegionOfInterestImageFilter;
  template <class TInputImage, class TOutputImage> class BinaryThresholdImageFilter;
  template <class TImage> class ImageLinearConstIteratorWithIndex;
}


// Forward references
class MeshOptions;
class VTKMeshPipeline;
class vtkPolyData;
class AllPurposeProgressAccumulator;


/**
 * \class MultiLabelMeshPipeline
 * \brief A small pipeline used to convert a multi-label segmentation image to
 * a collection of VTK meshes.
 *
 * For each label, the pipeline uses the checksum mechanism to keep track of
 * whether it has been updated relative to the corresponding mesh. This makes
 * it possible for selective mesh recomputation, leading to fast mesh computation
 * even for big segmentations.
 */
class MultiLabelMeshPipeline : public itk::Object
{
public:

  irisITKObjectMacro(MultiLabelMeshPipeline, itk::Object)

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
  void SetMeshOptions(const MeshOptions *options);

  /** Can we compute a mesh for this label? */
  bool CanComputeMesh(LabelType label)
  {
    return m_Histogram[label] > 0l;
  }

  /** Compute a mesh for a particular color label.  Returns true if 
   * the color label is not present in the image */
  bool ComputeMesh(LabelType label, vtkPolyData *outData);

  /** Update the meshes */
  void UpdateMeshes(itk::Command *progressCommand);

  /** Get the collection of computed meshes */
  std::map<LabelType, vtkSmartPointer<vtkPolyData> > GetMeshCollection();

  
  /** Get the progress accumulator from the VTK mesh pipeline */
  AllPurposeProgressAccumulator *GetProgressAccumulator();

protected:

  /** Constructor, which builds the pipeline */
  MultiLabelMeshPipeline();

  /** Deallocate the pipeline filters */
  ~MultiLabelMeshPipeline();

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
  SmartPtr<MeshOptions>       m_MeshOptions;

  // The input image
  InputImagePointer           m_InputImage;

  // The ROI extraction filter used for constructing a bounding box
  ROIFilterPointer            m_ROIFilter;

  // The thresholding filter used to map intensity in the bounding box to
  // standardized range
  ThresholdFilterPointer      m_ThrehsoldFilter;

  // Cached information about a VTK mesh
  struct MeshInfo
  {
    // The pointer to the mesh
    vtkSmartPointer<vtkPolyData> Mesh;

    // The checksum for the mesh
    unsigned long CheckSum;

    // The extents of the bounding box
    Vector3i BoundingBox[2];

    // The number of voxels
    unsigned long Count;

    MeshInfo();
    ~MeshInfo();
  };

  // Collection of mesh data for labels present in the image
  typedef std::map<LabelType, MeshInfo> MeshInfoMap;
  MeshInfoMap m_MeshInfo;

  // Set of bounding boxes
  itk::ImageRegion<3>         m_BoundingBox[MAX_COLOR_LABELS];

  // Histogram of the image
  long                        m_Histogram[MAX_COLOR_LABELS];

  // The VTK pipeline
  VTKMeshPipeline *           m_VTKPipeline;

  // Helper routine for the update command
  void UpdateMeshInfoHelper(
      MeshInfo *current_meshinfo,
      const itk::Index<3> &run_start,
      itk::ImageLinearConstIteratorWithIndex<InputImageType> &it,
      unsigned long pos);
};

#endif
