/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: VTKMeshPipeline.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:15 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __VTKMeshPipeline_h_
#define __VTKMeshPipeline_h_

#include "SNAPCommon.h"
#include "MeshOptions.h"
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

// Optional selection of patented or non-patented algorithms
#ifdef USE_VTK_PATENTED
  #include <vtkImageMarchingCubes.h>
  #include <vtkDecimate.h>
#else
  #include <vtkContourFilter.h>
  #include <vtkPolyDataNormals.h>
  #include <vtkDecimatePro.h>
#endif

#ifndef vtkFloatingPointType
# define vtkFloatingPointType vtkFloatingPointType
typedef float vtkFloatingPointType;
#endif

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
  void SetMeshOptions(const MeshOptions &options);

  /** Compute a mesh for a particular color label.  Returns true if 
   * the color label is not present in the image */
  void ComputeMesh(vtkPolyData *outData);

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
  MeshOptions m_MeshOptions;

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
  
#ifdef USE_VTK_PATENTED
  
  // Marching cubes filter
  vtkImageMarchingCubes *     m_MarchingCubesFilter;
  
  // The triangle decimation driver
  vtkDecimate *               m_DecimateFilter;

#else // USE_VTK_PATENTED

  // The contour filter
  vtkContourFilter *          m_ContourFilter;

  // A filter that computes normals
  vtkPolyDataNormals *        m_NormalsFilter;

  // The triangle decimation driver
  vtkDecimatePro *            m_DecimateProFilter;

#endif // USE_VTK_PATENTED

  // Progress event monitor
  AllPurposeProgressAccumulator::Pointer m_Progress;

};

#endif // __VTKMeshPipeline_h_
