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
#include "ImageCoordinateGeometry.h"

#include <vnl/vnl_inverse.h>
#include <iostream>

#include <itksys/SystemTools.hxx>

unsigned long GlobalImageWrapperIndex = 0;

template<class TPixel, class TBase>
ImageWrapper<TPixel,TBase>
::ImageWrapper() 
{
  CommonInitialization();
}

template<class TPixel, class TBase>
ImageWrapper<TPixel,TBase>
::~ImageWrapper() 
{
  Reset();
}

template<class TPixel, class TBase>
void 
ImageWrapper<TPixel,TBase>
::CommonInitialization()
{
  // Set the unique wrapper id
  m_UniqueId = ++GlobalImageWrapperIndex;

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

template<class TPixel, class TBase>
ImageWrapper<TPixel,TBase>
::ImageWrapper(const Self &copy)
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

template<class TPixel, class TBase>
const ImageCoordinateTransform &
ImageWrapper<TPixel,TBase>
::GetImageToDisplayTransform(unsigned int iSlice) const
{
  return m_ImageToDisplayTransform[iSlice];
}

template<class TPixel, class TBase>
void
ImageWrapper<TPixel,TBase>
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

template<class TPixel, class TBase>
Vector3ui
ImageWrapper<TPixel,TBase>
::GetSize() const
{
  // Cast the size to our vector format
  itk::Size<3> size = m_Image->GetLargestPossibleRegion().GetSize();
  return Vector3ui(
    (unsigned int) size[0],
    (unsigned int) size[1],
        (unsigned int) size[2]);
}

template<class TPixel, class TBase>
void
ImageWrapper<TPixel,TBase>
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

template<class TPixel, class TBase>
itk::ImageRegion<3>
ImageWrapper<TPixel,TBase>
::GetBufferedRegion() const
{
  return m_ImageBase->GetBufferedRegion();
}

template<class TPixel, class TBase>
size_t
ImageWrapper<TPixel,TBase>
::GetNumberOfVoxels() const
{
  return m_ImageBase->GetBufferedRegion().GetNumberOfPixels();
}

template<class TPixel, class TBase>
Vector3d
ImageWrapper<TPixel,TBase>
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

template<class TPixel, class TBase>
Vector3d
ImageWrapper<TPixel,TBase>
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

template<class TPixel, class TBase>
Vector3d
ImageWrapper<TPixel,TBase>
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


template<class TPixel, class TBase>
void 
ImageWrapper<TPixel,TBase>
::PrintDebugInformation() 
{
  std::cout << "=== Image Properties ===" << std::endl;
  std::cout << "   Dimensions         : " << m_Image->GetLargestPossibleRegion().GetSize() << std::endl;
  std::cout << "   Origin             : " << m_Image->GetOrigin() << std::endl;
  std::cout << "   Spacing            : " << m_Image->GetSpacing() << std::endl;
  std::cout << "------------------------" << std::endl;
}

template<class TPixel, class TBase>
void 
ImageWrapper<TPixel,TBase>
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
  m_NiftiSform = ImageWrapperBase::ConstructNiftiSform(
    m_Image->GetDirection().GetVnlMatrix(),
    m_Image->GetOrigin().GetVnlVector(),
    m_Image->GetSpacing().GetVnlVector());
  m_NiftiInvSform = vnl_inverse(m_NiftiSform);

  // We have been initialized
  m_Initialized = true;
}

template<class TPixel, class TBase>
void 
ImageWrapper<TPixel,TBase>
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

template<class TPixel, class TBase>
void 
ImageWrapper<TPixel,TBase>
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

template<class TPixel, class TBase>
void 
ImageWrapper<TPixel,TBase>
::SetImage(ImagePointer newImage) 
{
  UpdateImagePointer(newImage);
}


template<class TPixel, class TBase>
void 
ImageWrapper<TPixel,TBase>
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


template<class TPixel, class TBase>
inline const TPixel& 
ImageWrapper<TPixel,TBase>
::GetVoxel(const Vector3ui &index) const 
{
  return GetVoxel(index[0],index[1],index[2]);
}

template<class TPixel, class TBase>
inline TPixel& 
ImageWrapper<TPixel,TBase>
::GetVoxelForUpdate(const Vector3ui &index) 
{
  return GetVoxelForUpdate(index[0],index[1],index[2]);
}

template<class TPixel, class TBase>
inline TPixel& 
ImageWrapper<TPixel,TBase>
::GetVoxelForUpdate(unsigned int x, unsigned int y, unsigned int z) 
{
  itk::Index<3> index;
  index[0] = x;
  index[1] = y;
  index[2] = z;

  return GetVoxelForUpdate(index);
}

template<class TPixel, class TBase>
inline TPixel &
ImageWrapper<TPixel,TBase>
::GetVoxelForUpdate(const itk::Index<3> &index)
{
  // Verify that the pixel is contained by the image at debug time
  assert(m_Image && m_Image->GetLargestPossibleRegion().IsInside(index));

  // Return the pixel
  return m_Image->GetPixel(index);
}

template<class TPixel, class TBase>
inline const TPixel& 
ImageWrapper<TPixel,TBase>
::GetVoxel(unsigned int x, unsigned int y, unsigned int z) const
{
  itk::Index<3> index;
  index[0] = x;
  index[1] = y;
  index[2] = z;

  return GetVoxel(index);
}

template<class TPixel, class TBase>
inline const TPixel&
ImageWrapper<TPixel,TBase>
::GetVoxel(const itk::Index<3> &index) const
{
  // Verify that the pixel is contained by the image at debug time
  assert(m_Image && m_Image->GetLargestPossibleRegion().IsInside(index));

  // Return the pixel
  return m_Image->GetPixel(index);
}

template<class TPixel, class TBase>
typename ImageWrapper<TPixel,TBase>::ConstIterator
ImageWrapper<TPixel,TBase>
::GetImageConstIterator() const 
{
  ConstIterator it(m_Image,m_Image->GetLargestPossibleRegion());
  it.GoToBegin();
  return it;
}

template<class TPixel, class TBase>
typename ImageWrapper<TPixel,TBase>::Iterator
ImageWrapper<TPixel,TBase>
::GetImageIterator() 
{
  Iterator it(m_Image,m_Image->GetLargestPossibleRegion());
  it.GoToBegin();
  return it;
}

template<class TPixel, class TBase>
void 
ImageWrapper<TPixel,TBase>
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

template<class TPixel, class TBase>
void
ImageWrapper<TPixel,TBase>
::SetImageGeometry(const ImageCoordinateGeometry &geom)
{
  // Set the direction matrix in image
  itk::Matrix<double,3,3> dm(geom.GetImageDirectionCosineMatrix());
  this->GetImageBase()->SetDirection(dm);

  // Update the geometry for each slice
  for(unsigned int iSlice = 0;iSlice < 3;iSlice ++)
    {
    // Set the display transform (modifies the slicer axes)
    this->SetImageToDisplayTransform(
          iSlice, geom.GetImageToDisplayTransform(iSlice));

    // Invalidate the requested region in the display slice. This will
    // cause the RR to reset to largest possible region on next Update
    typename DisplaySliceType::RegionType invalidRegion;
    this->GetDisplaySlice(iSlice)->SetRequestedRegion(invalidRegion);
    }
}



template<class TPixel, class TBase>
void 
ImageWrapper<TPixel,TBase>
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

template<class TPixel, class TBase>
unsigned int 
ImageWrapper<TPixel,TBase>
::GetDisplaySliceImageAxis(unsigned int iSlice)
{
  return m_Slicer[iSlice]->GetSliceDirectionImageAxis();
}

template<class TPixel, class TBase>
typename ImageWrapper<TPixel,TBase>::SliceType*
ImageWrapper<TPixel,TBase>
::GetSlice(unsigned int dimension)
{
  return m_Slicer[dimension]->GetOutput();
}

template<class TPixel, class TBase>
TPixel *
ImageWrapper<TPixel,TBase>
::GetVoxelPointer() const
{
  return m_Image->GetBufferPointer();
}



template<class TPixel, class TBase>
unsigned int 
ImageWrapper<TPixel,TBase>
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

template<class TPixel, class TBase>
unsigned int 
ImageWrapper<TPixel,TBase>
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

template<class TPixel, class TBase>
void *
ImageWrapper<TPixel,TBase>
::GetVoxelVoidPointer() const
{
  return (void *) m_Image->GetBufferPointer();
}


/**
  Get the RGBA apperance of the voxel at the intersection of the three
  display slices.
  */
template<class TPixel, class TBase>
void
ImageWrapper<TPixel,TBase>
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

template<class TPixel, class TBase>
void
ImageWrapper<TPixel,TBase>
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

template<class TPixel, class TBase>
void
ImageWrapper<TPixel,TBase>
::DetachPreviewPipeline()
{
  for(int i = 0; i < 3; i++)
    {
    m_Slicer[i]->SetPreviewInput(NULL);
    }
}

template<class TPixel, class TBase>
bool
ImageWrapper<TPixel,TBase>
::IsPreviewPipelineAttached() const
{
  return m_Slicer[0]->GetPreviewInput() != NULL;
}

#include <itkImageFileWriter.h>
#include <itkResampleImageFilter.h>
#include <itkIdentityTransform.h>
#include <itkFlipImageFilter.h>
#include <itkUnaryFunctorImageFilter.h>

struct RemoveTransparencyFunctor
{
  typedef ImageWrapperBase::DisplayPixelType PixelType;
  PixelType operator()(const PixelType &p)
  {
    PixelType pnew = p;
    pnew[3] = 255;
    return pnew;
  }
};

template<class TPixel, class TBase>
void
ImageWrapper<TPixel,TBase>
::WriteThumbnail(const char *file, unsigned int maxdim)
{
  // Get the display slice
  // For now, just use the z-axis for exporting the thumbnails
  DisplaySliceType *slice = this->GetDisplaySlice(2);

  // The size of the slice
  Vector2ui slice_dim = slice->GetBufferedRegion().GetSize();

  // The physical extents of the slice
  Vector2d slice_extent(slice->GetSpacing()[0] * slice_dim[0],
                        slice->GetSpacing()[1] * slice_dim[1]);

  // The output thumbnail will have the extents as the slice, but its size
  // must be at max maxdim
  double slice_extent_max = slice_extent.max_value();

  // Create a simple square thumbnail
  Vector2ui thumb_size(maxdim, maxdim);

  // Spacing is such that the slice extent fits into the thumbnail
  Vector2d thumb_spacing(slice_extent_max / maxdim,
                         slice_extent_max / maxdim);

  // The origin of the thumbnail is such that the centers coincide
  Vector2d thumb_origin(0.5 * (slice_extent[0] - slice_extent_max),
                        0.5 * (slice_extent[1] - slice_extent_max));

  typedef typename itk::IdentityTransform<double, 2> TransformType;
  TransformType::Pointer transform = TransformType::New();

  typedef typename itk::ResampleImageFilter<
      DisplaySliceType, DisplaySliceType> ResampleFilter;

  // Background color for thumbnails
  unsigned char defrgb[] = {0,0,0,255};

  SmartPtr<ResampleFilter> filter = ResampleFilter::New();
  filter->SetInput(slice);
  filter->SetTransform(transform);
  filter->SetSize(to_itkSize(thumb_size));
  filter->SetOutputSpacing(thumb_spacing.data_block());
  filter->SetOutputOrigin(thumb_origin.data_block());
  filter->SetDefaultPixelValue(DisplayPixelType(defrgb));

  // For thumbnails, the image needs to be flipped
  typedef itk::FlipImageFilter<DisplaySliceType> FlipFilter;
  SmartPtr<FlipFilter> flipper = FlipFilter::New();
  flipper->SetInput(filter->GetOutput());
  typename FlipFilter::FlipAxesArrayType flipaxes;
  flipaxes[0] = false; flipaxes[1] = true;
  flipper->SetFlipAxes(flipaxes);

  // We also need to replace the transparency
  typedef itk::UnaryFunctorImageFilter<
      DisplaySliceType, DisplaySliceType, RemoveTransparencyFunctor> OpaqueFilter;
  SmartPtr<OpaqueFilter> opaquer = OpaqueFilter::New();
  opaquer->SetInput(flipper->GetOutput());

  // Write a PNG file
  typedef typename itk::ImageFileWriter<DisplaySliceType> WriterType;
  SmartPtr<WriterType> writer = WriterType::New();
  writer->SetInput(opaquer->GetOutput());
  writer->SetFileName(file);
  writer->Update();
}

// Allowed types of image wrappers
template class ImageWrapper<GreyType, GreyImageWrapperBase>;
template class ImageWrapper<float, ScalarImageWrapperBase>;
template class ImageWrapper<LabelType, ScalarImageWrapperBase>;
template class ImageWrapper<RGBType, RGBImageWrapperBase>;

