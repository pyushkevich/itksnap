/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GenericImageData.cxx,v $
  Language:  C++
  Date:      $Date: 2010/06/28 18:45:08 $
  Version:   $Revision: 1.14 $
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
// ITK Includes
#include "itkImage.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include <algorithm>
#include <list>
#include <map>
#include <iostream>
#include "GenericImageData.h"
#include "Rebroadcaster.h"
#include "LayerIterator.h"
#include "GuidedNativeImageIO.h"
#include "ImageAnnotationData.h"
#include "RLERegionOfInterestImageFilter.h"
#include "TimePointProperties.h"
#include "ImageMeshLayers.h"
#include "itkTransform.h"

// System includes
#include <iostream>


GenericImageData
::GenericImageData()
{
  // Make main image wrapper point to grey wrapper initially
  m_MainImageWrapper = nullptr;
  m_ActiveSegmentationWrapper = nullptr;

  // Add to the relevant lists
  m_Wrappers[MAIN_ROLE].push_back(m_MainImageWrapper);

  // Create empty annotations
  m_Annotations = ImageAnnotationData::New();

  // Create the viewport geometries
  for (auto index : DisplaySliceIndices)
    m_DisplayViewportGeometry[index] = ImageBaseType::New();

  // Create TimePointProperties
  m_TimePointProperties = TimePointProperties::New();
  m_TimePointProperties->SetParent(this);
	Rebroadcaster::Rebroadcast(m_TimePointProperties, WrapperGlobalMetadataChangeEvent(),
														 this, WrapperGlobalMetadataChangeEvent());
}

GenericImageData
::~GenericImageData()
{
  // This was causing virtual method called during destruction errors, seems pointless
  // as there are no actual pointers to free
  // UnloadMainImage();
}

Vector3d 
GenericImageData
::GetReferenceSpaceSpacing()
{
  return this->GetReferenceSpace()->GetSpacing().GetVnlVector();
}

Vector3d 
GenericImageData
::GetReferenceSpaceOrigin()
{
  return this->GetReferenceSpace()->GetSpacing().GetVnlVector();
}

template<unsigned int VDim>
itk::Index<VDim> elementwise_min(const itk::Index<VDim> &a, const itk::Index<VDim> &b)
{
  itk::Index<VDim> result;
  for(unsigned int i = 0; i < VDim; i++)
    result[i] = std::min(a[i], b[i]);
  return result;
}

template<unsigned int VDim>
itk::Index<VDim> elementwise_max(const itk::Index<VDim> &a, const itk::Index<VDim> &b)
{
  itk::Index<VDim> result;
  for(unsigned int i = 0; i < VDim; i++)
    result[i] = std::max(a[i], b[i]);
  return result;
}

template<unsigned int VDim>
void expand_region(itk::ImageRegion<VDim> &target, const itk::ImageRegion<VDim> &source)
{
  auto upper = target.GetUpperIndex();
  target.SetIndex(elementwise_min(target.GetIndex(), source.GetIndex()));
  target.SetUpperIndex(elementwise_max(upper, source.GetUpperIndex()));
}

GenericImageData::RegionType
GenericImageData::GetFullExtentImageRegion()
{
  // Start with the reference image itself
  auto *ref = this->GetReferenceSpaceWrapper();
  auto tran_ref = ref->GetITKTransform();
  auto  region = ref->GetBufferedRegion();

  // Iterate over layers
  for (LayerIterator it = this->GetLayers(ALL_ROLES); !it.IsAtEnd(); ++it)
  {
    auto *layer = it.GetLayer();
    auto *tran_layer = layer->GetITKTransform();

    // Extents of the region box
    Vector3d ext_layer[] = {
      to_double(layer->GetBufferedRegion().GetIndex()) - 0.5,
      to_double(layer->GetBufferedRegion().GetUpperIndex()) + 0.5
    };

    // Map the eight corners of the region box into reference space
    itk::ImageRegion<3> rgn_layer_ref_space;
    for(unsigned int corner = 0; corner < 8; ++corner)
    {
      Vector3d corner_point = { ext_layer[(corner & 1) ? 1 : 0][0],
                                ext_layer[(corner & 2) ? 1 : 0][1],
                                ext_layer[(corner & 4) ? 1 : 0][2] };

      // Transform the corner point to reference space
      auto idx_layer = to_itkContinuousIndex(corner_point);
      auto pt_lps = layer->GetImageBase()->TransformContinuousIndexToPhysicalPoint<double>(idx_layer);
      auto pt_lps_tt = tran_ref->TransformPoint( tran_layer->GetInverseTransform()->TransformPoint(pt_lps) );
      auto ci_ref = ref->GetImageBase()->TransformPhysicalPointToContinuousIndex<double>(pt_lps_tt);

      itk::ImageRegion<3> corner_region;
      for(unsigned int i = 0; i < 3; i++)
      {
        corner_region.SetIndex(i, (long) std::floor(ci_ref[i] - 0.5));
        corner_region.SetSize(i, 1);
      }

      // Update the extents of the layer in reference space
      // auto corner_idx = to_itkIndex(corner_ref - 0.5);
      if(corner == 0)
        rgn_layer_ref_space = corner_region;
      else
        expand_region(rgn_layer_ref_space, corner_region);
    }

    // Check if the extents overlap
    bool overlap = true;
    for (unsigned int i = 0; i < 3; i++)
    {
      if (rgn_layer_ref_space.GetUpperIndex()[i] < region.GetIndex()[i] ||
          rgn_layer_ref_space.GetIndex()[i] > region.GetUpperIndex()[i])
      {
        overlap = false;
        break;
      }
    }

    // If overlapping, extend the region by the layer's extents
    if(overlap)
    {
      expand_region(region, rgn_layer_ref_space);
    }
  }
  return region;
}


void
GenericImageData
::SetMainImageInternal(ImageWrapperBase *wrapper)
{
  // Clear the nickname counters and assign a the default nickname
  m_NicknameCounter.clear();
  wrapper->SetDefaultNickname(this->GenerateNickname(MAIN_ROLE));

  // Make the wrapper the main image
  SetSingleImageWrapper(MAIN_ROLE, wrapper);
  m_MainImageWrapper = wrapper;

  // Reset the segmentation state to a single empty image
  this->UnloadAllSegmentations();

  // Set opaque
  m_MainImageWrapper->SetAlpha(255);
}


class GenericImageDataWrapperCreator
{
public:
  typedef GenericImageData::ImageBaseType ImageBaseType;
  typedef GenericImageData::ITKTransformType ITKTransformType;
  typedef SmartPtr<ImageBaseType> ImageBasePointer;
  typedef SmartPtr<ImageWrapperBase> ImageWrapperBasePointer;

  /** Non-templated method that takes parameters */
  GenericImageDataWrapperCreator(GuidedNativeImageIO *io,
                                 ImageBaseType *ref_space,
                                 ITKTransformType *transform,
                                 IRISDisplayGeometry *display_geometry)
    : io(io), ref_space(ref_space), transform(transform), display_geometry(display_geometry) {}

  /** Function that actually creates the wrapper */
  template<typename TWrapperTraits, bool TLinearMapping> ImageWrapperBasePointer CreateFromTraits();
  template<typename TComponent, bool TLinearMapping> ImageWrapperBasePointer Create();

private:
  GuidedNativeImageIO *io;
  ImageBaseType *ref_space;
  ITKTransformType *transform;
  IRISDisplayGeometry *display_geometry;
};

template<typename TWrapperTraits, bool TLinearMapping>
typename GenericImageDataWrapperCreator::ImageWrapperBasePointer
GenericImageDataWrapperCreator::CreateFromTraits()
{
  // The output wrapper
  SmartPtr<ImageWrapperBase> out_wrapper;

  // The image will be cast to a vector anatomic image
  typedef typename TWrapperTraits::WrapperType WrapperType;
  typedef typename WrapperType::Image4DType Image4DType;

  // Optionally rescale the image and cast to TComponent
  RescaleNativeImageToIntegralType<Image4DType> rescaler;
  typename Image4DType::Pointer image = rescaler(io);

  // Create a main wrapper of fixed type.
  SmartPtr<WrapperType> wrapper = WrapperType::New();

  // Assign the image to the wrapper
  wrapper->SetDisplayGeometry(*display_geometry);
  wrapper->SetImage4D(image, ref_space, transform);

  // Create a mapper to native intensity
  if(TLinearMapping)
    {
    LinearInternalToNativeIntensityMapping mapper(
          rescaler.GetNativeScale(), rescaler.GetNativeShift());
    }

  // Return the wrapper
  return wrapper.GetPointer();
}

template<typename TComponent, bool TLinearMapping>
typename GenericImageDataWrapperCreator::ImageWrapperBasePointer
GenericImageDataWrapperCreator::Create()
{
  // Split depending on whether the image is scalar or vector
  if(io->GetNumberOfComponentsInNativeImage() > 1)
    {
    typedef AnatomicImageWrapperTraits<TComponent, TLinearMapping> TraitsType;
    return CreateFromTraits<TraitsType, TLinearMapping>();
    }
  else
    {
    typedef AnatomicScalarImageWrapperTraits<TComponent, TLinearMapping> TraitsType;
    return CreateFromTraits<TraitsType, TLinearMapping>();
    }
}

SmartPtr<ImageWrapperBase>
GenericImageData::CreateAnatomicWrapper(GuidedNativeImageIO *io, ITKTransformType *transform)
{
  // The output wrapper
  SmartPtr<ImageWrapperBase> out_wrapper;

  // If the transform is not NULL, the reference space must be specified
  ImageBaseType *refSpace = (transform) ? this->GetReferenceSpace() : NULL;

  // The object used to create the wrapper of correct type
  GenericImageDataWrapperCreator c(io, refSpace, transform, &m_DisplayGeometry);

  // Create the wrapper to match native type
  switch (io->GetComponentTypeInNativeImage())
  {
    case itk::IOComponentEnum::UCHAR:
      out_wrapper = c.Create<unsigned char, false>();
      break;
    case itk::IOComponentEnum::CHAR:
      out_wrapper = c.Create<char, false>();
      break;
    case itk::IOComponentEnum::USHORT:
      out_wrapper = c.Create<unsigned short, false>();
      break;
    case itk::IOComponentEnum::SHORT:
      out_wrapper = c.Create<short, false>();
      break;
    case itk::IOComponentEnum::DOUBLE:
      out_wrapper = c.Create<double, false>();
      break;
    default:
      out_wrapper = c.Create<float, false>();
      break;
  }

  // Additional configuration for the wrapper
  this->AssignDisplayViewportGeometriesToLayer(out_wrapper);

  // Set the interpolation mode
  out_wrapper->SetSlicingInterpolationMode(m_InterpolationMode);

  // Create an image coordinate geometry object
  return out_wrapper;
}

void GenericImageData::SetMainImage(GuidedNativeImageIO *io)
{
  // Create the wrapper from the Native IO (the wrapper will either be a scalar
  // or a vector-valued image, depending on the number of components)
  SmartPtr<ImageWrapperBase> wrapper = this->CreateAnatomicWrapper(io, nullptr);

  // Assign this wrapper to the main image
  this->SetMainImageInternal(wrapper);
}

void
GenericImageData
::UnloadMainImage()
{
  // First unload the overlays if exist
  UnloadOverlays();

  // Clear the main image wrappers
  RemoveSingleImageWrapper(MAIN_ROLE);
  m_MainImageWrapper = nullptr;

  // Unload all the segmentations
  this->RemoveAllWrappers(LABEL_ROLE);
  m_ActiveSegmentationWrapper = nullptr;

  // Unload all the meshes
  m_MeshLayers->Unload();

  // Clear the annotations
  m_Annotations->Reset();

  // Clear timepoint properties
  m_TimePointProperties->Reset();
}


void GenericImageData::UpdateReferenceImageInAllLayers()
{
  auto *ref_space = this->GetReferenceSpace();
  for (LayerIterator lit(this); !lit.IsAtEnd(); ++lit)
  {
    auto *wrapper = lit.GetLayer();
    if (wrapper && wrapper->IsInitialized() && wrapper->GetReferenceSpace() != ref_space)
      wrapper->SetReferenceSpace(ref_space);
  }
}

bool GenericImageData::IsSameGeometry(ImageWrapperBase *wrapper1, ImageWrapperBase *wrapper2)
{
  if (!wrapper1 || !wrapper2)
    return false;

  return ImageWrapperBase::IsSameGeometry(wrapper1->GetImageBase(), wrapper2->GetImageBase());
}

void
GenericImageData::UpdateActiveSegmentation(LabelImageWrapper *wrapper)
{
  // If both null or the same, nothing to do here
  if(wrapper == m_ActiveSegmentationWrapper)
    return;

  // Get the current cursor position in RAS space
  bool have_cursor = false;
  Vector3d cusror_ras;
  if(m_ActiveSegmentationWrapper && m_ActiveSegmentationWrapper->IsInitialized())
  {
    // Store the previous cursor position so we can transfer it to the new segmentation
    cusror_ras = this->GetCursorPositionRAS();
    have_cursor = true;
  }

  // Check if geometry is different
  bool geom_change = !GenericImageData::IsSameGeometry(wrapper, m_ActiveSegmentationWrapper);

  // Set the active segmentation wrapper
  m_ActiveSegmentationWrapper = wrapper;

  // Update the reference space in all layers
  this->UpdateReferenceImageInAllLayers();

  // Set the cursor position in all layers
  if(wrapper && have_cursor)
    this->SetCursorPositionRAS(cusror_ras);

  // Fire update event
  if(geom_change)
    InvokeEvent(ReferenceSpaceGeometryChangeEvent());
}

GenericImageData::ImageBaseType *
GenericImageData::GetReferenceSpace() const
{
  // Get the current segmentation
  if(m_ActiveSegmentationWrapper && m_ActiveSegmentationWrapper->IsInitialized())
    return m_ActiveSegmentationWrapper->GetImageBase();
  else
    return nullptr;
}


ImageWrapperBase *
GenericImageData::GetReferenceSpaceWrapper() const
{
  return m_ActiveSegmentationWrapper;
}

bool
GenericImageData::IsFreeRotation() const
{
  // TODO: I am not sure this is right...
  return !GetReferenceSpaceWrapper()->ImageSpaceMatchesReferenceSpace();
}

void GenericImageData::AddOverlay(ImageWrapperBase *new_layer)
{
  // Additional configuration for the wrapper (normally called in CreateAnatomicImageWrapper)
  this->AssignDisplayViewportGeometriesToLayer(new_layer);
  this->AddOverlayInternal(new_layer, true);
}

void
GenericImageData
::AddCoregOverlay(GuidedNativeImageIO *io, ITKTransformType *transform)
{
  // Create the wrapper from the Native IO (the wrapper will either be a scalar
  // or a vector-valued image, depending on the number of components)
  SmartPtr<ImageWrapperBase> wrapper = this->CreateAnatomicWrapper(io, transform);

  // Assign this wrapper to the main image
  this->AddOverlayInternal(wrapper, false);
}

void
GenericImageData
::AddOverlayInternal(ImageWrapperBase *overlay, bool checkSpace)
{
  // Check that the image matches the size of the main image
  if(checkSpace && m_MainImageWrapper->GetBufferedRegion() != overlay->GetBufferedRegion())
    {
    throw IRISException("Main and overlay data sizes are different");
    }

  // Pass the image to a Grey image wrapper
  overlay->SetAlpha(0.5);
  overlay->SetDefaultNickname(this->GenerateNickname(OVERLAY_ROLE));

  // Sync up spacing between the main and overlay image
  if(checkSpace)
    overlay->CopyImageCoordinateTransform(m_MainImageWrapper);

  // Add to the overlay wrapper list
  PushBackImageWrapper(OVERLAY_ROLE, overlay);
}

void
GenericImageData
::UnloadOverlays()
{
  while (m_Wrappers[OVERLAY_ROLE].size() > 0)
    UnloadOverlayLast();
}

void
GenericImageData
::UnloadOverlayLast()
{
  // Make sure at least one grey overlay is loaded
  if (!AreOverlaysLoaded())
    return;

  // Release the data associated with the last overlay
  PopBackImageWrapper(OVERLAY_ROLE);
}

void GenericImageData
::UnloadOverlay(ImageWrapperBase *overlay)
{
  this->RemoveImageWrapper(OVERLAY_ROLE, overlay);
}

void
GenericImageData::OnActiveSegmentationReload()
{
  this->UpdateReferenceImageInAllLayers();
}

void
GenericImageData
::UnloadMeshLayer(unsigned long id)
{
  m_MeshLayers->RemoveLayer(id);
}

SmartPtr<GenericImageData::LabelImage4DType>
GenericImageData
::CompressSegmentation(GuidedNativeImageIO *io)
{
  // Check that the image matches the size of the grey image
  itkAssertOrThrowMacro(
        m_MainImageWrapper->IsInitialized(),
        "Main image not initialized in GenericImageData::CompressSegmentation");

  // This is the uncompressed representation of the segmentation
  typedef itk::Image<LabelType, 4> UncompressedImage4DType;

  // Cast the native to label type
  CastNativeImage<UncompressedImage4DType> caster;
  UncompressedImage4DType::Pointer imgUncompressed = caster(io);

  //use specialized RoI filter to convert to RLEImage
  typedef LabelImageWrapper::Image4DType LabelImage4DType;
  typedef itk::RegionOfInterestImageFilter<UncompressedImage4DType, LabelImage4DType> inConverterType;
  inConverterType::Pointer inConv = inConverterType::New();
  inConv->SetInput(imgUncompressed);
  inConv->SetRegionOfInterest(imgUncompressed->GetLargestPossibleRegion());
  inConv->Update();
  LabelImage4DType::Pointer imgLabel = inConv->GetOutput();
  imgUncompressed = NULL; //deallocate intermediate image to save memory

  // Disconnect from the pipeline right away
  imgLabel->DisconnectPipeline();

  // The header of the label image is made to match that of the grey image
  // TODO: this old behavior should now be obsolete, but we might want to warn user
  // imgLabel->SetOrigin(this->GetMain()->GetImage4DBase()->GetOrigin());
  // imgLabel->SetSpacing(this->GetMain()->GetImage4DBase()->GetSpacing());
  // imgLabel->SetDirection(this->GetMain()->GetImage4DBase()->GetDirection());

  // Return the image
  return imgLabel;
}

void
GenericImageData::AssignDisplayViewportGeometriesToLayer(ImageWrapperBase *layer)
{
  for (auto index : DisplaySliceIndices)
    layer->SetDisplayViewportGeometry(index, GetDisplayViewportGeometry(index));
}

void
GenericImageData::ConfigureSegmentationInternal(LabelImageWrapper *wrapper)
{
  // Main Image should exist
  assert(m_MainImageWrapper->IsInitialized());

  // The slice index in the segmentation should be set based on current reference
  // image (TODO)

  // Set default nickname
  wrapper->SetDefaultNickname(this->GenerateNickname(LABEL_ROLE));

  // Send the color table to the new wrapper
  wrapper->GetDisplayMapping()->SetLabelColorTable(m_Parent->GetColorLabelTable());

  // Additional configuration for the wrapper
  this->AssignDisplayViewportGeometriesToLayer(wrapper);
}

LabelImageWrapper *
GenericImageData
::SetSegmentationImage(GuidedNativeImageIO *io, bool add_to_existing, bool make_active)
{
  // Check that the image matches the size of the grey image
  itkAssertOrThrowMacro(
        m_MainImageWrapper->IsInitialized(),
        "Main image not initialized in GenericImageData::AddSegmentationImage");

  // Create a compressed image from the IO
  SmartPtr<LabelImage4DType> imgLabel = CompressSegmentation(io);

  // Create a new wrapper of label type
  SmartPtr<LabelImageWrapper> seg_wrapper = LabelImageWrapper::New();
  seg_wrapper->InitializeToWrapper(m_MainImageWrapper, (LabelType) 0);
  seg_wrapper->SetImage4D(imgLabel);

  // Configure the new wrapper
  this->ConfigureSegmentationInternal(seg_wrapper);

  // Update filenames
  seg_wrapper->SetFileName(io->GetFileNameOfNativeImage());

  // Add the segmentation image to the list of segmentation wrappers
  PushBackImageWrapper(LABEL_ROLE, seg_wrapper);

  // Set the added image as the active segmentation / reference
  if(make_active)
    UpdateActiveSegmentation(seg_wrapper);

  // Remove loaded segmentation unless adding to existing
  if(!add_to_existing)
    this->RemoveAllWrappers(LABEL_ROLE, seg_wrapper);

  // Intensity changes in the image wrapper are broadcast as segmentation events
  Rebroadcaster::Rebroadcast(seg_wrapper, WrapperImageChangeEvent(),
                             this, SegmentationChangeEvent());

  // Return the newly added wrapper
  return seg_wrapper;
}

void GenericImageData
::UpdateSegmentationTimePoint(LabelImageWrapper *wrapper, GuidedNativeImageIO *io)
{
  // Check that the image matches the size of the grey image
  itkAssertOrThrowMacro(
        m_MainImageWrapper->IsInitialized(),
        "Main image not initialized in GenericImageData::UpdateSegmentationTimePoint");

  // Create a compressed image from the IO
  SmartPtr<LabelImage4DType> imgLabel = CompressSegmentation(io);

  // Drop the dimension of the 4D image to 3D. We can reuse existing code in ImageWrapper
  // for doing this
  SmartPtr<LabelImageWrapper> temp_wrapper = LabelImageWrapper::New();
  temp_wrapper->SetImage4D(imgLabel);

  // Update the target wrapper
  // TODO: make this operation undo-able!
  wrapper->UpdateTimePoint(temp_wrapper->GetModifiableImage());
}

LabelImageWrapper *GenericImageData::AddBlankSegmentation(bool make_active)
{
  assert(m_MainImageWrapper->IsInitialized());

  // Initialize the segmentation data to zeros
  LabelImageWrapper::Pointer seg_wrapper = LabelImageWrapper::New();
  seg_wrapper->InitializeToWrapper(m_MainImageWrapper, (LabelType) 0);

  // Configure the new wrapper
  this->ConfigureSegmentationInternal(seg_wrapper);
  this->PushBackImageWrapper(LABEL_ROLE, seg_wrapper.GetPointer());

  // Set the added image as the active segmentation / reference
  if(make_active)
    UpdateActiveSegmentation(seg_wrapper);

  // Update the reference space in all images
  this->UpdateReferenceImageInAllLayers();

  // Intensity changes in the image wrapper are broadcast as segmentation events
  Rebroadcaster::Rebroadcast(seg_wrapper, WrapperImageChangeEvent(),
                             this, SegmentationChangeEvent());

  // Return the added wrapper
  return seg_wrapper;
}

void GenericImageData
::UnloadSegmentation(ImageWrapperBase *seg)
{
  // Erase the segmentation image
  this->RemoveImageWrapper(LABEL_ROLE, seg);

  // Clear the active segmentation wrapper
  if(m_ActiveSegmentationWrapper == seg)
    UpdateActiveSegmentation(nullptr);

  // If we have a main image, we must have at least one segmentation and
  // at least one active segmentation.
  if(this->IsMainLoaded())
  {
    if(this->GetNumberOfLayers(LABEL_ROLE) == 0)
      this->AddBlankSegmentation(true);
    else if(m_ActiveSegmentationWrapper == nullptr)
      SetActiveSegmentationLayerInternal(this->GetFirstSegmentationLayer());
  }
}

void
GenericImageData::UnloadAllSegmentations()
{
  // The main image must be loaded
  assert(this->IsMainLoaded());

  // Unload all segmentations
  SetActiveSegmentationLayerInternal(nullptr);
  this->RemoveAllWrappers(LABEL_ROLE);

  // Add a new blank segmentation
  this->AddBlankSegmentation(true);
}

bool
GenericImageData
::AreOverlaysLoaded()
{
  return (m_Wrappers[OVERLAY_ROLE].size() > 0);
}

Vector3ui
GenericImageData::GetCursorPosition() const
{
  // Check if the crosshairs are already set
  itkAssertOrThrowMacro(m_ActiveSegmentationWrapper && m_ActiveSegmentationWrapper->IsInitialized(),
                        "Active segmentation not initialized in GenericImageData::GetCrosshairs");

  // Read crosshairs from the active segmentation wrapper
  return Vector3ui(m_ActiveSegmentationWrapper->GetSliceIndex());
}

Vector3d
GenericImageData::GetCursorPositionRAS() const
{
  auto pos_vox = to_double(this->GetCursorPosition());
  auto pos_ras = m_ActiveSegmentationWrapper->TransformVoxelCIndexToNIFTICoordinates(pos_vox);
  return pos_ras;
}

void
GenericImageData::SetCursorPositionRAS(const Vector3d &crosshairs_ras)
{
  // Check if the crosshairs are already set
  itkAssertOrThrowMacro(m_ActiveSegmentationWrapper && m_ActiveSegmentationWrapper->IsInitialized(),
                        "Active segmentation not initialized in GenericImageData::GetCrosshairs");

  // Get the coordinates in continious units
  auto pos_vox = m_ActiveSegmentationWrapper->TransformNIFTICoordinatesToVoxelCIndex(crosshairs_ras);

  // Round to the nearest integer
  auto idx = to_itkIndex(Vector3ui(std::round(pos_vox[0]), std::round(pos_vox[1]), std::round(pos_vox[2])));

  // If out of bounds, set to the center of the image
  auto crosshairs = m_ActiveSegmentationWrapper->GetBufferedRegion().IsInside(idx)
                      ? Vector3ui(idx)
                      : m_ActiveSegmentationWrapper->GetSize() / 2u;

  // Set the position
  this->SetCursorPosition(crosshairs);
}


void
GenericImageData
::SetCursorPosition(const Vector3ui &crosshairs)
{
  auto idx_ch = to_itkIndex(crosshairs);

  // Check if the crosshairs are already set
  itkAssertOrThrowMacro(m_ActiveSegmentationWrapper && m_ActiveSegmentationWrapper->IsInitialized(),
                        "Active segmentation not initialized in GenericImageData::SetCrosshairs");

  if(m_ActiveSegmentationWrapper->GetSliceIndex() == idx_ch)
    return;

  // The crosshairs have to be in the space of the active layer
  itkAssertOrThrowMacro(
    m_ActiveSegmentationWrapper->GetBufferedRegion().IsInside(idx_ch),
    "Crosshairs outside of active segmentation in GenericImageData::SetCrosshairs");

  // Set crosshairs in all wrappers
  for(LayerIterator lit(this); !lit.IsAtEnd(); ++lit)
    if(lit.GetLayer() && lit.GetLayer()->IsInitialized())
      lit.GetLayer()->SetSliceIndex(idx_ch);

  // Fire a cursor update event
  InvokeEvent(CursorUpdateEvent());
}

unsigned int
GenericImageData::GetCursorTimePoint() const
{
  // TODO: The timepoint is still anchored to the main image. For now I guess this is ok,
  // but have to really play around with different scenarios to see if this is the right
  // approach or it should be anchored to the active segmentation
  if(!this->IsMainLoaded())
    return 0u;

  return m_MainImageWrapper->GetTimePointIndex();
}

void
GenericImageData ::SetCursorTimePoint(unsigned int time_point)
{
  // Check if we need to change anything
  if (this->GetCursorTimePoint() == time_point)
    return;

  // Check if anything changes in the course of doing this
  bool modified = false;

  // Set the time point in all wrappers
  for (LayerIterator lit(this); !lit.IsAtEnd(); ++lit)
  {
    if (lit.GetLayer() && lit.GetLayer()->IsInitialized())
    {
      // Time point is handled a little differently from the cursor. The
      // main image and overlays may have different number of timepoints.
      // The idea is only to allow N-N, N-1 or 1-N combinations, i.e., the
      // main image has N time points, and then overlays have either 1 or N.
      // We can simply use modulo to make this work
      unsigned int tp = time_point % lit.GetLayer()->GetNumberOfTimePoints();
      if (tp != lit.GetLayer()->GetTimePointIndex())
      {
        modified = true;
        lit.GetLayer()->SetTimePointIndex(tp);
      }
    }
  }

  // Fire events
  if (modified)
  {
    InvokeEvent(CursorTimePointUpdateEvent());

    // TODO: why are these necessary?
    InvokeEvent(CursorUpdateEvent());
    InvokeEvent(SegmentationChangeEvent());
  }
}

void GenericImageData::SetDisplayGeometry(const IRISDisplayGeometry &dispGeom)
{
  m_DisplayGeometry = dispGeom;
  for(LayerIterator lit(this); !lit.IsAtEnd(); ++lit)
    if(lit.GetLayer())
      {
      // Set the direction matrix in the image
      lit.GetLayer()->SetDisplayGeometry(m_DisplayGeometry);
      }
}

void
GenericImageData::SetInterpolationMode(ImageWrapperBase::InterpolationMode interpMode)
{
  m_InterpolationMode = interpMode;
  for (LayerIterator lit(this); !lit.IsAtEnd(); ++lit)
    if (lit.GetLayer() && lit.GetRole() != LABEL_ROLE)
    {
      // Set the interpolation mode in the image
      lit.GetLayer()->SetSlicingInterpolationMode(interpMode);
    }
}

GenericImageData::ImageBaseType *
GenericImageData::GetDisplayViewportGeometry(DisplaySliceIndex index)
{
  return m_DisplayViewportGeometry[index];
}

void GenericImageData::SetDirectionMatrix(const vnl_matrix<double> &direction)
{
  // TODO: this is horrendous, why are we reorienting the overlays too? The logic should
  // be by image layer, and segmentation should be reoriented if it matches the main image.
  for(LayerIterator lit(this); !lit.IsAtEnd(); ++lit)
    if(lit.GetLayer())
      {
      // Set the direction matrix in the image
      lit.GetLayer()->SetDirectionMatrix(direction);
      }

  InvokeEvent(ReferenceSpaceGeometryChangeEvent());
}

const ImageCoordinateGeometry *GenericImageData::GetImageGeometry() const
{
  // Return the geometry from the reference wrapper
  return this->GetReferenceSpaceWrapper()->GetImageGeometry();
}

void GenericImageData::ClearUndoPoints()
{
  for(LayerIterator lit(this, LABEL_ROLE); !lit.IsAtEnd(); ++lit)
    {
    LabelImageWrapper *liw = dynamic_cast<LabelImageWrapper *>(lit.GetLayer());
    if(liw)
      liw->ClearUndoPointsForAllTimePoints();
    }
}

GenericImageData::RegionType
GenericImageData
::GetReferenceSpaceImageRegion() const
{
  // Return the geometry from the reference wrapper
  return this->GetReferenceSpaceWrapper()->GetBufferedRegion();
}


unsigned int GenericImageData::GetNumberOfLayers(int role_filter)
{
  unsigned int n = 0;

  LayerIterator it = this->GetLayers(role_filter);
  while(!it.IsAtEnd())
    {
    n++; ++it;
    }

  return n;
}

ImageWrapperBase *
GenericImageData
::FindLayer(unsigned long unique_id, bool search_derived, int role_filter)
{
  for(LayerIterator it = this->GetLayers(role_filter); !it.IsAtEnd(); ++it)
    {
    if(it.GetLayer()->GetUniqueId() == unique_id)
      {
      return it.GetLayer();
      }
    else if(search_derived)
      {
      VectorImageWrapperBase *vec = it.GetLayerAsVector();
      if(vec)
        {
        for(int j = SCALAR_REP_COMPONENT; j < NUMBER_OF_SCALAR_REPS; j++)
          {
          int n = (j == SCALAR_REP_COMPONENT) ? vec->GetNumberOfComponents() : 1;
          for(int k = 0; k < n; k++)
            {
            ImageWrapperBase *w = vec->GetScalarRepresentation((ScalarRepresentation) j, k);
            if(w && w->GetUniqueId() == unique_id)
              return w;
            }
          }
        }
      }
    }

  return NULL;
}

std::list<ImageWrapperBase *>
GenericImageData
::FindLayersByTag(std::string tag, int role_filter)
{
  std::list<ImageWrapperBase *> retval;
  for(LayerIterator it = this->GetLayers(role_filter); !it.IsAtEnd(); ++it)
    {
    if(it.GetLayer()->GetTags().Contains(tag))
      {
      retval.push_back(it.GetLayer());
      }
    }
  return retval;
}

std::list<ImageWrapperBase *> GenericImageData::FindLayersByRole(int role_filter)
{
  std::list<ImageWrapperBase *> retval;
  for(LayerIterator it = this->GetLayers(role_filter); !it.IsAtEnd(); ++it)
    retval.push_back(it.GetLayer());
  return retval;
}


int GenericImageData::GetNumberOfOverlays()
{
  return m_Wrappers[OVERLAY_ROLE].size();
}

unsigned int
GenericImageData
::GetNumberOfTimePoints() const
{
  if (!this->IsMainLoaded())
    return 0u;
  else
    return m_MainImageWrapper->GetNumberOfTimePoints();
}

double
GenericImageData::GetTimeSpacing() const
{
  if (!this->IsMainLoaded())
    return 0.;
  else
    return m_MainImageWrapper->GetImage4DBase()->GetSpacing()[3];
}

ImageWrapperBase *GenericImageData::GetLastOverlay()
{
  return m_Wrappers[OVERLAY_ROLE].back();
}

Vector3ui
GenericImageData::GetReferenceSpaceSize() const
{
  return this->GetReferenceSpaceWrapper()->GetSize();
}

LabelImageWrapper *GenericImageData::GetFirstSegmentationLayer()
{
  assert(m_Wrappers[LABEL_ROLE].size() > 0);
  return dynamic_cast<LabelImageWrapper *>(m_Wrappers[LABEL_ROLE].front().GetPointer());
}

LabelImageWrapper *
GenericImageData::GetActiveSegmentationLayer()
{
  return m_ActiveSegmentationWrapper;
}

void
GenericImageData::SetActiveSegmentationLayer(unsigned int unique_id)
{
  for(auto &wrapper : m_Wrappers[LABEL_ROLE])
  {
    if(wrapper->GetUniqueId() == unique_id)
    {
      this->SetActiveSegmentationLayerInternal(dynamic_cast<LabelImageWrapper *>(wrapper.GetPointer()));
      return;
    }
  }

  this->SetActiveSegmentationLayerInternal(nullptr);
}

void GenericImageData::SetActiveSegmentationLayerInternal(LabelImageWrapper *layer)
{
  this->UpdateActiveSegmentation(layer);
}

void GenericImageData::PushBackImageWrapper(LayerRole role,
                                            ImageWrapperBase *wrapper)
{
  // TODO: this is being called in too many places, but for now this is a band-aid bug
  // fix to avoid crash when the viewport geometry is not set in a wrapper
  this->AssignDisplayViewportGeometriesToLayer(wrapper);

  // Append the wrapper
  m_Wrappers[role].push_back(wrapper);

  // Rebroadcast the wrapper-related events as our own events
  Rebroadcaster::RebroadcastAsSourceEvent(wrapper, WrapperChangeEvent(), this);
  
  // Fire the layer change event
  this->InvokeEvent(LayerChangeEvent());
}

void GenericImageData::PopBackImageWrapper(LayerRole role)
{
  assert(m_Wrappers[role].size() > 0);
  if(m_Wrappers[role].back())
    this->BeforeRemoveImageWrapper(m_Wrappers[role].back());
  m_Wrappers[role].pop_back();

  // Fire the layer change event
  this->InvokeEvent(LayerChangeEvent());
}

void GenericImageData::MoveLayer(ImageWrapperBase *layer, int direction)
{
  // Find the layer
  LayerIterator it(this);
  it.Find(layer);
  if(!it.IsAtEnd())
    {
    WrapperList &wl = m_Wrappers[it.GetRole()];
    int k = it.GetPositionInRole();

    // Make sure the operation is legal!
    assert(k + direction >= 0 && k + direction < wl.size());

    // Do the swap
    std::swap(wl[k], wl[k+direction]);
    }

  // Fire the layer change event
  this->InvokeEvent(LayerChangeEvent());
}

void GenericImageData::RemoveImageWrapper(LayerRole role,
                                          ImageWrapperBase *wrapper)
{
  // Erase the segmentation image
  WrapperList &wrappers = m_Wrappers[role];
  WrapperIterator it =
      std::find(wrappers.begin(), wrappers.end(), wrapper);
  if(it != wrappers.end())
    {
    BeforeRemoveImageWrapper(it->GetPointer());
    wrappers.erase(it);
    }

  // Fire the layer change event
  this->InvokeEvent(LayerChangeEvent());
}

void GenericImageData::SetSingleImageWrapper(LayerRole role,
                                             ImageWrapperBase *wrapper)
{
  assert(m_Wrappers[role].size() == 1);

  // TODO: this is being called in too many places, but for now this is a band-aid bug
  // fix to avoid crash when the viewport geometry is not set in a wrapper
  this->AssignDisplayViewportGeometriesToLayer(wrapper);

  // Assign the first wrapper in role
  m_Wrappers[role].front() = wrapper;

  // Rebroadcast the wrapper-related events as our own events
  Rebroadcaster::RebroadcastAsSourceEvent(wrapper, WrapperChangeEvent(), this);

  // Fire the layer change event
  this->InvokeEvent(LayerChangeEvent());
}

void GenericImageData::RemoveSingleImageWrapper(LayerRole role)
{
  assert(m_Wrappers[role].size() == 1);
  auto *current = m_Wrappers[role].front().GetPointer();
  if(current)
    BeforeRemoveImageWrapper(current);
  m_Wrappers[role].front() = nullptr;

  // Fire the layer change event
  this->InvokeEvent(LayerChangeEvent());
}

void
GenericImageData::RemoveAllWrappers(LayerRole role, ImageWrapperBase *except)
{
  typename ImageWrapperBase::Pointer ptr_except = except;
  while(m_Wrappers[role].size() > 0)
    this->PopBackImageWrapper(role);
  if(except)
    this->PushBackImageWrapper(role, except);
}

void
GenericImageData::BeforeRemoveImageWrapper(ImageWrapperBase *wrapper)
{
  // If the wrapper has a volume, remove it
  auto *volume = wrapper->GetUserData("volume");
  if (volume)
    wrapper->RemoveUserData("volume");

  // If wrapper is linked to a mesh layer, remove it too
  if (m_MeshLayers->HasMeshForImage(wrapper->GetUniqueId()))
    m_MeshLayers->RemoveLayer(m_MeshLayers->GetMeshForImage(wrapper->GetUniqueId())->GetUniqueId());
}

std::string GenericImageData::GenerateNickname(LayerRole role)
{
  // Have we generated a nickname for this role already?
  int count = 0;
  if(m_NicknameCounter.find(role) != m_NicknameCounter.end())
    count = m_NicknameCounter[role];

  // Get the basename for the nickname
  std::string name;
  switch(role)
    {
    case MAIN_ROLE:
      name = "Main Image";
      break;
    case OVERLAY_ROLE:
      name = "Additional Image";
      break;
    case LABEL_ROLE:
      name = "Segmentation Image";
      break;
    default:
      name = "Undefined";
      break;
    }

  // If the count is zero, nickname has never been generated
  if(count > 0)
    {
    std::ostringstream oss;
    oss << name << " " << count;
    name = oss.str();
    }

  // Update the counter for this kind of image
  m_NicknameCounter[role] = count + 1;

  return name;
}


ImageMeshLayers *
GenericImageData::GetMeshLayers()
{
  return m_MeshLayers;
}


