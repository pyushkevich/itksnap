/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: IRISImageData.cxx,v $
  Language:  C++
  Date:      $Date: 2007/09/17 04:53:35 $
  Version:   $Revision: 1.4 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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
typedef IRISSlicer<unsigned char> IRISIMageDataDummyIRISSlicerTypeUchar;
typedef itk::SmartPointer<IRISIMageDataDummyIRISSlicerTypeUchar> IRISIMageDataDummySmartPointerSlicerType;
typedef IRISSlicer<short> IRISIMageDataDummyIRISSlicerTypeShort;
typedef itk::SmartPointer<IRISIMageDataDummyIRISSlicerTypeShort> IRISIMageDataDummySmartPointerSlicerShortType;
typedef itk::ImageRegion<3> IRISImageDataBorlandDummyImageRegionType;
typedef itk::ImageRegion<2> IRISImageDataBorlandDummyImageRegionType2;
typedef itk::ImageBase<3> IRISImageDataBorlandDummyImageBaseType;
typedef itk::ImageBase<2> IRISImageDataBorlandDummyImageBaseType2;
typedef itk::Image<unsigned char,3> IRISImageDataBorlandDummyImageType;
typedef itk::Image<unsigned char,2> IRISImageDataBorlandDummyImageType2;
typedef itk::ImageRegionConstIterator<IRISImageDataBorlandDummyImageType> IRISImageDataBorlandDummyConstIteratorType;
typedef itk::Image<short,3> IRISImageDataBorlandDummyShortImageType;
typedef itk::Image<short,2> IRISImageDataBorlandDummyShortImageType2;
typedef itk::Image<itk::RGBAPixel<unsigned char>,2> IRISImageDataBorlandDummyShortImageTypeRGBA;
typedef itk::ImageRegionConstIterator<IRISImageDataBorlandDummyShortImageType> IRISImageDataBorlandDummyConstIteratorShortType;
typedef itk::MinimumMaximumImageCalculator<IRISImageDataBorlandDummyShortImageType> IRISImageDataBorlandDummyMinMaxCalc;
#endif

#include "GreyImageWrapper.h"
#if defined(__BORLANDC__)
typedef CachingUnaryFunctor<short,unsigned char,GreyImageWrapper::IntensityFunctor> IRISImageDataBorlamdCachingUnaryFunctor;
typedef itk::UnaryFunctorImageFilter<IRISImageDataBorlandDummyShortImageType,IRISImageDataBorlandDummyImageType2,IRISImageDataBorlamdCachingUnaryFunctor> IRISIMageDataDummyFunctorType;
typedef itk::SmartPointer<IRISIMageDataDummyFunctorType> IRISIMageDataDummyFunctorTypePointerType;
#endif


#include "IRISImageData.h"

#include "LabelImageWrapper.h"

#include "RGBImageWrapper.h"

// System includes
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace itk;

void 
IRISImageData
::SetSegmentationVoxel(const Vector3ui &index, LabelType value)
{
  // Make sure that the grey data and the segmentation data exist
  assert(m_GreyWrapper.IsInitialized() && m_LabelWrapper.IsInitialized());

  // Store the voxel
  m_LabelWrapper.GetVoxelForUpdate(index) = value;

  // Mark the image as modified
  m_LabelWrapper.GetImage()->Modified();
}

IRISImageData
::IRISImageData(IRISApplication *parent) 
{
  // Copy the parent object
  m_Parent = parent;  

  // Pass the label table from the parent to the label wrapper
  m_LabelWrapper.SetLabelColorTable(m_Parent->GetColorLabelTable());

  // Populate the array of linked wrappers
  m_LinkedWrappers.push_back(&m_GreyWrapper);
  m_LinkedWrappers.push_back(&m_RGBWrapper);
  m_LinkedWrappers.push_back(&m_LabelWrapper);
}

Vector3d 
IRISImageData
::GetImageSpacing() 
{
  return m_GreyWrapper.GetImage()->GetSpacing().GetVnlVector();
}

Vector3d 
IRISImageData
::GetImageOrigin() 
{
  return m_GreyWrapper.GetImage()->GetOrigin().GetVnlVector();
}

void 
IRISImageData
::SetGreyImage(GreyImageType *newGreyImage,
               const ImageCoordinateGeometry &newGeometry) 
{
  // Make a new grey wrapper
  m_GreyWrapper.SetImage(newGreyImage);
  
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
IRISImageData
::SetRGBImage(RGBImageType *newRGBImage,
               const ImageCoordinateGeometry &newGeometry) 
{
  m_RGBWrapper.SetImage(newRGBImage);
  m_RGBWrapper.SetAlpha(255);
  
  m_GreyWrapper.InitializeToWrapper(&m_RGBWrapper, (GreyType) 0);
  
  m_LabelWrapper.InitializeToWrapper(&m_RGBWrapper, (LabelType) 0);
  
  // Store the image size info
  m_Size = m_RGBWrapper.GetSize();
  
  // Pass the coordinate transform to the wrappers
  SetImageGeometry(newGeometry);
}

void 
IRISImageData
::SetRGBImage(RGBImageType *newRGBImage)
{
  // Check that the image matches the size of the grey image
  assert(m_GreyWrapper.GetImage()->GetBufferedRegion() == 
         newRGBImage->GetBufferedRegion());
  
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
IRISImageData
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
IRISImageData
::IsGreyLoaded() 
{
  return m_GreyWrapper.IsInitialized();
}

bool 
IRISImageData
::IsRGBLoaded() 
{
  return m_RGBWrapper.IsInitialized();
}

bool 
IRISImageData
::IsSegmentationLoaded() 
{
  return m_LabelWrapper.IsInitialized();
}    

void
IRISImageData
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


IRISImageData::RegionType
IRISImageData
::GetImageRegion() const
{
  assert(m_GreyWrapper.IsInitialized());
  return m_GreyWrapper.GetImage()->GetBufferedRegion();
}

void 
IRISImageData
::SetImageGeometry(const ImageCoordinateGeometry &geometry)
{
  // Save the geometry
  m_ImageGeometry = geometry;

  // Propagate the geometry to the image wrappers
  std::list<ImageWrapperBase *>::iterator it = m_LinkedWrappers.begin();
  while(it != m_LinkedWrappers.end())
    {
    ImageWrapperBase *wrapper = *it++;
    if(wrapper->IsInitialized())
      {
      // Update the geometry for each slice
      for(unsigned int iSlice = 0;iSlice < 3;iSlice ++)
        {
        wrapper->SetImageToDisplayTransform(
          iSlice,m_ImageGeometry.GetImageToDisplayTransform(iSlice));
        }
      }
    }
}
