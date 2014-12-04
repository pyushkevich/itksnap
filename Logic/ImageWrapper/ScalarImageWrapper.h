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
#include "vtkSmartPointer.h"

// Forward references
template<class TIn> class ThreadedHistogramImageFilter;
namespace itk {
  template<class TIn> class MinimumMaximumImageFilter;
  template<class TInputImage> class VTKImageExport;
  template<class TOut> class ImageSource;
}

class vtkImageImport;


/**
 * \class ScalarImageWrapper
 * \brief A wrapper around an itk::Image and related pipelines.
 * 
 * ScalarImage Wrapper serves as a wrapper around an image pointer, and 
 * is used to unify the treatment of different kinds of scalar images in
 * SNaP.  
 */
template<class TTraits, class TBase = ScalarImageWrapperBase>
class ScalarImageWrapper : public ImageWrapper<TTraits, TBase>
{
public:

  // Standard ITK business
  typedef ScalarImageWrapper<TTraits, TBase>                              Self;
  typedef ImageWrapper<TTraits, TBase>                              Superclass;
  typedef SmartPtr<Self>                                               Pointer;
  typedef SmartPtr<const Self>                                    ConstPointer;
  itkTypeMacro(ScalarImageWrapper, ImageWrapper)
  itkNewMacro(Self)


  // Image Types
  typedef typename Superclass::ImageBaseType                     ImageBaseType;
  typedef typename Superclass::ImageType                             ImageType;
  typedef typename Superclass::ImagePointer                       ImagePointer;
  typedef typename Superclass::PixelType                             PixelType;
  typedef typename Superclass::CommonFormatImageType     CommonFormatImageType;

  // Floating point image type
  typedef itk::Image<float, 3>                                  FloatImageType;
  typedef itk::ImageSource<FloatImageType>                    FloatImageSource;

  // Slice image type
  typedef typename Superclass::SliceType                             SliceType;
  typedef typename Superclass::SlicePointer                       SlicePointer;

  // Slicer type
  typedef typename Superclass::SlicerType                           SlicerType;
  typedef typename Superclass::SlicerPointer                     SlicerPointer;

  // Display types
  typedef typename Superclass::DisplaySliceType               DisplaySliceType;
  typedef typename Superclass::DisplayPixelType               DisplayPixelType;

  // MinMax calculator type
  typedef itk::MinimumMaximumImageFilter<ImageType>               MinMaxFilter;

  // Histogram filter
  typedef ThreadedHistogramImageFilter<ImageType>          HistogramFilterType;

  // Iterator types
  typedef typename Superclass::Iterator                               Iterator;
  typedef typename Superclass::ConstIterator                     ConstIterator;

  // Output channels
  typedef typename Superclass::ExportChannel                     ExportChannel;

  // Pipeline objects wrapped around values
  typedef typename Superclass::ComponentTypeObject         ComponentTypeObject;

  typedef typename Superclass::NativeIntensityMapping   NativeIntensityMapping;

  typedef typename Superclass::ITKTransformType               ITKTransformType;


  virtual bool IsScalar() const { return true; }

  /**
   * This function just returns the pointer to itself, as the scalar representation
   * of a scalar image wrapper is itself.
   * @see ImageWrapperBase::GetScalarRepresentation
   */
  ScalarImageWrapperBase *GetDefaultScalarRepresentation() { return this; }

  /** Access the min/max filter */
  irisGetMacro(MinMaxFilter, MinMaxFilter *)

  /**
   * Get the scaling factor used to convert between intensities stored
   * in this image and the 'true' image intensities
   */
  virtual double GetImageScaleFactor();

  typedef typename ImageWrapperBase::ShortImageType ShortImageType;

  /** This image type has only one component */
  virtual size_t GetNumberOfComponents() const { return 1; }

  /** Voxel access */
  virtual double GetVoxelAsDouble(const itk::Index<3> &idx) const
    { return (double) Superclass::GetVoxel(idx); }

  virtual double GetVoxelAsDouble(const Vector3ui &v) const
    { return (double) Superclass::GetVoxel(v); }

  virtual void GetVoxelAsDouble(const Vector3ui &x, double *out) const
    { out[0] = Superclass::GetVoxel(x); }

  virtual void GetVoxelAsDouble(const itk::Index<3> &idx, double *out) const
    { out[0] = Superclass::GetVoxel(idx); }

  /**
   * Get voxel intensity in native space
   */
  double GetVoxelMappedToNative(const Vector3ui &vec) const
    { return this->m_NativeMapping(this->GetVoxel(vec)); }

  double GetVoxelMappedToNative(const itk::Index<3> &idx) const
    { return this->m_NativeMapping(this->GetVoxel(idx)); }

  virtual void GetVoxelMappedToNative(const Vector3ui &vec, double *out) const
    { out[0] = this->m_NativeMapping(Superclass::GetVoxel(vec)); }

  virtual void GetVoxelMappedToNative(const itk::Index<3> &idx, double *out) const
    { out[0] = this->m_NativeMapping(Superclass::GetVoxel(idx)); }

  /** Compute statistics over a run of voxels in the image starting at the index
   * startIdx. Appends the statistics to a running sum and sum of squared. The
   * statistics are returned in internal (not native mapped) format */
  virtual void GetRunLengthIntensityStatistics(
      const itk::ImageRegion<3> &region,
      const itk::Index<3> &startIdx, long runlength,
      double *out_sum, double *out_sumsq) const;

  /**
   * This method returns a vector of values for the voxel under the cursor.
   * This is the natural value or set of values that should be displayed to
   * the user. The value depends on the current display mode. For scalar
   * images, it's just the value of the voxel, but for multi-component images,
   * it's the value of the selected component (if there is one) or the value
   * of the multiple components when the mode is RGB. In the second parameter,
   * the method returns the RGB appearance of the voxel under the cursor
   */
  virtual void GetVoxelUnderCursorDisplayedValueAndAppearance(
      vnl_vector<double> &out_value, DisplayPixelType &out_appearance);

  virtual ComponentTypeObject *GetImageMinObject() const;

  virtual ComponentTypeObject *GetImageMaxObject() const;

  /**
    Compute the image histogram. The histogram is cached inside of the
    object, so repeated calls to this function with the same nBins parameter
    will not require additional computation.

    Calling with default parameter (0) will use the same number of bins that
    is currently in the histogram (i.e., return/recompute current histogram).
    If there is no current histogram, a default histogram with 128 entries
    will be generated.

    For multi-component data, the histogram is pooled over all components.
    */
  const ScalarImageHistogram *GetHistogram(size_t nBins = 0);

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
    This method creates an ITK mini-pipeline that can be used to cast the internal
    image to a floating point image. The ownership of the mini-pipeline is passed
    to the caller of this method. This method should be used with caution, since
    there is potential to create duplicates of the internally stored image without
    need. The best practice is to use this method with filters that only access a
    portion of the casted image at a time, such as streaming filters.

    When you call Update() on the returned mini-pipeline, the data will be cast to
    floating point, and if necessary, converted to the native intensity range.
    */
  SmartPtr<FloatImageSource> CreateCastToFloatPipeline() const;

  /**
   * Get an image cast to a common representation.
   * @see ScalarImageWrapperBase::GetCommonFormatImage()
   */
  CommonFormatImageType* GetCommonFormatImage(
      ExportChannel channel = ScalarImageWrapperBase::WHOLE_IMAGE);

  /** Return the intensity curve for this layer if it exists */
  virtual IntensityCurveInterface *GetIntensityCurve() const;

  /** Return the color map for this layer if it exists */
  virtual ColorMap *GetColorMap() const;

  /** Get a version of this image that is usable in VTK pipelines */
  vtkImageImport *GetVTKImporter();

  /** Extends parent method */
  virtual void SetNativeMapping(NativeIntensityMapping mapping);


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
   * The filter used for histogram computation
   */
  SmartPtr<HistogramFilterType> m_HistogramFilter;

  // The policy used to extract a common representation image
  typedef typename TTraits::CommonRepresentationPolicy CommonRepresentationPolicy;
  CommonRepresentationPolicy m_CommonRepresentationPolicy;

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
  virtual void UpdateImagePointer(ImageType *image,
                                  ImageBaseType *refSpace = NULL,
                                  ITKTransformType *tran = NULL);


  typedef itk::VTKImageExport<ImageType> VTKExporter;
  SmartPtr<VTKExporter> m_VTKExporter;

  vtkSmartPointer<vtkImageImport> m_VTKImporter;

  void SetupVTKImportExport();
};

#endif // __ScalarImageWrapper_h_
