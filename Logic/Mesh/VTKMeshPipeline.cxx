/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: VTKMeshPipeline.cxx,v $
  Language:  C++
  Date:      $Date: 2007/05/10 20:19:50 $
  Version:   $Revision: 1.2 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "VTKMeshPipeline.h"
#include "AllPurposeProgressAccumulator.h"
#include <map>

using namespace std;
using namespace itk;


VTKMeshPipeline
::VTKMeshPipeline()
{
  // Initialize the progress tracker
  m_Progress = AllPurposeProgressAccumulator::New();

  // Initialize all the filters involved in the transaction, but do not
  // pipe the inputs and outputs between these filters. The piping is quite
  // complicated and depends on the set of options that the user wishes to 
  // apply
  
  // Initialize the VTK Exporter
  m_VTKExporter = VTKExportType::New();
  m_VTKExporter->ReleaseDataFlagOn();
  
  // Initialize the VTK Importer
  m_VTKImporter = vtkImageImport::New();
  m_VTKImporter->ReleaseDataFlagOn();

  // Pipe the importer into the exporter (that's a lot of code)
  m_VTKImporter->SetUpdateInformationCallback(
    m_VTKExporter->GetUpdateInformationCallback());
  m_VTKImporter->SetPipelineModifiedCallback(
    m_VTKExporter->GetPipelineModifiedCallback());
  m_VTKImporter->SetWholeExtentCallback(
    m_VTKExporter->GetWholeExtentCallback());
  m_VTKImporter->SetSpacingCallback(
    m_VTKExporter->GetSpacingCallback());
  m_VTKImporter->SetOriginCallback(
    m_VTKExporter->GetOriginCallback());
  m_VTKImporter->SetScalarTypeCallback(
    m_VTKExporter->GetScalarTypeCallback());
  m_VTKImporter->SetNumberOfComponentsCallback(
    m_VTKExporter->GetNumberOfComponentsCallback());
  m_VTKImporter->SetPropagateUpdateExtentCallback(
    m_VTKExporter->GetPropagateUpdateExtentCallback());
  m_VTKImporter->SetUpdateDataCallback(
    m_VTKExporter->GetUpdateDataCallback());
  m_VTKImporter->SetDataExtentCallback(
    m_VTKExporter->GetDataExtentCallback());
  m_VTKImporter->SetBufferPointerCallback(
    m_VTKExporter->GetBufferPointerCallback());  
  m_VTKImporter->SetCallbackUserData(
    m_VTKExporter->GetCallbackUserData());

  // Initialize the Gaussian filter
  m_VTKGaussianFilter = vtkImageGaussianSmooth::New();
  m_VTKGaussianFilter->ReleaseDataFlagOn();
  
  // Create and configure a filter for polygon smoothing
  m_PolygonSmoothingFilter = vtkSmoothPolyDataFilter::New();
  m_PolygonSmoothingFilter->ReleaseDataFlagOn();

  // Create and configure a filter for triangle strip generation
  m_StripperFilter = vtkStripper::New();
  m_StripperFilter->ReleaseDataFlagOn();

#ifdef USE_VTK_PATENTED

  // Create and configure the marching cubes filter
  m_MarchingCubesFilter = vtkImageMarchingCubes::New();
  m_MarchingCubesFilter->ReleaseDataFlagOn();
  m_MarchingCubesFilter->ComputeScalarsOff();
  m_MarchingCubesFilter->ComputeGradientsOff();
  m_MarchingCubesFilter->SetNumberOfContours(1);
  m_MarchingCubesFilter->SetValue(0,0.0f);

  // Create and configure a filter for triangle decimation
  m_DecimateFilter = vtkDecimate::New();
  m_DecimateFilter->ReleaseDataFlagOn();  

#else // USE_VTK_PATENTED

  // Create and configure the contour filter
  m_ContourFilter = vtkContourFilter::New();
  m_ContourFilter->ReleaseDataFlagOn();
  m_ContourFilter->ComputeNormalsOff();
  m_ContourFilter->ComputeScalarsOff();
  m_ContourFilter->ComputeGradientsOff();
  m_ContourFilter->UseScalarTreeOn();
  m_ContourFilter->SetNumberOfContours(1);
  m_ContourFilter->SetValue(0,0.0f);

  // Create and configure the normal computer
  m_NormalsFilter = vtkPolyDataNormals::New();
  m_NormalsFilter->SplittingOff();
  m_NormalsFilter->ConsistencyOff();
  m_NormalsFilter->ReleaseDataFlagOn();

  // Create and configure a filter for triangle decimation
  m_DecimateProFilter = vtkDecimatePro::New();
  m_DecimateProFilter->ReleaseDataFlagOn();

#endif // USE_VTK_PATENTED  
  
}

VTKMeshPipeline
::~VTKMeshPipeline()
{
  // Destroy the filters
  m_VTKImporter->Delete();
  m_VTKGaussianFilter->Delete();
  m_PolygonSmoothingFilter->Delete();
  m_StripperFilter->Delete();

#ifdef USE_VTK_PATENTED
  m_MarchingCubesFilter->Delete();
  m_DecimateFilter->Delete();
#else  
  m_ContourFilter->Delete();
  m_NormalsFilter->Delete();
  m_DecimateProFilter->Delete();
#endif
}

void
VTKMeshPipeline
::SetMeshOptions(const MeshOptions &options)
{
  // Store the options
  m_MeshOptions = options;

  // Reset the weights in the progress meter
  m_Progress->UnregisterAllSources();

  // Define the current pipeline end-point
  vtkImageData *pipeImageTail = m_VTKImporter->GetOutput();
  vtkPolyData *pipePolyTail = NULL;

  // Route the pipeline according to the settings
  // 1. Check if Gaussian smoothing will be used

  if(options.GetUseGaussianSmoothing()) 
    {    
    // The Gaussian filter is enabled
    m_VTKGaussianFilter->SetInput(pipeImageTail);
    m_Progress->RegisterSource(m_VTKGaussianFilter, 10.0f);
    pipeImageTail = m_VTKGaussianFilter->GetOutput();

    // Apply parameters to the Gaussian filter
    float sigma = options.GetGaussianStandardDeviation();

    // Sigma is in millimeters
    const double *spacing = m_InputImage->GetSpacing().GetDataPointer();
    m_VTKGaussianFilter->SetStandardDeviation(
      sigma / spacing[0], sigma / spacing[1], sigma / spacing[2]);
    m_VTKGaussianFilter->SetRadiusFactors(
      3 * sigma / spacing[0], 3 * sigma / spacing[1], 3 * sigma / spacing[2]);
    }

  // 2. Set input to the appropriate contour filter

#ifdef USE_VTK_PATENTED
  
  // Marching cubes gets the tail
  m_MarchingCubesFilter->SetInput(pipeImageTail);
  m_Progress->RegisterSource(m_MarchingCubesFilter, 10.0);
  pipePolyTail = m_MarchingCubesFilter->GetOutput();

#else // USE_VTK_PATENTED

  // Contour filter gets the tail
  m_ContourFilter->SetInput(pipeImageTail);
  m_Progress->RegisterSource(m_ContourFilter, 10.0);
  pipePolyTail = m_ContourFilter->GetOutput();

#endif // USE_VTK_PATENTED  

  // 3. Check if decimation is required
  if(options.GetUseDecimation())
    {

#ifdef USE_VTK_PATENTED

    // Decimate filter gets the pipe tail
    m_DecimateFilter->SetInput(pipePolyTail);
    m_Progress->RegisterSource(m_DecimateFilter, 5.0);
    pipePolyTail = m_DecimateFilter->GetOutput();

    // Apply parameters to the decimation filter
    m_DecimateFilter->SetTargetReduction(
      options.GetDecimateTargetReduction());

    m_DecimateFilter->SetAspectRatio(
      options.GetDecimateAspectRatio());

    m_DecimateFilter->SetInitialError(
      options.GetDecimateInitialError());

    m_DecimateFilter->SetErrorIncrement(
      options.GetDecimateErrorIncrement());

    m_DecimateFilter->SetMaximumIterations(
      options.GetDecimateMaximumIterations());

    m_DecimateFilter->SetInitialFeatureAngle(
      options.GetDecimateFeatureAngle());

    m_DecimateFilter->SetPreserveTopology(
      options.GetDecimatePreserveTopology());

#else // USE_VTK_PATENTED    
    
    // Decimate Pro filter gets the pipe tail
    m_DecimateProFilter->SetInput(pipePolyTail);
    m_Progress->RegisterSource(m_DecimateProFilter, 5.0);
    pipePolyTail = m_DecimateProFilter->GetOutput();

    // Apply parameters to the decimation filter
    m_DecimateProFilter->SetTargetReduction(
      options.GetDecimateTargetReduction());

    m_DecimateProFilter->SetPreserveTopology(
      options.GetDecimatePreserveTopology());

#endif // USE_VTK_PATENTED  
    
    } // If decimate enabled

  // 4. Compute the normals (non-patented only)

#ifndef USE_VTK_PATENTED  

  m_NormalsFilter->SetInput(pipePolyTail);
  m_Progress->RegisterSource(m_NormalsFilter, 1.0);
  pipePolyTail = m_NormalsFilter->GetOutput();

#endif // USE_VTK_PATENTED

  // 5. Include/exclude mesh smoothing filter
  if(options.GetUseMeshSmoothing())
    {
    // Pipe smoothed output into the pipeline
    m_PolygonSmoothingFilter->SetInput(pipePolyTail);
    m_Progress->RegisterSource(m_PolygonSmoothingFilter, 3.0);
    pipePolyTail = m_PolygonSmoothingFilter->GetOutput();

    // Apply parameters to the mesh smoothing filter
    m_PolygonSmoothingFilter->SetNumberOfIterations(
      options.GetMeshSmoothingIterations());

    m_PolygonSmoothingFilter->SetRelaxationFactor(
      options.GetMeshSmoothingRelaxationFactor()); 

    m_PolygonSmoothingFilter->SetFeatureAngle(
      options.GetMeshSmoothingFeatureAngle());

    m_PolygonSmoothingFilter->SetFeatureEdgeSmoothing(
      options.GetMeshSmoothingFeatureEdgeSmoothing());

    m_PolygonSmoothingFilter->SetBoundarySmoothing(
      options.GetMeshSmoothingBoundarySmoothing());

    m_PolygonSmoothingFilter->SetConvergence(
      options.GetMeshSmoothingConvergence());
    }

  // 6. Pipe in the final output into the stripper
  m_StripperFilter->SetInput(pipePolyTail);
  m_Progress->RegisterSource(m_StripperFilter, 2.0);
}

#include <ctime>

void
VTKMeshPipeline
::ComputeMesh(vtkPolyData *outMesh)
{
  // Reset the progress meter
  m_Progress->ResetProgress();

  // Graft the polydata to the last filter in the pipeline
  m_StripperFilter->SetOutput(outMesh);
  
  // Connect importer and exporter
  m_VTKImporter->SetCallbackUserData(
    m_VTKExporter->GetCallbackUserData());

  // Update the ITK portion of the pipeline
  m_VTKExporter->SetInput(m_InputImage);

  // Update the pipeline
  m_StripperFilter->Update();

  // Set the source of outMesh to null
  m_StripperFilter->UnRegisterAllOutputs();
}

void
VTKMeshPipeline
::SetImage(ImageType *image)
{
  m_InputImage = image;
}

