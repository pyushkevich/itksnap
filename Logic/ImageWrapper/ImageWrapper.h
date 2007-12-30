/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ImageWrapper.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:14 $
  Version:   $Revision: 1.4 $
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
#include "SNAPCommon.h"
#include "ImageCoordinateTransform.h"
#include <itkImageRegionIterator.h>

// Forward declarations to IRIS classes
template <class TPixel> class IRISSlicer;
class SNAPSegmentationROISettings;
namespace itk {
  template <unsigned int VDimension> class ImageBase;
}

/**
 * \class ImageWrapper
 * \brief Abstract parent class for all image wrappers
 * \see ImageWrapper
 */
class ImageWrapperBase
{
public:
  virtual ~ImageWrapperBase() { /*To avoid compiler warning.*/ }
  virtual const ImageCoordinateTransform &GetImageToDisplayTransform(
    unsigned int) const = 0;
  virtual void SetImageToDisplayTransform(
    unsigned int, const ImageCoordinateTransform &) = 0;
  virtual Vector3ui GetSliceIndex() const = 0;
  virtual void SetSliceIndex(const Vector3ui &) = 0;
  virtual itk::ImageBase<3> *GetImageBase() const = 0;
  virtual bool IsInitialized() const = 0;
};

/**
 * \class ImageWrapper
 * \brief A wrapper around an itk::Image and related pipelines.
 * 
 * Image Wrapper serves as a wrapper around an image pointer, and 
 * is used to unify the treatment of different kinds of images in
 * SNaP.  
 */
template<class TPixel> class ImageWrapper : public ImageWrapperBase
{
public:

  // Basic type definitions
  typedef itk::Image<TPixel,3> ImageType;
  typedef typename itk::SmartPointer<ImageType> ImagePointer;

  // Slice image type
  typedef itk::Image<TPixel,2> SliceType;
  typedef typename itk::SmartPointer<SliceType> SlicePointer;

  // Slicer type
  typedef IRISSlicer<TPixel> SlicerType;
  typedef typename itk::SmartPointer<SlicerType> SlicerPointer;

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
  // virtual void InitializeToSize(unsigned int x, unsigned int y,unsigned int z);

  /**
   * Clear the data associated with storing an image
   */
  virtual void Reset();

  /**
   * Is the image initialized?
   */
  virtual bool IsInitialized() const;

  /**
   * Get reference to a voxel at a given position.
   */
  virtual TPixel &GetVoxelForUpdate(unsigned int x, 
                                    unsigned int y, 
                                    unsigned int z);
  
  virtual TPixel &GetVoxelForUpdate(const Vector3ui &index);
  
  /**
   * Get a constant reference to a voxel at a given position.
   */
  virtual const TPixel &GetVoxel(unsigned int x, 
                                 unsigned int y, 
                                 unsigned int z) const;

  virtual const TPixel &GetVoxel(const Vector3ui &index) const;

  /** Return some image info independently of pixel type */
  virtual itk::ImageBase<3> *GetImageBase() const
    { return m_Image.GetPointer(); }

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
   * Get the size of the image
   */
  virtual Vector3ui GetSize() const;

  /** Get the current slice index */
  virtual Vector3ui GetSliceIndex() const;

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

  /** Get the coordinate transform for each display slice */
  virtual const ImageCoordinateTransform &GetImageToDisplayTransform(
    unsigned int iSlice) const;

  /**
   * Use a default image-slice transformation, the first slice is along z,
   * the second along y, the third, along x, all directions of traversal are 
   * positive.
   */
  virtual void SetImageToDisplayTransformsToDefault();

  /**
   * Get a slice of the image in a given direction
   */
  virtual SliceType *GetSlice(unsigned int dimension);

  /**
   * This method exposes the scalar pointer in the image
   */
  virtual TPixel *GetVoxelPointer() const;

  /**
   * Get the number of voxels
   */
  size_t GetNumberOfVoxels() const;
    
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

  /** Transform a voxel index into a spatial position */
  Vector3d TransformVoxelIndexToPosition(const Vector3ui &iVoxel) const;

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

protected:

  /** The image that we are wrapping */
  ImagePointer m_Image;

  /** The associated slicer filters */
  SlicerPointer m_Slicer[3];

  /** The current cursor position (slice index) in image dimensions */
  Vector3ui m_SliceIndex;

  /**
   * Is the image wrapper initialized? That is a prerequisite for all 
   * operations.
   */
  bool m_Initialized;

  /** Transform from image space to display space */
  ImageCoordinateTransform m_ImageToDisplayTransform[3];

  /** Transform from image space to display space */
  ImageCoordinateTransform m_DisplayToImageTransform[3];

  /**
   * Handle a change in the image pointer (i.e., a load operation on the image or 
   * an initialization operation)
   */
  virtual void UpdateImagePointer(ImageType *);

  /** Common code for the different constructors */
  void CommonInitialization();
};

#endif // __ImageWrapper_h_
