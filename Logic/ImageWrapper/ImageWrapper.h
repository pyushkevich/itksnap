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
#include "ImageCoordinateGeometry.h"
#include <itkImageRegionIterator.h>
#include <itkVectorImage.h>
#include <itkRGBAPixel.h>
#include <DisplayMappingPolicy.h>
#include <itkSimpleDataObjectDecorator.h>

// Forward declarations to IRIS classes
template <class TInputImage, class TOutputImage> class IRISSlicer;
template <class TFunctor> class UnaryValueToValueFilter;

class SNAPSegmentationROISettings;
namespace itk {
  template <unsigned int VDimension> class ImageBase;
  template <class TImage> class ImageSource;
  template <typename TScalar, unsigned int V1, unsigned int V2> class Transform;
  template<typename TInputImage,
           typename TOutputImage,
           typename TInterpolatorPrecisionType,
           typename TTransformPrecisionType> class ResampleImageFilter;
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
template<class TTraits, class TBase = ImageWrapperBase>
class ImageWrapper : public TBase
{
public:

  // Standard ITK business
  typedef ImageWrapper<TTraits, TBase>                                    Self;
  typedef TBase                                                     Superclass;
  typedef SmartPtr<Self>                                               Pointer;
  typedef SmartPtr<const Self>                                    ConstPointer;
  itkTypeMacro(ImageWrapper, TBase)

  // Basic type definitions
  typedef typename Superclass::ImageBaseType                     ImageBaseType;
  typedef typename TTraits::ImageType                                ImageType;
  typedef SmartPtr<ImageType>                                     ImagePointer;
  typedef typename ImageType::PixelType                              PixelType;

  // This is the pixel type of the buffer pointer, i.e., internal representation
  typedef typename ImageType::InternalPixelType              InternalPixelType;

  // This is the type of the outwardly visible components in the image. For
  // wrappers that encapsulate 'real' images and vector images, this is the same
  // as the internal pixel type, but for wrappers that encapsulate an image
  // adaptor, the component type is the output type of the adaptor
  typedef typename TTraits::ComponentType                        ComponentType;

  // Slice image type
  typedef itk::Image<PixelType,2>                                    SliceType;
  typedef SmartPtr<SliceType>                                     SlicePointer;

  // Display slice type - inherited
  typedef typename Superclass::DisplayPixelType               DisplayPixelType;
  typedef typename Superclass::DisplaySliceType               DisplaySliceType;
  typedef typename Superclass::DisplaySlicePointer         DisplaySlicePointer;

  // Slicer type
  typedef IRISSlicer<ImageType, SliceType>                          SlicerType;
  typedef SmartPtr<SlicerType>                                   SlicerPointer;

  // Preview source for preview pipelines
  typedef itk::ImageSource<ImageType>                        PreviewFilterType;

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


  /**
   * Get the parent wrapper for this wrapper. For 'normal' wrappers, this method
   * returns NULL, indicating that the wrapper is a top-level wrapper. For derived
   * wrappers (i.e., components and scalar representations of vector wrappers),
   * this method returns the vector wrapper from which the wrapper is derived.
   *
   * You should not have to call the SetParentWrapper method. It's used internally
   */
  irisGetSetMacro(ParentWrapper, ImageWrapperBase *)

  /**
   * Initialize the image wrapper to match another image wrapper, setting the
   * image pixels to a default value. The slice indices and the transforms will
   * all be updated to match the source
   */
  virtual void InitializeToWrapper(
    const ImageWrapperBase *source, const PixelType &value);

  /** 
   * Initialize the image wrapper with a combination of another image wrapper and
   * an actual image. This will update the indices and transforms from the 
   * source wrapper, otherwise, it's equivalent to SetImage()
   */ 
  virtual void InitializeToWrapper(
    const ImageWrapperBase *source, ImageType *image, ImageBaseType *refSpace, ITKTransformType *tran);

  /**
    Get a unique id for this wrapper. All wrappers ever created have
    different ids.
    */
  irisGetMacro(UniqueId, unsigned long)

  /**
   * Clear the data associated with storing an image
   */
  virtual void Reset();

  /** Get the coordinate transform for each display slice */
  virtual const ImageCoordinateTransform &GetImageToDisplayTransform(
    unsigned int) const;

  /**
   * Set the coordinate transformation between the display coordinates and
   * the anatomical coordinates. This affects the behavior of the slicers
   */
  virtual void SetDisplayGeometry(IRISDisplayGeometry &dispGeom);

  /** Get the display to anatomy coordinate mapping */
  irisGetMacro(DisplayGeometry, const IRISDisplayGeometry &)

  /** Set the direction matrix of the image */
  virtual void SetDirectionMatrix(const vnl_matrix<double> &direction);

  /**
   * Set the image coordinate transform (origin, spacing, direction) to
   * match those of a reference wrapper
   */
  virtual void CopyImageCoordinateTransform(const ImageWrapperBase *source);

  /**
   * Get the image geometry from the wrapper
   */
  irisGetMacro(ImageGeometry, const ImageCoordinateGeometry &)


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
  irisSetWithEventMacro(Alpha, double, WrapperDisplayMappingChangeEvent)

  /** Set layer transparency */
  irisGetMacro(Alpha, double)

  /**
   * Get layer stickiness. A sticky layer always is shown 'on top' of other
   * layers, e.g., the segmentation layer, or the level set image. A layer that
   * is not sticky is shown in its own tile when the display is in tiled mode
   */
  irisSetWithEventMacro(Sticky, bool, WrapperVisibilityChangeEvent)

  /** Set layer stickiness */
  irisIsMacro(Sticky)

  /**
   * Whether the layer is drawable. Some layers may be initialized, but not
   * yet computed, in which case they should not yet be drawn.
   */
  virtual bool IsDrawable() const;

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

  /** Get the inverse NIFTI s-form matrix for this image */
  irisGetMacro(NiftiInvSform, TransformType)

  /** Set the voxel at a given position.*/
  void SetVoxel(const Vector3ui &index, const PixelType &value);
  void SetVoxel(const itk::Index<3> &index, const PixelType &value);

  /**
   * Get a constant reference to a voxel at a given position.
   */
  PixelType GetVoxel(unsigned int x,
                     unsigned int y,
                     unsigned int z) const;

  PixelType GetVoxel(const Vector3ui &index) const;

  PixelType GetVoxel(const itk::Index<3> &index) const;

  /**
   * Get the mapping between the internal data type and the 'native' range,
   * i.e., the range of values shown to the user. This may be a linear mapping
   * or an identity mapping. This method returns an abstract type;
   */
  virtual const AbstractNativeIntensityMapping *GetNativeIntensityMapping() const
    { return &m_NativeMapping; }

  /** These methods access the native mapping in its actual type */
  irisGetMacro(NativeMapping, NativeIntensityMapping)
  irisSetMacro(NativeMapping, NativeIntensityMapping)

  /** Get the intensity to display mapping */
  DisplayMapping *GetDisplayMapping()
    { return m_DisplayMapping; }

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
   * Get an ITK pipeline object holding the minimum value in the image. For
   * multi-component images, this is the minimum value over all components.
   */
  virtual ComponentTypeObject *GetImageMinObject() const = 0;
  virtual ComponentTypeObject *GetImageMaxObject() const = 0;

  /** Return componentwise minimum cast to double, without mapping to native range */
  virtual double GetImageMinAsDouble();

  /** Return componentwise maximum cast to double, without mapping to native range */
  virtual double GetImageMaxAsDouble();

  /** Return componentwise minimum cast to double, after mapping to native range */
  virtual double GetImageMinNative();

  /** Return componentwise maximum cast to double, after mapping to native range */
  virtual double GetImageMaxNative();

  /**
   * Get a slice of the image in a given direction
   */
  virtual SliceType *GetSlice(unsigned int dimension);

  /**
   * This method exposes the scalar pointer in the image
   */
  virtual InternalPixelType *GetVoxelPointer() const;

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
   * Set the wrapper to hold an image that is in a coordinate space that is
   * different from the program's main reference space
   */
  virtual void SetImage(ImagePointer newImage, ImageBaseType *refSpace, ITKTransformType *transform);

  /**
   * Update the transform between the coordinate space of this image and the program's
   * main reference space
   */
  virtual void SetITKTransform(ImageBaseType *referenceSpace, ITKTransformType *transform);

  /**
   * Extract a region of interest from the image wrapper, as a new wrapper of
   * the same type
   */
  virtual SmartPtr<ImageWrapperBase> ExtractROI(
      const SNAPSegmentationROISettings &roi, itk::Command *progressCommand) const;


  /**
   * This method is used to perform a deep copy of a region of this image 
   * into another image, potentially resampling the region to use a different
   * voxel size
   */
  virtual ImagePointer DeepCopyRegion(const SNAPSegmentationROISettings &roi,
                              itk::Command *progressCommand = NULL) const;


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
  virtual unsigned int ReplaceIntensity(PixelType iOld, PixelType iNew);

  /** 
   * Swap intensity values iFirst and iSecond
   * \return number of voxels that had been modified
   */
  virtual unsigned int SwapIntensities(PixelType iFirst, PixelType iSecond);

  /**
   * Get the display slice
   */
  DisplaySlicePointer GetDisplaySlice(unsigned int dim);

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
  irisIsMacro(PipelineReady)
  irisSetMacro(PipelineReady, bool)


  virtual void* GetVoxelVoidPointer() const;

  /**
   * Set the filename of the image wrapper. If the wrapper does not have a
   * nickname, the nickname will be changed to the file part of the filename.
   */
  void SetFileName(const std::string &name);


  // Access the filename
  irisGetStringMacro(FileName)

  /**
   * Fallback nickname - shown if no filename and no custom nickname set.
   */
  irisGetSetMacro(DefaultNickname, const std::string &)

  /**
   * Get the nickname of the image. A nickname is a shorter description of the
   * image that is displayed to the user. If a custom nickname is not set, it
   * defaults to the filename (without path). If there is no filename (i.e.,
   * the layer is internal), the default nickname is used.
   */
  const std::string &GetNickname() const;

  /**
   * Set the custom nickname for the wrapper.
   */
  virtual void SetCustomNickname(const std::string &nickname);
  irisGetMacro(CustomNickname, const std::string &);


  /**
   * Write the image to disk with the help of the GuidedNativeImageIO object
   */
  virtual void WriteToFile(const char *filename, Registry &hints);

  /**
   * Create a thumbnail from the image and write it to a .png file
   */
  void WriteThumbnail(const char *filename, unsigned int maxdim);

  /**
   * Save metadata to a Registry file. The metadata are data that are not
   * contained in the image header are need to be restored when the image
   * is reloaded. Currently, this mainly includes the display mapping, but
   * also the transparency, etc.
   */
  virtual void WriteMetaData(Registry &reg);

  /**
   * Restore metadata from a registry
   */
  virtual void ReadMetaData(Registry &reg);

  /**
   * Check if the image has unsaved changes
   */
  virtual bool HasUnsavedChanges() const;

  /**
   * The image wrapper has a generic mechanism for associating data with it.
   * For example, we can associate some parameter values for a specific
   * image processing algorithm with each layer. Do do that, we simply
   * assign a pointer to the data to a specific string role. Internally,
   * a smart pointer is used to point to the associated data.
   *
   * Users of this method might also want to rebroadcast events from the
   * associated object as events of type WrapperUserChangeEvent(). These
   * events will then propagate all the way up to the IRISApplication.
   */
  void SetUserData(const std::string &role, itk::Object *data);

  /**
   * Get the user data associated with this wrapper for a specific role. If
   * no association exists, NULL is returned. The method is templated over the
   * return type to avoid casting in user code.
   */
  itk::Object* GetUserData(const std::string &role) const;

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

  /** The image that we are wrapping */
  ImagePointer m_Image;

  /** The associated slicer filters */
  SlicerPointer m_Slicer[3];

  /** The wrapped image */
  SmartPtr<ImageBaseType> m_ImageBase;

  /** The reference space - this is the space into which the image is sliced */
  SmartPtr<ImageBaseType> m_ReferenceSpace;

  /** The current cursor position (slice index) in image dimensions */
  Vector3ui m_SliceIndex;

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

  // Mapping from native to internal format
  NativeIntensityMapping m_NativeMapping;

  // Mapping between the display coordinates and anatomical coordinates
  IRISDisplayGeometry m_DisplayGeometry;

  // This object encapsulates information about the coordinate mapping between
  // the image coordinate space, the anatomical coordinate space, and the
  // display coordinate space. It depends on three parameters: the size of the
  // image, the Direction cosine matrix of the image, and the DisplayGeometry
  // (transformation between the display and the anatomy spaces).
  //
  // The code should not directly modify this variable. It should only be
  // modified by the UpdateImageGeometry() method.
  ImageCoordinateGeometry m_ImageGeometry;

  // Transform from image index to NIFTI world coordinates. This is derived
  // from the origin, spacing, and direction cosine matrix in the image header.
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

  // A map to store user-associated data
  typedef std::map<std::string, SmartPtr<itk::Object> > UserDataMapType;
  UserDataMapType m_UserDataMap;


  /**
   * Handle a change in the image pointer (i.e., a load operation on the image or 
   * an initialization operation). This function can take two optional parameters:
   * the reference space and a transform. If these parameters are not NULL, then the
   * wrapper represents a spatially transformed image. The slicers in the wrapper will
   * slice not along the orthogonal directions in the image, but along directions in
   * the reference space.
   */
  virtual void UpdateImagePointer(ImageType *image,
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

  /** Common code for the different constructors */
  void CommonInitialization();

  /** Parent wrapper */
  ImageWrapperBase *m_ParentWrapper;


  typedef itk::ResampleImageFilter<ImageType, ImageType, double, double> ResampleFilter;
  SmartPtr<ResampleFilter> m_ResampleFilter[3];
};

#endif // __ImageWrapper_h_
