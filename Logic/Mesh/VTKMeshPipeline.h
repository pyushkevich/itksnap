/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: VTKMeshPipeline.h,v $
  Language:  C++
  Date:      $Date: 2009/01/23 20:09:38 $
  Version:   $Revision: 1.4 $
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
#ifndef __VTKMeshPipeline_h_
#define __VTKMeshPipeline_h_

#include "SNAPCommon.h"
#include "AllPurposeProgressAccumulator.h"

// ITK includes (this file is not widely included in SNAP, so it's OK
// to include a bunch of headers here).
#include <itkImage.h>
#include <itkVTKImageExport.h>

// VTK includes
#include <vtkCellArray.h>
#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkImageThreshold.h>
#include <vtkImageToStructuredPoints.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkStripper.h>
#include <vtkCallbackCommand.h>
#include <vtkMarchingCubes.h>
#include <vtkDecimatePro.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTransform.h>

#ifndef vtkFloatingPointType
# define vtkFloatingPointType vtkFloatingPointType
typedef float vtkFloatingPointType;
#endif

class MeshOptions;
class VTKProgressAccumulator;

/**
 * \class VTKMeshPipeline
 * \brief A small pipeline used to convert an ITK image with a level set into
 * a VTK contour, with optional blurring
 */
class VTKMeshPipeline 
{
public:
  /** Input image type */
  typedef itk::Image<float,3> ImageType;
  typedef itk::SmartPointer<ImageType> ImagePointer;
  
  /** Set the input segmentation image */
  void SetImage(ImageType *input);

  /** Set the mesh options for this filter */
  void SetMeshOptions(MeshOptions *options);

  /** Compute a mesh for a particular color label */
  void ComputeMesh(vtkPolyData *outData, itk::FastMutexLock *lock = NULL);

  /** Get the progress accumulator */
  AllPurposeProgressAccumulator *GetProgressAccumulator()
    { return m_Progress; }

  /** Constructor, which builds the pipeline */
  VTKMeshPipeline();

  /** Deallocate the pipeline filters */
  ~VTKMeshPipeline();

private:
  
  // VTK-ITK Connection typedefs
  typedef itk::VTKImageExport<ImageType> VTKExportType;
  typedef itk::SmartPointer<VTKExportType> VTKExportPointer;
  
  // Current set of mesh options
  SmartPtr<MeshOptions> m_MeshOptions;

  // The input image
  ImagePointer m_InputImage;

  // The VTK exporter for the data
  VTKExportPointer m_VTKExporter;

  // The VTK importer for the data
  vtkImageImport *m_VTKImporter;

  // VTK Gaussian (because we care about the speed and not so much about
  // precision)
  vtkImageGaussianSmooth *m_VTKGaussianFilter;

  // The polygon smoothing filter
  vtkSmoothPolyDataFilter *m_PolygonSmoothingFilter;

  // Triangle stripper
  vtkStripper *m_StripperFilter;  
  
  // Marching cubes filter
  vtkMarchingCubes *     m_MarchingCubesFilter;

  // Transform filter used to map to RAS space
  vtkTransformPolyDataFilter *m_TransformFilter;

  // The transform used
  vtkTransform * m_Transform;
  
  // The triangle decimation driver
  vtkDecimatePro *               m_DecimateFilter;

  // Progress event monitor
  AllPurposeProgressAccumulator::Pointer m_Progress;

};

#endif // __VTKMeshPipeline_h_
