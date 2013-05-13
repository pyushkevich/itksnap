/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IRISApplication.cxx,v $
  Language:  C++
  Date:      $Date: 2011/04/18 17:35:30 $
  Version:   $Revision: 1.37 $
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

#include "IRISException.h"
#include "IRISApplication.h"
#include "GlobalState.h"
#include "GuidedNativeImageIO.h"
#include "IRISImageData.h"
#include "IRISVectorTypesToITKConversion.h"
#include "SNAPImageData.h"
#include "MeshObject.h"
#include "MeshExportSettings.h"
#include "SegmentationStatistics.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkPasteImageFilter.h"
#include "itkImageRegionIterator.h"
#include "itkIdentityTransform.h"
#include "itkResampleImageFilter.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkBSplineInterpolateImageFunction.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkWindowedSincInterpolateImageFunction.h"
#include "itkImageFileWriter.h"
#include "itkFlipImageFilter.h"
#include <itksys/SystemTools.hxx>
#include "vtkAppendPolyData.h"
#include "vtkUnsignedShortArray.h"
#include "vtkPointData.h"
#include "SNAPRegistryIO.h"
#include "SNAPEventListenerCallbacks.h"
#include "HistoryManager.h"
#include "IRISSlicer.h"
#include "EdgePreprocessingSettings.h"
#include "ThresholdSettings.h"
#include "SlicePreviewFilterWrapper.h"
#include "PreprocessingFilterConfigTraits.h"
#include "SmoothBinaryThresholdImageFilter.h"
#include "EdgePreprocessingImageFilter.h"
#include "UnsupervisedClustering.h"



#include <stdio.h>
#include <sstream>
#include <iomanip>

// TODO: stick these traits some


IRISApplication
::IRISApplication() 
: m_UndoManager(4,200000)
{
  // Create a new system interface
  m_SystemInterface = new SystemInterface();
  m_HistoryManager = m_SystemInterface->GetHistoryManager();

  // Initialize the color table
  m_ColorLabelTable = ColorLabelTable::New();

  // Contruct the IRIS and SNAP data objects
  m_IRISImageData = IRISImageData::New();
  m_IRISImageData->SetParent(this);

  m_SNAPImageData = SNAPImageData::New();
  m_SNAPImageData->SetParent(this);

  // Set the current IRIS pointer
  m_CurrentImageData = m_IRISImageData.GetPointer();

  // Listen to events from wrappers and image data objects and refire them
  // as our own events.
  AddListener(m_IRISImageData, WrapperMetadataChangeEvent(),
              this, &Self::RefireEvent);

  AddListener(m_SNAPImageData, WrapperMetadataChangeEvent(),
              this, &Self::RefireEvent);

  AddListener(m_SNAPImageData, LevelSetImageChangeEvent(),
              this, &Self::RefireEvent);

  // Construct new global state object
  m_GlobalState = new GlobalState(this);

  // Initialize the display-anatomy transformation with RPI code
  m_DisplayToAnatomyRAI[0] = "RPS";
  m_DisplayToAnatomyRAI[1] = "AIR";
  m_DisplayToAnatomyRAI[2] = "RIP";

  // Initialize the preprocessing settings
  m_ThresholdSettings = ThresholdSettings::New();
  m_EdgePreprocessingSettings = EdgePreprocessingSettings::New();

  // Initialize the preprocessing filter preview wrappers
  m_ThresholdPreviewWrapper = ThresholdPreviewWrapperType::New();
  m_ThresholdPreviewWrapper->SetParameters(m_ThresholdSettings);

  m_EdgePreviewWrapper = EdgePreprocessingPreviewWrapperType::New();
  m_EdgePreviewWrapper->SetParameters(m_EdgePreprocessingSettings);

  m_PreprocessingMode = PREPROCESS_NONE;
}


bool
IRISApplication
::IsImageOrientationOblique()
{
  assert(m_CurrentImageData->IsMainLoaded());
  return ImageCoordinateGeometry::IsDirectionMatrixOblique(
    m_CurrentImageData->GetImageGeometry().GetImageDirectionCosineMatrix());
}


std::string
IRISApplication::
GetImageToAnatomyRAI()
{
  assert(m_CurrentImageData->IsMainLoaded());
  return ImageCoordinateGeometry::ConvertDirectionMatrixToClosestRAICode(
    m_CurrentImageData->GetImageGeometry().GetImageDirectionCosineMatrix());
}


std::string
IRISApplication::
GetDisplayToAnatomyRAI(unsigned int slice)
{
  return m_DisplayToAnatomyRAI[slice];
}


IRISApplication
::~IRISApplication() 
{
  delete m_GlobalState;
  delete m_SystemInterface;
}

void IRISApplication::RefireEvent(itk::Object *src, const itk::EventObject &event)
{
  // Fire the event down the line
  itk::EventObject *ev = event.MakeObject();
  std::cout << ev->GetEventName() << std::endl;
  this->InvokeEvent(*ev);
  delete ev;
}

void 
IRISApplication
::InitializeSNAPImageData(const SNAPSegmentationROISettings &roi,
                          CommandType *progressCommand)
{
  assert(m_IRISImageData->IsMainLoaded());

  // Create the SNAP image data object
  m_SNAPImageData->InitializeToROI(m_IRISImageData, roi, progressCommand);
  
  // Override the interpolator in ROI for label interpolation, or we will get
  // nonsense
  SNAPSegmentationROISettings roiLabel = roi;
  roiLabel.SetInterpolationMethod(SNAPSegmentationROISettings::NEAREST_NEIGHBOR);

  // Get chunk of the label image
  LabelImageType::Pointer imgNewLabel = 
    m_IRISImageData->GetSegmentation()->DeepCopyRegion(roiLabel,progressCommand);

  // Filter the segmentation image to only allow voxels of 0 intensity and 
  // of the current drawing color
  LabelType passThroughLabel = m_GlobalState->GetDrawingColorLabel();

  typedef itk::ImageRegionIterator<LabelImageType> IteratorType;
  IteratorType itLabel(imgNewLabel,imgNewLabel->GetBufferedRegion());
  unsigned int nCopied = 0;
  while(!itLabel.IsAtEnd())
    {
    if(itLabel.Value() != passThroughLabel)
      itLabel.Value() = (LabelType) 0;
    else
      nCopied++;
    ++itLabel;
    }

  // Record whether the segmentation has any values that are not zero
  m_GlobalState->SetSnakeInitializedWithManualSegmentation(nCopied > 0);

  // Pass the cleaned up segmentation image to SNAP
  m_SNAPImageData->SetSegmentationImage(imgNewLabel);

  // Pass the label description of the drawing label to the SNAP image data
  m_SNAPImageData->SetColorLabel(
    m_ColorLabelTable->GetColorLabel(passThroughLabel));

  // Initialize the speed image of the SNAP image data
  m_SNAPImageData->InitializeSpeed();

  // Remember the ROI object
  m_GlobalState->SetSegmentationROISettings(roi);

  // TODO: unify how mode-specific parameters are set
  m_GlobalState->SetSnakeParameters(
        SnakeParameters::GetDefaultInOutParameters());

  // Indicate that the speed image is invalid
  m_GlobalState->SetSpeedValid(false);

  // The set of layers has changed
  InvokeEvent(LayerChangeEvent());
}

void 
IRISApplication
::SetDisplayToAnatomyRAI(const char *rai0,const char *rai1,const char *rai2)
{
  // Store the new RAI code
  m_DisplayToAnatomyRAI[0] = rai0;
  m_DisplayToAnatomyRAI[1] = rai1;
  m_DisplayToAnatomyRAI[2] = rai2;

  if(!m_IRISImageData->IsMainLoaded())
    return;
  
  // Create the appropriate transform and pass it to the IRIS data
  m_IRISImageData->SetImageGeometry(
    ImageCoordinateGeometry(
      m_IRISImageData->GetImageGeometry().GetImageDirectionCosineMatrix(),
      m_DisplayToAnatomyRAI,
      m_IRISImageData->GetVolumeExtents()));

  // Do the same for the SNAP data if needed
  if(!m_SNAPImageData->IsMainLoaded())
    return;

  // Create the appropriate transform and pass it to the SNAP data
  m_SNAPImageData->SetImageGeometry(
    ImageCoordinateGeometry(
      m_SNAPImageData->GetImageGeometry().GetImageDirectionCosineMatrix(),
      m_DisplayToAnatomyRAI,
          m_SNAPImageData->GetVolumeExtents()));

  // Invoke the corresponding event
  InvokeEvent(DisplayToAnatomyCoordinateMappingChangeEvent());
}


void 
IRISApplication
::UpdateSNAPSpeedImage(SpeedImageType *newSpeedImage, 
                       SnakeType snakeMode)
{
  // This has to happen in SNAP mode
  assert(IsSnakeModeActive());

  // Make sure the dimensions of the speed image are appropriate
  assert(to_itkSize(m_SNAPImageData->GetMain()->GetSize())
    == newSpeedImage->GetBufferedRegion().GetSize());

  // Initialize the speed wrapper
  if(!m_SNAPImageData->IsSpeedLoaded())
    m_SNAPImageData->InitializeSpeed();
  
  // Send the speed image to the image data
  m_SNAPImageData->GetSpeed()->SetImage(newSpeedImage);

  // Save the snake mode 
  m_GlobalState->SetSnakeType(snakeMode);

  // Set the speed as valid
  m_GlobalState->SetSpeedValid(true);

  // Set the snake state
  // TODO: fix this!
  if(snakeMode == EDGE_SNAKE)
    {
    // m_SNAPImageData->GetSpeed()->SetModeToEdgeSnake();
    }
  else
    {
     // m_SNAPImageData->GetSpeed()->SetModeToInsideOutsideSnake();
    }
}

void
IRISApplication
::UnloadOverlays()
{
  // unload all the overlays
  m_IRISImageData->UnloadOverlays();

  // Fire event
  InvokeEvent(LayerChangeEvent());
}

void
IRISApplication
::UnloadOverlayLast()
{
  // unload the last overlay
  m_IRISImageData->UnloadOverlayLast();

  // for overlay, we don't want to change the cursor location
  // just force the IRISSlicer to update
  m_IRISImageData->SetCrosshairs(m_GlobalState->GetCrosshairsPosition());

  // Fire event
  InvokeEvent(LayerChangeEvent());
}

void
IRISApplication
::ClearIRISSegmentationImage()
{
  // This has to happen in 'pure' IRIS mode
  assert(!IsSnakeModeActive());

  // Fill the image with blanks
  this->m_IRISImageData->GetSegmentation()->GetImage()->FillBuffer(0);
  this->m_IRISImageData->GetSegmentation()->GetImage()->Modified();

  // Fill the undo image with blanks too
  this->m_IRISImageData->GetUndoImage()->GetImage()->FillBuffer(0);
  this->m_IRISImageData->GetUndoImage()->GetImage()->Modified();

  // Clear the undo buffer
  m_UndoManager.Clear();
}

void
IRISApplication
::UpdateIRISSegmentationImage(GuidedNativeImageIO *io)
{
  // This has to happen in 'pure' IRIS mode
  assert(!IsSnakeModeActive());

  // Cast the image to label type
  CastNativeImage<LabelImageType> caster;
  LabelImageType::Pointer imgLabel = caster(io);
  
  // The header of the label image is made to match that of the grey image
  imgLabel->SetOrigin(m_CurrentImageData->GetMain()->GetImageBase()->GetOrigin());
  imgLabel->SetSpacing(m_CurrentImageData->GetMain()->GetImageBase()->GetSpacing());
  imgLabel->SetDirection(m_CurrentImageData->GetMain()->GetImageBase()->GetDirection());

  // Update the iris data
  m_IRISImageData->SetSegmentationImage(imgLabel); 

  // Update filenames
  m_IRISImageData->GetSegmentation()->SetFileName(io->GetFileNameOfNativeImage());

  // Update the history
  m_SystemInterface->GetHistoryManager()->UpdateHistory(
        "LabelImage", io->GetFileNameOfNativeImage(), true);

  // Update the color labels, so that for every label in the image
  // there is a valid color label
  LabelImageWrapper::ConstIterator it = 
    m_IRISImageData->GetSegmentation()->GetImageConstIterator();
  for( ; !it.IsAtEnd(); ++it)
    if(!m_ColorLabelTable->IsColorLabelValid(it.Get()))
      m_ColorLabelTable->SetColorLabelValid(it.Get(), true);

  // Reset the UNDO manager
  m_UndoManager.Clear();
}

inline
LabelType
IRISApplication
::DrawOverLabel(LabelType iTarget)
{
  // Get the current merge settings
  const DrawOverFilter &filter = m_GlobalState->GetDrawOverFilter();
  const LabelType &iDrawing = m_GlobalState->GetDrawingColorLabel();

  // If mode is paint over all, the victim is overridden
  if(filter.CoverageMode == PAINT_OVER_ALL)
    return iDrawing;

  if(filter.CoverageMode == PAINT_OVER_ONE && filter.DrawOverLabel == iTarget)
    return iDrawing;

  if(filter.CoverageMode == PAINT_OVER_VISIBLE
     && m_ColorLabelTable->GetColorLabel(iTarget).IsVisible())
    return iDrawing;

  return iTarget;
}

void IRISApplication::BeginSegmentationUpdate(std::string undo_name)
{
  m_SegmentationUpdateName = undo_name;
  m_SegmentationChangeCount = 0;
}

void IRISApplication::UpdateSegmentationVoxel(const Vector3ui &pos)
{
  // Get the segmentation image
  LabelImageType *seg = m_CurrentImageData->GetSegmentation()->GetImage();
  LabelType &label = seg->GetPixel(to_itkIndex(pos));
  LabelType newlabel = DrawOverLabel(label);
  if(label != newlabel)
    {
    label = newlabel;
    m_SegmentationChangeCount++;
    }
}

int IRISApplication::EndSegmentationUpdate()
{
  if(m_SegmentationChangeCount > 0)
    {
    m_CurrentImageData->GetSegmentation()->GetImage()->Modified();
    this->StoreUndoPoint(m_SegmentationUpdateName.c_str());
    this->InvokeEvent(SegmentationChangeEvent());
    }

  m_SegmentationUpdateName = std::string();
  return m_SegmentationChangeCount;
}

unsigned int
IRISApplication
::UpdateSegmentationWithSliceDrawing(
    IRISApplication::SliceBinaryImageType *drawing,
    const ImageCoordinateTransform &xfmSliceToImage,
    double zSlice,
    const std::string &undoTitle)
{
  // Only in IRIS mode
  assert(!IsSnakeModeActive());

  // Get the segmentation image
  LabelImageType *seg = m_CurrentImageData->GetSegmentation()->GetImage();

  // Drawing parameters
  CoverageModeType iMode = m_GlobalState->GetDrawOverFilter().CoverageMode;
  LabelType iDrawing = m_GlobalState->GetDrawingColorLabel();
  LabelType iDrawOver = m_GlobalState->GetDrawOverFilter().DrawOverLabel;
  bool invert = m_GlobalState->GetPolygonInvert();

  // Keep track of the number of pixels changed
  unsigned int nUpdates = 0;

  // Iterate through the drawing
  for (itk::ImageRegionIteratorWithIndex<SliceBinaryImageType>
       it(drawing, drawing->GetBufferedRegion()); !it.IsAtEnd(); ++it)
    {
    // Get the current polygon pixel
    SliceBinaryImageType::PixelType px = it.Get();

    // Check for non-zero alpha of the pixel
    if((px != 0) ^ invert)
      {
      // Figure out the coordinate of the target image
      itk::Index<2> idx = it.GetIndex();
      Vector3f idxImageFloat = xfmSliceToImage.TransformPoint(
            Vector3f(idx[0] + 0.5, idx[1] + 0.5, zSlice));
      itk::Index<3> iseg = to_itkIndex(to_unsigned_int(idxImageFloat));

      // Access the voxel in the segmentation
      LabelType &voxel = seg->GetPixel(iseg);

      // Apply the label to the voxel
      if(iMode == PAINT_OVER_ALL ||
         (iMode == PAINT_OVER_ONE && voxel == iDrawOver) ||
         (iMode == PAINT_OVER_VISIBLE &&
          m_ColorLabelTable->GetColorLabel(voxel).IsVisible()))
        {
        if(voxel != iDrawing)
          {
          voxel = iDrawing;
          nUpdates++;
          }
        }
      }
    }

  // Has anything been changed?
  if(nUpdates > 0)
    {
    seg->Modified();
    StoreUndoPoint(undoTitle.c_str());
    InvokeEvent(SegmentationChangeEvent());
    }

  return nUpdates;
}

void 
IRISApplication
::UpdateIRISWithSnapImageData(CommandType *progressCommand)
{
  assert(IsSnakeModeActive());

  // Get pointers to the source and destination images
  typedef LevelSetImageWrapper::ImageType SourceImageType;
  typedef LabelImageWrapper::ImageType TargetImageType;

  // If the voxel size of the image does not match the voxel size of the 
  // main image, we need to resample the region  
  SourceImageType::Pointer source = m_SNAPImageData->GetSnake()->GetImage();
  TargetImageType::Pointer target = m_IRISImageData->GetSegmentation()->GetImage();

  // Construct are region of interest into which the result will be pasted
  SNAPSegmentationROISettings roi = m_GlobalState->GetSegmentationROISettings();

  // If the ROI has been resampled, resample the segmentation in reverse direction
  if(roi.GetResampleFlag())
    {
    // Create a resampling filter
    typedef itk::ResampleImageFilter<SourceImageType,SourceImageType> ResampleFilterType;
    ResampleFilterType::Pointer fltSample = ResampleFilterType::New();

    // Initialize the resampling filter with an identity transform
    fltSample->SetInput(source);
    fltSample->SetTransform(itk::IdentityTransform<double,3>::New());

    // Typedefs for interpolators
    typedef itk::NearestNeighborInterpolateImageFunction<
      SourceImageType,double> NNInterpolatorType;
    typedef itk::LinearInterpolateImageFunction<
      SourceImageType,double> LinearInterpolatorType;
    typedef itk::BSplineInterpolateImageFunction<
      SourceImageType,double> CubicInterpolatorType;

    // More typedefs are needed for the sinc interpolator
    const unsigned int VRadius = 5;
    typedef itk::Function::HammingWindowFunction<VRadius> WindowFunction;
    typedef itk::ConstantBoundaryCondition<SourceImageType> Condition;
    typedef itk::WindowedSincInterpolateImageFunction<
      SourceImageType, VRadius, 
      WindowFunction, Condition, double> SincInterpolatorType;

    // Choose the interpolator
    switch(roi.GetInterpolationMethod())
      {
      case SNAPSegmentationROISettings::NEAREST_NEIGHBOR :
        fltSample->SetInterpolator(NNInterpolatorType::New());
        break;

      case SNAPSegmentationROISettings::TRILINEAR : 
        fltSample->SetInterpolator(LinearInterpolatorType::New());
        break;

      case SNAPSegmentationROISettings::TRICUBIC :
        fltSample->SetInterpolator(CubicInterpolatorType::New());
        break;  

      case SNAPSegmentationROISettings::SINC_WINDOW_05 :
        fltSample->SetInterpolator(SincInterpolatorType::New());
        break;
      };

    // Set the image sizes and spacing. We are creating an image of the 
    // dimensions of the ROI defined in the IRIS image space. 
    fltSample->SetSize(roi.GetROI().GetSize());
    fltSample->SetOutputSpacing(target->GetSpacing());
    fltSample->SetOutputOrigin(source->GetOrigin());
    fltSample->SetOutputDirection(source->GetDirection());

    // Watch the segmentation progress
    if(progressCommand) 
      fltSample->AddObserver(itk::AnyEvent(),progressCommand);

    // Set the unknown intensity to positive value
    fltSample->SetDefaultPixelValue(4.0f);

    // Perform resampling
    fltSample->UpdateLargestPossibleRegion();
    
    // Change the source to the output
    source = fltSample->GetOutput();
    }  
  
  // Create iterators for copying from one to the other
  typedef itk::ImageRegionConstIterator<SourceImageType> SourceIteratorType;
  typedef itk::ImageRegionIterator<TargetImageType> TargetIteratorType;
  SourceIteratorType itSource(source,source->GetLargestPossibleRegion());
  TargetIteratorType itTarget(target,roi.GetROI());

  // Figure out which color draws and which color is clear
  unsigned int iClear = m_GlobalState->GetPolygonInvert() ? 1 : 0;

  // Construct a merge table that contains an output intensity for every 
  // possible combination of two input intensities (note that snap image only
  // has two possible intensities
  LabelType mergeTable[2][MAX_COLOR_LABELS];

  // Perform the merge
  for(unsigned int i=0;i<MAX_COLOR_LABELS;i++)
    {
    // Whe the SNAP image is clear, IRIS passes through to the output
    // except for the IRIS voxels of the drawing color, which get cleared out
    mergeTable[iClear][i] = (i!=m_GlobalState->GetDrawingColorLabel()) ? i : 0;

    // If mode is paint over all, the victim is overridden
    mergeTable[1-iClear][i] = DrawOverLabel((LabelType) i);
    }

  // Go through both iterators, copy the new over the old
  itSource.GoToBegin();
  itTarget.GoToBegin();
  while(!itSource.IsAtEnd())
    {
    // Get the two voxels
    LabelType &voxIRIS = itTarget.Value();    
    float voxSNAP = itSource.Value();

    // Check that we're ok (debug mode only)
    assert(!itTarget.IsAtEnd());

    // Perform the merge
    voxIRIS = mergeTable[voxSNAP <= 0 ? 1 : 0][voxIRIS];

    // Iterate
    ++itSource;
    ++itTarget;
    }

  // The target has been modified
  target->Modified();
}

void
IRISApplication
::SetCursorPosition(const Vector3ui cursor)
{
  if(cursor != this->GetCursorPosition())
    {
    m_GlobalState->SetCrosshairsPosition(cursor);
    this->GetCurrentImageData()->SetCrosshairs(cursor);

    // Fire the appropriate event
    InvokeEvent(CursorUpdateEvent());
    }
}

Vector3ui
IRISApplication
::GetCursorPosition() const
{
  return m_GlobalState->GetCrosshairsPosition();
}

void 
IRISApplication
::StoreUndoPoint(const char *text)
{
  // Set the current state as the undo point. We store the difference between
  // the last 'undo' image and the current segmentation image, and then copy
  // the current segmentation image into the undo image
  LabelImageWrapper *undo = m_IRISImageData->GetUndoImage();
  LabelImageWrapper *seg = m_IRISImageData->GetSegmentation();
  
  LabelType *dseg = seg->GetVoxelPointer();
  LabelType *dundo = undo->GetVoxelPointer();
  size_t n = seg->GetNumberOfVoxels();

  // Create the Undo delta object
  UndoManagerType::Delta *delta = new UndoManagerType::Delta();

  // Copy and encode
  for(size_t i = 0; i < n; i++)
    {
    LabelType vSrc = dseg[i], vDst = dundo[i];
    delta->Encode(vSrc - vDst);
    dundo[i] = vSrc;
    }

  // Important last step!
  delta->FinishEncoding();

  // Set modified flag on the undo image
  undo->GetImage()->Modified();

  // Add the delta object
  m_UndoManager.AppendDelta(delta);
}

void
IRISApplication
::ClearUndoPoints()
{
  m_UndoManager.Clear();
}

bool
IRISApplication
::IsUndoPossible()
{
  return m_UndoManager.IsUndoPossible();
}

void
IRISApplication
::Undo()
{
  // In order to undo, we must take the 'current' delta and apply
  // it to the image
  UndoManagerType::Delta *delta = m_UndoManager.GetDeltaForUndo();

  LabelImageWrapper *imUndo = m_IRISImageData->GetUndoImage();
  LabelImageWrapper *imSeg = m_IRISImageData->GetSegmentation();
  LabelType *dundo = imUndo->GetVoxelPointer();
  LabelType *dseg = imSeg->GetVoxelPointer();

  // Applying the delta means adding 
  for(size_t i = 0; i < delta->GetNumberOfRLEs(); i++)
    {
    size_t n = delta->GetRLELength(i);
    LabelType d = delta->GetRLEValue(i);
    if(d == 0)
      {
      dundo += n;
      dseg += n;
      }
    else
      {
      for(size_t j = 0; j < n; j++)
        {
        *dundo -= d;
        *dseg = *dundo;
        ++dundo; ++dseg;
        }
      }
    }

  // Set modified flags
  imSeg->GetImage()->Modified();
  imUndo->GetImage()->Modified();
  InvokeEvent(SegmentationChangeEvent());
}


bool
IRISApplication
::IsRedoPossible()
{
  return m_UndoManager.IsRedoPossible();
}

void
IRISApplication
::Redo()
{
  // In order to undo, we must take the 'current' delta and apply
  // it to the image
  UndoManagerType::Delta *delta = m_UndoManager.GetDeltaForRedo();

  LabelImageWrapper *imUndo = m_IRISImageData->GetUndoImage();
  LabelImageWrapper *imSeg = m_IRISImageData->GetSegmentation();
  LabelType *dundo = imUndo->GetVoxelPointer();
  LabelType *dseg = imSeg->GetVoxelPointer();

  // Applying the delta means adding 
  for(size_t i = 0; i < delta->GetNumberOfRLEs(); i++)
    {
    size_t n = delta->GetRLELength(i);
    LabelType d = delta->GetRLEValue(i);
    if(d == 0)
      {
      dundo += n;
      dseg += n;
      }
    else
      {
      for(size_t j = 0; j < n; j++)
        {
        *dundo += d;
        *dseg = *dundo;
        ++dundo; ++dseg;
        }
      }
    }

  // Set modified flags
  imSeg->GetImage()->Modified();
  imUndo->GetImage()->Modified();
  InvokeEvent(SegmentationChangeEvent());
}




void 
IRISApplication
::ReleaseSNAPImageData() 
{
  assert(m_SNAPImageData->IsMainLoaded() &&
         m_CurrentImageData != m_SNAPImageData);

  m_SNAPImageData->UnloadAll();
}

void 
IRISApplication
::SetCurrentImageDataToIRIS() 
{
  assert(m_IRISImageData);
  if(m_CurrentImageData != m_IRISImageData)
    {
    m_CurrentImageData = m_IRISImageData;
    InvokeEvent(MainImageDimensionsChangeEvent());
    }
}

void IRISApplication
::SetCurrentImageDataToSNAP() 
{
  assert(m_SNAPImageData->IsMainLoaded());
  if(m_CurrentImageData != m_SNAPImageData)
    {
    // The cursor needs to be modified to point to the same location
    // as before, or to the center of the image
    Vector3d cursorIRIS = to_double(this->GetCursorPosition());

    Vector3d xIRIS =
        m_IRISImageData->GetMain()->TransformVoxelIndexToNIFTICoordinates(cursorIRIS);

    Vector3d indexSNAP =
        m_SNAPImageData->GetMain()->TransformNIFTICoordinatesToVoxelIndex(xIRIS);

    itk::Index<3> idxSNAP = to_itkIndex(indexSNAP);

    Vector3ui newCursor;

    if(m_SNAPImageData->GetMain()->GetBufferedRegion().IsInside(idxSNAP))
      {
      // Set the cursor at the current physical position
      newCursor = Vector3ui(idxSNAP);
      }
    else
      {
      // Set the cursor at the center
      newCursor = m_SNAPImageData->GetMain()->GetSize() / 2u;
      }

    // Set the image data
    m_CurrentImageData = m_SNAPImageData;

    // Set the calculated cursor position
    this->SetCursorPosition(newCursor);

    // Fire the event
    InvokeEvent(MainImageDimensionsChangeEvent());
    }
}

size_t 
IRISApplication
::GetImageDirectionForAnatomicalDirection(AnatomicalDirection iAnat)
{
  std::string myrai = this->GetImageToAnatomyRAI();
  
  string rai1 = "SRA", rai2 = "ILP";
  
  char c1 = rai1[iAnat], c2 = rai2[iAnat];
  for(size_t j = 0; j < 3; j++)
    if(myrai[j] == c1 || myrai[j] == c2)
      return j;
  
  assert(0);
  return 0;
}

size_t 
IRISApplication
::GetDisplayWindowForAnatomicalDirection(
  AnatomicalDirection iAnat) const
{
  string rai1 = "SRA", rai2 = "ILP";
  char c1 = rai1[iAnat], c2 = rai2[iAnat];
  for(size_t j = 0; j < 3; j++)
    {
    char sd = m_DisplayToAnatomyRAI[j][2];
    if(sd == c1 || sd == c2)
      return j;
    }

  assert(0);
  return 0;
}

AnatomicalDirection
IRISApplication::GetAnatomicalDirectionForDisplayWindow(int iWin) const
{
  char sd = m_DisplayToAnatomyRAI[iWin][2];
  if(sd == 'S' || sd == 'I')
    return ANATOMY_AXIAL;
  else if (sd == 'R' || sd == 'L')
    return ANATOMY_SAGITTAL;
  else if (sd == 'A' || sd == 'P')
    return ANATOMY_CORONAL;

  assert(0);
  return(ANATOMY_NONSENSE);
}

void
IRISApplication
::ExportSlice(AnatomicalDirection iSliceAnat, const char *file)
{
  // Get the slice index in image coordinates
  size_t iSliceImg = 
    GetImageDirectionForAnatomicalDirection(iSliceAnat);

  // TODO: should this not export using the default scalar representation,
  // rather than RGB? Not sure...

  // Find the slicer that slices along that direction
  typedef ImageWrapperBase::DisplaySliceType SliceType;
  SmartPtr<SliceType> imgGrey = NULL;
  for(size_t i = 0; i < 3; i++)
    {
    if(iSliceImg == m_CurrentImageData->GetMain()->GetDisplaySliceImageAxis(i))
      {
      imgGrey = m_CurrentImageData->GetMain()->GetDisplaySlice(i);
      break;
      }
    }
  assert(imgGrey);

  // Flip the image in the Y direction
  typedef itk::FlipImageFilter<SliceType> FlipFilter;
  FlipFilter::Pointer fltFlip = FlipFilter::New();
  fltFlip->SetInput(imgGrey);
  
  FlipFilter::FlipAxesArrayType arrFlips;
  arrFlips[0] = false; arrFlips[1] = true;
  fltFlip->SetFlipAxes(arrFlips);

  // Create a writer for saving the image
  typedef itk::ImageFileWriter<SliceType> WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetInput(fltFlip->GetOutput());
  writer->SetFileName(file);
  writer->Update();
}

void 
IRISApplication
::ExportSegmentationStatistics(const char *file)
{
  // Make sure that the segmentation image exists
  assert(m_CurrentImageData->IsSegmentationLoaded());

  SegmentationStatistics stats;
  stats.Compute(m_CurrentImageData);

  // Open the selected file for writing
  std::ofstream fout(file);

  // Check if the file is readable
  if(!fout.good())
    throw itk::ExceptionObject(__FILE__, __LINE__,
                               "File can not be opened for writing");
  try 
    {
    stats.ExportLegacy(fout, *m_ColorLabelTable);
    }
  catch(...)
    {
    throw itk::ExceptionObject(__FILE__, __LINE__,
                           "File can not be written");
    }

  fout.close();
}



void
IRISApplication
::ExportSegmentationMesh(const MeshExportSettings &sets, itk::Command *progress) 
{
  // Based on the export settings, we will export one of the labels or all labels
  SmartPtr<MeshObject> mob = MeshObject::New();
  mob->Initialize(this);
  mob->GenerateVTKMeshes(progress);

  // If in SNAP mode, just save the first mesh
  if(m_SNAPImageData->IsMainLoaded())
    {
    // Get the VTK mesh for the label
    vtkPolyData *mesh = mob->GetVTKMesh(0);

    // Export the mesh
    GuidedMeshIO io;
    Registry rFormat = sets.GetMeshFormat();
    io.SaveMesh(sets.GetMeshFileName().c_str(), rFormat, mesh);
    }

  // If only one mesh is to be exported, life is easy
  else if(sets.GetFlagSingleLabel())
    {
    for(size_t i = 0; i < mob->GetNumberOfVTKMeshes(); i++)
      {
      if(mob->GetVTKMeshLabel(i) == sets.GetExportLabel())
        {
        // Get the VTK mesh for the label
        vtkPolyData *mesh = mob->GetVTKMesh(i);

        // Export the mesh
        GuidedMeshIO io;
        Registry rFormat = sets.GetMeshFormat();
        io.SaveMesh(sets.GetMeshFileName().c_str(), rFormat, mesh);
        }
      }
    }
  else if(sets.GetFlagSingleScene())
    {
    // Create an append filter
    vtkAppendPolyData *append = vtkAppendPolyData::New();
    std::vector<vtkUnsignedShortArray *> scalarArray;

    for(size_t i = 0; i < mob->GetNumberOfVTKMeshes(); i++)
      {
      // Get the VTK mesh for the label
      vtkPolyData *mesh = mob->GetVTKMesh(i);
      vtkUnsignedShortArray *scalar = vtkUnsignedShortArray::New();
      scalar->SetNumberOfComponents(1);
      scalar->Allocate(mesh->GetNumberOfPoints());
      for(int j = 0; j < mesh->GetNumberOfPoints(); j++)
        scalar->InsertNextTuple1(mob->GetVTKMeshLabel(i));
      mesh->GetPointData()->SetScalars(scalar);
      scalarArray.push_back(scalar);
      append->AddInput(mesh);
      }

    append->Update();

    // Export the mesh
    GuidedMeshIO io;
    Registry rFormat = sets.GetMeshFormat();
    io.SaveMesh(sets.GetMeshFileName().c_str(), rFormat, append->GetOutput());

    append->Delete();
    for(size_t i = 0; i < scalarArray.size(); i++)
      scalarArray[i]->Delete();
    }
  else
    {
    // Take apart the filename
    std::string full = itksys::SystemTools::CollapseFullPath(sets.GetMeshFileName().c_str());
    std::string path = itksys::SystemTools::GetFilenamePath(full.c_str());
    std::string file = itksys::SystemTools::GetFilenameWithoutExtension(full.c_str());
    std::string extn = itksys::SystemTools::GetFilenameExtension(full.c_str());
    std::string prefix = file;

    // Are the last 5 characters of the filename numeric?
    if(file.length() >= 5)
      {
      string suffix = file.substr(file.length()-5,5);
      if(count_if(suffix.begin(), suffix.end(), isdigit) == 5)
        prefix = file.substr(0, file.length()-5);
      }

    // Loop, saving each mesh into a filename
    for(size_t i = 0; i < mob->GetNumberOfVTKMeshes(); i++)
      {
      // Get the VTK mesh for the label
      vtkPolyData *mesh = mob->GetVTKMesh(i);

      // Generate filename
      char outfn[4096];
      sprintf(outfn, "%s/%s%05d%s", path.c_str(), prefix.c_str(), mob->GetVTKMeshLabel(i), extn.c_str());

      // Export the mesh
      GuidedMeshIO io;
      Registry rFormat = sets.GetMeshFormat();
      io.SaveMesh(outfn, rFormat, mesh);
      }
    }

  mob->DiscardVTKMeshes();
}

size_t
IRISApplication
::ReplaceLabel(LabelType drawing, LabelType drawover)
{
  // Get the label image
  assert(m_CurrentImageData->IsSegmentationLoaded());
  LabelImageWrapper::ImagePointer imgLabel = 
    m_CurrentImageData->GetSegmentation()->GetImage();

  // Get the number of voxels
  size_t nvoxels = 0;

  // Update the segmentation
  typedef itk::ImageRegionIterator<
    LabelImageWrapper::ImageType> IteratorType;
  for(IteratorType it(imgLabel, imgLabel->GetBufferedRegion());  
    !it.IsAtEnd(); ++it)
    {
    if(it.Get() == drawover)
      {
      it.Set(drawing);
      ++nvoxels;
      }
    }

  // Register that the image has been updated
  imgLabel->Modified();

  return nvoxels;
}

// TODO: This information should be cached at the segmentation layer level
// by keeping track of label counts after every update operation.
size_t
IRISApplication
::GetNumberOfVoxelsWithLabel(LabelType label)
{
  // Get the label image
  assert(m_CurrentImageData->IsSegmentationLoaded());
  LabelImageType *seg = m_CurrentImageData->GetSegmentation()->GetImage();

  // Get the number of voxels
  size_t nvoxels = 0;
  for(LabelImageWrapper::ConstIterator it(seg, seg->GetBufferedRegion());
      !it.IsAtEnd(); ++it)
    {
    if(it.Get() == label)
      ++nvoxels;
    }

  return nvoxels;
}


void 
IRISApplication
::RelabelSegmentationWithCutPlane(const Vector3d &normal, double intercept) 
{
  // Get the label image
  LabelImageWrapper::ImagePointer imgLabel = 
    m_CurrentImageData->GetSegmentation()->GetImage();
  
  // Get an iterator for the image
  typedef itk::ImageRegionIteratorWithIndex<
    LabelImageWrapper::ImageType> IteratorType;
  IteratorType it(imgLabel, imgLabel->GetBufferedRegion());

  // Compute a label mapping table based on the color labels
  LabelType table[MAX_COLOR_LABELS];
  
  // The clear label does not get painted over, no matter what
  table[0] = 0;

  // The other labels get painted over, depending on current settings
  for(unsigned int i = 1; i < MAX_COLOR_LABELS; i++)
    table[i] = DrawOverLabel(i);

  // Adjust the intercept by 0.5 for voxel offset
  intercept -= 0.5 * (normal[0] + normal[1] + normal[2]);

  // Iterate over the image, relabeling labels on one side of the plane
  while(!it.IsAtEnd())
    {
    // Compute the distance to the plane
    const long *index = it.GetIndex().GetIndex();
    double distance = 
      index[0]*normal[0] + 
      index[1]*normal[1] + 
      index[2]*normal[2] - intercept;

    // Check the side of the plane
    if(distance > 0)
      {
      LabelType &voxel = it.Value();
      voxel = table[voxel];
      }

    // Next voxel
    ++it;
    }
  
  // Register that the image has been updated
  imgLabel->Modified();
}

int 
IRISApplication
::GetRayIntersectionWithSegmentation(const Vector3d &point, 
                                     const Vector3d &ray, Vector3i &hit) const
{
  // Get the label wrapper
  LabelImageWrapper *xLabelWrapper = m_CurrentImageData->GetSegmentation();
  assert(xLabelWrapper->IsInitialized());

  Vector3ui lIndex;
  Vector3ui lSize = xLabelWrapper->GetSize();

  double delta[3][3] = {{0.,0.,0.},{0.,0.,0.},{0.,0.,0.}}, dratio[3] = {0., 0., 0.};
  int    signrx, signry, signrz;

  double rx = ray[0];
  double ry = ray[1];
  double rz = ray[2];

  double rlen = rx*rx+ry*ry+rz*rz;
  if(rlen == 0) return -1;

  double rfac = 1.0 / sqrt(rlen);
  rx *= rfac; ry *= rfac; rz *= rfac;

  if (rx >=0) signrx = 1; else signrx = -1;
  if (ry >=0) signry = 1; else signry = -1;
  if (rz >=0) signrz = 1; else signrz = -1;

  // offset everything by (.5, .5) [becuz samples are at center of voxels]
  // this offset will put borders of voxels at integer values
  // we will work with this offset grid and offset back to check samples
  // we really only need to offset "point"
  double px = point[0]+0.5;
  double py = point[1]+0.5;
  double pz = point[2]+0.5;

  // get the starting point within data extents
  int c = 0;
  while ( (px < 0 || px >= lSize[0]||
           py < 0 || py >= lSize[1]||
           pz < 0 || pz >= lSize[2]) && c < 10000)
    {
    px += rx;
    py += ry;
    pz += rz;
    c++;
    }
  if (c >= 9999) return -1;

  // walk along ray to find intersection with any voxel with val > 0
  while ( (px >= 0 && px < lSize[0]&&
           py >= 0 && py < lSize[1] &&
           pz >= 0 && pz < lSize[2]) )
    {

    // offset point by (-.5, -.5) [to account for earlier offset] and
    // get the nearest sample voxel within unit cube around (px,py,pz)
    //    lx = my_round(px-0.5);
    //    ly = my_round(py-0.5);
    //    lz = my_round(pz-0.5);
    lIndex[0] = (int)px;
    lIndex[1] = (int)py;
    lIndex[2] = (int)pz;

    LabelType hitlabel = m_CurrentImageData->GetSegmentation()->GetVoxel(lIndex);

    if (m_ColorLabelTable->IsColorLabelValid(hitlabel))
      {
      ColorLabel cl = m_ColorLabelTable->GetColorLabel(hitlabel);
      if(cl.IsVisible())
        {
        hit[0] = lIndex[0];
        hit[1] = lIndex[1];
        hit[2] = lIndex[2];
        return 1;
        }
      }

    // BEGIN : walk along ray to border of next voxel touched by ray

    // compute path to YZ-plane surface of next voxel
    if (rx == 0)
      { // ray is parallel to 0 axis
      delta[0][0] = 9999;
      }
    else
      {
      delta[0][0] = (int)(px+signrx) - px;
      }

    // compute path to XZ-plane surface of next voxel
    if (ry == 0)
      { // ray is parallel to 1 axis
      delta[1][0] = 9999;
      }
    else
      {
      delta[1][1] = (int)(py+signry) - py;
      dratio[1]   = delta[1][1]/ry;
      delta[1][0] = dratio[1] * rx;
      }

    // compute path to XY-plane surface of next voxel
    if (rz == 0)
      { // ray is parallel to 2 axis
      delta[2][0] = 9999;
      }
    else
      {
      delta[2][2] = (int)(pz+signrz) - pz;
      dratio[2]   = delta[2][2]/rz;
      delta[2][0] = dratio[2] * rx;
      }

    // choose the shortest path 
    if ( fabs(delta[0][0]) <= fabs(delta[1][0]) && fabs(delta[0][0]) <= fabs(delta[2][0]) )
      {
      dratio[0]   = delta[0][0]/rx;
      delta[0][1] = dratio[0] * ry;
      delta[0][2] = dratio[0] * rz;
      px += delta[0][0];
      py += delta[0][1];
      pz += delta[0][2];
      }
    else if ( fabs(delta[1][0]) <= fabs(delta[0][0]) && fabs(delta[1][0]) <= fabs(delta[2][0]) )
      {
      delta[1][2] = dratio[1] * rz;
      px += delta[1][0];
      py += delta[1][1];
      pz += delta[1][2];
      }
    else
      { //if (fabs(delta[2][0] <= fabs(delta[0][0] && fabs(delta[2][0] <= fabs(delta[0][0]) 
      delta[2][1] = dratio[2] * ry;
      px += delta[2][0];
      py += delta[2][1];
      pz += delta[2][2];
      }
    // END : walk along ray to border of next voxel touched by ray

    } // while ( (px
  return 0;
}

void
IRISApplication
::AddIRISOverlayImage(GuidedNativeImageIO *io)
{
  assert(!IsSnakeModeActive());
  assert(m_IRISImageData->IsMainLoaded());
  assert(io->IsNativeImageLoaded());

  // Rescale the image to grey
  RescaleNativeImageToIntegralType<AnatomyImageType> rescaler;
  AnatomyImageType::Pointer imgOverlay = rescaler(io);
  LinearInternalToNativeIntensityMapping mapper(rescaler.GetNativeScale(), rescaler.GetNativeShift());

  // Add the image as the current grayscale overlay
  m_IRISImageData->AddOverlay(imgOverlay, mapper);

  // Set the filename of the overlay
  // TODO: this is cumbersome, could we just initialize the wrapper from the
  // GuidedNativeImageIO without passing all this junk around?
  m_IRISImageData->GetLastOverlay()->SetFileName(io->GetFileNameOfNativeImage());

  // Add the overlay to the history
  m_HistoryManager->UpdateHistory("AnatomicImage", io->GetFileNameOfNativeImage(), true);

  // for overlay, we don't want to change the cursor location
  // just force the IRISSlicer to update
  m_IRISImageData->SetCrosshairs(m_GlobalState->GetCrosshairsPosition());

  // Fire event
  InvokeEvent(LayerChangeEvent());
}

void
IRISApplication
::UpdateIRISMainImage(GuidedNativeImageIO *io)
{
  // This has to happen in 'pure' IRIS mode
  assert(!IsSnakeModeActive());

  // Get the size of the image as a vector of uint
  Vector3ui size = io->GetNativeImage()->GetBufferedRegion().GetSize();

  // Compute the new image geometry for the IRIS data
  ImageCoordinateGeometry icg(
        io->GetNativeImage()->GetDirection().GetVnlMatrix(),
        m_DisplayToAnatomyRAI, size);

  // Rescale the image to desired number of bits
  RescaleNativeImageToIntegralType<AnatomyImageType> rescaler;
  AnatomyImageType::Pointer imgMain = rescaler(io);
  LinearInternalToNativeIntensityMapping mapper(rescaler.GetNativeScale(), rescaler.GetNativeShift());

  // Set the image as the current main anatomy image
  m_IRISImageData->SetMainImage(imgMain, icg, mapper);

  // TODO: the threshold settings should probably not be initialized here, but
  // when we are interacting with them. This way they can adapt to whatever
  // scalar representation is current.

  // Update the preprocessing settings to defaults.
  m_ThresholdSettings->InitializeToDefaultForImage(
        m_IRISImageData->GetMain()->GetDefaultScalarRepresentation());
  m_EdgePreprocessingSettings->InitializeToDefaults();

  // Set the filename and nickname of the image wrapper
  m_IRISImageData->GetMain()->SetFileName(io->GetFileNameOfNativeImage());

  // Update the system's history list
  m_HistoryManager->UpdateHistory("MainImage", io->GetFileNameOfNativeImage(), false);
  m_HistoryManager->UpdateHistory("AnatomicImage", io->GetFileNameOfNativeImage(), false);

  // Reset the segmentation ROI
  m_GlobalState->SetSegmentationROI(io->GetNativeImage()->GetBufferedRegion());

  // Try to load the settings associated with this main image
  Registry associated;
  if(m_SystemInterface->FindRegistryAssociatedWithFile(
       io->GetFileNameOfNativeImage().c_str(), associated))
    {
    // Load the settings using RegistryIO
    SNAPRegistryIO rio;
    rio.ReadImageAssociatedSettings(associated, this, true, true, true, true);
    }

  // Fire the dimensions change event
  InvokeEvent(MainImageDimensionsChangeEvent());

  // Update the crosshairs position to the center of the image
  Vector3ui cursor = size; cursor /= 2;
  this->SetCursorPosition(cursor);

  // This line forces the cursor to be propagated to the image even if the
  // crosshairs positions did not change from their previous values
  this->GetIRISImageData()->SetCrosshairs(cursor);

  // Reset the UNDO manager
  m_UndoManager.Clear();
}

void IRISApplication::LoadMainImage(const char *filename)
{
  // Load the settings associated with this file
  Registry regFull;
  m_SystemInterface->FindRegistryAssociatedWithFile(filename, regFull);
    
  // Get the folder dealing with grey image properties
  Registry &folder = regFull.Folder("Files.Grey");

  // Create a native image IO object
  GuidedNativeImageIO io;
  io.ReadNativeImage(filename, folder);

  // Do the work
  UpdateIRISMainImage(&io);
}

void
IRISApplication
::UnloadMainImage()
{
  // Save the settings for this image
  if(m_CurrentImageData->IsMainLoaded())
    {
    std::string fnMain = m_CurrentImageData->GetMain()->GetFileName();
    m_SystemInterface->AssociateCurrentSettingsWithCurrentImageFile(
          fnMain.c_str(), this);

    // Create a thumbnail from the one of the image slices
    string fnThumb =
        m_SystemInterface->GetThumbnailAssociatedWithFile(fnMain.c_str());

    m_CurrentImageData->GetMain()->WriteThumbnail(fnThumb.c_str(), 128);
    }

  // Reset the automatic segmentation ROI
  m_GlobalState->SetSegmentationROI(GlobalState::RegionType());

  // Unload the main image
  m_CurrentImageData->UnloadMainImage();

  // Let everyone know that the main image is gone!
  InvokeEvent(MainImageDimensionsChangeEvent());
}

void IRISApplication::Quit()
{
  if(IsSnakeModeActive())
    {
    // Before quitting the application, we need to exit snake mode
    SetCurrentImageDataToIRIS();
    ReleaseSNAPImageData();
    }

  UnloadOverlays();
  UnloadMainImage();
}

void
IRISApplication
::LoadOverlayImage(const char *filename)
{
  // Load the settings associated with this file
  Registry regFull;
  m_SystemInterface->FindRegistryAssociatedWithFile(filename, regFull);
    
  // Get the folder dealing with grey image properties
  Registry &folder = regFull.Folder("Files.Grey");

  // Create a native image IO object
  GuidedNativeImageIO io;
  io.ReadNativeImage(filename, folder);

  // Detemine the type
  AddIRISOverlayImage(&io);
}

bool IRISApplication::IsMainImageLoaded() const
{
  return this->GetCurrentImageData()->IsMainLoaded();
}





/*
void
IRISApplication
::LoadRGBImageFile(const char *filename, const bool isMain)
{
  // Load the settings associated with this file
  Registry regFull;
  m_SystemInterface->FindRegistryAssociatedWithFile(filename, regFull);
    
  // Get the folder dealing with grey image properties
  Registry &regRGB = regFull.Folder("Files.RGB");

  // Create the image reader
  GuidedImageIO<RGBType> io;
  
  // Load the image (exception may occur here)
  RGBImageType::Pointer imgRGB = io.ReadImage(filename, regRGB, false);

  if (isMain)
    {
    // Set the image as the current main image  
    UpdateIRISRGBImage(imgRGB);
    }
  else
    {
    // Set the image as the overlay
    UpdateIRISRGBOverlay(imgRGB);
    }

  // Save the filename for the UI
  m_GlobalState->SetRGBFileName(filename);  
}
*/

void 
IRISApplication
::LoadLabelImageFile(const char *filename)
{
  // Load the settings associated with this file
  Registry regFull;
  m_SystemInterface->FindRegistryAssociatedWithFile(filename, regFull);

  // Get the folder dealing with grey image properties
  // TODO: Figure out something about this!!!
  Registry &regGrey = regFull.Folder("Files.Grey");

  // Read the image in native format
  GuidedNativeImageIO io;
  io.ReadNativeImage(filename, regGrey);

  // Set the image as the current grayscale image
  UpdateIRISSegmentationImage(&io);
}

void 
IRISApplication
::ReorientImage(vnl_matrix_fixed<double, 3, 3> inDirection)
{
  // This should only be possible in IRIS mode
  assert(m_CurrentImageData == m_IRISImageData);

  // The main image should be loaded at this point
  assert(m_CurrentImageData->IsMainLoaded());

  // Compute a new coordinate transform object
  ImageCoordinateGeometry icg(
    inDirection, m_DisplayToAnatomyRAI, 
    m_CurrentImageData->GetMain()->GetSize());

  // Send this coordinate transform to the image data
  m_CurrentImageData->SetImageGeometry(icg);

  // Fire the pose change event
  InvokeEvent(MainImagePoseChangeEvent());
}

void IRISApplication::LoadLabelDescriptions(const char *file)
{
  // Read the labels from file
  this->m_ColorLabelTable->LoadFromFile(file);

  // Reset the current drawing and overlay labels
  m_GlobalState->SetDrawingColorLabel(m_ColorLabelTable->GetFirstValidLabel());
  m_GlobalState->SetDrawOverFilter(DrawOverFilter());

  // Update the history
  m_SystemInterface->GetHistoryManager()->
      UpdateHistory("LabelDescriptions", file, true);
}

void IRISApplication::SaveLabelDescriptions(const char *file)
{
  this->m_ColorLabelTable->SaveToFile(file);
  m_SystemInterface->GetHistoryManager()->
      UpdateHistory("LabelDescriptions", file, true);
}

bool IRISApplication::IsSnakeModeActive() const
{
  return (m_CurrentImageData == m_SNAPImageData);
}

bool IRISApplication::IsSnakeModeLevelSetActive() const
{
  return IsSnakeModeActive() && m_SNAPImageData->IsSnakeLoaded();
}

/*
void IRISApplication::ComputeSNAPSpeedImage(CommandType *progressCB)
{
  assert(IsSnakeModeActive());
  if(m_GlobalState->GetSnakeType() == EDGE_SNAKE)
    {
    m_SNAPImageData->DoEdgePreprocessing(progressCB);
    }
  else if(m_GlobalState->GetSnakeType() == IN_OUT_SNAKE)
    {
    m_SNAPImageData->DoInOutPreprocessing(progressCB);
    }

  // Mark the speed  image as valid
  m_GlobalState->SetSpeedValid(true);

  InvokeEvent(SpeedImageChangedEvent());
}
*/

void IRISApplication::SetSnakeMode(SnakeType mode)
{
  // We must be in snake mode
  assert(IsSnakeModeActive());

  // We can't be changing the snake type while there is an active preprocessing
  // mode. This should never happen in the code, so we use an assert
  assert(m_PreprocessingMode == PREPROCESS_NONE);

  // If the mode has changed, some modifications are needed
  if(mode != m_GlobalState->GetSnakeType())
    {
    // Set the actual snake mode
    m_GlobalState->SetSnakeType(mode);

    // Set the speed to invalud
    m_GlobalState->SetSpeedValid(false);

    // Set the snake parameters. TODO: see how we did this in the old
    // snap and preserve the information!!!
    m_GlobalState->SetSnakeParameters(
          mode == IN_OUT_SNAKE ?
            SnakeParameters::GetDefaultInOutParameters() :
            SnakeParameters::GetDefaultEdgeParameters());

    // Clear the speed layer
    m_SNAPImageData->InitializeSpeed();
    }
}

SnakeType IRISApplication::GetSnakeMode() const
{
  return m_GlobalState->GetSnakeType();
}


void IRISApplication::EnterPreprocessingMode(PreprocessingMode mode)
{
  // Do not reenter the same mode
  if(mode == m_PreprocessingMode)
    return;

  // Detach the current mode
  switch(m_PreprocessingMode)
    {
    case PREPROCESS_THRESHOLD:
      m_ThresholdPreviewWrapper->DetachInputsAndOutputs();
      break;

    case PREPROCESS_EDGE:
      m_EdgePreviewWrapper->DetachInputsAndOutputs();
      break;

    case PREPROCESS_GMM:
      // Deallocate whatever was created for GMM processing
      m_ClusteringEngine = NULL;
      break;

    default:
      break;
    }

  // Enter the new mode
  switch(mode)
    {
    case PREPROCESS_THRESHOLD:
      m_ThresholdPreviewWrapper->AttachInputs(m_SNAPImageData);
      m_ThresholdPreviewWrapper->AttachOutputWrapper(m_SNAPImageData->GetSpeed());
      break;

    case PREPROCESS_EDGE:
      m_EdgePreviewWrapper->AttachInputs(m_SNAPImageData);
      m_EdgePreviewWrapper->AttachOutputWrapper(m_SNAPImageData->GetSpeed());
      break;

    case PREPROCESS_GMM:

      // Create the EM class and maybe run the KNN plus method?
      m_ClusteringEngine = UnsupervisedClustering::New();
      m_ClusteringEngine->SetDataSource(m_SNAPImageData);
      break;

    default:
      break;
    }

  m_PreprocessingMode = mode;
}

PreprocessingMode IRISApplication::GetPreprocessingMode() const
{
  return m_PreprocessingMode;
}

AbstractSlicePreviewFilterWrapper *
IRISApplication
::GetPreprocessingFilterPreviewer(PreprocessingMode mode)
{
  switch(mode)
    {
    case PREPROCESS_THRESHOLD:
      return m_ThresholdPreviewWrapper;
    case PREPROCESS_EDGE:
      return m_EdgePreviewWrapper;
    default:
      return NULL;
    }
}

void
IRISApplication
::ApplyCurrentPreprocessingModeToSpeedVolume(itk::Command *progress)
{
  AbstractSlicePreviewFilterWrapper *wrapper =
      this->GetPreprocessingFilterPreviewer(m_PreprocessingMode);

  if(wrapper)
    {
    wrapper->ComputeOutputVolume(progress);
    m_GlobalState->SetSpeedValid(true);
    }
}

IRISApplication::BubbleArray&
IRISApplication::GetBubbleArray()
{
  return m_BubbleArray;
}

bool IRISApplication::InitializeActiveContourPipeline()
{
  // Initialize the segmentation with current bubbles and parameters
  if(m_SNAPImageData->InitializeSegmentation(
       m_GlobalState->GetSnakeParameters(),
       m_BubbleArray, m_GlobalState->GetDrawingColorLabel()))
    {
    // Fire an event indicating the layers have changed
    InvokeEvent(LayerChangeEvent());
    return true;
    }

  return false;
}





