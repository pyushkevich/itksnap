/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ImageWrapper.txx,v $
  Language:  C++
  Date:      $Date: 2010/10/14 16:21:04 $
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

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/

#include "ImageWrapper.h"
#include "itkImageRegionIterator.h"
#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkNumericTraits.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkIdentityTransform.h"
#include "IRISSlicer.h"
#include "SNAPSegmentationROISettings.h"
#include "itkCommand.h"

#include <vnl/vnl_inverse.h>
#include <iostream>

#include <itksys/SystemTools.hxx>

template <class TPixel> 
ImageWrapper<TPixel>
::ImageWrapper() 
{
  CommonInitialization();
}

template <class TPixel>
ImageWrapper<TPixel>
::~ImageWrapper() 
{
  Reset();
}

template <class TPixel>
void 
ImageWrapper<TPixel>
::CommonInitialization()
{
  // Set initial state    
  m_Initialized = false;

  // Create slicer objects
  m_Slicer[0] = SlicerType::New();
  m_Slicer[1] = SlicerType::New();
  m_Slicer[2] = SlicerType::New();

  // Set the transform to identity, which will initialize the directions of the
  // slicers
  this->SetImageToDisplayTransformsToDefault();
}

template <class TPixel>
ImageWrapper<TPixel>
::ImageWrapper(const ImageWrapper<TPixel> &copy) 
{
  CommonInitialization();

  // If the source contains an image, make a copy of that image
  if (copy.IsInitialized() && copy.GetImage())
    {
    // Create and allocate the image
    ImagePointer newImage = ImageType::New();
    newImage->SetRegions(copy.GetImage()->GetBufferedRegion());
    newImage->Allocate();

    // Copy the image contents
    TPixel *ptrTarget = newImage->GetBufferPointer();
    TPixel *ptrSource = copy.GetImage()->GetBufferPointer();
    memcpy(ptrTarget,ptrSource,
           sizeof(TPixel) * newImage->GetBufferedRegion().GetNumberOfPixels());
    
    UpdateImagePointer(newImage);
    }
}

template <class TPixel>
const ImageCoordinateTransform &
ImageWrapper<TPixel>
::GetImageToDisplayTransform(unsigned int iSlice) const
{
  return m_ImageToDisplayTransform[iSlice];
}

template <class TPixel>
void
ImageWrapper<TPixel>
::SetImageToDisplayTransformsToDefault()
{
  ImageCoordinateTransform id[3];
  id[0].SetTransform(Vector3i(1,2,3),Vector3ui(0,0,0));
  id[1].SetTransform(Vector3i(1,3,2),Vector3ui(0,0,0));
  id[2].SetTransform(Vector3i(2,3,1),Vector3ui(0,0,0));
  SetImageToDisplayTransform(0,id[0]);
  SetImageToDisplayTransform(1,id[1]);
  SetImageToDisplayTransform(2,id[2]);
}

template <class TPixel>
Vector3ui
ImageWrapper<TPixel>
::GetSize() const
{
  // Cast the size to our vector format
  itk::Size<3> size = m_Image->GetLargestPossibleRegion().GetSize();
  return Vector3ui(
    (unsigned int) size[0],
    (unsigned int) size[1],
        (unsigned int) size[2]);
}

template <class TPixel>
void
ImageWrapper<TPixel>
::ToggleVisibility()
{
  // If visible (alpha > 0), make invisible
  if(m_Alpha > 0)
    {
    m_ToggleAlpha = m_Alpha;
    SetAlpha(0);
    }
  // If invisible, return to saved alpha value
  else
    {
    SetAlpha(m_ToggleAlpha);
    }
}

template <class TPixel>
itk::ImageRegion<3>
ImageWrapper<TPixel>
::GetBufferedRegion() const
{
  return m_ImageBase->GetBufferedRegion();
}

template <class TPixel>
size_t
ImageWrapper<TPixel>
::GetNumberOfVoxels() const
{
  return m_ImageBase->GetBufferedRegion().GetNumberOfPixels();
}

template <class TPixel>
Vector3d
ImageWrapper<TPixel>
::TransformVoxelIndexToPosition(const Vector3ui &iVoxel) const
{
  // Use the ITK method to do this
  typename ImageBaseType::IndexType xIndex;
  for(size_t d = 0; d < 3; d++) xIndex[d] = iVoxel[d];

  itk::Point<double, 3> xPoint;
  m_ImageBase->TransformIndexToPhysicalPoint(xIndex, xPoint);

  Vector3d xOut;
  for(unsigned int q = 0; q < 3; q++) xOut[q] = xPoint[q];

  return xOut;
}

template <class TPixel>
Vector3d
ImageWrapper<TPixel>
::TransformVoxelIndexToNIFTICoordinates(const Vector3d &iVoxel) const
{
  // Create homogeneous vector
  vnl_vector_fixed<double, 4> x;
  for(size_t d = 0; d < 3; d++)
    x[d] = (double) iVoxel[d];
  x[3] = 1.0;

  // Transform to NIFTI coords
  vnl_vector_fixed<double, 4> p = m_NiftiSform * x;

  // Return the component
  return Vector3d(p[0], p[1], p[2]);
}

template <class TPixel>
Vector3d
ImageWrapper<TPixel>
::TransformNIFTICoordinatesToVoxelIndex(const Vector3d &vNifti) const
{
  // Create homogeneous vector
  vnl_vector_fixed<double, 4> x;
  for(size_t d = 0; d < 3; d++)
    x[d] = (double) vNifti[d];
  x[3] = 1.0;

  // Transform to NIFTI coords
  vnl_vector_fixed<double, 4> p = m_NiftiInvSform * x;

  // Return the component
  return Vector3d(p[0], p[1], p[2]);
}


template <class TPixel> 
void 
ImageWrapper<TPixel>
::PrintDebugInformation() 
{
  std::cout << "=== Image Properties ===" << std::endl;
  std::cout << "   Dimensions         : " << m_Image->GetLargestPossibleRegion().GetSize() << std::endl;
  std::cout << "   Origin             : " << m_Image->GetOrigin() << std::endl;
  std::cout << "   Spacing            : " << m_Image->GetSpacing() << std::endl;
  std::cout << "------------------------" << std::endl;
}

template <class TPixel>
void 
ImageWrapper<TPixel>
::UpdateImagePointer(ImageType *newImage) 
{
  // Check if the image size has changed
  bool hasSizeChanged = 
    (!m_Image) || 
    (newImage->GetLargestPossibleRegion().GetSize() !=
     m_Image->GetLargestPossibleRegion().GetSize());
  
  // Change the input of the slicers 
  m_Slicer[0]->SetInput(newImage);
  m_Slicer[1]->SetInput(newImage);
  m_Slicer[2]->SetInput(newImage);
    
  // If so, the coordinate transform needs to be reinitialized to identity
  if(hasSizeChanged)
    {
    // Reset the transform to identity
    this->SetImageToDisplayTransformsToDefault();

    // Reset the slice positions to zero
    this->SetSliceIndex(Vector3ui(0,0,0));
    }
  
  // Update the image
  this->m_ImageBase = newImage;
  m_Image = newImage;

  // Mark the image as Modified to enforce correct sequence of 
  // operations with MinMaxCalc
  m_Image->Modified();

  // Set the NIFTI/RAS transform
  m_NiftiSform = ConstructNiftiSform(
    m_Image->GetDirection().GetVnlMatrix(),
    m_Image->GetOrigin().GetVnlVector(),
    m_Image->GetSpacing().GetVnlVector());
  m_NiftiInvSform = vnl_inverse(m_NiftiSform);

  // We have been initialized
  m_Initialized = true;
}

template <class TPixel>
void 
ImageWrapper<TPixel>
::InitializeToWrapper(const ImageWrapperBase *source, ImageType *image) 
{
  // Call the common update method
  UpdateImagePointer(image);

  // Update the image-display transforms
  for(unsigned int d=0;d<3;d++)
    SetImageToDisplayTransform(d,source->GetImageToDisplayTransform(d));

  // Update the slice index
  SetSliceIndex(source->GetSliceIndex());
}

template <class TPixel>
void 
ImageWrapper<TPixel>
::InitializeToWrapper(const ImageWrapperBase *source, const TPixel &value) 
{
  // Allocate the image
  ImagePointer newImage = ImageType::New();
  newImage->SetRegions(source->GetImageBase()->GetBufferedRegion().GetSize());
  newImage->Allocate();
  newImage->FillBuffer(value);
  newImage->SetOrigin(source->GetImageBase()->GetOrigin());
  newImage->SetSpacing(source->GetImageBase()->GetSpacing());
  newImage->SetDirection(source->GetImageBase()->GetDirection());

  // Call the common update method
  UpdateImagePointer(newImage);

  // Update the image-display transforms
  for(unsigned int d=0;d<3;d++)
    SetImageToDisplayTransform(d,source->GetImageToDisplayTransform(d));

  // Update the slice index
  SetSliceIndex(source->GetSliceIndex());
}

template <class TPixel>
void 
ImageWrapper<TPixel>
::SetImage(ImagePointer newImage) 
{
  UpdateImagePointer(newImage);
}


template <class TPixel>
void 
ImageWrapper<TPixel>
::Reset() 
{
  if (m_Initialized)
    {
    m_Image->ReleaseData();
    m_Image = NULL;
    }
  m_Initialized = false;

  m_Alpha = 128;
  m_ToggleAlpha = 128;
}


template <class TPixel>
inline const TPixel& 
ImageWrapper<TPixel>
::GetVoxel(const Vector3ui &index) const 
{
  return GetVoxel(index[0],index[1],index[2]);
}

template <class TPixel>
inline TPixel& 
ImageWrapper<TPixel>
::GetVoxelForUpdate(const Vector3ui &index) 
{
  return GetVoxelForUpdate(index[0],index[1],index[2]);
}

template <class TPixel>
inline TPixel& 
ImageWrapper<TPixel>
::GetVoxelForUpdate(unsigned int x, unsigned int y, unsigned int z) 
{
  itk::Index<3> index;
  index[0] = x;
  index[1] = y;
  index[2] = z;

  return GetVoxelForUpdate(index);
}

template <class TPixel>
inline TPixel &
ImageWrapper<TPixel>
::GetVoxelForUpdate(const itk::Index<3> &index)
{
  // Verify that the pixel is contained by the image at debug time
  assert(m_Image && m_Image->GetLargestPossibleRegion().IsInside(index));

  // Return the pixel
  return m_Image->GetPixel(index);
}

template <class TPixel>
inline const TPixel& 
ImageWrapper<TPixel>
::GetVoxel(unsigned int x, unsigned int y, unsigned int z) const
{
  itk::Index<3> index;
  index[0] = x;
  index[1] = y;
  index[2] = z;

  return GetVoxel(index);
}

template <class TPixel>
inline const TPixel&
ImageWrapper<TPixel>
::GetVoxel(const itk::Index<3> &index) const
{
  // Verify that the pixel is contained by the image at debug time
  assert(m_Image && m_Image->GetLargestPossibleRegion().IsInside(index));

  // Return the pixel
  return m_Image->GetPixel(index);
}

template <class TPixel> 
typename ImageWrapper<TPixel>::ConstIterator 
ImageWrapper<TPixel>
::GetImageConstIterator() const 
{
  ConstIterator it(m_Image,m_Image->GetLargestPossibleRegion());
  it.GoToBegin();
  return it;
}

template <class TPixel> 
typename ImageWrapper<TPixel>::Iterator 
ImageWrapper<TPixel>
::GetImageIterator() 
{
  Iterator it(m_Image,m_Image->GetLargestPossibleRegion());
  it.GoToBegin();
  return it;
}

template <class TPixel>    
void 
ImageWrapper<TPixel>
::SetSliceIndex(const Vector3ui &cursor)
{
  // Save the cursor position
  m_SliceIndex = cursor;

  // Select the appropriate slice for each slicer
  for(unsigned int i=0;i<3;i++)
  {
    // Which axis does this slicer slice?
    unsigned int axis = m_Slicer[i]->GetSliceDirectionImageAxis();

    // Set the slice using that axis
    m_Slicer[i]->SetSliceIndex(cursor[axis]);
  }
}


template <class TPixel>    
void 
ImageWrapper<TPixel>
::SetImageToDisplayTransform(unsigned int iSlice,
                             const ImageCoordinateTransform &transform)
{
  // Get the transform and its inverse
  m_ImageToDisplayTransform[iSlice] = transform;
  m_DisplayToImageTransform[iSlice] = transform.Inverse();

  // Tell slicer in which directions to slice
  m_Slicer[iSlice]->SetSliceDirectionImageAxis(
    m_DisplayToImageTransform[iSlice].GetCoordinateIndexZeroBased(2));
  
  m_Slicer[iSlice]->SetLineDirectionImageAxis(
    m_DisplayToImageTransform[iSlice].GetCoordinateIndexZeroBased(1));

  m_Slicer[iSlice]->SetPixelDirectionImageAxis(
    m_DisplayToImageTransform[iSlice].GetCoordinateIndexZeroBased(0));

  m_Slicer[iSlice]->SetPixelTraverseForward(
    m_DisplayToImageTransform[iSlice].GetCoordinateOrientation(0) > 0);

  m_Slicer[iSlice]->SetLineTraverseForward(
    m_DisplayToImageTransform[iSlice].GetCoordinateOrientation(1) > 0);
}


  /** For each slicer, find out which image dimension does is slice along */

template <class TPixel>    
unsigned int 
ImageWrapper<TPixel>
::GetDisplaySliceImageAxis(unsigned int iSlice)
{
  return m_Slicer[iSlice]->GetSliceDirectionImageAxis();
}

template <class TPixel>
typename ImageWrapper<TPixel>::SliceType*
ImageWrapper<TPixel>
::GetSlice(unsigned int dimension)
{
  return m_Slicer[dimension]->GetOutput();
}

template <class TPixel>
TPixel *
ImageWrapper<TPixel>
::GetVoxelPointer() const
{
  return m_Image->GetBufferPointer();
}



template <class TPixel>
unsigned int 
ImageWrapper<TPixel>
::ReplaceIntensity(TPixel iOld, TPixel iNew)
{
  // Counter for the number of replaced voxels
  unsigned int nReplaced = 0;

  // Replace the voxels
  for(Iterator it = GetImageIterator(); !it.IsAtEnd(); ++it)
    if(it.Value() == iOld)
      {
      it.Set(iNew);
      ++nReplaced;
      }

  // Flag that changes have been made
  if(nReplaced > 0)
    m_Image->Modified();

  // Return the number of replacements
  return nReplaced;
}

template <class TPixel>
unsigned int 
ImageWrapper<TPixel>
::SwapIntensities(TPixel iFirst, TPixel iSecond)
{
  // Counter for the number of replaced voxels
  unsigned int nReplaced = 0;

  // Replace the voxels
  for(Iterator it = GetImageIterator(); !it.IsAtEnd(); ++it)
    if(it.Value() == iFirst)
      {
      it.Set(iSecond);
      ++nReplaced;
      }
    else if(it.Value() == iSecond)
      {
      it.Set(iFirst);
      ++nReplaced;
      }

  // Flag that changes have been made
  if(nReplaced > 0)
    m_Image->Modified();

  // Return the number of replacements
  return nReplaced;
}

template <class TPixel>
void *
ImageWrapper<TPixel>
::GetVoxelVoidPointer() const
{
  return (void *) m_Image->GetBufferPointer();
}


/**
  Get the RGBA apperance of the voxel at the intersection of the three
  display slices.
  */
template <class TPixel>
void
ImageWrapper<TPixel>
::GetVoxelUnderCursorAppearance(DisplayPixelType &out)
{
  // Make sure the display slice is updated
  this->GetDisplaySlice(0)->Update();

  // Find the correct voxel in the space of the first display slice
  Vector3ui idxDisp =
      m_ImageToDisplayTransform[0].TransformVoxelIndex(m_SliceIndex);

  // Get the RGB value
  typename DisplaySliceType::IndexType idx2D = {{idxDisp[0], idxDisp[1]}};
  out = this->GetDisplaySlice(0)->GetPixel(idx2D);
}

// Allowed types of image wrappers
template class ImageWrapper<GreyType>;
template class ImageWrapper<float>;
template class ImageWrapper<LabelType>;
template class ImageWrapper<RGBType>;

template <class TPixel>
void
ImageWrapper<TPixel>
::AttachPreviewPipeline(
    PreviewFilterType *f0, PreviewFilterType *f1, PreviewFilterType *f2)
{
  PreviewFilterType *filter[] = {f0, f1, f2};
  for(int i = 0; i < 3; i++)
    {
    // Update the preview inputs to the slicers
    m_Slicer[i]->SetPreviewInput(filter[i]->GetOutput());

    // Mark the preview filters as modified to ensure that the slicer
    // is going to use it. TODO: is this really needed?
    filter[i]->Modified();
    }
}

template <class TPixel>
void
ImageWrapper<TPixel>
::DetachPreviewPipeline()
{
  for(int i = 0; i < 3; i++)
    {
    m_Slicer[i]->SetPreviewInput(NULL);
    }
}

template <class TPixel>
bool
ImageWrapper<TPixel>
::IsPreviewPipelineAttached() const
{
  return m_Slicer[0]->GetPreviewInput() != NULL;
}
