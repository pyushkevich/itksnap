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
#include "itkImageIterator.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkMinimumMaximumImageCalculator.h"
#include "itkUnaryFunctorImageFilter.h"
#include "itkRGBAPixel.h"
#include "IRISSlicer.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include <algorithm>
#include <list>
#include <map>
#include <iostream>
#include "SNAPEventListenerCallbacks.h"
#include "GenericImageData.h"
#include "Rebroadcaster.h"
#include "LayerIterator.h"
#include "GuidedNativeImageIO.h"
#include "ImageAnnotationData.h"

// System includes
#include <fstream>
#include <iostream>
#include <iomanip>


void 
GenericImageData
::SetSegmentationVoxel(const Vector3ui &index, LabelType value)
{
  // Make sure that the main image data and the segmentation data exist
  assert(IsSegmentationLoaded());

  // Store the voxel
  m_LabelWrapper->SetVoxel(index, value);

  // Mark the image as modified
  m_LabelWrapper->GetImage()->Modified();
}

GenericImageData
::GenericImageData()
{
  // Make main image wrapper point to grey wrapper initially
  m_MainImageWrapper = NULL;

  // Pass the label table from the parent to the label wrapper
  m_LabelWrapper = NULL;
  
  // Add to the relevant lists
  m_Wrappers[MAIN_ROLE].push_back(m_MainImageWrapper);
  m_Wrappers[LABEL_ROLE].push_back(m_LabelWrapper.GetPointer());

  // Create empty annotations
  m_Annotations = ImageAnnotationData::New();
}

GenericImageData
::~GenericImageData()
{
  UnloadMainImage();
}

Vector3d 
GenericImageData
::GetImageSpacing() 
{
  assert(m_MainImageWrapper->IsInitialized());
  return m_MainImageWrapper->GetImageBase()->GetSpacing().GetVnlVector();
}

Vector3d 
GenericImageData
::GetImageOrigin() 
{
  assert(m_MainImageWrapper->IsInitialized());
  return m_MainImageWrapper->GetImageBase()->GetOrigin().GetVnlVector();
}


void
GenericImageData
::SetMainImageInternal(ImageWrapperBase *wrapper)
{
  // Set properties
  wrapper->SetDefaultNickname("Main Image");

  // Make the wrapper the main image
  SetSingleImageWrapper(MAIN_ROLE, wrapper);
  m_MainImageWrapper = wrapper;

  // Reset the segmentation image
  ResetSegmentationImage();

  // Set opaque
  m_MainImageWrapper->SetAlpha(255);
}

#include "itkIdentityTransform.h"

SmartPtr<ImageWrapperBase>
GenericImageData::CreateAnatomicWrapper(GuidedNativeImageIO *io, bool sameSpaceAsMainWrapper)
{
  // The output wrapper
  SmartPtr<ImageWrapperBase> out_wrapper;

  // Determine the reference image and transform
  ImageBaseType *refSpace = NULL;
  typedef itk::IdentityTransform<double, 3> TransformType;
  SmartPtr<TransformType> transform = NULL;

  if(!sameSpaceAsMainWrapper)
    {
    refSpace = this->GetMain()->GetImageBase();
    transform = TransformType::New();
    }

  // Split depending on whether the image is scalar or vector
  if(io->GetNumberOfComponentsInNativeImage() > 1)
    {
    // The image will be cast to a vector anatomic image
    typedef AnatomicImageWrapper::ImageType AnatomicImageType;

    // Rescale the image to desired number of bits
    RescaleNativeImageToIntegralType<AnatomicImageType> rescaler;
    AnatomicImageType::Pointer image = rescaler(io);

    // Create a mapper to native intensity
    LinearInternalToNativeIntensityMapping mapper(
          rescaler.GetNativeScale(), rescaler.GetNativeShift());

    // Create a main wrapper of fixed type.
    SmartPtr<AnatomicImageWrapper> wrapper = AnatomicImageWrapper::New();

    // Set properties
    wrapper->SetDisplayGeometry(m_DisplayGeometry);
    wrapper->SetImage(image, refSpace, transform);
    wrapper->SetNativeMapping(mapper);
    out_wrapper = wrapper.GetPointer();
    }

  else
    {
    // Rescale the image to desired number of bits
    typedef AnatomicScalarImageWrapper::ImageType AnatomicImageType;

    // Rescale the image to desired number of bits
    RescaleNativeImageToIntegralType<AnatomicImageType> rescaler;
    AnatomicImageType::Pointer image = rescaler(io);

    // Create a mapper to native intensity
    LinearInternalToNativeIntensityMapping mapper(
          rescaler.GetNativeScale(), rescaler.GetNativeShift());

    // Create a main wrapper of fixed type.
    SmartPtr<AnatomicScalarImageWrapper> wrapper = AnatomicScalarImageWrapper::New();

    // Set properties
    wrapper->SetDisplayGeometry(m_DisplayGeometry);
    wrapper->SetImage(image, refSpace, transform);
    wrapper->SetNativeMapping(mapper);
    out_wrapper = wrapper.GetPointer();
    }

  // Create an image coordinate geometry object
  return out_wrapper;
}

void GenericImageData::SetMainImage(GuidedNativeImageIO *io)
{
  // Create the wrapper from the Native IO (the wrapper will either be a scalar
  // or a vector-valued image, depending on the number of components)
  SmartPtr<ImageWrapperBase> wrapper = this->CreateAnatomicWrapper(io, true);

  // Assign this wrapper to the main image
  this->SetMainImageInternal(wrapper);
}

void
GenericImageData
::ResetSegmentationImage()
{
  // Initialize the segmentation data to zeros
  m_LabelWrapper = LabelImageWrapper::New();
  m_LabelWrapper->InitializeToWrapper(m_MainImageWrapper, (LabelType) 0);
  m_LabelWrapper->SetDefaultNickname("Segmentation Image");

  m_LabelWrapper->GetDisplayMapping()->SetLabelColorTable(m_Parent->GetColorLabelTable());
  SetSingleImageWrapper(LABEL_ROLE, m_LabelWrapper.GetPointer());
}

void
GenericImageData
::UnloadMainImage()
{
  // First unload the overlays if exist
  UnloadOverlays();

  // Clear the main image wrappers
  RemoveSingleImageWrapper(MAIN_ROLE);
  m_MainImageWrapper = NULL;

  // Reset the label wrapper
  RemoveSingleImageWrapper(LABEL_ROLE);
  m_LabelWrapper = NULL;

  // Clear the annotations
  m_Annotations->Reset();
}

void
GenericImageData
::AddOverlay(GuidedNativeImageIO *io)
{
  // Create the wrapper from the Native IO (the wrapper will either be a scalar
  // or a vector-valued image, depending on the number of components)
  SmartPtr<ImageWrapperBase> wrapper = this->CreateAnatomicWrapper(io, true);

  // Assign this wrapper to the main image
  this->AddOverlayInternal(wrapper);
}

void
GenericImageData
::AddCoregOverlay(GuidedNativeImageIO *io)
{
  // Create the wrapper from the Native IO (the wrapper will either be a scalar
  // or a vector-valued image, depending on the number of components)
  SmartPtr<ImageWrapperBase> wrapper = this->CreateAnatomicWrapper(io, false);

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
  overlay->SetDefaultNickname("Additional Image");

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
  if (!IsOverlayLoaded())
    return;

  // Release the data associated with the last overlay
  PopBackImageWrapper(OVERLAY_ROLE);
}

void GenericImageData
::UnloadOverlay(ImageWrapperBase *overlay)
{
  // Erase the overlay
  WrapperList &overlays = m_Wrappers[OVERLAY_ROLE];
  WrapperIterator it =
      std::find(overlays.begin(), overlays.end(), overlay);
  if(it != overlays.end())
    overlays.erase(it);
}

void
GenericImageData
::SetSegmentationImage(LabelImageType *newLabelImage) 
{
  // Check that the image matches the size of the grey image
  assert(m_MainImageWrapper->IsInitialized() &&
    m_MainImageWrapper->GetBufferedRegion() == 
         newLabelImage->GetBufferedRegion());

  // Pass the image to the segmentation wrapper (why this and not create a
  // new label wrapper? Why should a wrapper have longer lifetime than an
  // image that it wraps around
  m_LabelWrapper->SetImage(newLabelImage);

  // Sync up spacing between the main and label image
  m_LabelWrapper->CopyImageCoordinateTransform(m_MainImageWrapper);
}

bool
GenericImageData
::IsOverlayLoaded()
{
  return (m_Wrappers[OVERLAY_ROLE].size() > 0);
}

bool
GenericImageData
::IsSegmentationLoaded()
{
  return m_LabelWrapper && m_LabelWrapper->IsInitialized();
}

void
GenericImageData
::SetCrosshairs(const Vector3ui &crosshairs)
{
  // Set crosshairs in all wrappers
  for(LayerIterator lit(this); !lit.IsAtEnd(); ++lit)
    if(lit.GetLayer() && lit.GetLayer()->IsInitialized())
      lit.GetLayer()->SetSliceIndex(crosshairs);
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

void GenericImageData::SetDirectionMatrix(const vnl_matrix<double> &direction)
{
  for(LayerIterator lit(this); !lit.IsAtEnd(); ++lit)
    if(lit.GetLayer())
      {
      // Set the direction matrix in the image
      lit.GetLayer()->SetDirectionMatrix(direction);
      }
}

const ImageCoordinateGeometry &GenericImageData::GetImageGeometry() const
{
  assert(m_MainImageWrapper->IsInitialized());
  return m_MainImageWrapper->GetImageGeometry();
}

GenericImageData::RegionType
GenericImageData
::GetImageRegion() const
{
  assert(m_MainImageWrapper->IsInitialized());
  return m_MainImageWrapper->GetBufferedRegion();
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

int GenericImageData::GetNumberOfOverlays()
{
  return m_Wrappers[OVERLAY_ROLE].size();
}

ImageWrapperBase *GenericImageData::GetLastOverlay()
{
  return m_Wrappers[OVERLAY_ROLE].back();
}



void GenericImageData::PushBackImageWrapper(LayerRole role,
                                            ImageWrapperBase *wrapper)
{
  // Append the wrapper
  m_Wrappers[role].push_back(wrapper);

  // Rebroadcast the wrapper-related events as our own events
  Rebroadcaster::RebroadcastAsSourceEvent(wrapper, WrapperChangeEvent(), this);
}


void GenericImageData::PopBackImageWrapper(LayerRole role)
{
  m_Wrappers[role].pop_back();
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
}

void GenericImageData::RemoveImageWrapper(LayerRole role,
                                          ImageWrapperBase *wrapper)
{
  m_Wrappers[role].erase(
        std::find(m_Wrappers[role].begin(), m_Wrappers[role].end(), wrapper));
}

void GenericImageData::SetSingleImageWrapper(LayerRole role,
                                             ImageWrapperBase *wrapper)
{
  assert(m_Wrappers[role].size() == 1);
  m_Wrappers[role].front() = wrapper;

  // Rebroadcast the wrapper-related events as our own events
  Rebroadcaster::RebroadcastAsSourceEvent(wrapper, WrapperChangeEvent(), this);
}

void GenericImageData::RemoveSingleImageWrapper(LayerRole role)
{
  assert(m_Wrappers[role].size() == 1);
  m_Wrappers[role].front() = NULL;
}








void GenericImageData::AddOverlay(ImageWrapperBase *new_layer)
{
  this->AddOverlayInternal(new_layer, true);
}
