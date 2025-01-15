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
#include "SNAPExportITKToVTK.h"
#include <map>
#include <vtkCellArrayIterator.h>
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCellData.h"

using namespace std;

/**
 * Simple and fast VTK filter to swap the orientation of faces in a VTK polydata,
 * used when applying a spatial transform that introduces a flip.
 */
class vtkFlipPolyFaces : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkFlipPolyFaces, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override
  {
    os << indent << "Flip Faces: " << (this->FlipFaces ? "On\n" : "Off\n");
  }

  static vtkFlipPolyFaces *New();

  vtkSetMacro(FlipFaces, vtkTypeBool);
  vtkGetMacro(FlipFaces, vtkTypeBool);
  vtkBooleanMacro(FlipFaces, vtkTypeBool);

protected:
  vtkFlipPolyFaces() { this->FlipFaces = 0; }
  ~vtkFlipPolyFaces() override = default;

  vtkTypeBool FlipFaces;

  int RequestData(vtkInformation        *vtkNotUsed(request),
                  vtkInformationVector **inputVector,
                  vtkInformationVector  *outputVector) override
  {
    // get the info objects
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation* outInfo = outputVector->GetInformationObject(0);

    // get the input and output
    vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    // Copy the points and the structure
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());

    // iterate over the polys
    if(FlipFaces)
    {
      // Change the orientation of all the polys
      vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
      vtkSmartPointer<vtkCellArray> trg = vtkCellArray::New();
      vtkCellArray *src = input->GetPolys();
      trg->AllocateEstimate(src->GetNumberOfCells(), src->GetMaxCellSize());
      for (vtkSmartPointer<vtkCellArrayIterator> it = src->NewIterator(); !it->IsDoneWithTraversal();
           it->GoToNextCell())
      {
        it->GetCurrentCell(idList);
        vtkIdType numPoints = idList->GetNumberOfIds();
        if (numPoints >= 3)
        {
          for (vtkIdType i = 0; i < numPoints / 2; ++i)
          {
            vtkIdType temp = idList->GetId(i);
            idList->SetId(i, idList->GetId(numPoints - i - 1));
            idList->SetId(numPoints - i - 1, temp);
          }
        }
        trg->InsertNextCell(idList);
      }
      output->SetPolys(trg);

      return 1;
    }
    else
    {
      output->GetCellData()->PassData(input->GetCellData());
      return 1;
    }
  }

private:
  vtkFlipPolyFaces(const vtkFlipPolyFaces&) = delete;
  void operator=(const vtkFlipPolyFaces&) = delete;
};

vtkStandardNewMacro(vtkFlipPolyFaces);



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
  ConnectITKExporterToVTKImporter(m_VTKExporter.GetPointer(), m_VTKImporter);

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

  // Face flip filter - much faster than vtkPolyDataNormals
  m_FlipPolyFaces = vtkFlipPolyFaces::New();

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
  m_Pipeline.clear();
  m_Pipeline.push_back(m_VTKImporter);

  // Route the pipeline according to the settings
  // 1. Check if Gaussian smoothing will be used

  if(options->GetUseGaussianSmoothing())
    {    
    // The Gaussian filter is enabled
    m_VTKGaussianFilter->SetInputConnection(m_Pipeline.back()->GetOutputPort());
    m_Progress->RegisterSource(m_VTKGaussianFilter, 10.0f);
    m_Pipeline.push_back(m_VTKGaussianFilter);

    // Apply parameters to the Gaussian filter
    float sigma = options->GetGaussianStandardDeviation();

    // Sigma is in millimeters
    m_VTKGaussianFilter->SetStandardDeviation(sigma, sigma, sigma);

    // TODO: we are ignoring the maximum error parameter!
    m_VTKGaussianFilter->SetRadiusFactors(1.5, 1.5, 1.5);
    }

  // 2. Set input to the appropriate contour filter

  // Marching cubes gets the tail
  m_MarchingCubesFilter->SetInputConnection(m_Pipeline.back()->GetOutputPort());
  m_Progress->RegisterSource(m_MarchingCubesFilter, 10.0f);
  m_Pipeline.push_back(m_MarchingCubesFilter);

  // 2.5 Pipe marching cubes output to the transform
  m_TransformFilter->SetInputConnection(m_Pipeline.back()->GetOutputPort());
  m_Progress->RegisterSource(m_TransformFilter, 1.0f);
  m_Pipeline.push_back(m_TransformFilter);

  // 2.8 Normals
  m_FlipPolyFaces->SetInputConnection(m_Pipeline.back()->GetOutputPort());
  m_Progress->RegisterSource(m_FlipPolyFaces, 1.0f);
  m_Pipeline.push_back(m_FlipPolyFaces);

  // 3. Check if decimation is required
  if(options->GetUseDecimation())
    {

    // Decimate filter gets the pipe tail
    m_DecimateFilter->SetInputConnection(m_Pipeline.back()->GetOutputPort());
    m_Progress->RegisterSource(m_DecimateFilter, 5.0);
    m_Pipeline.push_back(m_DecimateFilter);

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
    m_PolygonSmoothingFilter->SetInputConnection(m_Pipeline.back()->GetOutputPort());
    m_Progress->RegisterSource(m_PolygonSmoothingFilter, 3.0);
    m_Pipeline.push_back(m_PolygonSmoothingFilter);

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
  m_StripperFilter->SetInputConnection(m_Pipeline.back()->GetOutputPort());
  m_Progress->RegisterSource(m_StripperFilter, 2.0);
  m_Pipeline.push_back(m_StripperFilter);
}

#include <ctime>

void
VTKMeshPipeline ::ComputeMesh(vtkPolyData *outMesh, long mesh_id, std::mutex *mutex)
{
  // Reset the progress meter
  m_Progress->ResetProgress();

  // Graft the polydata to the last filter in the pipeline
  m_StripperFilter->SetOutput(outMesh);

  // Connect importer and exporter
  m_VTKImporter->SetCallbackUserData(m_VTKExporter->GetCallbackUserData());

  // Update the ITK portion of the pipeline
  m_VTKExporter->SetInput(m_InputImage);
  m_VTKImporter->Modified();

  // Check if the face flipping should be done
  m_FlipPolyFaces->SetFlipFaces(m_Transform->GetMatrix()->Determinant() < 0);

  // Run and time each portion of the pipeline
  std::cout << "VTKMeshPipeline::ComputeMesh "
            << " label " << mesh_id << " timings: " << std::endl;
  for (auto *algorithm : m_Pipeline)
  {
    if (algorithm == m_VTKImporter && mutex)
      mutex->lock();

    auto t0 = std::chrono::system_clock::now();
    algorithm->Update();
    auto t1 = std::chrono::system_clock::now();

    if (algorithm == m_VTKImporter && mutex)
      mutex->unlock();

    // TODO: collect this timing information somewhere we can show to the user
    // const std::chrono::duration<double, std::milli> fp_ms_t = t1 - t0;
    // std::cout << "  " << std::setw(40) << algorithm->GetClassName() << ": " << fp_ms_t.count()
    //          << " ms." << std::endl;
  }

  // In the case that the jacobian of the transform is negative,
  // flip the normals around
  auto *last_filter = static_cast<vtkPolyDataAlgorithm *>(m_Pipeline.back());

  // Disconnect pipeline
  last_filter->SetOutput(nullptr);
}

void
VTKMeshPipeline ::SetImage(const ImageType *image)
{
  // Store the image
  m_InputImage = image;

  // Compute the transform from VTK coordinates to NIFTI/RAS coordinates
  vnl_matrix_fixed<double, 4, 4> vtk2nii =
    ImageWrapperBase::ConstructVTKtoNiftiTransform(image->GetDirection().GetVnlMatrix().as_ref(),
                                                   image->GetOrigin().GetVnlVector(),
                                                   image->GetSpacing().GetVnlVector());

  // Update the VTK transform to match
  m_Transform->SetMatrix(vtk2nii.data_block());

  // Pass the transform to the transform filter
  m_TransformFilter->SetTransform(m_Transform);
}
