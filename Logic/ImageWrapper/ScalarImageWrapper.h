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
  class VTKImageExportBase;
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
template<class TTraits>
class ScalarImageWrapper : public ImageWrapper<TTraits>
{
public:

  // Standard ITK business
  typedef ScalarImageWrapper<TTraits>                                     Self;
  typedef ImageWrapper<TTraits>                                     Superclass;
  typedef SmartPtr<Self>                                               Pointer;
  typedef SmartPtr<const Self>                                    ConstPointer;
  itkTypeMacro(ScalarImageWrapper, ImageWrapper)
  itkNewMacro(Self)

  // Image Types
  typedef typename Superclass::ImageBaseType                     ImageBaseType;
  typedef typename Superclass::ImageType                             ImageType;
  typedef typename Superclass::ImagePointer                       ImagePointer;
  typedef typename Superclass::PixelType                             PixelType;
  typedef typename Superclass::PreviewImageType               PreviewImageType;

  // 4D Image types
  typedef typename Superclass::Image4DType                         Image4DType;
  typedef typename Superclass::Image4DPointer                   Image4DPointer;

  // Slice image type
  typedef typename Superclass::SliceType                             SliceType;
  typedef typename Superclass::SlicePointer                       SlicePointer;

  // Slicer type
  typedef typename Superclass::SlicerType                           SlicerType;
  typedef typename Superclass::SlicerPointer                     SlicerPointer;

  // Display types
  typedef typename Superclass::DisplaySliceType               DisplaySliceType;
  typedef typename Superclass::DisplayPixelType               DisplayPixelType;

  // MinMax calculator type (works on the 4D image)
  typedef itk::MinimumMaximumImageFilter<Image4DType>             MinMaxFilter;

  // Histogram filter (works on the 4D image)
  typedef ThreadedHistogramImageFilter<Image4DType>        HistogramFilterType;

  // VTK Exporter
  // typedef itk::VTKImageExport<CommonFormatImageType>             VTKExportType;

  // Iterator types
  typedef typename Superclass::Iterator                               Iterator;
  typedef typename Superclass::ConstIterator                     ConstIterator;

  // Output channels
  typedef typename Superclass::ExportChannel                     ExportChannel;

  // Pipeline objects wrapped around values
  typedef typename Superclass::ComponentTypeObject         ComponentTypeObject;

  typedef typename Superclass::NativeIntensityMapping   NativeIntensityMapping;

  typedef typename Superclass::ITKTransformType               ITKTransformType;


  virtual bool IsScalar() const ITK_OVERRIDE { return true; }

  /**
   * This function just returns the pointer to itself, as the scalar representation
   * of a scalar image wrapper is itself.
   * @see ImageWrapperBase::GetScalarRepresentation
   */
  virtual ScalarImageWrapperBase *GetDefaultScalarRepresentation() ITK_OVERRIDE { return this; }

  /** Access the min/max filter */
  irisGetMacro(MinMaxFilter, MinMaxFilter *)

  /**
   * Get the scaling factor used to convert between intensities stored
   * in this image and the 'true' image intensities
   */
  virtual double GetImageScaleFactor() ITK_OVERRIDE;

  typedef typename ImageWrapperBase::ShortImageType ShortImageType;

  /** This image type has only one component */
  virtual size_t GetNumberOfComponents() const ITK_OVERRIDE { return 1; }

  /** Compute statistics over a run of voxels in the image starting at the index
   * startIdx. Appends the statistics to a running sum and sum of squared. The
   * statistics are returned in internal (not native mapped) format */
  virtual void GetRunLengthIntensityStatistics(
      const itk::ImageRegion<3> &region,
      const itk::Index<3> &startIdx, long runlength,
      double *out_sum, double *out_sumsq) const ITK_OVERRIDE;

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
      vnl_vector<double> &out_value, DisplayPixelType &out_appearance) ITK_OVERRIDE;

  virtual const ComponentTypeObject *GetImageMinObject() const ITK_OVERRIDE;

  virtual const ComponentTypeObject *GetImageMaxObject() const ITK_OVERRIDE;

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
  const ScalarImageHistogram *GetHistogram(size_t nBins = 0) ITK_OVERRIDE;

  /**
    Get the maximum possible value of the gradient magnitude. This will
    compute the gradient magnitude of the image (without Gaussian smoothing)
    and return the maximum. The value will be cached so repeated calls to
    this are not expensive.
    */
  double GetImageGradientMagnitudeUpperLimit() ITK_OVERRIDE;

  /**
    Get the maximum possible value of the gradient magnitude in native units
    */
  double GetImageGradientMagnitudeUpperLimitNative() ITK_OVERRIDE;

  /**
   * Get an image cast to a common representation.
   * @see ScalarImageWrapperBase::GetCommonFormatImage()
   */
  /*
  const CommonFormatImageType* GetCommonFormatImage(
      ExportChannel channel = ScalarImageWrapperBase::WHOLE_IMAGE) ITK_OVERRIDE;
      */

  /** Return the intensity curve for this layer if it exists */
  virtual IntensityCurveInterface *GetIntensityCurve() const ITK_OVERRIDE;

  /** Return the color map for this layer if it exists */
  virtual ColorMap *GetColorMap() const ITK_OVERRIDE;

  /** A data type representing a pipeline for exporting to VTK */
  typedef ScalarImageWrapperBase::VTKImporterMiniPipeline VTKImporterMiniPipeline;

  /**
   * Create a mini-pipeline that can be used to import the image to VTK. Like
   * other pipelines created with Create..., this is meant for temporary use
   * since the pipeline may have to allocate large amounts of memory and we'
   * don't want this memory lingering around when it is not used
   */
  VTKImporterMiniPipeline CreateVTKImporterPipeline() const ITK_OVERRIDE;

  /** Extends parent method */
  virtual void SetNativeMapping(NativeIntensityMapping mapping) ITK_OVERRIDE;

  /** Is volume rendering turned on for this layer */
  irisIsMacroWithOverride(VolumeRenderingEnabled)

  /** Turn on volume rendering for this layer */
  irisSetWithEventMacroWithOverride(VolumeRenderingEnabled, bool, WrapperVisibilityChangeEvent)

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

  // Volume rendering state
  bool m_VolumeRenderingEnabled = false;
  
  /**
   * Compute the intensity range of the image if it's out of date.  
   * This is done before calling GetImateMin, GetImateMax and GetImageScaleFactor methods.
   */
  void CheckImageIntensityRange();

  /**
   * Handle a change in the image pointer (i.e., a load operation on the image or 
   * an initialization operation)
   */
  virtual void UpdateWrappedImages(Image4DType *image_4d,
                                   ImageBaseType *refSpace = NULL,
                                   ITKTransformType *tran = NULL) ITK_OVERRIDE;


  /** Write the image to disk as a floating point image (scalar or vector) */
  virtual void WriteToFileAsFloat(const char *filename, Registry &hints) ITK_OVERRIDE;
};

#endif // __ScalarImageWrapper_h_
