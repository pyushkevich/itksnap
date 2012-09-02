/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ScalarImageWrapper.h,v $
  Language:  C++
  Date:      $Date: 2009/01/23 20:09:38 $
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
#ifndef __ScalarImageWrapper_h_
#define __ScalarImageWrapper_h_

// Smart pointers have to be included from ITK, can't forward reference them
#include "ImageWrapper.h"
#include <itkMinimumMaximumImageCalculator.h>

/**
 * \class ScalarImageWrapper
 * \brief A wrapper around an itk::Image and related pipelines.
 * 
 * ScalarImage Wrapper serves as a wrapper around an image pointer, and 
 * is used to unify the treatment of different kinds of scalar images in
 * SNaP.  
 */
template<class TPixel> class ScalarImageWrapper : public ImageWrapper<TPixel>
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

  // MinMax calculator type
  typedef itk::MinimumMaximumImageCalculator<ImageType> MinMaxCalculatorType;
  typedef typename itk::SmartPointer<MinMaxCalculatorType> 
    MinMaxCalculatorPointer;

  // Iterator types
  typedef typename itk::ImageRegionIterator<ImageType> Iterator;
  typedef typename itk::ImageRegionConstIterator<ImageType> ConstIterator;

  /** 
   * Default constructor.  Creates an image wrapper with a blank internal 
   * image 
   */
  ScalarImageWrapper() {};

  /** 
   * Copy constructor.  Copies the contents of the passed-in image wrapper. 
   */
  ScalarImageWrapper(const ScalarImageWrapper<TPixel> &copy);
  
  /** Destructor */
  virtual ~ScalarImageWrapper() {};

  /**
   * Get the minimum intensity value.  Call ComputeImageIntensityRange() 
   * first.
   */
  virtual TPixel GetImageMin();

  /**
   * Get the maximum intensity value.  Call ComputeImageIntensityRange() 
   * first.
   */
  virtual TPixel GetImageMax();

  /**
   * Get the scaling factor used to convert between intensities stored
   * in this image and the 'true' image intensities
   */
  virtual double GetImageScaleFactor();

  /**
   * Remap the intensity range of the image to a given range
   */
  virtual void RemapIntensityToRange(TPixel min, TPixel max);

  /**
   * Remap the intensity range to max possible range
   */
  virtual void RemapIntensityToMaximumRange();

  /**
   * This method is used to perform a deep copy of a region of this image 
   * into another image, potentially resampling the region to use a different
   * voxel size
   */
  ImagePointer DeepCopyRegion(const SNAPSegmentationROISettings &roi,
                              itk::Command *progressCommand = NULL) const;

protected:

  /** 
   * The min-max calculator used to hold min/max values.  This should be
   * replaced by a filter, when the latter is more efficient
   */
  MinMaxCalculatorPointer m_MinMaxCalc;

  /** The intensity scaling factor */
  double m_ImageScaleFactor;
  
  /**
   * Compute the intensity range of the image if it's out of date.  
   * This is done before calling GetImateMin, GetImateMax and GetImageScaleFactor methods.
   */
  void CheckImageIntensityRange();

  /**
   * Handle a change in the image pointer (i.e., a load operation on the image or 
   * an initialization operation)
   */
  virtual void UpdateImagePointer(ImageType *);

};

#endif // __ScalarImageWrapper_h_
