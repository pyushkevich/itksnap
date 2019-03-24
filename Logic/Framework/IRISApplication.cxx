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
#include "MeshManager.h"
#include "MeshExportSettings.h"
#include "SegmentationStatistics.h"
#include "RLEImageRegionIterator.h"
#include "RLERegionOfInterestImageFilter.h"
#include "itkPasteImageFilter.h"
#include "itkIdentityTransform.h"
#include "itkResampleImageFilter.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkBSplineInterpolateImageFunction.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkWindowedSincInterpolateImageFunction.h"
#include "itkImageFileWriter.h"
#include "itkFlipImageFilter.h"
#include "itkConstantBoundaryCondition.h"
#include <itksys/SystemTools.hxx>
#include "vtkAppendPolyData.h"
#include "vtkUnsignedShortArray.h"
#include "vtkPointData.h"
#include "SNAPRegistryIO.h"
#include "Rebroadcaster.h"
#include "HistoryManager.h"
#include "IRISSlicer.h"
#include "EdgePreprocessingSettings.h"
#include "ThresholdSettings.h"
#include "SlicePreviewFilterWrapper.h"
#include "PreprocessingFilterConfigTraits.h"
#include "SmoothBinaryThresholdImageFilter.h"
#include "EdgePreprocessingImageFilter.h"
#include "UnsupervisedClustering.h"
#include "GMMClassifyImageFilter.h"
#include "DefaultBehaviorSettings.h"
#include "ColorMapPresetManager.h"
#include "ImageIODelegates.h"
#include "IRISDisplayGeometry.h"
#include "RFClassificationEngine.h"
#include "RandomForestClassifyImageFilter.h"
#include "LabelUseHistory.h"
#include "ImageAnnotationData.h"
#include "SegmentationUpdateIterator.h"
#include "AffineTransformHelper.h"

#include <stdio.h>
#include <sstream>
#include <iomanip>

IRISApplication
::IRISApplication() 
{
  // Create a new system interface
  m_SystemInterface = new SystemInterface();
  m_HistoryManager = m_SystemInterface->GetHistoryManager();

  // Create a color map preset manager
  m_ColorMapPresetManager = ColorMapPresetManager::New();
  m_ColorMapPresetManager->Initialize(m_SystemInterface);

  // Initialize the color table
  m_ColorLabelTable = ColorLabelTable::New();

  // Initialize the label use history
  m_LabelUseHistory = LabelUseHistory::New();
  m_LabelUseHistory->SetColorLabelTable(m_ColorLabelTable);

  // Contruct the IRIS and SNAP data objects
  m_IRISImageData = IRISImageData::New();
  m_IRISImageData->SetParent(this);

  m_SNAPImageData = SNAPImageData::New();
  m_SNAPImageData->SetParent(this);

  // Set the current IRIS pointer
  m_CurrentImageData = m_IRISImageData.GetPointer();

  // Listen to events from wrappers and image data objects and refire them
  // as our own events.
  Rebroadcaster::RebroadcastAsSourceEvent(m_IRISImageData, WrapperChangeEvent(), this);
  Rebroadcaster::RebroadcastAsSourceEvent(m_SNAPImageData, WrapperChangeEvent(), this);

  Rebroadcaster::RebroadcastAsSourceEvent(m_IRISImageData, LayerChangeEvent(), this);
  Rebroadcaster::RebroadcastAsSourceEvent(m_SNAPImageData, LayerChangeEvent(), this);

  // TODO: should this also be a generic Wrapper Image Data change event?
  Rebroadcaster::RebroadcastAsSourceEvent(m_SNAPImageData, LevelSetImageChangeEvent(), this);

  // Construct new global state object
  m_GlobalState = GlobalState::New();
  m_GlobalState->SetDriver(this);

  // Rebroadcast the change to the selected segmentation layer in the global state as the
  // segmentation change event.
  Rebroadcaster::Rebroadcast(m_GlobalState->GetSelectedSegmentationLayerIdModel(), ValueChangedEvent(),
                             this, SegmentationChangeEvent());

  // Initialize the preprocessing settings
  // TODO: m_ThresholdSettings = ThresholdSettings::New();
  m_EdgePreprocessingSettings = EdgePreprocessingSettings::New();

  // Initialize the preprocessing filter preview wrappers
  m_ThresholdPreviewWrapper = ThresholdPreviewWrapperType::New();
  // TODO: m_ThresholdPreviewWrapper->SetParameters(m_ThresholdSettings);

  m_EdgePreviewWrapper = EdgePreprocessingPreviewWrapperType::New();
  m_EdgePreviewWrapper->SetParameters(m_EdgePreprocessingSettings);

  m_GMMPreviewWrapper = GMMPreprocessingPreviewWrapperType::New();

  m_RandomForestPreviewWrapper = RFPreprocessingPreviewWrapperType::New();
  m_LastUsedRFClassifierComponents = 0;

  m_PreprocessingMode = PREPROCESS_NONE;

  // Initialize the mesh management object
  m_MeshManager = MeshManager::New();
  m_MeshManager->Initialize(this);

  // Data saved for restoring IRIS state while in SNAP state
  m_SavedIRISSelectedSegmentationLayerId = 0;
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

IRISApplication
::~IRISApplication() 
{
  delete m_SystemInterface;
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
  roiLabel.SetInterpolationMethod(NEAREST_NEIGHBOR);

  // Get chunk of the label image. Only the selected segmentation layer gets sent to SNAP
  LabelImageWrapper *seg = this->GetSelectedSegmentationLayer();
  LabelImageType::Pointer imgNewLabel = seg->DeepCopyRegion(roiLabel,progressCommand);

  // Filter the segmentation image to only allow voxels of 0 intensity and 
  // of the current drawing color
  LabelType passThroughLabel = m_GlobalState->GetDrawingColorLabel();

  typedef itk::ImageRegionIterator<LabelImageType> IteratorType;
  IteratorType itLabel(imgNewLabel,imgNewLabel->GetBufferedRegion());
  unsigned int nCopied = 0;
  while(!itLabel.IsAtEnd())
    {
    if(itLabel.Get() != passThroughLabel || !roi.IsSeedWithCurrentSegmentation())
      itLabel.Set((LabelType) 0);
    else
      nCopied++;
    ++itLabel;
    }

  // Record whether the segmentation has any values that are not zero
  m_GlobalState->SetSnakeInitializedWithManualSegmentation(nCopied > 0);

  // Pass the cleaned up segmentation image to SNAP
  m_SNAPImageData->SetSingleSegmentationImage(imgNewLabel);

  // Pass the label description of the drawing label to the SNAP image data
  m_SNAPImageData->SetColorLabel(
    m_ColorLabelTable->GetColorLabel(passThroughLabel));

  // Initialize the speed image of the SNAP image data
  m_SNAPImageData->InitializeSpeed();

  // Remember the ROI object
  m_GlobalState->SetSegmentationROISettings(roi);

  // Indicate that the speed image is invalid
  m_GlobalState->SetSpeedValid(false);
}

void 
IRISApplication
::SetDisplayGeometry(const IRISDisplayGeometry &dispGeom)
{
  // Store the new geometry
  m_DisplayGeometry = dispGeom;

  // Pass the geometry to the image data objects
  m_IRISImageData->SetDisplayGeometry(dispGeom);
  m_SNAPImageData->SetDisplayGeometry(dispGeom);

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

void IRISApplication::UnloadSegmentation(ImageWrapperBase *seg)
{
  // This has to happen in 'pure' IRIS mode, we are not allowed to just close segmentations in SNAP mode
  assert(!IsSnakeModeActive());

  // If the requested segmentation is the only segmentation, then call the reset method
  m_IRISImageData->UnloadSegmentation(seg);

  // Update the selected segmentation image ID to that of the added blank image
  if(m_IRISImageData->FindLayer(
       m_GlobalState->GetSelectedSegmentationLayerId(), false, LABEL_ROLE) == NULL)
    {
    m_GlobalState->SetSelectedSegmentationLayerId(
     m_IRISImageData->GetFirstSegmentationLayer()->GetUniqueId());
    }

  // Fire the appropriate event
  InvokeEvent(SegmentationChangeEvent());
}

void IRISApplication::AddBlankSegmentation()
{
  // This has to happen in 'pure' IRIS mode, we are not allowed to just close segmentations in SNAP mode
  assert(!IsSnakeModeActive());

  // Add the blank layer and set it as selected
  LabelImageWrapper *new_seg = m_IRISImageData->AddBlankSegmentation();
  m_GlobalState->SetSelectedSegmentationLayerId(new_seg->GetUniqueId());

  // Fire the appropriate event
  InvokeEvent(SegmentationChangeEvent());
}

void IRISApplication::UnloadOverlay(ImageWrapperBase *ovl)
{
  // Save the overlay associated settings
  SaveMetaDataAssociatedWithLayer(ovl, OVERLAY_ROLE);

  // Unload this overlay
  unsigned long ovl_id = ovl->GetUniqueId();
  m_IRISImageData->UnloadOverlay(ovl);

  // for overlay, we don't want to change the cursor location
  // just force the IRISSlicer to update
  m_IRISImageData->SetCrosshairs(m_GlobalState->GetCrosshairsPosition());

  // Check if the selected layer needs to be updated (default to main)
  if(m_GlobalState->GetSelectedLayerId() == ovl_id)
    m_GlobalState->SetSelectedLayerId(m_IRISImageData->GetMain()->GetUniqueId());
}

void IRISApplication::UnloadAllOverlays()
{
  LayerIterator it = m_IRISImageData->GetLayers(OVERLAY_ROLE);
  for(; !it.IsAtEnd(); ++it)
    SaveMetaDataAssociatedWithLayer(it.GetLayer(), OVERLAY_ROLE);

  m_IRISImageData->UnloadOverlays();

  // for overlay, we don't want to change the cursor location
  // just force the IRISSlicer to update
  m_IRISImageData->SetCrosshairs(m_GlobalState->GetCrosshairsPosition());

  // The selected layer should revert to main
  m_GlobalState->SetSelectedLayerId(m_IRISImageData->GetMain()->GetUniqueId());
}

void IRISApplication
::ChangeOverlayPosition(ImageWrapperBase *overlay, int dir)
{
  m_IRISImageData->MoveLayer(overlay, dir);
}

void
IRISApplication
::ResetIRISSegmentationImage()
{
  // This has to happen in 'pure' IRIS mode
  assert(!IsSnakeModeActive());

  // Reset the segmentation image
  m_IRISImageData->ResetSegmentations();

  // Update the selected segmentation image ID to that of the added blank image
  m_GlobalState->SetSelectedSegmentationLayerId(
        m_IRISImageData->GetFirstSegmentationLayer()->GetUniqueId());

  // Fire the appropriate event
  InvokeEvent(SegmentationChangeEvent());
}

void
IRISApplication
::ResetSNAPSegmentationImage()
{
  assert(m_SNAPImageData);

  // Reset the segmentation image
  m_SNAPImageData->ResetSegmentations();
  m_GlobalState->SetSelectedSegmentationLayerId(
        m_SNAPImageData->GetFirstSegmentationLayer()->GetUniqueId());

  // Fire the appropriate event
  InvokeEvent(SegmentationChangeEvent());
}

void IRISApplication::SetColorLabelsInSegmentationAsValid(LabelImageWrapper *seg_wrapper)
{
  // Iterate over the RLEs in the label image
  LabelType last_label = 0;
  typedef itk::ImageRegionConstIterator<LabelImageType::BufferType> RLLineIter;
  RLLineIter rlit(seg_wrapper->GetImage()->GetBuffer(),
                  seg_wrapper->GetImage()->GetBuffer()->GetBufferedRegion());
  for(; !rlit.IsAtEnd(); ++rlit)
    {
    // Get the line
    const LabelImageType::RLLine &line = rlit.Value();

    // Iterate over the entries
    for(int i = 0; i < line.size(); i++)
      {
      LabelType label = line[i].second;
      if(label != last_label)
        {
        m_ColorLabelTable->SetColorLabelValid(label, true);
        last_label = label;
        }
      }
    }
}


LabelImageWrapper *IRISApplication::UpdateSNAPSegmentationImage(GuidedNativeImageIO *io)
{
  // This has to happen in 'pure' SNAP mode
  assert(IsSnakeModeActive());

  typedef itk::Image<LabelType, 3> UncompressedImageType;

  // Cast the native to label type
  CastNativeImage<UncompressedImageType> caster;
  UncompressedImageType::Pointer imgUncompressed = caster(io);

  //use specialized RoI filter to convert to RLEImage
  typedef itk::RegionOfInterestImageFilter<UncompressedImageType, LabelImageType> inConverterType;
  inConverterType::Pointer inConv = inConverterType::New();
  inConv->SetInput(imgUncompressed);
  inConv->SetRegionOfInterest(imgUncompressed->GetLargestPossibleRegion());
  inConv->Update();
  LabelImageType::Pointer imgLabel = inConv->GetOutput();
  imgUncompressed = NULL; //deallocate intermediate image to save memory

  // The header of the label image is made to match that of the grey image
  imgLabel->SetOrigin(m_CurrentImageData->GetMain()->GetImageBase()->GetOrigin());
  imgLabel->SetSpacing(m_CurrentImageData->GetMain()->GetImageBase()->GetSpacing());
  imgLabel->SetDirection(m_CurrentImageData->GetMain()->GetImageBase()->GetDirection());

  // Update the iris data
  LabelImageWrapper *snap_seg = m_CurrentImageData->SetSingleSegmentationImage(imgLabel);

  // Update filenames
  snap_seg->SetFileName(io->GetFileNameOfNativeImage());

  // Set the loaded labels as valid
  this->SetColorLabelsInSegmentationAsValid(snap_seg);

  // Update the selected segmentation image
  m_GlobalState->SetSelectedSegmentationLayerId(snap_seg->GetUniqueId());

  // Let the GUI know that segmentation changed
  InvokeEvent(SegmentationChangeEvent());

  // Return pointer to the new layer
  return snap_seg;
}

LabelImageWrapper *
IRISApplication
::UpdateIRISSegmentationImage(GuidedNativeImageIO *io, Registry *metadata, bool add_to_existing)
{
  // This has to happen in 'pure' IRIS mode
  assert(!IsSnakeModeActive());

  typedef itk::Image<LabelType, 3> UncompressedImageType;

  // Cast the native to label type
  CastNativeImage<UncompressedImageType> caster;
  UncompressedImageType::Pointer imgUncompressed = caster(io);

  //use specialized RoI filter to convert to RLEImage
  typedef itk::RegionOfInterestImageFilter<UncompressedImageType, LabelImageType> inConverterType;
  inConverterType::Pointer inConv = inConverterType::New();
  inConv->SetInput(imgUncompressed);
  inConv->SetRegionOfInterest(imgUncompressed->GetLargestPossibleRegion());
  inConv->Update();
  LabelImageType::Pointer imgLabel = inConv->GetOutput();
  imgUncompressed = NULL; //deallocate intermediate image to save memory

  // Disconnect from the pipeline right away
  imgLabel->DisconnectPipeline();
  
  // The header of the label image is made to match that of the grey image
  imgLabel->SetOrigin(m_CurrentImageData->GetMain()->GetImageBase()->GetOrigin());
  imgLabel->SetSpacing(m_CurrentImageData->GetMain()->GetImageBase()->GetSpacing());
  imgLabel->SetDirection(m_CurrentImageData->GetMain()->GetImageBase()->GetDirection());

  // Update the iris data
  LabelImageWrapper *seg_wrapper =
      add_to_existing
      ? m_IRISImageData->AddSegmentationImage(imgLabel)
      : m_IRISImageData->SetSingleSegmentationImage(imgLabel);

  // Update filenames
  seg_wrapper->SetFileName(io->GetFileNameOfNativeImage());

  // Load the metadata for this layer
  LoadMetaDataAssociatedWithLayer(seg_wrapper, LABEL_ROLE, metadata);

  // Update the history
  m_SystemInterface->GetHistoryManager()->UpdateHistory(
        "LabelImage", io->GetFileNameOfNativeImage(), true);

  // Update the color label table with the segmentation values in the current segmentation
  // Iterate over the RLEs in the label image
  this->SetColorLabelsInSegmentationAsValid(seg_wrapper);

  // Update the selected segmentation image
  m_GlobalState->SetSelectedSegmentationLayerId(seg_wrapper->GetUniqueId());

  // Let the GUI know that segmentation changed
  InvokeEvent(SegmentationChangeEvent());

  // Return the pointer to the new layer
  return seg_wrapper;
}


LabelImageWrapper *IRISApplication::GetSelectedSegmentationLayer() const
{
  GenericImageData *id = this->GetCurrentImageData();
  return dynamic_cast<LabelImageWrapper *>(
        id->FindLayer(m_GlobalState->GetSelectedSegmentationLayerId(), false, LABEL_ROLE));
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

unsigned int
IRISApplication
::UpdateSegmentationWithSliceDrawing(
    IRISApplication::SliceBinaryImageType *drawing,
    const ImageCoordinateTransform *xfmSliceToImage,
    double zSlice,
    const std::string &undoTitle)
{
  // Get the segmentation image
  LabelImageType *seg = this->GetSelectedSegmentationLayer()->GetImage();

  // Turn the 2D region of the drawing into a 3D region in the segmentation
  IRISApplication::SliceBinaryImageType::RegionType r_draw = drawing->GetBufferedRegion();

  // Array of corners of the drawing region
  Vector2ui corners[4];
  corners[0][0] = r_draw.GetIndex()[0];
  corners[0][1] = r_draw.GetIndex()[1];
  corners[1][0] = r_draw.GetUpperIndex()[0];
  corners[1][1] = r_draw.GetIndex()[1];
  corners[2][0] = r_draw.GetIndex()[0];
  corners[2][1] = r_draw.GetUpperIndex()[1];
  corners[3][0] = r_draw.GetUpperIndex()[0];
  corners[3][1] = r_draw.GetUpperIndex()[1];

  // Compute 3D extents of the region
  Vector3ui pos_min, pos_max;
  for(int i = 0; i < 4; i++)
    {
    // Get the 3D coordinate of the corner
    Vector3ui idxVol = to_unsigned_int(
                         xfmSliceToImage->TransformPoint(
                           Vector3d(corners[i][0] + 0.5, corners[i][1] + 0.5, zSlice)));

    if(i == 0)
      {
      pos_min = idxVol;
      pos_max = idxVol;
      }
    else
      {
      for(int j = 0; j < 3; j++)
        {
        if(pos_min[j] > idxVol[j]) pos_min[j] = idxVol[j];
        if(pos_max[j] < idxVol[j]) pos_max[j] = idxVol[j];
        }
      }
    }

  // Define the volumetric region
  LabelImageType::RegionType r_vol;
  r_vol.SetIndex(to_itkIndex(pos_min));
  r_vol.SetUpperIndex(to_itkIndex(pos_max));
  r_vol.Crop(seg->GetBufferedRegion());

  // Create an iterator for painting
  SegmentationUpdateIterator itVol(seg, r_vol,
                                   m_GlobalState->GetDrawingColorLabel(),
                                   m_GlobalState->GetDrawOverFilter());

  // Drawing parameters
  bool invert = m_GlobalState->GetPolygonInvert();

  // Inverse transform
  ImageCoordinateTransform::Pointer xfmImageToSlice = ImageCoordinateTransform::New();
  xfmSliceToImage->ComputeInverse(xfmImageToSlice);

  // Iterate over the volume region
  for(; !itVol.IsAtEnd(); ++itVol)
    {
    // Find the coordinate of the voxel in the slice
    itk::Index<3> idx_vol = itVol.GetIndex();
    Vector3d x_slice = xfmImageToSlice->TransformPoint(
                         Vector3d(idx_vol[0] + 0.5, idx_vol[1] + 0.5, idx_vol[2] + 0.5));
    itk::Index<2> idx_slice;
    idx_slice[0] = (int) x_slice[0];
    idx_slice[1] = (int) x_slice[1];

    // Check value
    SliceBinaryImageType::PixelType px = drawing->GetPixel(idx_slice);
    if((px != 0) ^ invert)
      itVol.PaintAsForeground();


    }

  // Finalize
  itVol.Finalize();

  // Store update
  if(itVol.GetNumberOfChangedVoxels() > 0)
    {
    this->GetSelectedSegmentationLayer()->StoreUndoPoint(undoTitle.c_str(), itVol.RelinquishDelta());
    this->RecordCurrentLabelUse();
    InvokeEvent(SegmentationChangeEvent());
    }

  return itVol.GetNumberOfChangedVoxels();
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

  // The target segmentation is whatever was last selected in IRIS, which we stored
  // in a local variable before entering SNAP mode
  LabelImageWrapper *iris_seg = dynamic_cast<LabelImageWrapper *>(
                                  m_IRISImageData->FindLayer(
                                    m_SavedIRISSelectedSegmentationLayerId, false));
  TargetImageType::Pointer target = iris_seg->GetImage();

  // Construct are region of interest into which the result will be pasted
  SNAPSegmentationROISettings roi = m_GlobalState->GetSegmentationROISettings();

  // If the ROI has been resampled, resample the segmentation in reverse direction
  if(roi.IsResampling())
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
      case NEAREST_NEIGHBOR :
        fltSample->SetInterpolator(NNInterpolatorType::New());
        break;

      case TRILINEAR :
        fltSample->SetInterpolator(LinearInterpolatorType::New());
        break;

      case TRICUBIC :
        fltSample->SetInterpolator(CubicInterpolatorType::New());
        break;  

      case SINC_WINDOW_05 :
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

  // Creat the source iterator
  typedef itk::ImageRegionConstIterator<SourceImageType> SourceIteratorType;
  SourceIteratorType itSource(source,source->GetLargestPossibleRegion());

  // Create the smart target iterator
  SegmentationUpdateIterator itTarget(
        target, roi.GetROI(),
        m_GlobalState->GetDrawingColorLabel(), m_GlobalState->GetDrawOverFilter());

  // Inversion state
  bool invert = m_GlobalState->GetPolygonInvert();

  // Go through both iterators, copy the new over the old
  while(!itSource.IsAtEnd())
    {
    // Get the level set value
    float voxSNAP = itSource.Value();
    if((!invert && voxSNAP <= 0) || (invert && voxSNAP >= 0))
      itTarget.PaintAsForeground();
    else
      itTarget.PaintAsBackground();

    // Iterate
    ++itSource;
    ++itTarget;
    }

  // Finalize the segmentation
  itTarget.Finalize();

  // Store the undo delta
  if(itTarget.GetNumberOfChangedVoxels() > 0)
    {
    iris_seg->StoreUndoPoint("Automatic Segmentation", itTarget.RelinquishDelta());
    RecordCurrentLabelUse();
    InvokeEvent(SegmentationChangeEvent());
    }
}

void
IRISApplication
::SetCursorPosition(const Vector3ui cursor, bool force)
{
  if(cursor != this->GetCursorPosition() || force)
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
::RecordCurrentLabelUse()
{
  m_LabelUseHistory->RecordLabelUse(
        m_GlobalState->GetDrawingColorLabel(),
        m_GlobalState->GetDrawOverFilter());
}

void
IRISApplication
::ClearUndoPoints()
{
  m_IRISImageData->ClearUndoPoints();
  m_SNAPImageData->ClearUndoPoints();
}

bool
IRISApplication
::IsUndoPossible()
{
  LabelImageWrapper *seg = this->GetSelectedSegmentationLayer();
  return seg && seg->IsUndoPossible();
}

void
IRISApplication
::Undo()
{
  this->GetSelectedSegmentationLayer()->Undo();
  InvokeEvent(SegmentationChangeEvent());
}

bool
IRISApplication
::IsRedoPossible()
{
  LabelImageWrapper *seg = this->GetSelectedSegmentationLayer();
  return seg && seg->IsRedoPossible();
}

void
IRISApplication
::Redo()
{
  this->GetSelectedSegmentationLayer()->Redo();
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
::TransferCursor(GenericImageData *source, GenericImageData *target)
{
  Vector3d cursorSource = to_double(this->GetCursorPosition());

  Vector3d xyzSource =
      source->GetMain()->TransformVoxelCIndexToNIFTICoordinates(cursorSource);

  itk::Index<3> indexTarget =
      to_itkIndex(target->GetMain()->TransformNIFTICoordinatesToVoxelCIndex(xyzSource));

  Vector3ui newCursor =
      target->GetMain()->GetBufferedRegion().IsInside(indexTarget)
      ? Vector3ui(indexTarget)
      : target->GetMain()->GetSize() / 2u;

  // Store the cursor position in the global state and the target image data
  m_GlobalState->SetCrosshairsPosition(newCursor);
  target->SetCrosshairs(newCursor);

  // Fire the appropriate event
  InvokeEvent(CursorUpdateEvent());
}

void 
IRISApplication
::SetCurrentImageDataToIRIS() 
{
  assert(m_IRISImageData);
  if(m_CurrentImageData != m_IRISImageData)
    {
    m_CurrentImageData = m_IRISImageData;
    TransferCursor(m_SNAPImageData, m_IRISImageData);
    InvokeEvent(MainImageDimensionsChangeEvent());

    // Set the selected layer ID to the main image
    m_GlobalState->SetSelectedLayerId(m_IRISImageData->GetMain()->GetUniqueId());

    // Restore the last selected segmentation layer in IRIS
    m_GlobalState->SetSelectedSegmentationLayerId(m_SavedIRISSelectedSegmentationLayerId);
    m_SavedIRISSelectedSegmentationLayerId = 0;
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
    TransferCursor(m_IRISImageData, m_SNAPImageData);

    // Set the image data
    m_CurrentImageData = m_SNAPImageData;

    // Fire the event
    InvokeEvent(MainImageDimensionsChangeEvent());

    // Upon entering this mode, we need reset the active tools
    m_GlobalState->SetToolbarMode(CROSSHAIRS_MODE);
    m_GlobalState->SetToolbarMode3D(TRACKBALL_MODE);

    // Set the selected layer ID to the main image
    m_GlobalState->SetSelectedLayerId(m_SNAPImageData->GetMain()->GetUniqueId());

    // Save the currently selected segmentation layer Id so that we can restore it later
    m_SavedIRISSelectedSegmentationLayerId = m_GlobalState->GetSelectedSegmentationLayerId();
    m_GlobalState->SetSelectedSegmentationLayerId(m_SNAPImageData->GetFirstSegmentationLayer()->GetUniqueId());
    }
}

int IRISApplication::GetImageDirectionForAnatomicalDirection(AnatomicalDirection iAnat)
{
  std::string myrai = this->GetImageToAnatomyRAI();
  
  string rai1 = "SRA", rai2 = "ILP";
  
  char c1 = rai1[iAnat], c2 = rai2[iAnat];
  for(int j = 0; j < 3; j++)
    if(myrai[j] == c1 || myrai[j] == c2)
      return j;
  
  assert(0);
  return 0;
}

int
IRISApplication
::GetDisplayWindowForAnatomicalDirection(
  AnatomicalDirection iAnat) const
{
  return m_DisplayGeometry.GetDisplayWindowForAnatomicalDirection(iAnat);
}

AnatomicalDirection
IRISApplication::GetAnatomicalDirectionForDisplayWindow(int iWin) const
{
  return m_DisplayGeometry.GetAnatomicalDirectionForDisplayWindow(iWin);
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
  SegmentationStatistics stats;
  stats.Compute(this);

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
  // Update the list of VTK meshes
  m_MeshManager->UpdateVTKMeshes(progress);

  // Get the list of available labels
  MeshManager::MeshCollection meshes = m_MeshManager->GetMeshes();
  MeshManager::MeshCollection::iterator it;

  // If in SNAP mode, just save the first mesh
  if(m_SNAPImageData->IsMainLoaded())
    {
    if(meshes.size() != 1)
      throw IRISException("Unexpected number of meshes in SNAP mode");

    // Get the VTK mesh for the label
    it = meshes.begin();
    vtkPolyData *mesh = it->second;

    // Export the mesh
    GuidedMeshIO io;
    Registry rFormat = sets.GetMeshFormat();
    io.SaveMesh(sets.GetMeshFileName().c_str(), rFormat, mesh);
    }

  // If only one mesh is to be exported, life is easy
  else if(sets.GetFlagSingleLabel())
    {
    // Get the VTK mesh for the label
    it = meshes.find(sets.GetExportLabel());
    if(it == meshes.end())
      throw IRISException("Missing mesh for the selected label");

    vtkPolyData *mesh = it->second;

    // Export the mesh
    GuidedMeshIO io;
    Registry rFormat = sets.GetMeshFormat();
    io.SaveMesh(sets.GetMeshFileName().c_str(), rFormat, mesh);
    }
  else if(sets.GetFlagSingleScene())
    {
    // Create an append filter
    vtkSmartPointer<vtkAppendPolyData> append = vtkSmartPointer<vtkAppendPolyData>::New();

    for(it = meshes.begin(); it != meshes.end(); it++)
      {
      // Get the VTK mesh for the label
      vtkPolyData *mesh = it->second;
      vtkSmartPointer<vtkUnsignedShortArray> scalar =
          vtkSmartPointer<vtkUnsignedShortArray>::New();

      scalar->SetNumberOfComponents(1);

      scalar->Allocate(mesh->GetNumberOfPoints());
      for(int j = 0; j < mesh->GetNumberOfPoints(); j++)
        scalar->InsertNextTuple1(it->first);

      mesh->GetPointData()->SetScalars(scalar);
      append->AddInputData(mesh);
      }

    append->Update();

    // Export the mesh
    GuidedMeshIO io;
    Registry rFormat = sets.GetMeshFormat();
    io.SaveMesh(sets.GetMeshFileName().c_str(), rFormat, append->GetOutput());

    // Remove the scalars from the meshes
    for(it = meshes.begin(); it != meshes.end(); it++)
      it->second->GetPointData()->SetScalars(NULL);
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
    for(it = meshes.begin(); it != meshes.end(); it++)
      {
      // Get the VTK mesh for the label
      vtkPolyData *mesh = it->second;

      // Generate filename
      char outfn[4096];
      sprintf(outfn, "%s/%s%05d%s", path.c_str(), prefix.c_str(), it->first, extn.c_str());

      // Export the mesh
      GuidedMeshIO io;
      Registry rFormat = sets.GetMeshFormat();
      io.SaveMesh(outfn, rFormat, mesh);
      }
    }
}

size_t
IRISApplication
::ReplaceLabel(LabelType drawing, LabelType drawover)
{
  // Get the label image
  LabelImageWrapper::ImageType *imgLabel = this->GetSelectedSegmentationLayer()->GetImage();

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
  // Number of voxels matching current label
  size_t nvoxels = 0;

  // We must iterate over all the label images
  for(LayerIterator it = this->GetCurrentImageData()->GetLayers(LABEL_ROLE);
      !it.IsAtEnd(); ++it)
    {
    LabelImageWrapper *wrapper = dynamic_cast<LabelImageWrapper *>(it.GetLayer());
    LabelImageType *seg = wrapper->GetImage();

    // Get the number of voxels
    for(LabelImageWrapper::ConstIterator it(seg, seg->GetBufferedRegion());
        !it.IsAtEnd(); ++it)
      {
      if(it.Get() == label)
        ++nvoxels;
      }
    }

  return nvoxels;
}


int
IRISApplication
::RelabelSegmentationWithCutPlane(const Vector3d &normal, double intercept) 
{
  // Get the label image
  LabelImageWrapper::ImageType *imgLabel = this->GetSelectedSegmentationLayer()->GetImage();
  
  // Create the smart target iterator
  SegmentationUpdateIterator it(
        imgLabel, imgLabel->GetBufferedRegion(),
        m_GlobalState->GetDrawingColorLabel(), m_GlobalState->GetDrawOverFilter());

  // Adjust the intercept by 0.5 for voxel offset
  intercept -= 0.5 * (normal[0] + normal[1] + normal[2]);

  // Iterate over the image, relabeling labels on one side of the plane
  while(!it.IsAtEnd())
    {
    // Compute the distance to the plane
    itk::Index<3> index = it.GetIndex();
    double distance = 
      index[0]*normal[0] + 
      index[1]*normal[1] + 
      index[2]*normal[2] - intercept;

    // Check the side of the plane
    if(distance > 0)
      it.PaintAsForegroundPreserveClear();

    // Next voxel
    ++it;
    }

  // Finalize
  it.Finalize();

  // Store the undo point if needed
  if(it.GetNumberOfChangedVoxels() > 0)
    {
    this->GetSelectedSegmentationLayer()->StoreUndoPoint("3D scalpel", it.RelinquishDelta());
    RecordCurrentLabelUse();
    InvokeEvent(SegmentationChangeEvent());
    }

  return it.GetNumberOfChangedVoxels();
}

int 
IRISApplication
::GetRayIntersectionWithSegmentation(const Vector3d &point, 
                                     const Vector3d &ray, Vector3i &hit) const
{
  // Get the label wrapper
  LabelImageWrapper *xLabelWrapper = this->GetSelectedSegmentationLayer();
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

    LabelType hitlabel = xLabelWrapper->GetVoxel(lIndex);

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
::AddIRISOverlayImage(GuidedNativeImageIO *io, Registry *metadata)
{
  assert(!IsSnakeModeActive());
  assert(m_IRISImageData->IsMainLoaded());
  assert(io->IsNativeImageLoaded());

  // Test if the image is in the same size as the main image
  ImageWrapperBase *main = this->m_IRISImageData->GetMain();
  bool same_size = (main->GetSize() == io->GetDimensionsOfNativeImage());

  // Now test the 3D geometry of the image to see if it occupies the same space
  bool same_space = true;

  // Read the transform from the registry. This method will return an identity transform
  // even if no registry was provided
  SmartPtr<AffineTransformHelper::ITKTransformBase> transform =
      AffineTransformHelper::ReadFromRegistry(metadata);

  // We use a tolerance for header comparisons here
  double tol = 1e-5;

  for(int i = 0; i < 3; i++)
    {
    if(fabs(io->GetNativeImage()->GetOrigin()[i] - main->GetImageBase()->GetOrigin()[i]) > tol)
      same_space = false;
    if(fabs(io->GetNativeImage()->GetSpacing()[i] - main->GetImageBase()->GetSpacing()[i]) > tol)
      same_space = false;
    for(int j = 0; j < 3; j++)
      {
      if(fabs(io->GetNativeImage()->GetDirection()[i][j] - main->GetImageBase()->GetDirection()[i][j]) > tol)
        same_space = false;
      }
    }

  // TODO: in situations where the size is the same and space is different, we may want
  // to ask the user how to handle it, or at least display a warning? For now, we just use
  // the header information, which may be different from how old ITK-SNAP handled this

  // Old code - prevented registration of same-size images
  // if(same_size && same_space && id_transform)
  //  m_IRISImageData->AddOverlay(io);
  // else
  m_IRISImageData->AddCoregOverlay(io, transform);

  ImageWrapperBase *layer = m_IRISImageData->GetLastOverlay();

  // Set the filename of the overlay
  // TODO: this is cumbersome, could we just initialize the wrapper from the
  // GuidedNativeImageIO without passing all this junk around?
  layer->SetFileName(io->GetFileNameOfNativeImage());

  // Add the overlay to the history
  m_HistoryManager->UpdateHistory("AnatomicImage", io->GetFileNameOfNativeImage(), true);

  // for overlay, we don't want to change the cursor location
  // just force the IRISSlicer to update
  m_IRISImageData->SetCrosshairs(m_GlobalState->GetCrosshairsPosition());

  // Apply the default color map for overlays
  std::string deflt_preset =
      m_GlobalState->GetDefaultBehaviorSettings()->GetOverlayColorMapPreset();
  m_ColorMapPresetManager->SetToPreset(layer->GetDisplayMapping()->GetColorMap(),
                                       deflt_preset);

  // Initialize the layer-specific segmentation parameters
  CreateSegmentationSettings(layer, OVERLAY_ROLE);

  // Read and apply the project-level settings associated with the main image
  LoadMetaDataAssociatedWithLayer(layer, OVERLAY_ROLE, metadata);

  // If the default is to auto-contrast, perform the contrast adjustment
  // operation on the image
  if(m_GlobalState->GetDefaultBehaviorSettings()->GetAutoContrast())
    AutoContrastLayerOnLoad(layer);

  // Set the selected layer ID to be the new selected overlay - but only if it is
  // not sticky!
  if(!layer->IsSticky())
    m_GlobalState->SetSelectedLayerId(layer->GetUniqueId());
}

void
IRISApplication
::AddDerivedOverlayImage(
    const ImageWrapperBase *sourceLayer,
    ImageWrapperBase *overlay,
    bool inherit_colormap)
{
  assert(this->IsMainImageLoaded());

  // Add the image as the current grayscale overlay
  m_CurrentImageData->AddOverlay(overlay);

  // for overlay, we don't want to change the cursor location
  // just force the IRISSlicer to update
  m_CurrentImageData->SetCrosshairs(m_GlobalState->GetCrosshairsPosition());

  // Apply the default color map for overlays
  if(inherit_colormap)
    {
    const ColorMap *cmSource = sourceLayer->GetDisplayMapping()->GetColorMap();
    ColorMap *cmOverlay = overlay->GetDisplayMapping()->GetColorMap();
    if(cmSource && cmOverlay)
      cmOverlay->CopyInformation(cmSource);
    }
  else
    {
    std::string deflt_preset =
        m_GlobalState->GetDefaultBehaviorSettings()->GetOverlayColorMapPreset();
    m_ColorMapPresetManager->SetToPreset(overlay->GetDisplayMapping()->GetColorMap(),
                                         deflt_preset);
    }

  // Initialize the layer-specific segmentation parameters
  CreateSegmentationSettings(overlay, OVERLAY_ROLE);

  // If the default is to auto-contrast, perform the contrast adjustment
  // operation on the image
  if(m_GlobalState->GetDefaultBehaviorSettings()->GetAutoContrast())
    AutoContrastLayerOnLoad(overlay);

  // Set the selected layer ID to be the new overlay
  if(!overlay->IsSticky())
    m_GlobalState->SetSelectedLayerId(overlay->GetUniqueId());
}

void
IRISApplication
::AutoContrastLayerOnLoad(ImageWrapperBase *layer)
{
  // Get a pointer to the policy for this layer
  AbstractContinuousImageDisplayMappingPolicy *policy =
      dynamic_cast<AbstractContinuousImageDisplayMappingPolicy *>(
        m_IRISImageData->GetMain()->GetDisplayMapping());

  // The policy must be of the right type to proceed
  if(policy)
    {
    // Check if the image contrast is already set by the user
    if(policy->IsContrastInDefaultState())
      policy->AutoFitContrast();
    }
}

void
IRISApplication
::CreateSegmentationSettings(ImageWrapperBase *wrapper, LayerRole role)
{
  // Create threshold settings for every scalar component of this wrapper
  if(wrapper->IsScalar())
    {
    // Create threshold settings for this wrapper
    SmartPtr<ThresholdSettings> ts = ThresholdSettings::New();
    wrapper->SetUserData("ThresholdSettings", ts);
    }
  else
    {
    // Call the method recursively for the components
    VectorImageWrapperBase *vec = dynamic_cast<VectorImageWrapperBase *>(wrapper);
    for(ScalarRepresentationIterator it(vec); !it.IsAtEnd(); ++it)
      CreateSegmentationSettings(vec->GetScalarRepresentation(it), role);
    }
}

void
IRISApplication
::UpdateIRISMainImage(GuidedNativeImageIO *io, Registry *metadata)
{
  // This has to happen in 'pure' IRIS mode
  assert(!IsSnakeModeActive());

  // Load the image into the current image data object
  m_IRISImageData->SetMainImage(io);

  // Get a pointer to the resulting wrapper
  ImageWrapperBase *layer = m_IRISImageData->GetMain();

  // Set the filename and nickname of the image wrapper
  layer->SetFileName(io->GetFileNameOfNativeImage());

  // Initialize the color label table to defaults
  m_ColorLabelTable->InitializeToDefaults();

  // Update the preprocessing settings to defaults.
  m_EdgePreprocessingSettings->InitializeToDefaults();

  // Initialize the layer-specific segmentation parameters
  CreateSegmentationSettings(layer, MAIN_ROLE);

  // Update the system's history list
  m_HistoryManager->UpdateHistory("MainImage", io->GetFileNameOfNativeImage(), false);
  m_HistoryManager->UpdateHistory("AnatomicImage", io->GetFileNameOfNativeImage(), false);

  // Reset the segmentation ROI
  m_GlobalState->SetSegmentationROI(io->GetNativeImage()->GetBufferedRegion());

  // Read and apply the project-level settings associated with the main image
  LoadMetaDataAssociatedWithLayer(layer, MAIN_ROLE, metadata);

  // The main image may not be sticky, but in old versions of SNAP that was
  // allowed, so we force override
  if(layer->IsSticky())
    layer->SetSticky(false);

  // Fire the dimensions change event
  InvokeEvent(MainImageDimensionsChangeEvent());

  // Update the crosshairs position to the center of the image
  this->SetCursorPosition(layer->GetSize() / 2u);

  // This line forces the cursor to be propagated to the image even if the
  // crosshairs positions did not change from their previous values
  this->GetIRISImageData()->SetCrosshairs(layer->GetSize() / 2u);

  // If the default is to auto-contrast, perform the contrast adjustment
  // operation on the image
  if(m_GlobalState->GetDefaultBehaviorSettings()->GetAutoContrast())
    AutoContrastLayerOnLoad(layer);

  // Save the thumbnail for the current image. This ensures that a thumbnail
  // is created even if the application crashes or is killed.
  ImageWrapperBase::DisplaySlicePointer thumbnail = layer->MakeThumbnail(128);
  m_SystemInterface->WriteThumbnail(io->GetFileNameOfNativeImage().c_str(), thumbnail);

  // We also want to reset the label history at this point, as these are
  // very different labels
  m_LabelUseHistory->Reset();

  // Make the main image 'selected'
  m_GlobalState->SetSelectedLayerId(layer->GetUniqueId());

  // Make the new segmentation selected (at this point there is only one to choose from)
  m_GlobalState->SetSelectedSegmentationLayerId(
        this->GetIRISImageData()->GetFirstSegmentationLayer()->GetUniqueId());
}

void IRISApplication::LoadMetaDataAssociatedWithLayer(
    ImageWrapperBase *layer, int role, Registry *override)
{
  Registry assoc, *folder;


  if(override)
    folder = override;
  else if(m_SystemInterface->FindRegistryAssociatedWithFile(layer->GetFileName(), assoc))
    {
    LayerRole role_cast = (LayerRole) role;

    // Determine the group under which the association is stored. This is to
    // deal with the situation when the same image is loaded as a main and as
    // a segmentation, for example
    std::string roletype;
    if(role_cast == MAIN_ROLE || role_cast == OVERLAY_ROLE)
      roletype = "AnatomicImage";
    else
      roletype = SNAPRegistryIO::GetEnumMapLayerRole()[role_cast].c_str();

    folder = &assoc.Folder(Registry::Key("Role[%s]", roletype.c_str()));
    }
  else
    return;

  // Read the image-level metadata (display map, etc) for the image
  layer->ReadMetaData(folder->Folder("LayerMetaData"));

  // Read the tags for the image. This should have probably been placed into the LayerMeteData
  // but it's a pain to move things around in the registry file
  TagList tags;
  (*folder)["Tags"].GetList(tags);
  layer->SetTags(tags);

  // Read and apply the project-level settings associated with the main image
  if(role == MAIN_ROLE)
    {
    SNAPRegistryIO rio;
    rio.ReadImageAssociatedSettings(folder->Folder("ProjectMetaData"), this, true, true, true, true);
    }
}


void IRISApplication
::SaveMetaDataAssociatedWithLayer(ImageWrapperBase *layer, int role, Registry *override)
{
  Registry assoc, *folder;

  // Load the current associations for the main image
  if(override)
    {
    folder = override;
    }
  else
    {
    m_SystemInterface->FindRegistryAssociatedWithFile(layer->GetFileName(), assoc);

    LayerRole role_cast = (LayerRole) role;

    // Determine the group under which the association is stored. This is to
    // deal with the situation when the same image is loaded as a main and as
    // a segmentation, for example
    std::string roletype;
    if(role_cast == MAIN_ROLE || role_cast == OVERLAY_ROLE)
      roletype = "AnatomicImage";
    else
      roletype = SNAPRegistryIO::GetEnumMapLayerRole()[role_cast].c_str();

    folder = &assoc.Folder(Registry::Key("Role[%s]", roletype.c_str()));
    }

  // Write the metadata for the specific layer
  layer->WriteMetaData(folder->Folder("LayerMetaData"));

  // Write the tags
  (*folder)["Tags"].PutList(layer->GetTags());

  // Write the layer IO hints - overriding the association file data
  if(!layer->GetIOHints().IsEmpty())
    {
    folder->Folder("IOHints").Clear();
    folder->Folder("IOHints").Update(layer->GetIOHints());
    }

  // For the main image layer, write the project-level settings
  if(role == MAIN_ROLE)
    {
    // Write the project-level associations
    SNAPRegistryIO io;
    io.WriteImageAssociatedSettings(this, folder->Folder("ProjectMetaData"));
    }

  // Save the settings
  if(!override)
    m_SystemInterface->AssociateRegistryWithFile(layer->GetFileName(), assoc);
}

void
IRISApplication
::UnloadMainImage()
{
  // Save the settings for this image
  if(m_CurrentImageData->IsMainLoaded())
    {
    ImageWrapperBase *main_image = m_CurrentImageData->GetMain();
    const char *fnMain = main_image->GetFileName();

    // Reset the toolbar mode to default
    m_GlobalState->SetToolbarMode(CROSSHAIRS_MODE);

    // Write the image-level and project-level associations
    SaveMetaDataAssociatedWithLayer(main_image, MAIN_ROLE);

    // Create a thumbnail from the one of the image slices
    ImageWrapperBase::DisplaySlicePointer thumbnail = main_image->MakeThumbnail(128);
    m_SystemInterface->WriteThumbnail(fnMain, thumbnail);

    // Do likewise for the project if one exists
    if(m_GlobalState->GetProjectFilename().length())
      {
      // TODO: it would look nicer if we actually saved the state of the SNAP
      // windows rather than just the image in its current colormap. But this
      // would require doing this elsewhere
      m_SystemInterface->WriteThumbnail(m_GlobalState->GetProjectFilename().c_str(), thumbnail);
      }
    }

  // Reset the automatic segmentation ROI
  m_GlobalState->SetSegmentationROI(GlobalState::RegionType());

  // Unload the main image
  m_CurrentImageData->UnloadMainImage();

  // After unloading the main image, we reset the workspace filename
  m_GlobalState->SetProjectFilename("");

  // Reset the project registry
  m_LastSavedProjectState = Registry();

  // Reset the local history
  m_HistoryManager->ClearLocalHistory();

  // Let everyone know that the main image is gone!
  InvokeEvent(MainImageDimensionsChangeEvent());
}

ImageWrapperBase *
IRISApplication
::LoadImageViaDelegate(const char *fname,
                       AbstractLoadImageDelegate *del,
                       IRISWarningList &wl,
                       Registry *ioHints)
{
  Registry regAssoc;

  // When hints are not provided, we load them using the association system
  if(!ioHints)
    {
    // Load the settings associated with this file
    m_SystemInterface->FindRegistryAssociatedWithFile(fname, regAssoc);

    // Get the folder dealing with grey image properties
    ioHints = &regAssoc.Folder("Files.Grey");
    }

  // Create a native image IO object
  SmartPtr<GuidedNativeImageIO> io = GuidedNativeImageIO::New();

  // Load the header of the image
  io->ReadNativeImageHeader(fname, *ioHints);

  // Validate the header
  del->ValidateHeader(io, wl);

  // Unload the current image data
  del->UnloadCurrentImage();

  // Read the image body
  io->ReadNativeImageData();

  // Validate the image data
  del->ValidateImage(io, wl);

  // Put the image in the right place
  ImageWrapperBase *layer = del->UpdateApplicationWithImage(io);

  // Store the IO hints inside of the image - in case it ever gets added
  // to a project
  layer->SetIOHints(*ioHints);

  return layer;
}

IRISApplication::DicomSeriesTree
IRISApplication::ListAvailableSiblingDicomSeries()
{
  // Create an empty listing
  DicomSeriesTree available_dicoms;

  // Create a structure to keep track of already loaded DICOM series so they
  // are not included
  std::map< std::string, std::set<std::string> > loaded_dicoms;

  // Iterate through the loaded image layers
  LayerIterator it = this->GetIRISImageData()->GetLayers(MAIN_ROLE | OVERLAY_ROLE);
  for(; !it.IsAtEnd(); ++it)
    {
    // Get the IO hints registry
    Registry io_hints = it.GetLayer()->GetIOHints();

    // Is this image a DICOM?
    if(io_hints.HasFolder("DICOM"))
      {
      // Get the directory of the DICOM files
      std::string layer_fn = it.GetLayer()->GetFileName();
      if(!itksys::SystemTools::FileIsDirectory(layer_fn))
        layer_fn = itksys::SystemTools::GetParentDirectory(layer_fn);

      // Get the series ID of the DICOM files
      std::string layer_series_id = io_hints["DICOM.SeriesId"][""];
      loaded_dicoms[layer_fn].insert(layer_series_id);

      // Has this directory already been included? Then we can skip the rest
      if(available_dicoms.find(layer_fn) == available_dicoms.end())
        {
        // Get the number of DICOM siblings
        int n_entries = io_hints["DICOM.DirectoryInfo.ArraySize"][0];
        for(int i = 0; i < n_entries; i++)
          {
          // Read the entry for this ID
          DicomSeriesDescriptor desc;
          Registry &r = io_hints.Folder(io_hints.Key("DICOM.DirectoryInfo.Entry[%d]", i));
          desc.series_id = r["SeriesId"][""];
          if(desc.series_id.length())
            {
            desc.series_desc = r["SeriesDescription"][""];
            desc.dimensions = r["Dimensions"][""];
            desc.layer_uid = it.GetLayer()->GetUniqueId();
            available_dicoms[layer_fn].push_back(desc);
            }
          }
        }
      }
    }

  // Loop again and remove series that are already loaded
  DicomSeriesTree::iterator it_map = available_dicoms.begin();
  while(it_map != available_dicoms.end())
    {
    DicomSeriesListing::iterator it_list = it_map->second.begin();
    while(it_list != it_map->second.end())
      {
      if(loaded_dicoms[it_map->first].count(it_list->series_id))
        it_map->second.erase(it_list++);
      else
        it_list++;
      }

    if(it_map->second.size() == 0)
      available_dicoms.erase(it_map++);
    else
      it_map++;
    }

  // Return the map
  return available_dicoms;
}

#include "MetaDataAccess.h"

void IRISApplication
::AssignNicknameFromDicomMetadata(ImageWrapperBase *layer)
{
  const std::string tag = "0008|103e";
  MetaDataAccess mda(layer->GetImageBase());
  if(mda.HasKey(tag))
    layer->SetCustomNickname(mda.GetValueAsString(tag));
}

void IRISApplication
::LoadAnotherDicomSeriesViaDelegate(unsigned long reference_layer_id,
                                    const char *series_id,
                                    AbstractLoadImageDelegate *del,
                                    IRISWarningList &wl)
{
  // We will use the main image's IO hints to create the IO hints for the
  // image that is being loaded.
  ImageWrapperBase *ref =
      this->GetIRISImageData()->FindLayer(reference_layer_id, false);

  if(ref)
    {
    // Create a copy of these hints for the new image we are loading
    Registry io_hints = ref->GetIOHints();

    // Replace the SeriesID with the one we are intending to load
    io_hints["DICOM.SeriesId"] << series_id;

    // Use the current filename of the main image
    ImageWrapperBase *layer =
        this->LoadImageViaDelegate(ref->GetFileName(), del, wl, &io_hints);

    // Assign the series ID of the loaded image as the nickname
    if(layer->GetCustomNickname().length() == 0)
      this->AssignNicknameFromDicomMetadata(layer);
    }
}

void IRISApplication
::LoadImage(const char *fname, LayerRole role, IRISWarningList &wl,
            Registry *meta_data_reg, Registry *io_hints_reg, bool additive)
{
  // Pointer to the delegate
  SmartPtr<AbstractLoadImageDelegate> delegate;

  switch(role)
    {
    case MAIN_ROLE:
      delegate = LoadMainImageDelegate::New().GetPointer();
      break;
    case OVERLAY_ROLE:
      delegate = LoadOverlayImageDelegate::New().GetPointer();
      break;
    case LABEL_ROLE:
      {
      SmartPtr<LoadSegmentationImageDelegate> d = LoadSegmentationImageDelegate::New();
      d->SetAdditiveMode(additive);
      delegate = d.GetPointer();
      }
      break;
    default:
      throw IRISException("LoadImage does not support role %d", role);
    }

  delegate->Initialize(this);
  if(meta_data_reg)
    delegate->SetMetaDataRegistry(meta_data_reg);

  // Load via delegate, providing the IO hints
  this->LoadImageViaDelegate(fname, delegate, wl, io_hints_reg);
}

SmartPtr<AbstractSaveImageDelegate>
IRISApplication::CreateSaveDelegateForLayer(ImageWrapperBase *layer, LayerRole role)
{
  // TODO: need some unified way of handling histories and categories

  // Which history does the image belong under? This goes beyond the role
  // of the image, as in snake mode, there are sub-roles that the wrappers
  // have. The safest thing is to have the history information be stored
  // as a kind of user data in each wrapper. However, for now, we will just
  // infer it from the role and type
  std::string history, category;
  if(role == MAIN_ROLE)
    {
    history = "AnatomicImage";
    category = "Image";
    }

  else if(role == LABEL_ROLE)
    {
    history = "LabelImage";
    category = "Segmentation Image";

    if(this->IsSnakeModeActive() && this->GetPreprocessingMode() == PREPROCESS_RF)
      {
      history = "ClassifierSamples";
      category = "Classifier Samples Image";
      }
    }

  else if(role == OVERLAY_ROLE)
    {
    history = "AnatomicImage";
    category = "Image";
    }

  else if(role == SNAP_ROLE)
    {
    if(dynamic_cast<SpeedImageWrapper *>(layer))
      {
      history = "SpeedImage";
      category = "Speed Image";
      }

    else if(dynamic_cast<LevelSetImageWrapper *>(layer))
      {
      history = "LevelSetImage";
      category = "Level Set Image";
      }
    }

  // Create delegate
  SmartPtr<DefaultSaveImageDelegate> delegate = DefaultSaveImageDelegate::New();
  delegate->Initialize(this, layer, history);
  delegate->SetCategory(category);

  // Return the delegate
  return delegate.GetPointer();
}


void IRISApplication::SaveProjectToRegistry(Registry &preg, const std::string proj_file_full)
{
  // Clear the registry contents
  preg.Clear();

  // Get the directory in which the project will be saved
  std::string project_dir = itksys::SystemTools::GetParentDirectory(proj_file_full.c_str());

  // Put version information - later versions may not be compatible
  preg["Version"] << SNAPCurrentVersionReleaseDate;

  // Save the directory to the project file. This allows us to deal with the project
  // being moved elsewhere in the filesystem
  preg["SaveLocation"] << project_dir;

  // Save each of the layers with 'saveable' roles
  int i = 0;
  for(LayerIterator it = GetCurrentImageData()->GetLayers(
        MAIN_ROLE | LABEL_ROLE | OVERLAY_ROLE); !it.IsAtEnd(); ++it)
    {
    ImageWrapperBase *layer = it.GetLayer();

    // Get the filename of the layer
    const char *filename = layer->GetFileName();

    // If the layer does not have a filename, skip it
    if(!filename || strlen(filename) == 0)
      continue;

    // Get the full name of the image file
    std::string layer_file_full = itksys::SystemTools::CollapseFullPath(filename);

    // Create a folder for this layer
    Registry &folder = preg.Folder(Registry::Key("Layers.Layer[%03d]", i++));

    // Put the filename and relative filename into the folder
    folder["AbsolutePath"] << layer_file_full;

    // Put the role associated with the file into the folder
    folder["Role"].PutEnum(SNAPRegistryIO::GetEnumMapLayerRole(), it.GetRole());

    // Save the metadata associated with the layer
    SaveMetaDataAssociatedWithLayer(layer, it.GetRole(), &folder);

    // Save the layer transform - relevant only for overlays
    if(it.GetRole() == OVERLAY_ROLE)
      {
      AffineTransformHelper::WriteToRegistry(&folder, layer->GetITKTransform());
      }
    }

  // Save the annotations in the workspace
  Registry &ann_folder = preg.Folder("Annotations");
  this->m_IRISImageData->GetAnnotations()->SaveAnnotations(ann_folder);

  // Recursively search and delete empty folders
  preg.CleanZeroSizeArrays();
  preg.CleanEmptyFolders();
}

void IRISApplication::SaveProject(const std::string &proj_file)
{
  // Header for ITK-SNAP projects
  static const char *header =
      "ITK-SNAP (itksnap.org) Project File\n"
      "\n"
      "This file can be moved/copied along with the images that it references\n"
      "as long as the relative location of the images to the project file is \n"
      "the same. Do not modify the SaveLocation entry, or this will not work.\n";

  // Get the full name of the project file
  std::string proj_file_full = itksys::SystemTools::CollapseFullPath(proj_file.c_str());

  // Create a registry that will be used to save the project
  Registry preg;

  // Do the actual writing to the registry
  SaveProjectToRegistry(preg, proj_file_full);

  // Finally, save the registry
  preg.WriteToXMLFile(proj_file_full.c_str(), header);

  // Save the project filename
  m_GlobalState->SetProjectFilename(proj_file_full.c_str());

  // Update the history
  m_SystemInterface->GetHistoryManager()->
      UpdateHistory("Project", proj_file_full, false);

  // Store the project registry
  m_LastSavedProjectState = preg;
}

void IRISApplication::OpenProject(
    const std::string &proj_file, IRISWarningList &warn)
{
  // Load the registry file
  Registry preg;
  preg.ReadFromXMLFile(proj_file.c_str());

  // Get the full name of the project file
  std::string proj_file_full = itksys::SystemTools::CollapseFullPath(proj_file.c_str());

  // Get the directory in which the project will be saved
  std::string project_dir = itksys::SystemTools::GetParentDirectory(proj_file_full.c_str());

  // Read the location where the file was saved initially
  std::string project_save_dir = preg["SaveLocation"][""];

  // If the locations are different, we will attempt to find relative paths first
  bool moved = (project_save_dir != project_dir);

  // Read all the layers
  std::string key;
  bool main_loaded = false;
  int n_segs_loaded = 0;
  for(int i = 0;
      preg.HasFolder(key = Registry::Key("Layers.Layer[%03d]", i));
      i++)
    {
    // Get the key for the next image
    Registry &folder = preg.Folder(key);

    // Read the role
    LayerRole role = folder["Role"].GetEnum(
          SNAPRegistryIO::GetEnumMapLayerRole(), NO_ROLE);

    // Validate the role
    if(role == MAIN_ROLE && i != 0)
      throw IRISException("Layer %d in a project may not be of type 'Main Image'", i);

    if(role != MAIN_ROLE && i == 0)
      throw IRISException("Layer 0 in a project must be of type 'Main Image'");

    if(role != MAIN_ROLE && role != LABEL_ROLE && role != OVERLAY_ROLE)
      throw IRISException("Layer %d has an unrecognized type", i);

    // Get the filenames for the layer
    std::string layer_file_full = folder["AbsolutePath"][""];

    // If the project has moved, try finding a relative location
    if(moved)
      {
      // Get the relative path of the layer wrt project
      string relative_path;

      // Test the simple thing: is the project location included in the file path
      if(layer_file_full.compare(0, project_save_dir.length(), project_save_dir) == 0)
        {
        // Get the balance of the path
        relative_path = layer_file_full.substr(project_save_dir.length());

        // Strip the leading slashes
        itksys::SystemTools::ConvertToUnixSlashes(relative_path);
        relative_path = relative_path.substr(relative_path.find_first_not_of('/'));
        }
      else
        {
        // Fallback: use relative path mechanism
        relative_path = itksys::SystemTools::RelativePath(
                          project_save_dir.c_str(), layer_file_full.c_str());
        }

      std::string moved_file_full = itksys::SystemTools::CollapseFullPath(
            relative_path.c_str(), project_dir.c_str());

      if(itksys::SystemTools::FileExists(moved_file_full.c_str(), true))
        layer_file_full = moved_file_full;
      }

    // Load the IO hints for the image from the project - but only if this
    // folder is actually present (otherwise some projects from before 2016
    // will not load hints)
    Registry *io_hints = NULL;
    if(folder.HasFolder("IOHints"))
      io_hints = &folder.Folder("IOHints");

    // TODO: this is spaggetti code
    bool load_additive = false;
    if(role == LABEL_ROLE && n_segs_loaded > 0)
      load_additive = true;

    // Load the image and its metadata
    LoadImage(layer_file_full.c_str(), role, warn, &folder, io_hints, load_additive);

    // Check if the main has been loaded
    if(role == MAIN_ROLE)
      {
      main_loaded = true;      
      }
    else if(role == LABEL_ROLE)
      {
      n_segs_loaded++;
      }
    }

  // If main has not been loaded, throw an exception
  if(!main_loaded)
    throw IRISException("Empty or invalid project (main image not found in the project file).");

  // Set the selected segmentation layer to be the first one
  m_GlobalState->SetSelectedSegmentationLayerId(
        m_CurrentImageData->GetFirstSegmentationLayer()->GetUniqueId());

  // Save the project filename
  m_GlobalState->SetProjectFilename(proj_file_full.c_str());

  // Update the history
  m_SystemInterface->GetHistoryManager()->
      UpdateHistory("Project", proj_file_full, false);

  // Load the annotations
  if(preg.HasFolder("Annotations"))
    {
    Registry &ann_folder = preg.Folder("Annotations");
    m_IRISImageData->GetAnnotations()->LoadAnnotations(ann_folder);
    }

  // Simulate saving the project into a registy that will be cached. This
  // allows us to check later whether the project state has changed.
  SaveProjectToRegistry(m_LastSavedProjectState, proj_file_full);
}

bool IRISApplication::IsProjectUnsaved()
{
  // Place the current state of the project into the registry
  Registry reg_current;
  SaveProjectToRegistry(reg_current, m_GlobalState->GetProjectFilename());

  // Compare with the last saved state
  return (reg_current != m_LastSavedProjectState);
}

bool IRISApplication::IsProjectFile(const char *filename)
{
  // This is pretty weak. What we really need is XML validation to check
  // that this is a real registry, and then some minimal check to see that
  // this is a project file
  try
  {
  Registry preg;
  preg.ReadFromXMLFile(filename);
  return (preg.HasEntry("SaveLocation") && preg.HasEntry("Version"));
  }
  catch(...)
  {
  return false;
  }
}


void IRISApplication::SaveAnnotations(const char *filename)
{
  Registry reg;
  m_CurrentImageData->GetAnnotations()->SaveAnnotations(reg);
  reg.WriteToXMLFile(filename);

  m_SystemInterface->GetHistoryManager()->UpdateHistory("Annotations", filename, true);
}

void IRISApplication::LoadAnnotations(const char *filename)
{
  Registry reg;
  reg.ReadFromXMLFile(filename);
  m_CurrentImageData->GetAnnotations()->LoadAnnotations(reg);

  m_SystemInterface->GetHistoryManager()->UpdateHistory("Annotations", filename, true);
}




void IRISApplication::Quit()
{
  if(IsSnakeModeActive())
    {
    // Before quitting the application, we need to exit snake mode
    SetCurrentImageDataToIRIS();
    ReleaseSNAPImageData();
    }

  // Delete all the overlays
  LayerIterator itovl = m_CurrentImageData->GetLayers(OVERLAY_ROLE);
  while(!itovl.IsAtEnd())
    {
    UnloadOverlay(itovl.GetLayer());
    itovl = m_CurrentImageData->GetLayers(OVERLAY_ROLE);
    }

  // Unload the main image
  UnloadMainImage();
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
::ReorientImage(vnl_matrix_fixed<double, 3, 3> inDirection)
{
  // This should only be possible in IRIS mode
  assert(m_CurrentImageData == m_IRISImageData);

  // The main image should be loaded at this point
  assert(m_CurrentImageData->IsMainLoaded());

  // Perform reorientation in the current image data
  m_CurrentImageData->SetDirectionMatrix(inDirection);

  /*
  // Compute a new coordinate transform object
  ImageCoordinateGeometry icg(
    inDirection, m_DisplayToAnatomyRAI,
    m_CurrentImageData->GetMain()->GetSize());


  // Send this coordinate transform to the image data
  m_CurrentImageData->SetImageGeometry(icg);
  */

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

  // We also want to reset the label history at this point, as these are
  // very different labels
  m_LabelUseHistory->Reset();
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

void IRISApplication::LeaveGMMPreprocessingMode()
{
  m_GMMPreviewWrapper->DetachInputsAndOutputs();

  // Before deleting the clustering engine, we store the mixture model
  // The smart pointer mechanism makes sure the mixture model lives on
  // even if the clustering engine is deleted
  m_LastUsedMixtureModel = m_ClusteringEngine->GetMixtureModel();

  // Update the m_time on the mixture model, so in the future we can test
  // if it is current
  m_LastUsedMixtureModel->Modified();

  // Deallocate whatever was created for GMM processing
  m_ClusteringEngine = NULL;
}

void IRISApplication::EnterGMMPreprocessingMode()
{
  // Create a new clustering engine with some samples
  m_ClusteringEngine = UnsupervisedClustering::New();
  m_ClusteringEngine->SetDataSource(m_SNAPImageData);
  m_ClusteringEngine->InitializeClusters();

  // Check if the last used mixture model matches the number of componetns
  bool can_use_saved_mixture =
      (m_LastUsedMixtureModel &&
       m_LastUsedMixtureModel->GetNumberOfComponents() ==
       m_ClusteringEngine->GetMixtureModel()->GetNumberOfComponents());

  if(can_use_saved_mixture)
    {
    // Check if the m-time on any of the images in IRISImageData has been
    // updated, indicating that this is new/different data
    for(LayerIterator lit = m_IRISImageData->GetLayers(MAIN_ROLE | OVERLAY_ROLE);
        !lit.IsAtEnd(); ++lit)
      {
      if(lit.GetLayer()->GetImageBase()->GetMTime() > m_LastUsedMixtureModel->GetMTime())
        {
        can_use_saved_mixture = false;
        break;
        }
      }

    // If the timestamp check has been passed, we can use the original mixture.
    // TODO: clean up this code, currently it does a lot of unnecessary calls to Kmeans++
    if(can_use_saved_mixture)
      {
      m_ClusteringEngine->SetNumberOfClusters(m_LastUsedMixtureModel->GetNumberOfGaussians());
      m_ClusteringEngine->InitializeClusters();
      m_ClusteringEngine->SetMixtureModel(m_LastUsedMixtureModel);
      }
    }

  m_GMMPreviewWrapper->AttachInputs(m_SNAPImageData);
  m_GMMPreviewWrapper->AttachOutputWrapper(m_SNAPImageData->GetSpeed());
  m_GMMPreviewWrapper->SetParameters(m_ClusteringEngine->GetMixtureModel());
}

void IRISApplication::EnterRandomForestPreprocessingMode()
{
  // Create a random forest classification engine
  m_ClassificationEngine = RFEngine::New();
  m_ClassificationEngine->SetDataSource(m_SNAPImageData);

  // Check if we can reuse the classifier from the last run
  bool can_use_saved_classifier =
      (m_LastUsedRFClassifier &&
       m_LastUsedRFClassifierComponents ==
       m_ClassificationEngine->GetNumberOfComponents() &&
       m_LastUsedRFClassifier->IsValidClassifier());

  if(can_use_saved_classifier)
    {
    // Check if the m-time on any of the images in IRISImageData has been
    // updated, indicating that this is new/different data
    for(LayerIterator lit = m_IRISImageData->GetLayers(MAIN_ROLE | OVERLAY_ROLE);
        !lit.IsAtEnd(); ++lit)
      {
      if(lit.GetLayer()->GetImageBase()->GetMTime() > m_LastUsedRFClassifier->GetMTime())
        {
        can_use_saved_classifier = false;
        break;
        }
      }

    if(can_use_saved_classifier)
      {
      m_ClassificationEngine->SetClassifier(m_LastUsedRFClassifier);
      }
    }

  // Connect to the preview wrapper
  m_RandomForestPreviewWrapper->AttachInputs(m_SNAPImageData);
  m_RandomForestPreviewWrapper->AttachOutputWrapper(m_SNAPImageData->GetSpeed());
  m_RandomForestPreviewWrapper->SetParameters(m_ClassificationEngine->GetClassifier());

  // Switch segmentation to examples
  m_SNAPImageData->SwitchLabelImageToExamples();
  InvokeEvent(SegmentationChangeEvent());
}

#include "RandomForestClassifier.h"

void IRISApplication::LeaveRandomForestPreprocessingMode()
{
  m_RandomForestPreviewWrapper->DetachInputsAndOutputs();

  // Before deleting the classification engine, we store the classifier
  // The smart pointer mechanism makes sure the classifier lives on
  // even if the engine is deleted
  m_LastUsedRFClassifier = m_ClassificationEngine->GetClassifier();
  m_LastUsedRFClassifierComponents = m_ClassificationEngine->GetNumberOfComponents();

  // Update the m_time on the classifier, so in the future we can test
  // if it is current
  m_LastUsedRFClassifier->Modified();

  // TODO: delete this code
  // m_RandomForestPreviewWrapper->SetParameters(NULL);

  // Clear the classification engine
  m_ClassificationEngine = NULL;

  // Switch segmentation to examples
  m_SNAPImageData->SwitchLabelImageToMainSegmentation();
  InvokeEvent(SegmentationChangeEvent());
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
      this->LeaveGMMPreprocessingMode();
      break;

    case PREPROCESS_RF:
      this->LeaveRandomForestPreprocessingMode();
      break;

    default:
      break;
    }

  // As we enter the new mode, we also determine what the target snake mode should be
  // (snake mode is either edge == Casseles or input = Zhu+Yuille)
  SnakeType target_snake_type = m_GlobalState->GetSnakeType();

  // Enter the new mode
  switch(mode)
    {
    case PREPROCESS_THRESHOLD:
      m_ThresholdPreviewWrapper->AttachInputs(m_SNAPImageData);
      m_ThresholdPreviewWrapper->AttachOutputWrapper(m_SNAPImageData->GetSpeed());
      target_snake_type = IN_OUT_SNAKE;
      break;

    case PREPROCESS_EDGE:
      m_EdgePreviewWrapper->AttachInputs(m_SNAPImageData);
      m_EdgePreviewWrapper->AttachOutputWrapper(m_SNAPImageData->GetSpeed());
      target_snake_type = EDGE_SNAKE;
      break;

    case PREPROCESS_GMM:
      this->EnterGMMPreprocessingMode();
      target_snake_type = IN_OUT_SNAKE;
      break;

    case PREPROCESS_RF:
      this->EnterRandomForestPreprocessingMode();
      target_snake_type = IN_OUT_SNAKE;
      break;

    default:
      break;
    }

  m_PreprocessingMode = mode;

  // Reset the current snake parameters if necessary
  if(m_GlobalState->GetSnakeType() != target_snake_type)
    {
    m_GlobalState->SetSnakeType(target_snake_type);
    m_GlobalState->SetSnakeParameters(
          target_snake_type == EDGE_SNAKE
          ? SnakeParameters::GetDefaultEdgeParameters()
          : SnakeParameters::GetDefaultInOutParameters());
    }

  // Record the mode if it's not a bogus mode
  if(mode != PREPROCESS_NONE)
    m_GlobalState->SetLastUsedPreprocessingMode(mode);
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
    case PREPROCESS_GMM:
      return m_GMMPreviewWrapper;
    case PREPROCESS_RF:
      return m_RandomForestPreviewWrapper;
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
  return m_SNAPImageData->InitializeSegmentation(
        m_GlobalState->GetSnakeParameters(),
        m_BubbleArray, m_GlobalState->GetDrawingColorLabel());
}





