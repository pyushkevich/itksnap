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

#include "ImageWrapper.h"

// Forward references
namespace itk {
  template<class TIn> class MinimumMaximumImageFilter;
  template<class TIn, class TOut> class GradientMagnitudeImageFilter;
}


/**
 * \class ScalarImageWrapper
 * \brief A wrapper around an itk::Image and related pipelines.
 * 
 * ScalarImage Wrapper serves as a wrapper around an image pointer, and 
 * is used to unify the treatment of different kinds of scalar images in
 * SNaP.  
 */
template<class TImage, class TBase = ScalarImageWrapperBase>
class ScalarImageWrapper : public ImageWrapper<TImage, TBase>
{
public:

  // Standard ITK business
  typedef ScalarImageWrapper<TImage, TBase>                               Self;
  typedef ImageWrapper<TImage, TBase>                               Superclass;
  typedef SmartPtr<Self>                                               Pointer;
  typedef SmartPtr<const Self>                                    ConstPointer;
  itkTypeMacro(ScalarImageWrapper, ImageWrapper)

  // Image Types
  typedef typename Superclass::ImageType                             ImageType;
  typedef typename Superclass::ImagePointer                       ImagePointer;
  typedef typename Superclass::PixelType                             PixelType;

  // Slice image type
  typedef typename Superclass::SliceType                             SliceType;
  typedef typename Superclass::SlicePointer                       SlicePointer;

  // Slicer type
  typedef typename Superclass::SlicerType                           SlicerType;
  typedef typename Superclass::SlicerPointer                     SlicerPointer;

  // MinMax calculator type
  typedef itk::MinimumMaximumImageFilter<ImageType>               MinMaxFilter;

  // Iterator types
  typedef typename Superclass::Iterator                               Iterator;
  typedef typename Superclass::ConstIterator                     ConstIterator;

  virtual bool IsScalar() const { return true; }

  /**
   * Get the minimum intensity value.  Call ComputeImageIntensityRange() 
   * first.
   */
  virtual PixelType GetImageMin();

  /**
   * Get the maximum intensity value.  Call ComputeImageIntensityRange() 
   * first.
   */
  virtual PixelType GetImageMax();

  /**
   * Get the scaling factor used to convert between intensities stored
   * in this image and the 'true' image intensities
   */
  virtual double GetImageScaleFactor();

  /**
   * Remap the intensity range of the image to a given range
   */
  virtual void RemapIntensityToRange(double min, double max);

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

  typedef typename ImageWrapperBase::ShortImageType ShortImageType;

  /** This image type has only one component */
  virtual size_t GetNumberOfComponents() const { return 1; }

  /** Voxel access */
  virtual double GetVoxelAsDouble(const itk::Index<3> &idx) const
  {
    return (double) this->GetVoxel(idx);
  }

  /** Voxel access */
  virtual double GetVoxelAsDouble(const Vector3ui &v) const
  {
    return (double) this->GetVoxel(v);
  }

  virtual void GetVoxelAsDouble(const Vector3ui &x, double *out) const
  {
    out[0] = this->GetVoxel(x);
  }

  virtual void GetVoxelAsDouble(const itk::Index<3> &idx, double *out) const
  {
    out[0] = this->GetVoxel(idx);
  }

  /**
    Get the maximum intensity
    */
  virtual double GetImageMaxAsDouble()
  {
    return (double) this->GetImageMax();
  }

  /**
    Get the minimum intensity
    */
  virtual double GetImageMinAsDouble()
  {
    return (double) this->GetImageMin();
  }

  /**
    Get the maximum possible value of the gradient magnitude. This will
    compute the gradient magnitude of the image (without Gaussian smoothing)
    and return the maximum. The value will be cached so repeated calls to
    this are not expensive.
    */
  double GetImageGradientMagnitudeUpperLimit();

  /**
    Get the maximum possible value of the gradient magnitude in native units
    */
  double GetImageGradientMagnitudeUpperLimitNative();

  /**
   * Get voxel intensity in native space
   */
  double GetVoxelMappedToNative(const Vector3ui &vec) const
    { return m_NativeMapping(this->GetVoxel(vec)); }

  /**
   * Get min/max voxel intensity in native space
   */
  double GetImageMinNative()
    { return m_NativeMapping(this->GetImageMin()); }
  double GetImageMaxNative()
    { return m_NativeMapping(this->GetImageMax()); }

  /**
    There may be a linear mapping between internal values stored in the
    image and the values stored in the native image format.
    */
  irisGetMacro(NativeMapping, InternalToNativeFunctor)
  irisSetMacro(NativeMapping, InternalToNativeFunctor)


  /**
    Compute the image histogram. The histogram is cached inside of the
    object, so repeated calls to this function with the same nBins parameter
    will not require additional computation.

    Calling with default parameter (0) will use the same number of bins that
    is currently in the histogram (i.e., return/recompute current histogram).
    If there is no current histogram, a default histogram with 128 entries
    will be generated.
    */
  const ScalarImageHistogram *GetHistogram(size_t nBins = 0);

protected:

  /**
   * Default constructor.  Creates an image wrapper with a blank internal
   * image
   */
  ScalarImageWrapper();

  /**
   * Copy constructor.  Copies the contents of the passed-in image wrapper.
   */
  ScalarImageWrapper(const Self &copy);

  /** Destructor */
  virtual ~ScalarImageWrapper();

  /** 
   * The min-max filter used to compute the range of the image on demand.
   */
  SmartPtr<MinMaxFilter> m_MinMaxFilter;

  /**
    A mini-pipeline to compute the maximum value of the gradient of
    the input image on demand.
    */
  typedef itk::Image<float ,3> FloatImageType;
  typedef itk::GradientMagnitudeImageFilter<ImageType, FloatImageType> GradMagFilter;
  typedef itk::MinimumMaximumImageFilter<FloatImageType> GradMagMaxFilter;

  SmartPtr<GradMagFilter> m_GradientMagnitudeFilter;
  SmartPtr<GradMagMaxFilter> m_GradientMagnitudeMaximumFilter;

  // Mapping from native to internal format (get rid of in the future?)
  InternalToNativeFunctor m_NativeMapping;

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

  // The histogram for this scalar wrapper. It is computed only when asked for
  SmartPtr<ScalarImageHistogram> m_Histogram;
};

#endif // __ScalarImageWrapper_h_
