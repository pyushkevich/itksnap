/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IRISMeshPipeline.cxx,v $
  Language:  C++
  Date:      $Date: 2010/06/28 18:45:08 $
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
// Borland compiler is very lazy so we need to instantiate the template
//  by hand 
#if defined(__BORLANDC__)
#include "SNAPBorlandDummyTypes.h"
#endif

#include "IRISMeshPipeline.h"

// SNAP includes
#include "IRISVectorTypesToITKConversion.h"
#include "VTKMeshPipeline.h"

// ITK includes
#include "itkRegionOfInterestImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkImageRegionConstIteratorWithIndex.h"

using namespace std;

IRISMeshPipeline
::IRISMeshPipeline()
{
  // Initialize the region of interest filter
  m_ROIFilter = ROIFilter::New();
  m_ROIFilter->ReleaseDataFlagOn();

  // Define the binary thresholding filter that will map the image onto the 
  // range -1 to 1
  m_ThrehsoldFilter = ThresholdFilter::New();
  m_ThrehsoldFilter->SetInput(m_ROIFilter->GetOutput());
  m_ThrehsoldFilter->ReleaseDataFlagOn();
  m_ThrehsoldFilter->SetInsideValue(1.0f);
  m_ThrehsoldFilter->SetOutsideValue(-1.0f);

  // Initialize the VTK Processing Pipeline
  m_VTKPipeline = new VTKMeshPipeline();
  m_VTKPipeline->SetImage(m_ThrehsoldFilter->GetOutput());
}

IRISMeshPipeline
::~IRISMeshPipeline()
{
  delete m_VTKPipeline;
}

void
IRISMeshPipeline
::SetMeshOptions(const MeshOptions &options)
{
  // Store the options
  m_MeshOptions = options;

  // Apply the options to the internal pipeline
  m_VTKPipeline->SetMeshOptions(m_MeshOptions);
}

unsigned long
IRISMeshPipeline
::ComputeBoundingBoxes()
{
  unsigned int i;
  unsigned long nTotalVoxels = 0;

  // Get the dimensions of the image
  InputImageType::SizeType size = 
    m_InputImage->GetLargestPossibleRegion().GetSize();

  // These vectors represent the extents of the image.  
  Vector3l extLower(0l);
  Vector3l extUpper = 
    Vector3l(reinterpret_cast<const long *>(size.GetSize())) - Vector3l(1l);

  // For each intensity present in the image, we will compute a bounding
  // box.  Intialize a list of bounding boxes with 'inverted' boxes
  Vector3l bbMin[MAX_COLOR_LABELS];
  Vector3l bbMax[MAX_COLOR_LABELS];

  // Initialize the histogram and bounding boxes
  for(i=1;i<MAX_COLOR_LABELS;i++)
    {
    m_Histogram[i] = 0l;
    bbMin[i] = extUpper;
    bbMax[i] = extLower;
    }

  // Create an iterator for parsing the image
  typedef itk::ImageRegionConstIteratorWithIndex<InputImageType> InputIterator;
  InputIterator it(m_InputImage,m_InputImage->GetLargestPossibleRegion());

  // Parse through the image using an iterator and compute the bounding boxes
  for(it.GoToBegin();!it.IsAtEnd();++it)
    {
    // Get the intensity at current pixel
    LabelType label = it.Value();

    // For non-zero labels, compute the bounding box
    if(label != 0)
      {
      // Increment the histogram
      m_Histogram[label]++;

      // Get the current image index
      Vector3l point(it.GetIndex().GetIndex());

      // Update the bounding box extents
      bbMin[label] = vector_min(bbMin[label],point);
      bbMax[label] = vector_max(bbMax[label],point);        
      }
    }

  // Convert the bounding box to a region
  for(i=1;i<MAX_COLOR_LABELS;i++)
    {
    Vector3l bbSize = Vector3l(1l) + bbMax[i] - bbMin[i];
    m_BoundingBox[i].SetSize(to_itkSize(bbSize));
    m_BoundingBox[i].SetIndex(to_itkIndex(bbMin[i]));
    nTotalVoxels += m_BoundingBox[i].GetNumberOfPixels();
    }  

  return nTotalVoxels;
}

unsigned long
IRISMeshPipeline
::GetVoxelsInBoundingBox(LabelType label) const
{
  return m_BoundingBox[label].GetNumberOfPixels();
}

AllPurposeProgressAccumulator *
IRISMeshPipeline
::GetProgressAccumulator()
{
  return m_VTKPipeline->GetProgressAccumulator();
}
  

#include <ctime>

bool
IRISMeshPipeline
::ComputeMesh(LabelType label, vtkPolyData *outMesh)
{
  // The label must be present in the image
  if(m_Histogram[label] == 0)
    return false;

  // TODO: make this more elegant
  InputImageType::RegionType bbWiderRegion = m_BoundingBox[label];
  bbWiderRegion.PadByRadius(5);
  bbWiderRegion.Crop(m_InputImage->GetLargestPossibleRegion()); 

  // Pass the region to the ROI filter and propagate the filter
  m_ROIFilter->SetInput(m_InputImage);
  m_ROIFilter->SetRegionOfInterest(bbWiderRegion);
  m_ROIFilter->Update();
  
  // Set the parameters for the thresholding filter
  m_ThrehsoldFilter->SetLowerThreshold(label);
  m_ThrehsoldFilter->SetUpperThreshold(label);
  m_ThrehsoldFilter->UpdateLargestPossibleRegion();

  // Graft the polydata to the last filter in the pipeline
  m_VTKPipeline->SetImage(m_ThrehsoldFilter->GetOutput());
  m_VTKPipeline->ComputeMesh(outMesh);

  // Done
  return true;
}

void 
IRISMeshPipeline
::SetImage(IRISMeshPipeline::InputImageType *image)
{
  m_InputImage = image;
}

