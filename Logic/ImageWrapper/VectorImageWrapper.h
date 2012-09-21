/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: VectorImageWrapper.h,v $
  Language:  C++
  Date:      $Date: 2009/01/23 20:09:38 $
  Version:   $Revision: 1.2 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __VectorImageWrapper_h_
#define __VectorImageWrapper_h_

// Smart pointers have to be included from ITK, can't forward reference them
#include "ImageWrapper.h"

/**
 * \class VectorImageWrapper
 * \brief A wrapper around an itk::Image and related pipelines.
 * 
 * VectorImage Wrapper serves as a wrapper around an image pointer, and 
 * is used to unify the treatment of different kinds of vector images in
 * SNaP.  
 */
template<class TPixel, class TBase = VectorImageWrapperBase>
class VectorImageWrapper : public ImageWrapper<TPixel, TBase>
{
public:

  // Standard ITK business
  typedef VectorImageWrapper<TPixel, TBase>                               Self;
  typedef ImageWrapper<TPixel, TBase>                                Superclass;
  typedef SmartPtr<Self>                                               Pointer;
  typedef SmartPtr<const Self>                                    ConstPointer;
  itkTypeMacro(VectorImageWrapper, ImageWrapper)

  // Image Types
  typedef typename Superclass::ImageType ImageType;
  typedef typename Superclass::ImagePointer ImagePointer;

  // Slice image type
  typedef typename Superclass::SliceType SliceType;
  typedef typename Superclass::SlicerPointer SlicePointer;

  // Slicer type
  typedef typename Superclass::SlicerType SlicerType;
  typedef typename Superclass::SlicerPointer SlicerPointer;

  // Iterator types
  typedef typename Superclass::Iterator Iterator;
  typedef typename Superclass::ConstIterator ConstIterator;

  virtual bool IsScalar() const { return false; }

  /**
   * This method is used to perform a deep copy of a region of this image 
   * into another image, potentially resampling the region to use a different
   * voxel size
   */
  ImagePointer DeepCopyRegion(const SNAPSegmentationROISettings &roi,
                              itk::Command *progressCommand = NULL) const;


  /** This image type has only one component */
  virtual size_t GetNumberOfComponents() const
  {
    return TPixel::Dimension;
  }

  /** Voxel access */
  virtual double GetVoxelAsDouble(const itk::Index<3> &idx) const;

  /** Voxel access */
  virtual double GetVoxelAsDouble(const Vector3ui &v) const;

  virtual void GetVoxelAsDouble(const Vector3ui &x, double *out) const
  {
    const TPixel &p = this->GetVoxel(x);
    for(unsigned int i = 0; i < this->GetNumberOfComponents(); i++)
      out[i] = p[i];
  }

  virtual void GetVoxelAsDouble(const itk::Index<3> &idx, double *out) const
  {
    const TPixel &p = this->GetVoxel(idx);
    for(unsigned int i = 0; i < this->GetNumberOfComponents(); i++)
      out[i] = p[i];
  }

protected:

  /**
   * Default constructor.  Creates an image wrapper with a blank internal
   * image
   */
  VectorImageWrapper() {}

  /**
   * Copy constructor.  Copies the contents of the passed-in image wrapper.
   */
  VectorImageWrapper(const Self &copy) : Superclass(copy) {}

  /** Destructor */
  virtual ~VectorImageWrapper() {}
};

#endif // __VectorImageWrapper_h_
