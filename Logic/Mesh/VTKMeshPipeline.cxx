/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: VTKMeshPipeline.cxx,v $
  Language:  C++
  Date:      $Date: 2010/06/28 18:45:08 $
  Version:   $Revision: 1.7 $
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
#include "VTKMeshPipeline.h"
#include "AllPurposeProgressAccumulator.h"
#include "ImageWrapper.h"
#include "MeshOptions.h"
#include <map>

using namespace std;


VTKMeshPipeline
::VTKMeshPipeline()
{
  // Initialize the progress tracker
  m_Progress = AllPurposeProgressAccumulator::New();

  // Initialize the options
  m_MeshOptions = MeshOptions::New();

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

  // Create and configure the marching cubes filter
  m_MarchingCubesFilter = vtkMarchingCubes::New();
  m_MarchingCubesFilter->ReleaseDataFlagOn();
  m_MarchingCubesFilter->ComputeScalarsOff();
  m_MarchingCubesFilter->ComputeGradientsOff();
  m_MarchingCubesFilter->SetNumberOfContours(1);
  m_MarchingCubesFilter->SetValue(0,0.0f);

  // Create the transform filter
  m_TransformFilter = vtkTransformPolyDataFilter::New();
  m_TransformFilter->ReleaseDataFlagOn();

  // Create the transform
  m_Transform = vtkTransform::New();

  // Create and configure a filter for triangle decimation
  m_DecimateFilter = vtkDecimatePro::New();
  m_DecimateFilter->ReleaseDataFlagOn();  
}

VTKMeshPipeline
::~VTKMeshPipeline()
{
  // Destroy the filters
  m_VTKImporter->Delete();
  m_VTKGaussianFilter->Delete();
  m_PolygonSmoothingFilter->Delete();
  m_StripperFilter->Delete();

  m_MarchingCubesFilter->Delete();
  m_TransformFilter->Delete();
  m_Transform->Delete();
  m_DecimateFilter->Delete();
}

void
VTKMeshPipeline
::SetMeshOptions(MeshOptions *options)
{
  // Store the options
  m_MeshOptions = options;

  // Reset the weights in the progress meter
  m_Progress->UnregisterAllSources();

  // Define the current pipeline end-point
  vtkAlgorithmOutput *pipeImageTail = m_VTKImporter->GetOutputPort();
  vtkAlgorithmOutput *pipePolyTail = NULL;

  // Route the pipeline according to the settings
  // 1. Check if Gaussian smoothing will be used

  if(options->GetUseGaussianSmoothing())
    {    
    // The Gaussian filter is enabled
    m_VTKGaussianFilter->SetInputConnection(pipeImageTail);
    m_Progress->RegisterSource(m_VTKGaussianFilter, 10.0f);
    pipeImageTail = m_VTKGaussianFilter->GetOutputPort();

    // Apply parameters to the Gaussian filter
    float sigma = options->GetGaussianStandardDeviation();

    // Sigma is in millimeters
    m_VTKGaussianFilter->SetStandardDeviation(sigma, sigma, sigma);

    // TODO: we are ignoring the maximum error parameter!
    m_VTKGaussianFilter->SetRadiusFactors(1.5, 1.5, 1.5);
    }

  // 2. Set input to the appropriate contour filter

  // Marching cubes gets the tail
  m_MarchingCubesFilter->SetInputConnection(pipeImageTail);
  m_Progress->RegisterSource(m_MarchingCubesFilter, 10.0f);
  pipePolyTail = m_MarchingCubesFilter->GetOutputPort();

  // 2.5 Pipe marching cubes output to the transform
  m_TransformFilter->SetInputConnection(pipePolyTail);
  m_Progress->RegisterSource(m_TransformFilter, 1.0f);
  pipePolyTail = m_TransformFilter->GetOutputPort();

  // 3. Check if decimation is required
  if(options->GetUseDecimation())
    {

    // Decimate filter gets the pipe tail
    m_DecimateFilter->SetInputConnection(pipePolyTail);
    m_Progress->RegisterSource(m_DecimateFilter, 5.0);
    pipePolyTail = m_DecimateFilter->GetOutputPort();

    // Apply parameters to the decimation filter
    m_DecimateFilter->SetTargetReduction(
      options->GetDecimateTargetReduction());

    m_DecimateFilter->SetMaximumError(
      options->GetDecimateMaximumError());

    m_DecimateFilter->SetFeatureAngle(
      options->GetDecimateFeatureAngle());

    m_DecimateFilter->SetPreserveTopology(
      options->GetDecimatePreserveTopology());

    } // If decimate enabled

  // 4. Compute the normals (non-patented only)

  // 5. Include/exclude mesh smoothing filter
  if(options->GetUseMeshSmoothing())
    {
    // Pipe smoothed output into the pipeline
    m_PolygonSmoothingFilter->SetInputConnection(pipePolyTail);
    m_Progress->RegisterSource(m_PolygonSmoothingFilter, 3.0);
    pipePolyTail = m_PolygonSmoothingFilter->GetOutputPort();

    // Apply parameters to the mesh smoothing filter
    m_PolygonSmoothingFilter->SetNumberOfIterations(
      options->GetMeshSmoothingIterations());

    m_PolygonSmoothingFilter->SetRelaxationFactor(
      options->GetMeshSmoothingRelaxationFactor());

    m_PolygonSmoothingFilter->SetFeatureAngle(
      options->GetMeshSmoothingFeatureAngle());

    m_PolygonSmoothingFilter->SetFeatureEdgeSmoothing(
      options->GetMeshSmoothingFeatureEdgeSmoothing());

    m_PolygonSmoothingFilter->SetBoundarySmoothing(
      options->GetMeshSmoothingBoundarySmoothing());

    m_PolygonSmoothingFilter->SetConvergence(
      options->GetMeshSmoothingConvergence());
    }

  // 6. Pipe in the final output into the stripper
  m_StripperFilter->SetInputConnection(pipePolyTail);
  m_Progress->RegisterSource(m_StripperFilter, 2.0);
}

#include <ctime>
#include "itkImageFileWriter.h"

void
VTKMeshPipeline
::ComputeMesh(vtkPolyData *outMesh, itk::FastMutexLock *lock)
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
  m_VTKImporter->Modified();

  // Update the importer
  if(lock) lock->Lock();
  m_VTKImporter->Update();
  if(lock) lock->Unlock();

  // Update the pipeline
  m_StripperFilter->Update();

  // In the case that the jacobian of the transform is negative,
  // flip the normals around
  if(m_Transform->GetMatrix()->Determinant() < 0)
    {
    vtkPointData *pd = m_StripperFilter->GetOutput()->GetPointData();
    vtkDataArray *nrm = pd->GetNormals();
    for(size_t i = 0; i < (size_t)nrm->GetNumberOfTuples(); i++)
      for(size_t j = 0; j < (size_t)nrm->GetNumberOfComponents(); j++)
        nrm->SetComponent(i,j,-nrm->GetComponent(i,j));
    nrm->Modified();
    }

  // Disconnect pipeline
  m_StripperFilter->SetOutput(NULL);
}

void
VTKMeshPipeline
::SetImage(ImageType *image)
{
  // Store the image 
  m_InputImage = image;

  // Compute the transform from VTK coordinates to NIFTI/RAS coordinates
  vnl_matrix_fixed<double, 4, 4> vtk2nii = 
    ImageWrapperBase::ConstructVTKtoNiftiTransform(
      image->GetDirection().GetVnlMatrix(),
      image->GetOrigin().GetVnlVector(),
      image->GetSpacing().GetVnlVector());

  // Update the VTK transform to match
  m_Transform->SetMatrix(vtk2nii.data_block());

  // Pass the transform to the transform filter
  m_TransformFilter->SetTransform(m_Transform);
}


