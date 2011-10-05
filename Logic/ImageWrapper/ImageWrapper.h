/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ImageWrapper.h,v $
  Language:  C++
  Date:      $Date: 2009/11/13 00:59:47 $
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
#ifndef __ImageWrapper_h_
#define __ImageWrapper_h_

// Smart pointers have to be included from ITK, can't forward reference them
#include "ImageWrapperBase.h"
#include "ImageCoordinateTransform.h"
#include <itkImageRegionIterator.h>
#include <itkOrientedImage.h>
#include <itkRGBAPixel.h>

// Forward declarations to IRIS classes
template <class TPixel> class IRISSlicer;
class SNAPSegmentationROISettings;
namespace itk {
  template <unsigned int VDimension> class ImageBase;
}





/**
 * \class ImageWrapper
 * \brief A wrapper around an itk::OrientedImage and related pipelines.
 * 
 * Image Wrapper serves as a wrapper around an image pointer, and 
 * is used to unify the treatment of different kinds of images in
 * SNaP.  
 */
template<class TPixel> class ImageWrapper
    : public virtual ImageWrapperBase
{
public:

  // Basic type definitions
  typedef itk::OrientedImage<TPixel,3> ImageType;
  typedef SmartPtr<ImageType> ImagePointer;

  // Slice image type
  typedef itk::Image<TPixel,2> SliceType;
  typedef SmartPtr<SliceType> SlicePointer;

  // Slicer type
  typedef IRISSlicer<TPixel> SlicerType;
  typedef SmartPtr<SlicerType> SlicerPointer;

  // Iterator types
  typedef typename itk::ImageRegionIterator<ImageType> Iterator;
  typedef typename itk::ImageRegionConstIterator<ImageType> ConstIterator;

  /** 
   * Default constructor.  Creates an image wrapper with a blank internal 
   * image 
   */
  ImageWrapper();

  /** 
   * Copy constructor.  Copies the contents of the passed-in image wrapper. 
   */
  ImageWrapper(const ImageWrapper<TPixel> &copy);
  
  /** Destructor */
  virtual ~ImageWrapper();

  /**
   * Initialize the image wrapper to match another image wrapper, setting the
   * image pixels to a default value. The slice indices and the transforms will
   * all be updated to match the source
   */
  virtual void InitializeToWrapper(
    const ImageWrapperBase *source, const TPixel &value);

  /** 
   * Initialize the image wrapper with a combination of another image wrapper and
   * an actual image. This will update the indices and transforms from the 
   * source wrapper, otherwise, it's equivalent to SetImage()
   */ 
  virtual void InitializeToWrapper(
    const ImageWrapperBase *source, ImageType *image);

  /**
   * Initialize the internal image to a blank image of size x,y,z.
   */

  /**
   * Clear the data associated with storing an image
   */
  virtual void Reset();

  /** Get the coordinate transform for each display slice */
  virtual const ImageCoordinateTransform &GetImageToDisplayTransform(
    unsigned int) const;

  /**
   * Use a default image-slice transformation, the first slice is along z,
   * the second along y, the third, along x, all directions of traversal are
   * positive.
   */
  virtual void SetImageToDisplayTransformsToDefault();

  /** Get the current slice index */
  irisGetMacro(SliceIndex, Vector3ui)

  /** Return some image info independently of pixel type */
  ImageBaseType* GetImageBase() const { return m_Image; }

  /**
   * Is the image initialized?
   */
  irisIsMacro(Initialized)

  /**
   * Get the size of the image
   */
  Vector3ui GetSize() const;

  /** Get layer transparency */
  irisSetMacro(Alpha, unsigned char)

  /** Set layer transparency */
  irisGetMacro(Alpha, unsigned char)

  /** Switch on/off visibility */
  virtual void ToggleVisibility();

  /** Get the buffered region of the image */
  virtual itk::ImageRegion<3> GetBufferedRegion() const;

  /** Transform a voxel index into a spatial position */
  virtual Vector3d TransformVoxelIndexToPosition(const Vector3ui &iVoxel) const;

  /** Transform a voxel index into NIFTI coordinates (RAS) */
  virtual Vector3d TransformVoxelIndexToNIFTICoordinates(const Vector3d &iVoxel) const;

  /** Transform NIFTI coordinates to a continuous voxel index */
  virtual Vector3d TransformNIFTICoordinatesToVoxelIndex(const Vector3d &vNifti) const;

  /** Get the NIFTI s-form matrix for this image */
  irisGetMacro(NiftiSform, TransformType)

  /**
   * Get reference to a voxel at a given position.
   */
  TPixel &GetVoxelForUpdate(unsigned int x,
                                    unsigned int y, 
                                    unsigned int z);
  
  TPixel &GetVoxelForUpdate(const Vector3ui &index);
  
  TPixel &GetVoxelForUpdate(const itk::Index<3> &index);

  /**
   * Get a constant reference to a voxel at a given position.
   */
  const TPixel &GetVoxel(unsigned int x,
                                 unsigned int y, 
                                 unsigned int z) const;

  const TPixel &GetVoxel(const Vector3ui &index) const;

  const TPixel &GetVoxel(const itk::Index<3> &index) const;

  /** 
   * Return the pointed to the ITK image encapsulated by this wrapper.
   */
  virtual ImageType *GetImage() const
    { return m_Image; }

  /** 
   * Get the slicer inside this wrapper
   */
  virtual SlicerType *GetSlicer(unsigned int iDirection) const
    { return m_Slicer[iDirection]; }

  /**
   * Set the current slice index in all three dimensions.  The index should
   * be specified in the image coordinates, the slices will be generated
   * in accordance with the transforms that are specified
   */
  virtual void SetSliceIndex(const Vector3ui &cursor);

  /**
   * Set the trasforms from image space to one of the three display slices (be
   * sure to set all three, or you'll get weird looking slices!
   */
  virtual void SetImageToDisplayTransform(
    unsigned int iSlice,const ImageCoordinateTransform &transform);


  /**
   * Get a slice of the image in a given direction
   */
  virtual SliceType *GetSlice(unsigned int dimension);

  /**
   * This method exposes the scalar pointer in the image
   */
  virtual TPixel *GetVoxelPointer() const;

  /** Number of voxels */
  virtual size_t GetNumberOfVoxels() const;

  /**
   * Pring debugging info
   * TODO: Delete this or make is worthwhile
   */
  virtual void PrintDebugInformation();
                       
  /**
   * Replace the image internally stored in this wrapper by another image.
   */
  virtual void SetImage(ImagePointer newImage);

  /**
   * This method is used to perform a deep copy of a region of this image 
   * into another image, potentially resampling the region to use a different
   * voxel size
   */
  virtual ImagePointer DeepCopyRegion(const SNAPSegmentationROISettings &roi,
                              itk::Command *progressCommand = NULL) const = 0;


  /**
   * Get an iterator for traversing the image.  The iterator is initialized
   * to point to the beginning of the image
   */
  virtual ConstIterator GetImageConstIterator() const;
  virtual Iterator GetImageIterator();

  /** For each slicer, find out which image dimension does is slice along */
  unsigned int GetDisplaySliceImageAxis(unsigned int slice);

  /** 
   * Replace all voxels with intensity values iOld with values iNew. 
   * \return number of voxels that had been modified
   */
  virtual unsigned int ReplaceIntensity(TPixel iOld, TPixel iNew);

  /** 
   * Swap intensity values iFirst and iSecond
   * \return number of voxels that had been modified
   */
  virtual unsigned int SwapIntensities(TPixel iFirst, TPixel iSecond);


  virtual void* GetVoxelVoidPointer() const;

  // Access the filename
  irisSetStringMacro(FileName)
  irisGetStringMacro(FileName)

  // Access the nickname
  irisSetStringMacro(Nickname)
  irisGetStringMacro(Nickname)



protected:

  /** The image that we are wrapping */
  ImagePointer m_Image;

  /** The associated slicer filters */
  SlicerPointer m_Slicer[3];

  /** The wrapped image */
  SmartPtr<ImageBaseType> m_ImageBase;

  /** The current cursor position (slice index) in image dimensions */
  Vector3ui m_SliceIndex;

  /**
   * Is the image wrapper initialized? That is a prerequisite for all
   * operations.
   */
  bool m_Initialized;

  /** Transparency */
  unsigned char m_Alpha;

  /** A 'saved' value of alpha for when visibility is toggled */
  unsigned char m_ToggleAlpha;

  /** Transform from image space to display space */
  ImageCoordinateTransform m_ImageToDisplayTransform[3];

  /** Transform from image space to display space */
  ImageCoordinateTransform m_DisplayToImageTransform[3];

  // Transform from image index to NIFTI world coordinates
  TransformType m_NiftiSform, m_NiftiInvSform;

  // Each layer has a filename and a nickname
  std::string m_FileName, m_Nickname;

  /**
   * Handle a change in the image pointer (i.e., a load operation on the image or 
   * an initialization operation)
   */
  virtual void UpdateImagePointer(ImageType *);

  /** Common code for the different constructors */
  void CommonInitialization();
};

#endif // __ImageWrapper_h_
