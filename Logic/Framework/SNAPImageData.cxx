/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPImageData.cxx,v $
  Language:  C++
  Date:      $Date: 2011/04/18 17:35:30 $
  Version:   $Revision: 1.11 $
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

#include "SNAPLevelSetDriver.h"
#include "itkGroupSpatialObject.h"
#include "itkEllipseSpatialObject.h"
#include "itkSpatialObjectToImageFilter.h"
#include "itkMaximumImageFilter.h"
#include "itkDanielssonDistanceMapImageFilter.h"
#include "itkSubtractImageFilter.h"
#include "itkUnaryFunctorImageFilter.h"

#include "GlobalState.h"
#include "EdgePreprocessingImageFilter.h"
#include "IRISVectorTypesToITKConversion.h"
#include "LabelImageWrapper.h"
#include "LevelSetImageWrapper.h"
#include "SignedDistanceFilter.h"
#include "SmoothBinaryThresholdImageFilter.h"
#include "SpeedImageWrapper.h"


#include "SNAPImageData.h"


SNAPImageData
::SNAPImageData(IRISApplication *parent)
: GenericImageData(parent)
{
  // Update the list of linked wrappers
  m_MainWrappers.push_back(&m_SpeedWrapper);
  m_MainWrappers.push_back(&m_SnakeInitializationWrapper);
  m_MainWrappers.push_back(&m_SnakeWrapper);

  // Initialize the level set driver to NULL
  m_LevelSetDriver = NULL;

  // Set the initial label color
  m_SnakeColorLabel = 0;
}


SNAPImageData
::~SNAPImageData() 
{
  if(m_LevelSetDriver)
    delete m_LevelSetDriver;
}

void 
SNAPImageData
::InitializeSpeed()
{
  // The Grey image wrapper should be present
  assert(m_GreyWrapper.IsInitialized());

  // Intialize the speed based on the current grey image
  m_SpeedWrapper.InitializeToWrapper(&m_GreyWrapper, 0.0f);
}

void 
SNAPImageData
::DoEdgePreprocessing(const EdgePreprocessingSettings &settings,
                      itk::Command *progressCallback)
{ 
  // Define an edge filter to use for preprocessing
  typedef EdgePreprocessingImageFilter<
    GreyImageWrapper::ImageType,SpeedImageWrapper::ImageType> FilterType;
  
  // Configure the edge filter
  FilterType::Pointer filter = FilterType::New();
  
  // Pass the settings to the filter
  filter->SetEdgePreprocessingSettings(settings);

  // Set the filter's input
  filter->SetInput(m_GreyWrapper.GetImage());

  // Provide a progress callback (if one is provided)
  if(progressCallback)
    filter->AddObserver(itk::ProgressEvent(),progressCallback);

  // Run the filter on the whole image
  filter->UpdateLargestPossibleRegion();
  
  // Pass the output of the filter to the speed wrapper
  m_SpeedWrapper.SetImage(filter->GetOutput());
  
  // Dismantle this pipeline
  m_SpeedWrapper.GetImage()->DisconnectPipeline();
}

void 
SNAPImageData
::DoInOutPreprocessing(const ThresholdSettings &settings,
                       itk::Command *progressCallback)
{
  // Define an edge filter to use for preprocessing
  typedef SmoothBinaryThresholdImageFilter<
    GreyImageWrapper::ImageType,SpeedImageWrapper::ImageType> FilterType;
  
  // Create an edge filter for whole-image preprocessing
  FilterType::Pointer filter = FilterType::New();
  
  // Pass the settings to the filter
  filter->SetThresholdSettings(settings);

  // Set the filter's input
  filter->SetInput(m_GreyWrapper.GetImage());

  // Provide a progress callback (if one is provided)
  if(progressCallback)
    filter->AddObserver(itk::ProgressEvent(),progressCallback);

  // Run the filter
  filter->UpdateLargestPossibleRegion();
  
  // Pass the output of the filter to the speed wrapper
  m_SpeedWrapper.SetImage(filter->GetOutput());
  
  // Dismantle this pipeline
  m_SpeedWrapper.GetImage()->DisconnectPipeline();
}

SpeedImageWrapper* 
SNAPImageData
::GetSpeed() 
{
  // Make sure it exists
  assert(m_SpeedWrapper.IsInitialized());
  return &m_SpeedWrapper;
}

bool 
SNAPImageData
::IsSpeedLoaded() 
{
  return m_SpeedWrapper.IsInitialized();
}

LevelSetImageWrapper* 
SNAPImageData
::GetSnakeInitialization() 
{
  assert(m_SnakeInitializationWrapper.IsInitialized());
  return &m_SnakeInitializationWrapper;
}

bool 
SNAPImageData
::IsSnakeInitializationLoaded() 
{
  return (m_SnakeInitializationWrapper.IsInitialized());
}

LevelSetImageWrapper* 
SNAPImageData
::GetSnake() 
{
  assert(m_SnakeWrapper.IsInitialized());
  return &m_SnakeWrapper;
}

bool 
SNAPImageData
::IsSnakeLoaded() 
{
  return (m_SnakeWrapper.IsInitialized());
}

bool
SNAPImageData
::InitializeSegmentation(
  const SnakeParameters &parameters, 
  const std::vector<Bubble> &bubbles, unsigned int labelColor)
{
  assert(m_SpeedWrapper.IsInitialized());

  // Inside/outside values
  const float INSIDE_VALUE = -1.0, OUTSIDE_VALUE = 1.0;
  
  // Store the label color
  m_SnakeColorLabel = labelColor;

  // Types of images used here
  typedef itk::Image<float,3> FloatImageType;

  // Initialize the level set initialization wrapper, set pixels to OUTSIDE_VALUE
  m_SnakeInitializationWrapper.InitializeToWrapper(&m_GreyWrapper, OUTSIDE_VALUE);

  // Create the initial level set image by merging the segmentation data from
  // IRIS region with the bubbles
  LabelImageType::Pointer imgInput = m_LabelWrapper.GetImage();
  FloatImageType::Pointer imgLevelSet = m_SnakeInitializationWrapper.GetImage();

  // Get the target region. This really should be a region relative to the IRIS image
  // data, not an image into a needless copy of an IRIS region.
  LabelImageType::RegionType region = imgInput->GetBufferedRegion();

  // Create iterators to perform the copy
  typedef itk::ImageRegionConstIterator<LabelImageType> SourceIterator;
  typedef itk::ImageRegionIteratorWithIndex<FloatImageType> TargetIterator;
  SourceIterator itSource(imgInput,region);
  TargetIterator itTarget(imgLevelSet,region);

  // During the copy loop, compute the extents of the initialization
  Vector3l bbLower(reinterpret_cast<const long *>(region.GetSize().GetSize()));
  Vector3l bbUpper(reinterpret_cast<const long *>(region.GetIndex().GetIndex()));

  unsigned long nInitVoxels = 0;

  // Convert the input label image into a binary function whose 0 level set
  // is the boundary of the current label's region
  while(!itSource.IsAtEnd())
    {
    if(itSource.Value() == m_SnakeColorLabel)
      {
      // Expand the bounding box accordingly
      Vector3l point(itTarget.GetIndex().GetIndex());
      bbLower = vector_min(bbLower,point);
      bbUpper = vector_max(bbUpper,point);
      
      // Increase the number of initialization voxels
      nInitVoxels++;

      // Set the target value to inside
      itTarget.Value() = INSIDE_VALUE;
      }

    // Go to the next pixel
    ++itTarget; ++itSource;
    }

  // Fill in the bubbles by computing their
  for(unsigned int iBubble=0; iBubble < bubbles.size(); iBubble++)
    {
    // Compute the extents of the bubble
    typedef itk::Point<double,3> PointType;
    PointType ptLower,ptUpper,ptCenter;

    // Compute the physical position of the bubble center
    imgLevelSet->TransformIndexToPhysicalPoint(
      to_itkIndex(bubbles[iBubble].center),ptCenter);

    // Extents of the bounding box
    FloatImageType::IndexType idxLower = to_itkIndex(bubbles[iBubble].center);
    FloatImageType::IndexType idxUpper = to_itkIndex(bubbles[iBubble].center);

    // Map all vertices in a cube of radius r around the physical center of
    // the bubble into index space, and compute a bounding box
    for(int jx=-1; jx<=1; jx+=2) for(int jy=-1; jy<=1; jy+=2) for(int jz=-1; jz<=1; jz+=2)
      {
      PointType ptTest;
      ptTest[0] = ptCenter[0] + jx * bubbles[iBubble].radius;
      ptTest[1] = ptCenter[1] + jy * bubbles[iBubble].radius;
      ptTest[2] = ptCenter[2] + jz * bubbles[iBubble].radius;

      FloatImageType::IndexType idxTest;
      imgLevelSet->TransformPhysicalPointToIndex(ptTest,idxTest);

      for(unsigned int k=0; k<3; k++)
        {
        if(idxLower[k] > idxTest[k]) 
          idxLower[k] = idxTest[k];
        if(idxUpper[k] < idxTest[k]) 
          idxUpper[k] = idxTest[k];
        }
      }

    // Create a region
    FloatImageType::SizeType szBubble;
    szBubble[0] = 1 + idxUpper[0] - idxLower[0];
    szBubble[1] = 1 + idxUpper[1] - idxLower[1];
    szBubble[2] = 1 + idxUpper[2] - idxLower[2];
    FloatImageType::RegionType regBubble(idxLower,szBubble);
    regBubble.Crop(region);

    // Stretch the overall bounding box if necessary
    bbLower = vector_min(bbLower,Vector3l(idxLower.GetIndex()));
    bbUpper = vector_max(bbUpper,Vector3l(idxUpper.GetIndex()));

    // Create an iterator with an index to fill out the bubble
    TargetIterator itThisBubble(imgLevelSet, regBubble);

    // Need the squared radius for this
    float r2 = bubbles[iBubble].radius * bubbles[iBubble].radius;

    // Fill in the bubble
    while(!itThisBubble.IsAtEnd())
      {
      PointType pt; 
      imgLevelSet->TransformIndexToPhysicalPoint(itThisBubble.GetIndex(),pt);
      
      if(pt.SquaredEuclideanDistanceTo(ptCenter) <= r2)
        {
        itThisBubble.Value() = INSIDE_VALUE;
        nInitVoxels++;
        }

      ++itThisBubble;
      }
    }

  // At this point, we should have an initialization image and a bounding
  // box in bbLower and bbUpper.  End the routine if there are no initialization
  // voxels
  if (nInitVoxels == 0) 
    {
    m_SnakeInitializationWrapper.Reset();
    return false;
    }

  // Make sure that the correct color label is being used
  m_SnakeInitializationWrapper.SetColorLabel(m_ColorLabel);

  // Initialize the snake driver
  InitalizeSnakeDriver(parameters);

  // Success
  return true;
}

void
SNAPImageData
::SetExternalAdvectionField( 
  FloatImageType *imgX, FloatImageType *imgY, FloatImageType *imgZ)
{
  m_ExternalAdvectionField = VectorImageType::New();
  m_ExternalAdvectionField->SetRegions(
    m_SpeedWrapper.GetImage()->GetBufferedRegion());
  m_ExternalAdvectionField->Allocate();
  m_ExternalAdvectionField->SetSpacing(
    m_GreyWrapper.GetImage()->GetSpacing());
  m_ExternalAdvectionField->SetOrigin(
    m_GreyWrapper.GetImage()->GetOrigin());

  typedef itk::ImageRegionConstIterator<FloatImageType> Iterator;
  Iterator itX(imgX,imgX->GetBufferedRegion());
  Iterator itY(imgY,imgY->GetBufferedRegion());
  Iterator itZ(imgZ,imgZ->GetBufferedRegion());

  typedef itk::ImageRegionIterator<VectorImageType> Vectorator;
  Vectorator itTarget(
    m_ExternalAdvectionField,
    m_ExternalAdvectionField->GetBufferedRegion());

  while(!itTarget.IsAtEnd())
    {
    VectorType v;
    
    v[0] = itX.Get();
    v[1] = itY.Get();
    v[2] = itZ.Get();

    itTarget.Set(v);
    
    ++itTarget; 
    ++itX; ++itY; ++itZ;
    }
}
  

void 
SNAPImageData
::InitalizeSnakeDriver(const SnakeParameters &p) 
{
  // Create a new level set driver, deleting the current one if it's there
  if (m_LevelSetDriver) { delete m_LevelSetDriver; }
    
  // This is a good place to check that the parameters are valid
  if(p.GetSnakeType()  == SnakeParameters::REGION_SNAKE)
    {
    // There is no advection 
    assert(p.GetAdvectionWeight() == 0);

    // There is no curvature speed
    assert(p.GetCurvatureSpeedExponent() == -1);

    // Propagation is modulated by probability
    assert(p.GetPropagationSpeedExponent() == 1);

    // There is no smoothing speed
    assert(p.GetLaplacianSpeedExponent() == 0);
    }

  // Copy the configuration parameters
  m_CurrentSnakeParameters = p;

  // Initialize the snake driver and pass the parameters
  m_LevelSetDriver = new SNAPLevelSetDriver3d(
    m_SnakeInitializationWrapper.GetImage(),
    m_SpeedWrapper.GetImage(),
    m_CurrentSnakeParameters,
    m_ExternalAdvectionField);

  // Initialize the level set wrapper with the image from the level set 
  // driver and other settings from the other wrappers
  m_SnakeWrapper.InitializeToWrapper(
    &m_GreyWrapper,m_LevelSetDriver->GetCurrentState());
  m_SnakeWrapper.GetImage()->SetOrigin( m_GreyWrapper.GetImage()->GetOrigin() );
  m_SnakeWrapper.GetImage()->SetSpacing( m_GreyWrapper.GetImage()->GetSpacing() );
  
  // Make sure that the correct color label is being used
  m_SnakeWrapper.SetColorLabel(m_ColorLabel);
}

void 
SNAPImageData
::RunSegmentation(unsigned int nIterations)
{
  // Should be in level set mode
  assert(m_LevelSetDriver);

  // Pass through to the level set driver
  m_LevelSetDriver->Run(nIterations);
}

void 
SNAPImageData
::RestartSegmentation()
{
  // Should be in level set mode
  assert(m_LevelSetDriver);

  // Pass through to the level set driver
  m_LevelSetDriver->Restart();

  // Update the image pointed to by the snake wrapper
  m_SnakeWrapper.SetImage(m_LevelSetDriver->GetCurrentState());
}

void 
SNAPImageData
::TerminateSegmentation()
{
  // Should be in level set mode
  assert(m_LevelSetDriver);

  // Delete the level set driver and all the problems that go along with it
  delete m_LevelSetDriver; m_LevelSetDriver = NULL;
}

void 
SNAPImageData
::SetSegmentationParameters(const SnakeParameters &parameters)
{
  // Should be in level set mode
  assert(m_LevelSetDriver);

  // Pass through to the level set driver
  m_LevelSetDriver->SetSnakeParameters(parameters);
}

unsigned int 
SNAPImageData::
GetElapsedSegmentationIterations() const
{
  return m_LevelSetDriver->GetElapsedIterations();
}

SNAPImageData::LevelSetImageType *
SNAPImageData
::GetLevelSetImage()
{
  assert(m_LevelSetDriver);
  return m_LevelSetDriver->GetCurrentState();
}

SNAPLevelSetFunction<SpeedImageWrapper::ImageType> *
SNAPImageData
::GetLevelSetFunction()
{
  return m_LevelSetDriver->GetLevelSetFunction();
}

