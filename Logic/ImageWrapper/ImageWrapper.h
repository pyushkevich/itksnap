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
#include "RLEImageScanlineIterator.h"
#include "ImageWrapperBase.h"
#include "ImageCoordinateGeometry.h"
#include <itkVectorImage.h>
#include <itkRGBAPixel.h>
#include <DisplayMappingPolicy.h>
#include <itkSimpleDataObjectDecorator.h>
#include <array>
#include <vector>

// Forward declarations to IRIS classes
template <class TFunctor> class UnaryValueToValueFilter;

template <class TInputImage, class TOutputImage, class TTraits>
class AdaptiveSlicingPipeline;

template <class TInputImage, class TTag> class InputSelectionImageFilter;

template<class TIn> class TDigestImageFilter;
class TDigestDataObject;

class SNAPSegmentationROISettings;

namespace itk {
  template <unsigned int VDimension> class ImageBase;
  template <class TImage> class ImageSource;
  template <typename TScalar, unsigned int V1, unsigned int V2> class Transform;
  template<typename TInputImage,
           typename TOutputImage> class ExtractImageFilter;
}

#include <itkImageSource.h>



/**
 * \class ImageWrapper
 * \brief A wrapper around an itk::OrientedImage and related pipelines.
 * 
 * Image Wrapper serves as a wrapper around an image pointer, and 
 * is used to unify the treatment of different kinds of images in
 * SNaP. The image wrapper is parameterized by a base class TBase.
 * This is done to avoid multiple inheritance. TBase must be a
 * subclass of ImageWrapperBase.
 */
template<class TTraits>
class ImageWrapper : public TTraits::WrapperBaseType
{
public:

  // Standard ITK business
  typedef ImageWrapper<TTraits>                                           Self;
  typedef typename TTraits::WrapperBaseType                         Superclass;
  typedef SmartPtr<Self>                                               Pointer;
  typedef SmartPtr<const Self>                                    ConstPointer;
  itkTypeMacro(ImageWrapper, TBase)

  // Basic type definitions
  typedef typename Superclass::ImageBaseType                     ImageBaseType;
  typedef typename TTraits::ImageType                                ImageType;
  typedef SmartPtr<ImageType>                                     ImagePointer;
  typedef typename ImageType::PixelType                              PixelType;

  // 4D Image typedefs
  typedef typename Superclass::Image4DBaseType                 Image4DBaseType;
  typedef typename TTraits::Image4DType                            Image4DType;
  typedef SmartPtr<Image4DType>                                 Image4DPointer;

  // Floating image sources
  typedef typename Superclass::FloatImageType                   FloatImageType;
  typedef typename Superclass::FloatVectorImageType       FloatVectorImageType;
  typedef typename Superclass::FloatSliceType                   FloatSliceType;
  typedef typename Superclass::FloatVectorSliceType       FloatVectorSliceType;
  typedef typename Superclass::MiniPipeline                       MiniPipeline;

  // This is the pixel type of the buffer pointer, i.e., internal representation
  typedef typename ImageType::InternalPixelType              InternalPixelType;

  // This is the type of the outwardly visible components in the image. For
  // wrappers that encapsulate 'real' images and vector images, this is the same
  // as the internal pixel type, but for wrappers that encapsulate an image
  // adaptor, the component type is the output type of the adaptor
  typedef typename TTraits::ComponentType                        ComponentType;

  // This is the type of the intermediate 'preview' images used in the slicing
  // pipeline. For regular image and vector image wrappers, this is the same as
  // ImageType, but for adapter-based image wrappers, this is a concrete image
  typedef itk::Image<PixelType, 3>                            PreviewImageType;

  // Slice image type
  typedef typename TTraits::SliceType                                SliceType;
  typedef SmartPtr<SliceType>                                     SlicePointer;

  // Display slice type - inherited
  typedef typename Superclass::DisplayPixelType               DisplayPixelType;
  typedef typename Superclass::DisplaySliceType               DisplaySliceType;
  typedef typename Superclass::DisplaySlicePointer         DisplaySlicePointer;

  // Slicer type
  typedef AdaptiveSlicingPipeline<ImageType, SliceType, PreviewImageType> SlicerType;
  typedef SmartPtr<SlicerType>                                   SlicerPointer;

  // Preview source for preview pipelines
  typedef itk::ImageSource<PreviewImageType>                 PreviewFilterType;

  // Filter used for fast approximation of image quantiles
  typedef TDigestImageFilter<Image4DType>                    TDigestFilterType;

  // Iterator types
  typedef typename itk::ImageRegionIterator<ImageType>                Iterator;
  typedef typename itk::ImageRegionConstIterator<ImageType>      ConstIterator;

  // Other types from parent
  typedef typename Superclass::TransformType                     TransformType;

  // Internal intensity to display intensity mapping
  typedef typename TTraits::DisplayMapping                      DisplayMapping;

  // Native intensity mapping
  typedef typename TTraits::NativeIntensityMapping      NativeIntensityMapping;

  // Objects used to represent min/max intensity in the image
  typedef itk::SimpleDataObjectDecorator<ComponentType>    ComponentTypeObject;
  typedef itk::SimpleDataObjectDecorator<double>                  DoubleObject;

  // ITK's coordinate transform (rigid, affine, etc)
  typedef typename Superclass::ITKTransformType               ITKTransformType;

  // Metadata access object
  typedef typename Superclass::MetaDataAccessType           MetaDataAccessType;

  // Index and size types
  typedef typename Superclass::IndexType                             IndexType;
  typedef typename Superclass::SizeType                               SizeType;

  // Interpolation mode
  typedef typename Superclass::InterpolationMode             InterpolationMode;

  /**
   * Get the parent wrapper for this wrapper. For 'normal' wrappers, this method
   * returns NULL, indicating that the wrapper is a top-level wrapper. For derived
   * wrappers (i.e., components and scalar representations of vector wrappers),
   * this method returns the vector wrapper from which the wrapper is derived.
   *
   * You should not have to call the SetParentWrapper method. It's used internally
   */
  irisGetSetMacroWithOverride(ParentWrapper, ImageWrapperBase *)

  /**
   * Initialize the image wrapper to match another image wrapper, setting the
   * image pixels to a default value. The slice indices and the transforms will
   * all be updated to match the source
   */
  virtual void InitializeToWrapper(
    const ImageWrapperBase *source, const PixelType &value);

  /** 
   * Initialize the image wrapper with a source wrapper and a 4D image. This will take
   * the metadata from the source wrapper but assign the provided image. The reference
   * space and transform are taken from the source wrapper, but the transform is copied
   * so that if the transform of the source wrapper changes, this image will not be
   * affected.
   */
  virtual void InitializeToWrapper(const ImageWrapperBase *source, Image4DType *image_4d);

  /**
   * Initialize the image wrapper with a source wrapper and a 4D image. This will take
   * the metadata from the source wrapper but assign the provided image. The reference
   * space and transform are specified explicitly in this version of the method and
   * will be assigned to the wrapper.
   */
  virtual void InitializeToWrapper(const ImageWrapperBase *source, Image4DType *image_4d,
                                   ImageBaseType *referenceSpace, ITKTransformType *transform);

  /**
    Does this wrapper use the non-orthogonal slicing pipeline?
    */
  virtual bool IsSlicingOrthogonal() const ITK_OVERRIDE;

  /**
   * Clear the data associated with storing an image
   */
  virtual void Reset() ITK_OVERRIDE;

  /** Get the coordinate transform for each display slice */
  virtual const ImageCoordinateTransform *GetImageToDisplayTransform(
    unsigned int) const ITK_OVERRIDE;

  /**
   * Set the coordinate transformation between the display coordinates and
   * the anatomical coordinates. This affects the behavior of the slicers
   */
  virtual void SetDisplayGeometry(const IRISDisplayGeometry &dispGeom) ITK_OVERRIDE;

  /** Get the display to anatomy coordinate mapping */
  irisGetMacroWithOverride(DisplayGeometry, const IRISDisplayGeometry &)

  /** Set the direction matrix of the image */
  virtual void SetDirectionMatrix(const vnl_matrix<double> &direction) ITK_OVERRIDE;

  /**
   * Set the image coordinate transform (origin, spacing, direction) to
   * match those of a reference wrapper
   */
  virtual void CopyImageCoordinateTransform(const ImageWrapperBase *source) ITK_OVERRIDE;

  /**
   * Get the image geometry from the wrapper
   */
  const ImageCoordinateGeometry *GetImageGeometry() const override { return m_ImageGeometry; }

  /** Get the current slice index - which really means cursor position */
  irisGetMacroWithOverride(SliceIndex, IndexType)

  /** Return some image info independently of pixel type */
  ImageBaseType* GetImageBase() const ITK_OVERRIDE;

  /** Return 4D image metadata */
  Image4DBaseType* GetImage4DBase() const ITK_OVERRIDE { return m_Image4D; }

  /** Get the number of time points (how many 3D images in 4D array) */
  virtual unsigned int GetNumberOfTimePoints() const ITK_OVERRIDE { return m_ImageTimePoints.size(); }

  /** Get the time index (which 3D volume in the 4D array is currently shown) */
  irisGetMacroWithOverride(TimePointIndex, unsigned int)

  /**
   * Is the image initialized?
   */
  irisIsMacroWithOverride(Initialized)

  /**
   * Get the size of the image
   */
  Vector3ui GetSize() const ITK_OVERRIDE;

  /** Get layer transparency */
  irisSetWithEventMacroWithOverride(Alpha, double, WrapperDisplayMappingChangeEvent)

  /** Set layer transparency */
  irisGetMacroWithOverride(Alpha, double)

  /**
   * Get layer stickiness. A sticky layer always is shown 'on top' of other
   * layers, e.g., the segmentation layer, or the level set image. A layer that
   * is not sticky is shown in its own tile when the display is in tiled mode
   */
  virtual void SetSticky(bool value) override;

  /** Set layer stickiness */
  irisIsMacroWithOverride(Sticky)

  /**
   * Whether the layer is drawable. Some layers may be initialized, but not
   * yet computed, in which case they should not yet be drawn.
   */
  virtual bool IsDrawable() const ITK_OVERRIDE;

  /** Get the buffered region of the image */
  virtual itk::ImageRegion<3> GetBufferedRegion() const ITK_OVERRIDE;

  /** Transform a voxel index into a spatial position */
  virtual Vector3d TransformVoxelIndexToLPSCoordinates(const Vector3i &iVoxel) const ITK_OVERRIDE;

  /** Transform a voxel index into a spatial position */
  virtual Vector3d TransformVoxelCIndexToLPSCoordinates(const Vector3d &iVoxel) const ITK_OVERRIDE;

  /** Transform spatial position to voxel continuous index (LPS) */
  virtual Vector3d TransformLPSCoordinatesToVoxelCIndex(const Vector3d &vLPS) const ITK_OVERRIDE;

  /** Transform spatial position to voxel index (LPS) */
  virtual Vector3i TransformLPSCoordinatesToVoxelIndex(const Vector3d &vLPS) const ITK_OVERRIDE;

  /** Transform a voxel index into NIFTI coordinates (RAS) */
  virtual Vector3d TransformVoxelCIndexToNIFTICoordinates(const Vector3d &iVoxel) const ITK_OVERRIDE;

  /** Transform NIFTI coordinates to a continuous voxel index */
  virtual Vector3d TransformNIFTICoordinatesToVoxelCIndex(const Vector3d &vNifti) const ITK_OVERRIDE;

  /**
   * Transform a reference space index to a continuous index in the voxel space of
   * the wrapped image. For images whose geometry matches the reference space, this
   * is the identity transform, but for images that are not in referene space, this
   * will use the S-form of the reference space, S-form of the wrapped image and the
   * registration transform applied to the image to compute the coordinate.
   */
  virtual void TransformReferenceCIndexToWrappedImageCIndex(
      const itk::ContinuousIndex<double, 3> &ref_index, itk::ContinuousIndex<double, 3> &img_index) const override;

  virtual bool ImageSpaceMatchesReferenceSpace() const ITK_OVERRIDE;

  /** Get the NIFTI s-form matrix for this image */
  irisGetMacroWithOverride(NiftiSform, TransformType)

  /** Get the inverse NIFTI s-form matrix for this image */
  irisGetMacroWithOverride(NiftiInvSform, TransformType)

  /** Set the voxel at a given position.*/
  virtual void SetVoxel(const Vector3ui &index, const PixelType &value);
  virtual void SetVoxel(const itk::Index<3> &index, const PixelType &value);

  /**
   * Lookup a voxel at a particular location in the image. The input coordinates
   * are of the wrapped image, not of the reference image.
   */
  PixelType GetVoxel(const itk::Index<3> &index, int time_point = -1) const;

  /** Offset table used for random access sampling of patches */
  typedef ImageWrapperBase::PatchOffsetTable PatchOffsetTable;

  /**
   * Support for random access sampling of patches from the image. This method
   * generates a set of offsets in the image that can be used efficiently to
   * sample patches from the image.
   */
  virtual PatchOffsetTable GetPatchOffsetTable(const SizeType &radius) const ITK_OVERRIDE;

  /**
   * Sample the image patch around a pixel location. No bounds checking is done,
   * so it is assumed that the location has enough margin to sample from. It is
   * also assumed that the output vector has been allocated already
   */
  virtual void SamplePatchAsDouble(const IndexType &idx, const PatchOffsetTable &offset_table,
                                   double *out_patch) const ITK_OVERRIDE;

  /**
   * Get current interpolation mode
   */
  virtual typename Superclass::InterpolationMode GetSlicingInterpolationMode() const override;

  /**
   * Set interpolation mode for non-orthogonal slicing
   */
  virtual void SetSlicingInterpolationMode(InterpolationMode mode) override;

  /**
   * Sample image intensity at a 4D position in the reference space. If the reference
   * space does not match the native space, the intensity will be interpolated based
   * on the current affine transform applied to the image.
   *
   * You can specify a time point, in which case only n_components will be sampled. If the
   * time point is -1, all timepoints will be sampled.
   *
   * The flag map_to_native specifies whether the output intensities are raw or transformed
   * into the native intensity space.
   */
  virtual void SampleIntensityAtReferenceIndex(
      const itk::Index<3> &index, int time_point,
      bool map_to_native, vnl_vector<double> &out) const ITK_OVERRIDE;

  /**
   * Get the mapping between the internal data type and the 'native' range,
   * i.e., the range of values shown to the user. This may be a linear mapping
   * or an identity mapping. This method returns an abstract type;
   */
  virtual const AbstractNativeIntensityMapping *GetNativeIntensityMapping() const ITK_OVERRIDE
    { return &m_NativeMapping; }

  /** These methods access the native mapping in its actual type */
  irisGetMacro(NativeMapping, NativeIntensityMapping)

  /** Set the native mapping */
  virtual void SetNativeMapping(NativeIntensityMapping nim);

  /** Get the intensity to display mapping */
  DisplayMapping *GetDisplayMapping() ITK_OVERRIDE
    { return m_DisplayMapping; }

  /** Get the intensity to display mapping */
  const DisplayMapping *GetDisplayMapping() const ITK_OVERRIDE
    { return m_DisplayMapping; }

  /**
   * Return the pointer to the ITK image encapsulated by this wrapper. In order to restrict
   * write operations to the image, only a const pointer is returned in the public method.
   */
  virtual const ImageType *GetImage() const;

  /**
   * Return the pointer to the 4D ITK image encapsulated by this wrapper. In order to restrict
   * write operations to the image, only a const pointer is returned in the public method.
   */
  virtual const Image4DType *GetImage4D() const
    { return m_Image4D; }

  /**
   * Get an image for modification. After making modifications, PixelsModified() should be
   * called in order for slicing and other pipelines to be updated
   */
  virtual ImageType *GetModifiableImage() { return m_Image; }

  /**
   * This function should be called whenever the pixels in the image returned via GetModifableImage
   * are modified. This will cause pipelines to update correctly.
   */
  void PixelsModified();

  /**
   * Replace the pixel data in the wrapped 4D image with a new data array. This method should be
   * used in very rare circumstances where it is not possible/desirable to update the pixels in
   * the image directly using iterators or to graft the image to an ITK filter. This filter is not
   * implemented for wrappers around ImageAdaptors
   */
  virtual void SetPixelContainer(typename ImageType::PixelContainer *container);

  /** 
   * Get the slicer inside this wrapper
   */
  virtual SlicerType *GetSlicer(unsigned int iDirection) const;

  /**
   * Set the current slice index in all three dimensions.  The index should
   * be specified in the image coordinates, the slices will be generated
   * in accordance with the transforms that are specified
   */
  virtual void SetSliceIndex(const IndexType &cursor) ITK_OVERRIDE;

  /** Set the current time index */
  virtual void SetTimePointIndex(unsigned int index) ITK_OVERRIDE;

  const ImageBaseType* GetDisplayViewportGeometry(unsigned int index) const;

  virtual void SetDisplayViewportGeometry(
      unsigned int index,
      const ImageBaseType *viewport_image) ITK_OVERRIDE;

  /**
    Compute the image t-digest, from which the quantiles of the image can be
    approximated. The t-digest is a fast algorithm for approximating image
    statistics such as quantiles, histogram, CDF, etc. It replaces earlier
    code that separately computed the image min/max and histogram using
    separate filters and required the number of bins to be specified explicitly

    The t-digest is over the 4D image.
    For multi-component data, the t-digest is pooled over all components.
    */
  virtual TDigestDataObject *GetTDigest() ITK_OVERRIDE;

  typedef itk::SimpleDataObjectDecorator<ComponentType> MinMaxObjectType;

  /** Legacy code returning image min as an object. TODO: refactor this out */
  virtual const MinMaxObjectType *GetImageMinObject();

  /** Legacy code returning image max as an object. TODO: refactor this out */
  virtual const MinMaxObjectType *GetImageMaxObject();

  /** Return componentwise minimum cast to double, without mapping to native range */
  virtual double GetImageMinAsDouble() ITK_OVERRIDE;

  /** Return componentwise maximum cast to double, without mapping to native range */
  virtual double GetImageMaxAsDouble() ITK_OVERRIDE;

  /** Return componentwise minimum cast to double, after mapping to native range */
  virtual double GetImageMinNative() ITK_OVERRIDE;

  /** Return componentwise maximum cast to double, after mapping to native range */
  virtual double GetImageMaxNative() ITK_OVERRIDE;

  /**
   * Get a slice of the image in a given direction
   */
  virtual SliceType *GetSlice(unsigned int dimension);

  /**
   * This method exposes the scalar pointer in the image
   */
  //virtual InternalPixelType *GetVoxelPointer() const;

  /** Number of voxels */
  virtual size_t GetNumberOfVoxels() const ITK_OVERRIDE;

  /**
   * Pring debugging info
   * TODO: Delete this or make is worthwhile
   */
  virtual void PrintDebugInformation();
                       
  /**
   * Replace the 4D image internally stored in this wrapper by another image.
   */
  virtual void SetImage4D(Image4DType *image_4d);

  /**
   * Set the wrapper to hold an image that is in a coordinate space that is
   * different from the program's main reference space
   */
  virtual void SetImage4D(Image4DType *image_4d,
                          ImageBaseType *refSpace, ITKTransformType *transform);

  /**
   * Update a single timepoint in the image with the supplied image. If time point
   * is not specified, the current time point will be updated.
   */
  virtual void UpdateTimePoint(ImageType *image, int time_point = -1);

  /**
   * Update the transform between the coordinate space of this image and the program's
   * main reference space
   */
  virtual void SetITKTransform(ImageBaseType *referenceSpace, ITKTransformType *transform) ITK_OVERRIDE;

  /**
   * Set the reference image without changing the transform
   */
  virtual void SetReferenceSpace(ImageBaseType *referenceSpace) ITK_OVERRIDE;

  /**
   * Get the ITK transform between this layer and its reference space
   */
  virtual const ITKTransformType *GetITKTransform() const ITK_OVERRIDE;

  /**
   * Get the reference space space in which this image is defined
   */
  virtual ImageBaseType* GetReferenceSpace() const ITK_OVERRIDE;

  /**
   * Extract a region of interest from the image wrapper, as a new wrapper of
   * the same type
   */
  virtual SmartPtr<ImageWrapperBase> ExtractROI(
      const SNAPSegmentationROISettings &roi, itk::Command *progressCommand) const ITK_OVERRIDE;

  /**
   * Extract a 3d region of interest from all time points in the image wrapper,
   * as a new wrapper of the same type
   */
  virtual SmartPtr<ImageWrapperBase> ExtractROI4D(
      const SNAPSegmentationROISettings &roi, itk::Command *progressCommand) const ITK_OVERRIDE;


  /**
   * This method is used to perform a deep copy of a region of this image 
   * into another image, potentially resampling the region to use a different
   * voxel size
   */
  virtual ImagePointer DeepCopyRegion(const SNAPSegmentationROISettings &roi,
                              itk::Command *progressCommand = NULL) const;

  /**
   * This method is used to perform a deep copy of a 3D region of each time points
   * of this image into another 4D image, potentially resampling the region to use a
   * different voxel size
   */
  virtual Image4DPointer DeepCopyRegion4D(const SNAPSegmentationROISettings &roi,
                              itk::Command *progressCommand = NULL) const;


  /**
   * Get an iterator for traversing the image.  The iterator is initialized
   * to point to the beginning of the image
   */
  virtual ConstIterator GetImageConstIterator() const;
  virtual Iterator GetImageIterator();

  /** For each slicer, find out which image dimension does is slice along */
  unsigned int GetDisplaySliceImageAxis(unsigned int slice) ITK_OVERRIDE;

  /** 
   * Replace all voxels with intensity values iOld with values iNew. 
   * \return number of voxels that had been modified
   */
  virtual unsigned int ReplaceIntensity(PixelType iOld, PixelType iNew);

  /** 
   * Swap intensity values iFirst and iSecond
   * \return number of voxels that had been modified
   */
  virtual unsigned int SwapIntensities(PixelType iFirst, PixelType iSecond);

  /**
   * Get the display slice
   */
  DisplaySlicePointer GetDisplaySlice(unsigned int dim) ITK_OVERRIDE;

  /**
    Attach a preview pipeline to the wrapper. This is used with wrappers that
    represent results of image processing operations, such as speed images.
    A preview pipeline consists of a filter for each of the three slice
    directions. When the upstream information in these filters is newer than
    the image volume stored in the wrapper, the slices returned with
    GetDisplaySlice() will be those from the preview filters rather than from
    the image volume.
    */
  virtual void AttachPreviewPipeline(
      PreviewFilterType *f0, PreviewFilterType *f1, PreviewFilterType *f2);

  /**
    Detach the preview pipeline from the wrapper. The wrapper will always
    return slices from the internally stored image volume.
    */
  virtual void DetachPreviewPipeline();

  /**
    Report whether the preview pipeline is attached.
    */
  virtual bool IsPreviewPipelineAttached() const;

  /**
   * A flag governing whether the preview pipeline is ready or not. This is
   * used to determine whether the layer is drawable (only relevant if the
   * pipeline is attached)
   */
  irisIsMacroWithOverride(PipelineReady)
  irisSetMacroWithOverride(PipelineReady, bool)

  /**
   * Set the filename of the image wrapper. If the wrapper does not have a
   * nickname, the nickname will be changed to the file part of the filename.
   */
  void SetFileName(const std::string &name) ITK_OVERRIDE;


  // Access the filename
  irisGetStringMacroWithOverride(FileName)

  /**
   * Fallback nickname - shown if no filename and no custom nickname set.
   */
  irisGetSetMacroWithOverride(DefaultNickname, const std::string &)

  /**
   * Get the nickname of the image. A nickname is a shorter description of the
   * image that is displayed to the user. If a custom nickname is not set, it
   * defaults to the filename (without path). If there is no filename (i.e.,
   * the layer is internal), the default nickname is used.
   */
  const std::string &GetNickname() const ITK_OVERRIDE;

  /**
   * Set the custom nickname for the wrapper.
   */
  virtual void SetCustomNickname(const std::string &nickname) ITK_OVERRIDE;
  irisGetMacroWithOverride(CustomNickname, const std::string &);


  /**
   * Access the tags for this image layer. Tags are just strings assigned to the
   * layer that may be useful to other software
   */
  irisGetMacroWithOverride(Tags, const TagList &)
  irisSetMacroWithOverride(Tags, const TagList &)


  /**
   * Access the "IO hints" registry associated with this wrapper. The IO hints
   * are used to help read the image when the filename alone is not sufficient.
   * For example, it may contain the DICOM series ID of the image, or for a raw
   * image the dimensions.
   */
  virtual const Registry &GetIOHints() const ITK_OVERRIDE;

  /**
   * Set the IO hints
   */
  virtual void SetIOHints(const Registry &io_hints) ITK_OVERRIDE;

  /**
   * Write the image to disk with the help of the GuidedNativeImageIO object
   */
  virtual void WriteToFile(const char *filename, Registry &hints) ITK_OVERRIDE;

  /**
   * Create a thumbnail from the image and write it to a .png file
   */
  DisplaySlicePointer MakeThumbnail(unsigned int maxdim) ITK_OVERRIDE;

  /**
   * Save metadata to a Registry file. The metadata are data that are not
   * contained in the image header are need to be restored when the image
   * is reloaded. Currently, this mainly includes the display mapping, but
   * also the transparency, etc.
   */
  virtual void WriteMetaData(Registry &reg) ITK_OVERRIDE;

  /**
   * Restore metadata from a registry
   */
  virtual void ReadMetaData(Registry &reg) ITK_OVERRIDE;

  /**
   * Get the meta data accessor object, useful for inspecting metadata
   */
  virtual MetaDataAccessType GetMetaDataAccess() ITK_OVERRIDE;

  /**
   * Check if the image has unsaved changes
   */
  virtual bool HasUnsavedChanges() const ITK_OVERRIDE;

  /**
   * Check if the time point has unsaved changes
   */
  virtual bool HasUnsavedChanges(unsigned int tp) const ITK_OVERRIDE;

  /**
   * This method is only used when this wrapper is around an image adaptor
   * (e.g., magnitude of component vector) and we need to update the native
   * mapping used in the adapter as part of the calculation. This is because
   * these adapters should include the native mapping as part of the calculation
   */
  void SetSourceNativeMapping(double scale, double shift);

  /**
    * Get the image from a specific timepoint
    */
  virtual const ImagePointer GetImageByTimePoint(unsigned int timepoint) const;


  /** Write timepoint image to file */
  void WriteCurrentTPImageToFile(const char *filename);

  /**
    Cast the internally stored image to a floating point image. The returned
    image is connected to the internally stored image by a mini-pipeline that
    may include a cast filter or a scale/shift filter, depending on the internal
    format of the image and the internal-to-native intensity mapping. The wrapper
    retains a smart pointer to the filters in the pipeline until the pipeline is
    released by calling ReleaseInternalPipeline()

    The pipeline is identified with a key and an optional index, which should be
    passed to ReleaseInternalPipeline() when it is no longer needed.

    The method is intended for use with external pipelines that don't know what
    the internal data representation is for the image. There is a cost with using
    this method in terms of memory, so the recommended use is in conjunction with
    streaming filters, so that the cast mini-pipeline does not allocate the whole
    floating point image all at once.
    */
  virtual FloatImageType* CreateCastToFloatPipeline(const char *key, int index = 0) ITK_OVERRIDE;

  /** Same as CreateCastToFloatPipeline, but for vector images of single dimension */
  virtual FloatVectorImageType* CreateCastToFloatVectorPipeline(const char *key, int index = 0) ITK_OVERRIDE;

  /** Create a pipeline for casting an image slice to floating point */
  virtual FloatSliceType* CreateCastToFloatSlicePipeline(const char *key, unsigned int slice) ITK_OVERRIDE;

  /** Create a pipeline for casting an image slice to floating point vector image */
  virtual FloatVectorSliceType* CreateCastToFloatVectorSlicePipeline(const char *key, unsigned int slice) ITK_OVERRIDE;

  /**
   * Release the filters and images in an internally managed pipeline. Passing -1 for
   * the index will release all the indices for this key
   */
  virtual void ReleaseInternalPipeline(const char *key, int index = -1) ITK_OVERRIDE;

  /** Get the format of the image for display */
  virtual std::string GetPixelFormatDescription() ITK_OVERRIDE;


protected:

  /**
   * Default constructor.  Creates an image wrapper with a blank internal
   * image
   */
  ImageWrapper();

  /**
   * Copy constructor.  Copies the contents of the passed-in image wrapper.
   */
  ImageWrapper(const Self &copy);

  /** Destructor */
  virtual ~ImageWrapper();

  /** A unique Id of this wrapper. Used for the LayerAssociation code */
  unsigned long m_UniqueId;

  /**
   * A time-varying image is represented as a 4D image, although we also keep
   * an array of pointers to the individual 3D timepoints. This allows us to
   * use 4D image filters/operations when needed, and 3D at other times.
   */
  Image4DPointer m_Image4D;

  /**
   * Array of 3D image pointers referencing the 3D volumes in the 4D image
   */
  std::vector<ImagePointer> m_ImageTimePoints;

  /** The current time point (index into m_ImageTimePoints) */
  unsigned int m_TimePointIndex = 0;

  /** This image selector is used to pull out the current time point */
  typedef InputSelectionImageFilter<ImageType, unsigned int> TimePointSelectFilter;
  typedef SmartPtr<TimePointSelectFilter> TimePointSelectPointer;
  TimePointSelectPointer m_TimePointSelectFilter;

  /**
   * The currently selected 3D image from the 4D image. This is the output
   * of the time point select filter.
   */
  ImageType *m_Image;

  /**
   * Same as m_Image, but cast to ImageBase so that we can call functions
   * without knowing what the specific type of the wrapped image is
   */
  ImageBaseType *m_ImageBase;

  /**
   * The reference space - this is the 3D image into the space of which the
   * image is resliced. For the main image, this points to the image itself
   * and for overlays, it points to the main image
   */
  itk::WeakPointer<ImageBaseType> m_ReferenceSpace;

  /**
   * A flag indicating whether the image 'lives' in reference space. This
   * flag is true if the transform applid to the image is identity and if the
   * reference space and image space are the same
   */
  bool m_ImageSpaceMatchesReferenceSpace;

  /**
   * The affine transform between the reference space and the image. This transform
   * is manipulated during image registration
   */
  SmartPtr<ITKTransformType> m_AffineTransform;

  /** The current cursor position (slice index) in image dimensions */
  IndexType m_SliceIndex;

  /** The associated slicer filters */
  std::array<SlicerPointer, 3> m_Slicers;

  /**
   * Is the image wrapper initialized? That is a prerequisite for all
   * operations.
   */
  bool m_Initialized;

  /** Pipeline readiness */
  bool m_PipelineReady;

  /** Transparency */
  double m_Alpha;

  /** Stickiness (whether the layer can be tiled or not) */
  bool m_Sticky;

  /** Time when the internal image was allocated, and when it was last saved */
  itk::TimeStamp m_ImageAssignTime, m_ImageSaveTime;

  /** The pipeline that handles mapping intensities to the display slices */
  SmartPtr<DisplayMapping> m_DisplayMapping;

  /**
   * The native intensity mapping is a linear mapping from the format in which
   * the image is stored internally (e.g., 16-bit integer) to the values that
   * the user interacts with (e.g., float). For some images in ITK-SNAP, we
   * represent them internally as 16-bit integers even though they are meant
   * to represent floating point numbers in some range (e.g., -1 to 1). This
   * is to save memory.
   */
  NativeIntensityMapping m_NativeMapping;

  /**
   * This holds the transformation between the physical image coordinates (LPS)
   * and display coordinates (i.e., slices shown to the user). Through version
   * ITK-SNAP 4.0, this is an orthogonal transformation, but we intend to allow
   * non-orthogonal transformations as well.
   */
  IRISDisplayGeometry m_DisplayGeometry;

  /**
   * This object encapsulates information about the coordinate mapping between
   * the image coordinate space, the anatomical coordinate space, and the
   * display coordinate space. It depends on three parameters: the size of the
   * image, the Direction cosine matrix of the image, and the DisplayGeometry
   * (transformation between the display and the anatomy spaces).
   * The code should not directly modify this variable. It should only be
   * modified by the UpdateImageGeometry() method.
   */
  SmartPtr<ImageCoordinateGeometry> m_ImageGeometry;

  /**
   * The filter that computes the t-digest
   */
  SmartPtr<TDigestFilterType> m_TDigestFilter;

  /**
   * Internally cached transform from image coordinates to RAS (NIFTI) physical coordinates.
   * This is derived from the origin, spacing, and direction cosine matrix in the image header.
   */
  TransformType m_NiftiSform, m_NiftiInvSform;

  // Each layer has a filename, from which it is belived to have come
  std::string m_FileName, m_FileNameShort;

  // Each layer has a nickname. But this gets complicated, because the nickname
  // can be set by the user, or it can be default for the wrapper, or it can be
  // derived from the filename. The nicknames are used in the following order.
  // - if there is a custom nickname, it is shown
  // - if there is a filename, nickname is set to the shortened filename
  // - if there is no filename, nickname is set to the default nickname
  std::string m_DefaultNickname, m_CustomNickname;

  // Tags for this image layer
  TagList m_Tags;

  // IO Hints registry
  Registry *m_IOHints;

  /**
   * Storage for the internally-managed mini-pipelines, i.e., pipelines that
   * are created with CreateXYZPipeline() and removed with ReleaseXYZPipeline()
   */
  std::map< std::string, std::map<int, MiniPipeline> > m_ManagedPipelines;

  /** Internally used method to create a mini-pipeline */
  virtual void AddInternalPipeline(const MiniPipeline &mp, const char *key, int index);



  /**
   * Handle a change in the image data (i.e., a load operation on the image or
   * an initialization operation). This function can take two optional parameters:
   * the reference space and a transform. If these parameters are not NULL, then the
   * wrapper represents a spatially transformed image. The slicers in the wrapper will
   * slice not along the orthogonal directions in the image, but along directions in
   * the reference space.
   */
  virtual void UpdateWrappedImages(
      Image4DType *image_4d,
      ImageBaseType *refSpace = NULL,
      ITKTransformType *tran = NULL);

  /**
   * Update the image geometry (combining the information in the image and the
   * information in the m_DisplayGeometry variable)
   */
  virtual void UpdateImageGeometry();

  /**
   * Update the NIFTI header. This should be called whenever the spacing,
   * origin, or direction cosine matrix of the image are changed
   */
  virtual void UpdateNiftiTransforms();

  /**
   * Parent wrapper, this is used for scalar image wrappers derived from
   * vector image wrappers
   */
  ImageWrapperBase *m_ParentWrapper;

  // An internal array to store intensity samples for SampleIntensityAtReferenceIndex function
  mutable vnl_vector<ComponentType> m_IntensitySamplingArray;

  // Compare the geometry (size and header) of two images. Returns true if the headers are
  // within tolerance of each other.
  static bool CompareGeometry(ImageBaseType *image1, ImageBaseType *image2, double tol = 0.0);

  // Check if the orthogonal slicer can be used for the given image, ref space and transform
  static bool CanOrthogonalSlicingBeUsed(
      ImageType *image, ImageBaseType *referenceSpace, ITKTransformType *transform);

  /** Write the image to disk with whatever the internal format is */
  virtual void WriteToFileInInternalFormat(const char *filename, Registry &hints) ITK_OVERRIDE;

  /** Common code invoked when voxels in the image are changed */
  void OnVoxelsUpdated(unsigned int n_replaced);

  /** Sample voxels at a reference index and place sampled values in m_IntensitySamplingArray */
  void SampleIntensityAtReferenceIndexInternal(
      const itk::Index<3> &index, unsigned int tp_begin, unsigned int tp_end) const;

  /** Whether the current wrapper is of vector type or scalar type (for internal use) */
  typedef std::is_base_of<itk::VectorImage<ComponentType, 3>, ImageType> IsVector;

  /**
   * A type representing the 'concrete' image corresponding to the wrapped image. An image wrapper
   * may wrap something derived from an image, like an image adaptor, or a special kind of image,
   * like an RLEImage. A concrete image is either an itk::VectorImage or an itk::Image corresponding
   * to whatever is wrapped by this wrappper
   */
  typedef typename std::conditional<IsVector::value, ImageType, itk::Image<ComponentType, 3> >::type ConcreteImageType;

  /** A concrete image source, used to return pipelines */
  typedef itk::ImageSource<ConcreteImageType> ConcreteImageSource;

  /**
    Create a mini-pipeline that casts the current time point to the concrete image type
    */
  virtual std::pair<MiniPipeline, ConcreteImageType*> CreateCastToConcreteImagePipeline() const;
};

#endif // __ImageWrapper_h_
