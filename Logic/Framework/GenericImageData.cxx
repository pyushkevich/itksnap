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
#include "UnaryFunctorCache.h"
#include "itkRGBAPixel.h"
#include "IRISSlicer.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include <list>

/** Borland compiler is very lazy so we need to instantiate the template
 *  by hand */
#if defined(__BORLANDC__)
typedef IRISSlicer<unsigned char> GenericImageDataDummyIRISSlicerTypeUchar;
typedef itk::SmartPointer<GenericImageDataDummyIRISSlicerTypeUchar> GenericImageDataDummySmartPointerSlicerType;
typedef IRISSlicer<short> GenericImageDataDummyIRISSlicerTypeShort;
typedef itk::SmartPointer<GenericImageDataDummyIRISSlicerTypeShort> GenericImageDataDummySmartPointerSlicerShortType;
typedef itk::ImageRegion<3> GenericImageDataBorlandDummyImageRegionType;
typedef itk::ImageRegion<2> GenericImageDataBorlandDummyImageRegionType2;
typedef itk::ImageBase<3> GenericImageDataBorlandDummyImageBaseType;
typedef itk::ImageBase<2> GenericImageDataBorlandDummyImageBaseType2;
typedef itk::Image<unsigned char,3> GenericImageDataBorlandDummyImageType;
typedef itk::Image<unsigned char,2> GenericImageDataBorlandDummyImageType2;
typedef itk::ImageRegionConstIterator<GenericImageDataBorlandDummyImageType> GenericImageDataBorlandDummyConstIteratorType;
typedef itk::Image<short,3> GenericImageDataBorlandDummyShortImageType;
typedef itk::Image<short,2> GenericImageDataBorlandDummyShortImageType2;
typedef itk::Image<itk::RGBAPixel<unsigned char>,2> GenericImageDataBorlandDummyShortImageTypeRGBA;
typedef itk::ImageRegionConstIterator<GenericImageDataBorlandDummyShortImageType> GenericImageDataBorlandDummyConstIteratorShortType;
typedef itk::MinimumMaximumImageCalculator<GenericImageDataBorlandDummyShortImageType> GenericImageDataBorlandDummyMinMaxCalc;
#endif

#include "GreyImageWrapper.h"
#if defined(__BORLANDC__)
typedef CachingUnaryFunctor<short,unsigned char,GreyImageWrapper::IntensityFunctor> GenericImageDataBorlamdCachingUnaryFunctor;
typedef itk::UnaryFunctorImageFilter<GenericImageDataBorlandDummyShortImageType,GenericImageDataBorlandDummyImageType2,GenericImageDataBorlamdCachingUnaryFunctor> GenericImageDataDummyFunctorType;
typedef itk::SmartPointer<GenericImageDataDummyFunctorType> GenericImageDataDummyFunctorTypePointerType;
#endif

#include "GenericImageData.h"

#include "LabelImageWrapper.h"

#include "RGBImageWrapper.h"

// System includes
#include <fstream>
#include <iostream>
#include <iomanip>


void 
GenericImageData
::SetSegmentationVoxel(const Vector3ui &index, LabelType value)
{
  // Make sure that the main image data and the segmentation data exist
  assert(m_MainImageWrapper->IsInitialized() && m_LabelWrapper.IsInitialized());

  // Store the voxel
  m_LabelWrapper.GetVoxelForUpdate(index) = value;

  // Mark the image as modified
  m_LabelWrapper.GetImage()->Modified();
}

GenericImageData
::GenericImageData(IRISApplication *parent) 
{
  // Copy the parent object
  m_Parent = parent;

  // Make main image wrapper point to grey wrapper initially
  m_MainImageWrapper = &m_GreyWrapper;

  // Pass the label table from the parent to the label wrapper
  m_LabelWrapper.SetLabelColorTable(m_Parent->GetColorLabelTable());
  
  // Add to the primary wrapper list
  m_MainWrappers.push_back(m_MainImageWrapper);
  m_MainWrappers.push_back(&m_LabelWrapper);
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
::SetGreyImage(GreyImageType *newGreyImage,
               const ImageCoordinateGeometry &newGeometry,
               const GreyTypeToNativeFunctor &native) 
{
  m_GreyWrapper.SetImage(newGreyImage);
  m_GreyWrapper.SetNativeMapping(native);
  m_GreyWrapper.SetAlpha(255);
  m_GreyWrapper.UpdateIntensityMapFunction();

  SetMainImageCommon(&m_GreyWrapper, newGeometry);
}

void
GenericImageData
::SetRGBImage(RGBImageType *newRGBImage,
              const ImageCoordinateGeometry &newGeometry) 
{
  m_RGBWrapper.SetImage(newRGBImage);
  m_RGBWrapper.SetAlpha(255);

  SetMainImageCommon(&m_RGBWrapper, newGeometry);
}

void
GenericImageData
::SetMainImageCommon(ImageWrapperBase *wrapper,
                      const ImageCoordinateGeometry &newGeometry)
{
  // Make the wrapper the main image
  m_MainImageWrapper = wrapper;
  m_MainWrappers.pop_front();
  m_MainWrappers.push_front(m_MainImageWrapper);

  // Initialize the segmentation data to zeros
  m_LabelWrapper.InitializeToWrapper(m_MainImageWrapper, (LabelType) 0);

  // Pass the coordinate transform to the wrappers
  SetImageGeometry(newGeometry);
}

void
GenericImageData
::UnloadMainImage()
{
  // First unload the overlays if exist
  UnloadOverlays();

  // Clear the main image wrappers
  m_LabelWrapper.Reset();
  m_MainImageWrapper->Reset();
}

void
GenericImageData
::SetGreyOverlay(GreyImageType *newGreyImage,
                 const GreyTypeToNativeFunctor &native)
{
  // Check that the image matches the size of the main image
  //Octavian_2012_08_24_16:20: changed assert into this test as a response to:
  //bug: ID: 3023489: "-o flag size check" 
  if(m_MainImageWrapper->GetBufferedRegion() !=
	  newGreyImage->GetBufferedRegion()) {
	throw IRISException("Main and overlay data sizes are different");
  }

  // Pass the image to a Grey image wrapper
  GreyImageWrapper *newGreyOverlayWrapper = new GreyImageWrapper;
  newGreyOverlayWrapper->SetImage(newGreyImage);
  newGreyOverlayWrapper->SetNativeMapping(native);
  newGreyOverlayWrapper->SetAlpha(128);
  newGreyOverlayWrapper->UpdateIntensityMapFunction();

  SetOverlayCommon(newGreyOverlayWrapper);
}

void
GenericImageData
::SetRGBOverlay(RGBImageType *newRGBImage)
{
  // Check that the image matches the size of the main image
  //Octavian_2012_08_24_16:20: changed assert into this test as a response to:
  //bug: ID: 3023489: "-o flag size check" 
  if(m_MainImageWrapper->GetBufferedRegion() !=
	  newRGBImage->GetBufferedRegion()) {
		throw IRISException("Main and overlay data sizes are different");
  }

  // Pass the image to a RGB image wrapper
  RGBImageWrapper *newRGBOverlayWrapper = new RGBImageWrapper;
  newRGBOverlayWrapper->SetImage(newRGBImage);
  newRGBOverlayWrapper->SetAlpha(128);

  SetOverlayCommon(newRGBOverlayWrapper);
}

void
GenericImageData
::SetOverlayCommon(ImageWrapperBase *overlay)
{
  // Sync up spacing between the main and overlay image
  overlay->GetImageBase()->SetSpacing(m_MainImageWrapper->GetImageBase()->GetSpacing());
  overlay->GetImageBase()->SetOrigin(m_MainImageWrapper->GetImageBase()->GetOrigin());

  // Propagate the geometry information to this wrapper
  for(unsigned int iSlice = 0;iSlice < 3;iSlice ++)
    {
    overlay->SetImageToDisplayTransform(
      iSlice,m_ImageGeometry.GetImageToDisplayTransform(iSlice));
    }

  // Add to the overlay wrapper list
  m_OverlayWrappers.push_back(overlay);
}

void
GenericImageData
::UnloadOverlays()
{
  while (m_OverlayWrappers.size() > 0)
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
  ImageWrapperBase *wrapper = m_OverlayWrappers.back();
  delete wrapper;
  wrapper = NULL;

  // Clear it off the wrapper lists
  m_OverlayWrappers.pop_back();
}

void
GenericImageData
::SetSegmentationImage(LabelImageType *newLabelImage) 
{
  // Check that the image matches the size of the grey image
  assert(m_MainImageWrapper->IsInitialized() &&
    m_MainImageWrapper->GetBufferedRegion() == 
         newLabelImage->GetBufferedRegion());

  // Pass the image to the segmentation wrapper
  m_LabelWrapper.SetImage(newLabelImage);

  // Sync up spacing between the main and label image
  newLabelImage->SetSpacing(m_MainImageWrapper->GetImageBase()->GetSpacing());
  newLabelImage->SetOrigin(m_MainImageWrapper->GetImageBase()->GetOrigin());
}

bool
GenericImageData
::IsGreyLoaded()
{
  return m_GreyWrapper.IsInitialized();
}

bool
GenericImageData
::IsOverlayLoaded()
{
  return (m_OverlayWrappers.size() > 0);
}

bool
GenericImageData
::IsRGBLoaded()
{
  return m_RGBWrapper.IsInitialized();
}

bool
GenericImageData
::IsSegmentationLoaded()
{
  return m_LabelWrapper.IsInitialized();
}

void
GenericImageData
::SetCrosshairs(const Vector3ui &crosshairs)
{
  SetCrosshairs(m_MainWrappers, crosshairs);
  SetCrosshairs(m_OverlayWrappers, crosshairs);
}

void
GenericImageData
::SetCrosshairs(WrapperList &list, const Vector3ui &crosshairs)
{
  WrapperIterator it = list.begin();
  while (it != list.end())
    {
    ImageWrapperBase *wrapper = *it++;
    if (wrapper->IsInitialized())
      wrapper->SetSliceIndex(crosshairs);
    }
}

GenericImageData::RegionType
GenericImageData
::GetImageRegion() const
{
  assert(m_MainImageWrapper->IsInitialized());
  return m_MainImageWrapper->GetBufferedRegion();
}

void
GenericImageData
::SetImageGeometry(const ImageCoordinateGeometry &geometry)
{
  SetImageGeometry(m_MainWrappers, geometry);
  SetImageGeometry(m_OverlayWrappers, geometry);

  WrapperList temp;
  temp.push_back(&m_LabelWrapper);
  SetImageGeometry(temp, geometry);
}

void
GenericImageData
::SetImageGeometry(WrapperList &list, const ImageCoordinateGeometry &geometry)
{
  // Save the geometry
  m_ImageGeometry = geometry;

  // Propagate the geometry to the image wrappers
  for(WrapperIterator it = list.begin();
    it != list.end(); ++it)
    {
    ImageWrapperBase *wrapper = *it;
    if(wrapper->IsInitialized())
      {
      // Set the direction matrix in the image
      wrapper->SetDirectionCosineMatrix(
        itk::Matrix<double,3,3>(geometry.GetImageDirectionCosineMatrix()));

      // Update the geometry for each slice
      for(unsigned int iSlice = 0;iSlice < 3;iSlice ++)
        {
        wrapper->SetImageToDisplayTransform(
          iSlice,m_ImageGeometry.GetImageToDisplayTransform(iSlice));
        }
      }
    }
}

