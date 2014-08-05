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
#include "itkFastMutexLock.h"

#include "SmoothBinaryThresholdImageFilter.h"
#include "GlobalState.h"
#include "EdgePreprocessingImageFilter.h"
#include "IRISVectorTypesToITKConversion.h"
#include "SignedDistanceFilter.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "ThresholdSettings.h"
#include "ColorMap.h"
#include "SNAPImageData.h"

#include "SlicePreviewFilterWrapper.h"
#include "PreprocessingFilterConfigTraits.h"


SNAPImageData
::SNAPImageData()
{
  // Set the names of the wrapeprs

  // Initialize the level set driver to NULL
  m_LevelSetDriver = NULL;

  // Set the initial label color
  m_SnakeColorLabel = 0;

  // Create the mutex lock
  m_LevelSetPipelineMutexLock = itk::FastMutexLock::New();
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
  assert(m_MainImageWrapper->IsInitialized());

  // Intialize the speed based on the current grey image
  if(m_SpeedWrapper.IsNull())
    {
    m_SpeedWrapper = SpeedImageWrapper::New();
    m_SpeedWrapper->SetDefaultNickname("Speed Image");
    PushBackImageWrapper(SNAP_ROLE, m_SpeedWrapper.GetPointer());
    }

  m_SpeedWrapper->InitializeToWrapper(m_MainImageWrapper, (GreyType) 0);
  InvokeEvent(LayerChangeEvent());

  // Here or after it's computed?
  m_SpeedWrapper->SetAlpha(1.0);
}

SpeedImageWrapper* 
SNAPImageData
::GetSpeed() 
{
  // Make sure it exists
  assert(IsSpeedLoaded());
  return m_SpeedWrapper;
}

bool 
SNAPImageData
::IsSpeedLoaded() 
{
  return m_SpeedWrapper && m_SpeedWrapper->IsInitialized();
}

LevelSetImageWrapper* 
SNAPImageData
::GetSnake() 
{
  assert(IsSnakeLoaded());
  return m_SnakeWrapper;
}

bool 
SNAPImageData
::IsSnakeLoaded() 
{
  return (m_SnakeWrapper && m_SnakeWrapper->IsInitialized());
}


bool
SNAPImageData
::InitializeSegmentation(
  const SnakeParameters &parameters, 
  const std::vector<Bubble> &bubbles, unsigned int labelColor)
{
  assert(IsSpeedLoaded());

  // Inside/outside values
  const float INSIDE_VALUE = -4.0, OUTSIDE_VALUE = 4.0;
  
  // Store the label color
  m_SnakeColorLabel = labelColor;

  // Types of images used here
  typedef itk::Image<float,3> FloatImageType;

  // If a initialization wrapper does not exist, create it
  if(!m_SnakeWrapper)
    {
    m_SnakeWrapper = LevelSetImageWrapper::New();
    m_SnakeWrapper->SetDefaultNickname("Evolving Contour");
    PushBackImageWrapper(SNAP_ROLE, m_SnakeWrapper.GetPointer());
    }

  // Initialize the level set initialization wrapper, set pixels to OUTSIDE_VALUE
  m_SnakeWrapper->InitializeToWrapper(m_MainImageWrapper, OUTSIDE_VALUE);

  InvokeEvent(LayerChangeEvent());

  // Create the initial level set image by merging the segmentation data from
  // IRIS region with the bubbles
  LabelImageType::Pointer imgInput = m_LabelWrapper->GetImage();
  FloatImageType::Pointer imgLevelSet = m_SnakeWrapper->GetImage();

  // Get the target region. This really should be a region relative to the IRIS image
  // data, not an image into a needless copy of an IRIS region.
  LabelImageType::RegionType region = imgInput->GetBufferedRegion();

  // Create iterators to perform the copy
  typedef itk::ImageRegionConstIterator<LabelImageType> SourceIterator;
  typedef itk::ImageRegionIteratorWithIndex<FloatImageType> TargetIterator;
  SourceIterator itSource(imgInput,region);
  TargetIterator itTarget(imgLevelSet,region);

  // During the copy loop, compute the extents of the initialization
  Vector3l bbLower = region.GetSize();
  Vector3l bbUpper = region.GetIndex();

  unsigned long nInitVoxels = 0;

  // Convert the input label image into a binary function whose 0 level set
  // is the boundary of the current label's region
  while(!itSource.IsAtEnd())
    {
    if(itSource.Value() == m_SnakeColorLabel)
      {
      // Expand the bounding box accordingly
      Vector3l point = itTarget.GetIndex();
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
    bbLower = vector_min(bbLower,Vector3l(idxLower));
    bbUpper = vector_max(bbUpper,Vector3l(idxUpper));

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
    this->RemoveImageWrapper(SNAP_ROLE, m_SnakeWrapper);
    m_SnakeWrapper = NULL;
    InvokeEvent(LayerChangeEvent());
    return false;
    }

  // Make sure that the correct color label is being used
  // TODO: restore this functionality once you figure out how to display
  // level set representations properly !!!
  // m_SnakeInitializationWrapper->SetColorLabel(m_ColorLabel);

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
    m_SpeedWrapper->GetImage()->GetBufferedRegion());
  m_ExternalAdvectionField->Allocate();
  m_ExternalAdvectionField->SetSpacing(
    m_MainImageWrapper->GetImageBase()->GetSpacing());
  m_ExternalAdvectionField->SetOrigin(
    m_MainImageWrapper->GetImageBase()->GetOrigin());

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

  // Enter a thread-safe section
  m_LevelSetPipelineMutexLock->Lock();

  // Initialize the snake driver and pass the parameters
  m_LevelSetDriver = new SNAPLevelSetDriver3d(
    m_SnakeWrapper->GetImage(),
    m_SpeedWrapper->GetImage(),
    m_CurrentSnakeParameters,
    m_ExternalAdvectionField);

  // This makes sure that m_SnakeWrapper->IsDrawable() returns true
  m_SnakeWrapper->SetImage(m_LevelSetDriver->GetCurrentState());
  m_SnakeWrapper->GetImage()->Modified();

  // Finish thread-safe section
  m_LevelSetPipelineMutexLock->Unlock();

  // Fire events (layers changed and level set image changed)
  this->InvokeEvent(LayerChangeEvent());
  this->InvokeEvent(LevelSetImageChangeEvent());

  // Why use segmentation's alpha?
  m_SnakeWrapper->SetAlpha(
        (unsigned char)(255 * m_Parent->GetGlobalState()->GetSegmentationAlpha()));

}

void 
SNAPImageData
::RunSegmentation(unsigned int nIterations)
{
  // Should be in level set mode
  assert(m_LevelSetDriver);

  // Pass through to the level set driver

  // Enter a thread-safe section
  m_LevelSetPipelineMutexLock->Lock();

  // clock_t c1 = clock();
  m_LevelSetDriver->Run(nIterations);
  // clock_t c2 = clock();

  // Leave a thread-safe section
  m_LevelSetPipelineMutexLock->Unlock();

  /*
  std::cout << (c2 - c1) * 1.0 / (CLOCKS_PER_SEC * nIterations)
            << " sec per iteration." << std::endl; */

  // Fire the update event
  this->InvokeEvent(LevelSetImageChangeEvent());
}

bool
SNAPImageData
::IsEvolutionConverged()
{
  // Make the method reentrant
  itk::MutexLockHolder<itk::FastMutexLock> holder(*m_LevelSetPipelineMutexLock);

  return m_LevelSetDriver->IsEvolutionConverged();
}

void 
SNAPImageData
::RestartSegmentation()
{
  // Should be in level set mode
  assert(m_LevelSetDriver);

  // Enter a thread-safe section
  m_LevelSetPipelineMutexLock->Lock();

  // Pass through to the level set driver
  m_LevelSetDriver->Restart();

  // Leave a thread-safe section
  m_LevelSetPipelineMutexLock->Unlock();

  // Fire the update event
  this->InvokeEvent(LevelSetImageChangeEvent());
}

void 
SNAPImageData
::TerminateSegmentation()
{
  // Should be in level set mode
  assert(m_LevelSetDriver);

  // Enter a thread-safe section
  m_LevelSetPipelineMutexLock->Lock();

  // Delete the level set driver and all the problems that go along with it
  delete m_LevelSetDriver; m_LevelSetDriver = NULL;

  // Leave a thread-safe section
  m_LevelSetPipelineMutexLock->Unlock();

  // Fire the update event
  this->InvokeEvent(LevelSetImageChangeEvent());
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

SNAPLevelSetDriver<3>::LevelSetFunctionType *
SNAPImageData
::GetLevelSetFunction()
{
  return m_LevelSetDriver->GetLevelSetFunction();
}

void
SNAPImageData
::InitializeToROI(GenericImageData *source,
                  const SNAPSegmentationROISettings &roi,
                  itk::Command *progressCommand)
{
  // Get the source main wrapper
  ImageWrapperBase *srcMain = source->GetMain();

  // Extract the ROI into a generic type
  SmartPtr<ImageWrapperBase> roiMain = srcMain->ExtractROI(roi, progressCommand);

  // Assign the new wrapper to the target
  this->SetMainImageInternal(roiMain);

  // Copy metadata
  this->CopyLayerMetadata(this->GetMain(), source->GetMain());

  // Repeat all of this for the overlays
  for(LayerIterator lit = source->GetLayers(OVERLAY_ROLE);
      !lit.IsAtEnd(); ++lit)
    {
    // Do the same for all the anatomic wrappers
    SmartPtr<ImageWrapperBase> roiOvl =
        lit.GetLayer()->ExtractROI(roi, progressCommand);

    // Add the overlay
    this->AddOverlayInternal(roiOvl);

    // Copy metadata
    this->CopyLayerMetadata(this->GetLastOverlay(), lit.GetLayer());
    }
}

void SNAPImageData::CopyLayerMetadata(
    ImageWrapperBase *target, ImageWrapperBase *source)
{
  // Nickname
  target->SetDefaultNickname(source->GetNickname());

  // This is a little bit of overhead, but not enough to be a big deal:
  // we just save the display mapping to a Registry and then restore it
  // in the target wrapper.
  Registry folder;
  source->GetDisplayMapping()->Save(folder);
  target->GetDisplayMapping()->Restore(folder);

  // Threshold settings. These should be copied for each scalar component
  if(source->IsScalar())
    {
    target->SetUserData("ThresholdSettings", source->GetUserData("ThresholdSettings"));
    }
  else
    {
    // Copy threshold settings for all the scalar components
    VectorImageWrapperBase *v_source = dynamic_cast<VectorImageWrapperBase *>(source);
    VectorImageWrapperBase *v_target = dynamic_cast<VectorImageWrapperBase *>(target);

    for(ScalarRepresentationIterator it(v_source); !it.IsAtEnd(); ++it)
      {
      ImageWrapperBase *c_source = v_source->GetScalarRepresentation(it);
      ImageWrapperBase *c_target = v_target->GetScalarRepresentation(it);
      c_target->SetUserData("ThresholdSettings", c_source->GetUserData("ThresholdSettings"));
      }
    }

  // TODO: alpha, stickiness?
}


void SNAPImageData::UnloadAll()
{
  // Unload all the data
  this->UnloadOverlays();
  this->UnloadMainImage();

  // We need to unload all the SNAP layers
  while(this->m_Wrappers[SNAP_ROLE].size())
    PopBackImageWrapper(SNAP_ROLE);
  m_SpeedWrapper = NULL;
  m_SnakeWrapper = NULL;

  InvokeEvent(LayerChangeEvent());
}





