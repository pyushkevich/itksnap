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

/**
 * \class VectorImageWrapper
 * \brief A wrapper around an itk::Image and related pipelines.
 * 
 * VectorImage Wrapper serves as a wrapper around an image pointer, and 
 * is used to unify the treatment of different kinds of vector images in
 * SNaP.  
 */
template<class TTraits>
class VectorImageWrapper : public ImageWrapper<TTraits>
{
public:

  // Standard ITK business
  typedef VectorImageWrapper<TTraits>                                     Self;
  typedef ImageWrapper<TTraits>                                     Superclass;
  typedef SmartPtr<Self>                                               Pointer;
  typedef SmartPtr<const Self>                                    ConstPointer;
  itkTypeMacro(VectorImageWrapper, ImageWrapper)
  itkNewMacro(Self)

  // Image Types
  typedef typename Superclass::ImageBaseType                     ImageBaseType;
  typedef typename Superclass::ImageType                             ImageType;
  typedef typename Superclass::ImagePointer                       ImagePointer;
  typedef typename Superclass::PreviewImageType               PreviewImageType;

  // 4D Image types
  typedef typename Superclass::Image4DType                         Image4DType;
  typedef typename Superclass::Image4DPointer                   Image4DPointer;

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
  typedef typename Superclass::DisplaySlicePointer         DisplaySlicePointer;
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

  // Index and size types
  typedef typename Superclass::IndexType                             IndexType;
  typedef typename Superclass::SizeType                               SizeType;

  // Interpolation mode
  typedef typename Superclass::InterpolationMode             InterpolationMode;

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
    return this->m_Image4D->GetNumberOfComponentsPerPixel();
  }

  /** Compute statistics over a run of voxels in the image starting at the index
   * startIdx. Appends the statistics to a running sum and sum of squared. The
   * statistics are returned in internal (not native mapped) format */
  virtual void GetRunLengthIntensityStatistics(
      const itk::ImageRegion<3> &region,
      const itk::Index<3> &startIdx, long runlength,
      double *out_nvalid, double *out_sum, double *out_sumsq) const ITK_OVERRIDE;

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

  /**
   * This method samples a 2D slice based on a reference geometry from
   * the current image and maps it using the current display mapping. It
   * is used to generate thumbnails and for other sampling of image
   * appearance that is outside of the main display pipeline
   */
  virtual DisplaySlicePointer SampleArbitraryDisplaySlice(const ImageBaseType *ref_space) ITK_OVERRIDE;

  virtual void SetNativeMapping(NativeIntensityMapping mapping) ITK_OVERRIDE;

  virtual void SetSliceIndex(const IndexType &cursor) ITK_OVERRIDE;

  virtual void SetDisplayGeometry(const IRISDisplayGeometry &dispGeom) ITK_OVERRIDE;

  virtual void SetDisplayViewportGeometry(unsigned int index, const ImageBaseType *viewport_image) ITK_OVERRIDE;

  virtual void SetDirectionMatrix(const vnl_matrix<double> &direction) ITK_OVERRIDE;

  virtual void CopyImageCoordinateTransform(const ImageWrapperBase *source) ITK_OVERRIDE;

  virtual void SetSticky(bool value) ITK_OVERRIDE;

  virtual void SetITKTransform(ImageBaseType *referenceSpace, ITKTransformType *transform) ITK_OVERRIDE;

  virtual void SetReferenceSpace(ImageBaseType *referenceSpace) ITK_OVERRIDE;

  virtual void SetSlicingInterpolationMode(InterpolationMode mode) override;

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

  virtual void UpdateWrappedImages(Image4DType *image_4d,
                                   ImageBaseType *refSpace = NULL,
                                   ITKTransformType *tran = NULL) ITK_OVERRIDE;

  /** Destructor */
  virtual ~VectorImageWrapper();

  /** Write the image to disk as a floating point image (scalar or vector) */
  virtual void WriteToFileAsFloat(const char *filename, Registry &hints) ITK_OVERRIDE;

  /** Create a derived wrapper of a certain type */
  template <class TFunctor>
  SmartPtr<ScalarImageWrapperBase> CreateDerivedWrapper(
      Image4DType *image_4d, ImageBaseType *refSpace, ITKTransformType *transform);

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

  // Other derived wrappers
  typedef VectorToScalarMagnitudeFunctor<InternalPixelType,float> MagnitudeFunctor;
  typedef VectorToScalarMaxFunctor<InternalPixelType, float> MaxFunctor;
  typedef VectorToScalarMeanFunctor<InternalPixelType,float> MeanFunctor;

};

#endif // __VectorImageWrapper_h_
