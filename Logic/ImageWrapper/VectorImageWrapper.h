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
#include "itkVariableLengthVector.h"
#include "itkVectorImageToImageAdaptor.h"
#include "ScalarImageWrapper.h"
#include "itkImageAdaptor.h"
#include "VectorToScalarImageAccessor.h"

template<class TIn> class ThreadedHistogramImageFilter;
namespace itk
{
template<class TIn> class MinimumMaximumImageFilter;
}

/**
 * \class VectorImageWrapper
 * \brief A wrapper around an itk::Image and related pipelines.
 * 
 * VectorImage Wrapper serves as a wrapper around an image pointer, and 
 * is used to unify the treatment of different kinds of vector images in
 * SNaP.  
 */
template<class TTraits, class TBase = VectorImageWrapperBase>
class VectorImageWrapper
    : public ImageWrapper<TTraits, TBase>
{
public:

  // Standard ITK business
  typedef VectorImageWrapper<TTraits, TBase>                              Self;
  typedef ImageWrapper<TTraits, TBase>                              Superclass;
  typedef SmartPtr<Self>                                               Pointer;
  typedef SmartPtr<const Self>                                    ConstPointer;
  itkTypeMacro(VectorImageWrapper, ImageWrapper)
  itkNewMacro(Self)

  // Image Types
  typedef typename Superclass::ImageBaseType                     ImageBaseType;
  typedef typename Superclass::ImageType                             ImageType;
  typedef typename Superclass::ImagePointer                       ImagePointer;
  typedef typename Superclass::PreviewImageType               PreviewImageType;

  // Floating point image type
  typedef itk::Image<float, 3>                                  FloatImageType;
  typedef itk::ImageSource<FloatImageType>                    FloatImageSource;

  // Double precision floating point image type
  typedef itk::Image<double, 3>                                DoubleImageType;
  typedef itk::ImageSource<DoubleImageType>                  DoubleImageSource;

  // Vector image types
  typedef itk::VectorImage<float, 3>                      FloatVectorImageType;
  typedef itk::ImageSource<FloatVectorImageType>        FloatVectorImageSource;
  typedef itk::VectorImage<double, 3>                    DoubleVectorImageType;
  typedef itk::ImageSource<DoubleVectorImageType>      DoubleVectorImageSource;

  // Pixel type
  typedef typename Superclass::PixelType                             PixelType;
  typedef typename ImageType::InternalPixelType              InternalPixelType;

  // Slice image type
  typedef typename Superclass::SliceType                             SliceType;
  typedef typename Superclass::SlicePointer                       SlicePointer;

  // Slicer type
  typedef typename Superclass::SlicerType                           SlicerType;
  typedef typename Superclass::SlicerPointer                     SlicerPointer;

  // Display types
  typedef typename Superclass::DisplaySliceType               DisplaySliceType;
  typedef typename Superclass::DisplayPixelType               DisplayPixelType;

  // Iterator types
  typedef typename Superclass::Iterator                               Iterator;
  typedef typename Superclass::ConstIterator                     ConstIterator;

  typedef typename Superclass::NativeIntensityMapping   NativeIntensityMapping;

  // Access to the components
  typedef typename TTraits::ComponentWrapperType          ComponentWrapperType;

  // Pipeline objects wrapped around values
  typedef typename Superclass::ComponentTypeObject         ComponentTypeObject;

  typedef typename Superclass::ITKTransformType               ITKTransformType;

  virtual bool IsScalar() const ITK_OVERRIDE { return false; }

  /**
   * This function returns whatever scalar representation is current. The
   * current representation depends on the display mode of the wrapper. If the
   * display mode is RGB, the default scalar representation in MaximumComponent,
   * which is somewhat arbitrary. If the components are being animated, there
   * is also some ambiguity as to what the default scalar component should be.
   * @see ImageWrapperBase::GetScalarRepresentation
   */
  ScalarImageWrapperBase *GetDefaultScalarRepresentation() ITK_OVERRIDE;

  /**
   * Get a pointer to the given scalar representation of this vector image.
   */
  ScalarImageWrapperBase *GetScalarRepresentation(
      ScalarRepresentation type,
      int index = 0) ITK_OVERRIDE;

  /**
   * Access a scalar representation using an iterator
   */
  virtual ScalarImageWrapperBase *GetScalarRepresentation(
      const ScalarRepresentationIterator &it) ITK_OVERRIDE;

  /**
   * If scalar_rep is a scalar representation of the vector image wrapper, find
   * the type of the representation and the index. Otherwise return false;
   */
  bool FindScalarRepresentation(
      ImageWrapperBase *scalar_rep, ScalarRepresentation &type, int &index) const ITK_OVERRIDE;

  /**
   * This returns the same as GetScalarRepresentation(SCALAR_REP_COMPONENT, i),
   * but cast to its true type, rather than ScalarImageWrapperBase.
   */
  ComponentWrapperType *GetComponentWrapper(unsigned int index);

  /**
   * This method is used to perform a deep copy of a region of this image 
   * into another image, potentially resampling the region to use a different
   * voxel size
   */
  ImagePointer DeepCopyRegion(const SNAPSegmentationROISettings &roi,
                              itk::Command *progressCommand = NULL) const ITK_OVERRIDE;


  /** This image type has only one component */
  virtual size_t GetNumberOfComponents() const ITK_OVERRIDE
  {
    return this->m_Image->GetNumberOfComponentsPerPixel();
  }

  /** Voxel access */
  // virtual double GetVoxelAsDouble(const itk::Index<3> &idx) const;

  /** Voxel access */
  // virtual double GetVoxelAsDouble(const Vector3ui &v) const;

  virtual void GetVoxelAsDouble(const Vector3ui &x, double *out) const ITK_OVERRIDE
  {
    const PixelType &p = Superclass::GetVoxel(x);
    for(unsigned int i = 0; i < this->GetNumberOfComponents(); i++)
      out[i] = p[i];
  }

  virtual void GetVoxelAsDouble(const itk::Index<3> &idx, double *out) const ITK_OVERRIDE
  {
    const PixelType &p = Superclass::GetVoxel(idx);
    for(unsigned int i = 0; i < this->GetNumberOfComponents(); i++)
      out[i] = p[i];
  }

  virtual void GetVoxelMappedToNative(const Vector3ui &x, double *out) const ITK_OVERRIDE
  {
    const PixelType &p = Superclass::GetVoxel(x);
    for(unsigned int i = 0; i < this->GetNumberOfComponents(); i++)
      out[i] = this->m_NativeMapping(p[i]);
  }

  virtual void GetVoxelMappedToNative(const itk::Index<3> &idx, double *out) const ITK_OVERRIDE
  {
    const PixelType &p = Superclass::GetVoxel(idx);
    for(unsigned int i = 0; i < this->GetNumberOfComponents(); i++)
      out[i] = this->m_NativeMapping(p[i]);
  }

  /**
   * Get the component-wise minimum and maximum of the image in native format
   */
  virtual ComponentTypeObject *GetImageMinObject() const ITK_OVERRIDE;
  virtual ComponentTypeObject *GetImageMaxObject() const ITK_OVERRIDE;

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

  virtual void SetNativeMapping(NativeIntensityMapping mapping) ITK_OVERRIDE;

  virtual void SetSliceIndex(const Vector3ui &cursor) ITK_OVERRIDE;

  virtual void SetDisplayGeometry(const IRISDisplayGeometry &dispGeom) ITK_OVERRIDE;

  virtual void SetDisplayViewportGeometry(unsigned int index, ImageBaseType *viewport_image);

  virtual void SetDirectionMatrix(const vnl_matrix<double> &direction) ITK_OVERRIDE;

  virtual void CopyImageCoordinateTransform(const ImageWrapperBase *source) ITK_OVERRIDE;

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
    This method creates an ITK mini-pipeline that can be used to cast the internal
    image to a floating point image. The ownership of the mini-pipeline is passed
    to the caller of this method. This method should be used with caution, since
    there is potential to create duplicates of the internally stored image without
    need. The best practice is to use this method with filters that only access a
    portion of the casted image at a time, such as streaming filters.

    When you call Update() on the returned mini-pipeline, the data will be cast to
    floating point, and if necessary, converted to the native intensity range.
    */
  virtual SmartPtr<FloatImageSource> CreateCastToFloatPipeline() const ITK_OVERRIDE;

  /** Same as above, but casts to double. For compatibility with C3D, until we
   * safely switch C3D to use float instead of double */
  virtual SmartPtr<DoubleImageSource> CreateCastToDoublePipeline() const ITK_OVERRIDE;

  /** Same as CreateCastToFloatPipeline, but for vector images of single dimension */
  virtual SmartPtr<FloatVectorImageSource> CreateCastToFloatVectorPipeline() const ITK_OVERRIDE;

  /** Same as CreateCastToFloatPipeline, but for vector images of single dimension */
  virtual SmartPtr<DoubleVectorImageSource> CreateCastToDoubleVectorPipeline() const ITK_OVERRIDE;

protected:

  /**
   * Default constructor.  Creates an image wrapper with a blank internal
   * image
   */
  VectorImageWrapper();

  /**
   * Copy constructor.  Copies the contents of the passed-in image wrapper.
   */
  VectorImageWrapper(const Self &copy) : Superclass(copy) {}

  virtual void UpdateImagePointer(ImageType *image,
                                  ImageBaseType *refSpace = NULL,
                                  ITKTransformType *tran = NULL) ITK_OVERRIDE;

  virtual void SetITKTransform(ImageBaseType *referenceSpace, ITKTransformType *transform) ITK_OVERRIDE;

  /** Destructor */
  virtual ~VectorImageWrapper();

  /** Write the image to disk as a floating point image (scalar or vector) */
  virtual void WriteToFileAsFloat(const char *filename, Registry &hints) ITK_OVERRIDE;

  /** Create a derived wrapper of a certain type */
  template <class TFunctor>
  SmartPtr<ScalarImageWrapperBase> CreateDerivedWrapper(
      ImageType *image, ImageBaseType *refSpace, ITKTransformType *transform);

  template <class TFunctor>
  void SetNativeMappingInDerivedWrapper(
      ScalarImageWrapperBase *w, NativeIntensityMapping &mapping);

  // Array of derived quantities
  typedef SmartPtr<ScalarImageWrapperBase> ScalarWrapperPointer;
  typedef std::pair<ScalarRepresentation, int> ScalarRepIndex;

  typedef std::map<ScalarRepIndex, ScalarWrapperPointer> ScalarRepMap;
  typedef typename ScalarRepMap::iterator ScalarRepIterator;
  typedef typename ScalarRepMap::const_iterator ScalarRepConstIterator;
  ScalarRepMap m_ScalarReps;

  // For computing image statistics, we can represent the image as a one-dimensional
  // image of size n_voxels * n_components. This image can then be fed as input to
  // the min/max and histogram computation filters. Of course the flat image
  // shares the buffer with the 3D image so there is no memory waste
  typedef itk::Image<InternalPixelType, 1>                       FlatImageType;
  SmartPtr<FlatImageType> m_FlatImage;

  // Min/max filter
  typedef itk::MinimumMaximumImageFilter<FlatImageType> MinMaxFilterType;
  SmartPtr<MinMaxFilterType> m_MinMaxFilter;

  // Histogram filter
  typedef ThreadedHistogramImageFilter<FlatImageType> HistogramFilterType;
  SmartPtr<HistogramFilterType> m_HistogramFilter;

  // Other derived wrappers
  typedef VectorToScalarMagnitudeFunctor<InternalPixelType,float> MagnitudeFunctor;
  typedef VectorToScalarMaxFunctor<InternalPixelType, float> MaxFunctor;
  typedef VectorToScalarMeanFunctor<InternalPixelType,float> MeanFunctor;

};

#endif // __VectorImageWrapper_h_
