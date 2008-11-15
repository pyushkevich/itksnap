/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GenericImageData.cxx,v $
  Language:  C++
  Date:      $Date: 2008/11/15 12:20:38 $
  Version:   $Revision: 1.3 $
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

using namespace itk;

void 
GenericImageData
::SetSegmentationVoxel(const Vector3ui &index, LabelType value)
{
  // Make sure that the grey data and the segmentation data exist
  assert(m_GreyWrapper.IsInitialized() && m_LabelWrapper.IsInitialized());

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

  // Pass the label table from the parent to the label wrapper
  m_LabelWrapper.SetLabelColorTable(m_Parent->GetColorLabelTable());

  // Populate the array of linked wrappers
  m_LinkedWrappers.push_back(&m_GreyWrapper);
  m_LinkedWrappers.push_back(&m_RGBWrapper);
  m_LinkedWrappers.push_back(&m_LabelWrapper);

  // Start out with the main image pointing to the grey
  m_MainImage = &m_GreyWrapper;
}

Vector3d 
GenericImageData
::GetImageSpacing() 
{
  return m_GreyWrapper.GetImage()->GetSpacing().GetVnlVector();
}

Vector3d 
GenericImageData
::GetImageOrigin() 
{
  return m_GreyWrapper.GetImage()->GetOrigin().GetVnlVector();
}

void 
GenericImageData
::SetGreyImage(GreyImageType *newGreyImage,
               const ImageCoordinateGeometry &newGeometry) 
{
  // Make a new grey wrapper
  m_GreyWrapper.SetImage(newGreyImage);

  // Make the grey wrapper the main image
  m_MainImage = &m_GreyWrapper;
  
  // Clear the segmentation data to zeros
  m_LabelWrapper.InitializeToWrapper(&m_GreyWrapper, (LabelType) 0);
  
  // Clear the RGB data to zeros
  RGBType zero;
  zero[0] = 0;
  zero[1] = 0;
  zero[2] = 0;

  // If the RGB wrapper is loaded, we unload it
  if(IsRGBLoaded())
    {
    m_RGBWrapper.Reset();
    }
  
  // Store the image size info
  m_Size = m_GreyWrapper.GetSize();
  
  // Pass the coordinate transform to the wrappers
  SetImageGeometry(newGeometry);
}

void 
GenericImageData
::SetRGBImage(RGBImageType *newRGBImage,
               const ImageCoordinateGeometry &newGeometry) 
{
  m_RGBWrapper.SetImage(newRGBImage);
  m_RGBWrapper.SetAlpha(255);

  // Set the RGB as the main image
  m_MainImage = &m_RGBWrapper;
  
  // Initialize other wrappers to match this one
  m_GreyWrapper.InitializeToWrapper(&m_RGBWrapper, (GreyType) 0);  
  m_LabelWrapper.InitializeToWrapper(&m_RGBWrapper, (LabelType) 0);
  
  // Store the image size info
  m_Size = m_RGBWrapper.GetSize();
  
  // Pass the coordinate transform to the wrappers
  SetImageGeometry(newGeometry);
}

void 
GenericImageData
::SetRGBImageAsOverlay(RGBImageType *newRGBImage)
{
  // Check that the image matches the size of the grey image
  assert(m_GreyWrapper.GetImage()->GetBufferedRegion() == 
         newRGBImage->GetBufferedRegion());
  assert(m_MainImage == &m_GreyWrapper);
  
  // Pass the image to the segmentation wrapper
  m_RGBWrapper.SetImage(newRGBImage);
  m_RGBWrapper.SetAlpha(255);
  
  // Sync up spacing between the grey and RGB image
  newRGBImage->SetSpacing(m_GreyWrapper.GetImage()->GetSpacing());
  newRGBImage->SetOrigin(m_GreyWrapper.GetImage()->GetOrigin());

  // Propagate the geometry information to this wrapper
  for(unsigned int iSlice = 0;iSlice < 3;iSlice ++)
    {
    m_RGBWrapper.SetImageToDisplayTransform(
      iSlice,m_ImageGeometry.GetImageToDisplayTransform(iSlice));
    }
}

void 
GenericImageData
::SetSegmentationImage(LabelImageType *newLabelImage) 
{
  // Check that the image matches the size of the grey image
  assert(m_GreyWrapper.IsInitialized() &&
    m_GreyWrapper.GetImage()->GetBufferedRegion() == 
         newLabelImage->GetBufferedRegion());

  // Pass the image to the segmentation wrapper
  m_LabelWrapper.SetImage(newLabelImage);

  // Sync up spacing between the grey and label image
  newLabelImage->SetSpacing(m_GreyWrapper.GetImage()->GetSpacing());
  newLabelImage->SetOrigin(m_GreyWrapper.GetImage()->GetOrigin());
}

bool 
GenericImageData
::IsGreyLoaded() 
{
  return m_GreyWrapper.IsInitialized();
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
  std::list<ImageWrapperBase *>::iterator it = m_LinkedWrappers.begin();
  while(it != m_LinkedWrappers.end())
    {
    ImageWrapperBase *wrapper = *it++;
    if(wrapper->IsInitialized())
      wrapper->SetSliceIndex(crosshairs);
    }
}


GenericImageData::RegionType
GenericImageData
::GetImageRegion() const
{
  assert(m_GreyWrapper.IsInitialized());
  return m_GreyWrapper.GetImage()->GetBufferedRegion();
}

void 
GenericImageData
::SetImageGeometry(const ImageCoordinateGeometry &geometry)
{
  // Save the geometry
  m_ImageGeometry = geometry;

  // Propagate the geometry to the image wrappers
  for(WrapperIterator it = m_LinkedWrappers.begin(); 
    it != m_LinkedWrappers.end(); ++it)
    {
    ImageWrapperBase *wrapper = *it;
    if(wrapper->IsInitialized())
      {
      // Set the direction matrix in the image
      wrapper->GetImageBase()->SetDirection(
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
