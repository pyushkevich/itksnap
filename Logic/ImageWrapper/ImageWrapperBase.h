#ifndef IMAGEWRAPPERBASE_H
#define IMAGEWRAPPERBASE_H

#include "ImageCoordinateTransform.h"
#include "itkImageRegion.h"
#include "WrapperBase.h"
#include "vtkSmartPointer.h"

namespace itk {
  template <unsigned int VDim> class ImageBase;
  template <class TPixel, unsigned int VDim> class Image;
  template <class TPixel, unsigned int VDim> class VectorImage;
  template <class TPixel> class RGBAPixel;
  template <class TOutputImage> class ImageSource;
  template <class T, unsigned int VDim1, unsigned int VDim2> class Transform;
  template <class TCoordRep, unsigned int VDim> class ContinuousIndex;
  class DataObject;
  class ProcessObject;

  namespace Statistics {
    class DenseFrequencyContainer;
    template <class TReal, unsigned int VDim, class TContainer> class Histogram;
  }
}

class ScalarImageWrapperBase;
class VectorImageWrapperBase;
class IntensityCurveInterface;
class ScalarImageHistogram;
class ColorMap;
class ImageCoordinateGeometry;
class AbstractNativeIntensityMapping;
class AbstractDisplayMappingPolicy;
class SNAPSegmentationROISettings;
class GuidedNativeImageIO;
class Registry;
class vtkImageImport;
struct IRISDisplayGeometry;
class TDigestDataObject;


template <unsigned int VDim> class MetaDataAccess;

/**
 * Supported ways of extracting a scalar value from vector-valued data.
 * These modes allow the image to be cast to a scalar image and used in
 * single-modality pipelines
 */
enum ScalarRepresentation
{
  SCALAR_REP_COMPONENT = 0,
  SCALAR_REP_MAGNITUDE,
  SCALAR_REP_MAX,
  SCALAR_REP_AVERAGE,
  NUMBER_OF_SCALAR_REPS
};

/**
 * Volume rendering modes. These modes multiply the transfer function used for 2D
 * display mapping by either 1 or a linear function from 0 to 1. By default, this
 * is set to off.
 */
enum VolumeRenderingTransferFunctionScalingMode
{
  VOLUME_RENDERING_LINEAR01 = 0,
  VOLUME_RENDERING_LINEAR10,
  VOLUME_RENDERING_CONST1
};

/**
 \class ImageWrapperBase
 \brief Abstract parent class for all image wrappers

 This class is at the head of the ImageWrapper hierarchy. In fact, there are
 two parallel hierarchies: the untyped hierarchy (xxxWrapperBase) and the
 hierarchy templated over a type (xxxWrapper).

 The idea is that most SNAP code will work with the untyped hierarches. Thus,
 the code will not know what the underlying format of the image is. The typed
 hierarchy is invisible to most of the SNAP classes, and accessed on special
 occasions, where the raw data of the image is needed.
 */
class ImageWrapperBase : public WrapperBase
{
public:

  // Definition for the display slice type
  typedef itk::RGBAPixel<unsigned char>                       DisplayPixelType;
  typedef itk::Image<DisplayPixelType,2>                      DisplaySliceType;
  typedef SmartPtr<DisplaySliceType>                       DisplaySlicePointer;

  // Image base
  typedef itk::ImageBase<3> ImageBaseType;
  typedef itk::ImageBase<4> Image4DBaseType;

  // Floating point images and sources to which data may be cast
  typedef itk::Image<float, 3>                                  FloatImageType;
  typedef itk::VectorImage<float, 3>                      FloatVectorImageType;
  typedef itk::Image<float, 2>                                  FloatSliceType;
  typedef itk::VectorImage<float, 2>                      FloatVectorSliceType;

  // Transform matrices
  typedef vnl_matrix_fixed<double, 4, 4>                         TransformType;

  // ITK's coordinate transform (rigid, affine, etc)
  typedef itk::Transform<double, 3, 3>                        ITKTransformType;

  // Metadata access object
  typedef MetaDataAccess<4>                                 MetaDataAccessType;

  // Index and size types
  typedef itk::Index<3>                                              IndexType;
  typedef itk::Size<3>                                                SizeType;

  // Interpolation mode enum
  enum InterpolationMode { NEAREST=0, LINEAR };

  /** This class is used to store mini-pipeline created by this class */
  struct MiniPipeline
  {
    // A list of named filters in the pipeline
    std::list< itk::SmartPointer<itk::ProcessObject> > filters;

    // Output of the pipeline
    itk::SmartPointer<itk::DataObject> output;
  };

  /**
   * The image wrapper fires a WrapperMetadataChangeEvent when properties
   * such as nickname are modified. It fires a WrapperDisplayMappingChangeEvent
   * when the factors affecting the mapping from internal data to the slice
   * display (e.g., color map) are modified.
   */
  FIRES(WrapperMetadataChangeEvent)
  FIRES(WrapperDisplayMappingChangeEvent)

  virtual ~ImageWrapperBase() { }

  /**
   * Every wrapper, whether it is a scalar wrapper or a vector wrapper, has a
   * scalar representation. For scalar wrappers, this function just returns a
   * pointer to itself. For vector wrappers, the behavior of this function
   * depends on which scalar representation has been selected as the default
   * scalar representation (e.g., one of the components, magnitude, max, mean).
   */
  virtual ScalarImageWrapperBase *GetDefaultScalarRepresentation() = 0;

  /**
   * Get the parent wrapper for this wrapper. For 'normal' wrappers, this method
   * returns NULL, indicating that the wrapper is a top-level wrapper. For derived
   * wrappers (i.e., components and scalar representations of vector wrappers),
   * this method returns the vector wrapper from which the wrapper is derived
   */
  virtual ImageWrapperBase *GetParentWrapper() const = 0;

  /** Set the parent wrapper */
  virtual void SetParentWrapper(ImageWrapperBase *parent) = 0;

  /** Get the coordinate transform for each display slice */
  virtual const ImageCoordinateTransform *GetImageToDisplayTransform(
    unsigned int) const = 0;

  /**
   * Set the coordinate transformation between the display coordinates and
   * the anatomical coordinates. This affects the behavior of the slicers
   */
  virtual void SetDisplayGeometry(const IRISDisplayGeometry &dispGeom) = 0;

  /** Get the display to anatomy coordinate mapping */
  virtual const IRISDisplayGeometry &GetDisplayGeometry() const = 0;

  /** Set the direction matrix of the image */
  virtual void SetDirectionMatrix(const vnl_matrix<double> &direction) = 0;

  /**
   * Set the image coordinate transform (origin, spacing, direction) to
   * match those of a reference wrapper
   */
  virtual void CopyImageCoordinateTransform(const ImageWrapperBase *source) = 0;

  /**
   * Get the image geometry from the wrapper. The image geometry captures
   * the transforms between each of the display slices and the 3D image.
   */
  virtual const ImageCoordinateGeometry *GetImageGeometry() const = 0;

  /** Get the current slice index */
  irisVirtualGetMacro(SliceIndex, IndexType)

  /**
   * Set the current slice index in all three dimensions.  The index should
   * be specified in the image coordinates, the slices will be generated
   * in accordance with the transforms that are specified
   */
  virtual void SetSliceIndex(const IndexType &) = 0;

  /** Get the number of time points (how many 3D images in 4D array) */
  irisVirtualGetMacro(NumberOfTimePoints, unsigned int)

  /** Get the time index (which 3D volume in the 4D array is currently shown) */
  irisVirtualGetMacro(TimePointIndex, unsigned int)

  /** Set the current time index */
  virtual void SetTimePointIndex(unsigned int index) = 0;

  /**
   * Set the viewport rectangle onto which the three display slices
   * will be rendered
   */
  virtual void SetDisplayViewportGeometry(
      unsigned int index,
      const ImageBaseType *viewport_image) = 0;


  /** Return some image info independently of pixel type */
  irisVirtualGetMacro(ImageBase, ImageBaseType *)

  /** Return some image info independently of pixel type */
  irisVirtualGetMacro(Image4DBase, Image4DBaseType *)

  /**
   * If the image wrapper is an output of a preview pipeline, is the pipeline ready?
   */
  irisVirtualIsMacro(PipelineReady)
  irisVirtualSetMacro(PipelineReady, bool)

  /** Is this image of scalar type? */
  virtual bool IsScalar() const = 0;

  /**
   * Get the size of the image
   */
  virtual Vector3ui GetSize() const = 0;



  /**
   * Get layer stickiness. A sticky layer always is shown 'on top' of other
   * layers, e.g., the segmentation layer, or the level set image. A layer that
   * is not sticky is shown in its own tile when the display is in tiled mode
   */
  irisVirtualSetMacro(Sticky, bool)

  /** Set layer stickiness */
  irisVirtualIsMacro(Sticky)

  /**
   * Whether the layer is drawable. Some layers may be initialized, but not
   * yet computed, in which case they should not yet be drawn.
   */
  irisVirtualIsMacro(Drawable)

  /**
   * Whether the layer is initialized to use orthogonal slicing or non-orthogonal
   * slicing. There are two slicing pipelines, one for the images whose slicing
   * directions are parallel to the display planes, and one for the opposite case.
   */
  irisVirtualIsMacro(SlicingOrthogonal)

  /**
   * Get the buffered region of the image
   */
  virtual itk::ImageRegion<3> GetBufferedRegion() const = 0;

  /**
   * Extract a region of interest from the image wrapper, as a new wrapper of
   * the same type
   */
  virtual SmartPtr<ImageWrapperBase> ExtractROI(
      const SNAPSegmentationROISettings &roi, itk::Command *progressCommand) const = 0;

  /**
   * Extract a 3d region of interest from all time points in the image wrapper,
   * as a new wrapper of the same type
   */
  virtual SmartPtr<ImageWrapperBase> ExtractROI4D(
      const SNAPSegmentationROISettings &roi, itk::Command *progressCommand) const = 0;

  /** Transform a voxel index into a spatial position */
  virtual Vector3d TransformVoxelIndexToLPSCoordinates(const Vector3i &iVoxel) const = 0;

  /** Transform a voxel index into a spatial position */
  virtual Vector3d TransformVoxelCIndexToLPSCoordinates(const Vector3d &iVoxel) const = 0;

  /** Transform spatial position to voxel continuous index (LPS) */
  virtual Vector3d TransformLPSCoordinatesToVoxelCIndex(const Vector3d &vLPS) const = 0;

  /** Transform spatial position to voxel index (LPS) */
  virtual Vector3i TransformLPSCoordinatesToVoxelIndex(const Vector3d &vLPS) const = 0;

  /** Transform a voxel index into NIFTI coordinates (RAS) */
  virtual Vector3d TransformVoxelCIndexToNIFTICoordinates(const Vector3d &iVoxel) const = 0;

  /** Transform NIFTI coordinates to a continuous voxel index */
  virtual Vector3d TransformNIFTICoordinatesToVoxelCIndex(const Vector3d &vNifti) const = 0;

  /**
   * Transform a reference space index to a continuous index in the voxel space of
   * the wrapped image. For images whose geometry matches the reference space, this
   * is the identity transform, but for images that are not in referene space, this
   * will use the S-form of the reference space, S-form of the wrapped image and the
   * registration transform applied to the image to compute the coordinate.
   */
  virtual void TransformReferenceCIndexToWrappedImageCIndex(
      const itk::ContinuousIndex<double, 3> &ref_index, itk::ContinuousIndex<double, 3> &img_index) const = 0;

  /** Get the NIFTI s-form matrix for this image */
  irisVirtualGetMacro(NiftiSform, TransformType)

  /** Get the inverse NIFTI s-form matrix for this image */
  irisVirtualGetMacro(NiftiInvSform, TransformType)

  /** Get a display slice correpsponding to the current index */
  virtual DisplaySlicePointer GetDisplaySlice(unsigned int dim) = 0;

  /** For each slicer, find out which image dimension does is slice along */
  virtual unsigned int GetDisplaySliceImageAxis(unsigned int slice) = 0;

  /** Get the number of voxels */
  virtual size_t GetNumberOfVoxels() const = 0;

  /** Get the number of components per voxel */
  virtual size_t GetNumberOfComponents() const = 0;

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
      bool map_to_native, vnl_vector<double> &out) const = 0;

  /**
   * Does the image match the reference space? If true, the wrapped image and the
   * reference image have the same 3D geometry (size, origin, spacing, direction).
   */
  virtual bool ImageSpaceMatchesReferenceSpace() const = 0;

  /** Return componentwise minimum cast to double, without mapping to native range */
  virtual double GetImageMinAsDouble() = 0;

  /** Return componentwise maximum cast to double, without mapping to native range */
  virtual double GetImageMaxAsDouble() = 0;

  /** Return componentwise minimum cast to double, after mapping to native range */
  virtual double GetImageMinNative() = 0;

  /** Return componentwise maximum cast to double, after mapping to native range */
  virtual double GetImageMaxNative() = 0;

  /** Compute statistics over a run of voxels in the image starting at the index
   * startIdx. Appends the statistics to a running sum and sum of squared. The
   * statistics are returned in internal (not native mapped) format */
  virtual void GetRunLengthIntensityStatistics(
      const itk::ImageRegion<3> &region,
      const itk::Index<3> &startIdx, long runlength,
      double *out_nvalid, double *out_sum, double *out_sumsq) const = 0;

  /**
   * Get current interpolation mode
   */
  virtual InterpolationMode GetSlicingInterpolationMode() const = 0;

  /**
   * Set interpolation mode for non-orthogonal slicing
   */
  virtual void SetSlicingInterpolationMode(InterpolationMode mode) = 0;

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
      vnl_vector<double> &out_value, DisplayPixelType &out_appearance) = 0;

  /**
   * This method samples a 2D slice based on a reference geometry from
   * the current image and maps it using the current display mapping. It
   * is used to generate thumbnails and for other sampling of image
   * appearance that is outside of the main display pipeline
   */
  virtual DisplaySlicePointer SampleArbitraryDisplaySlice(const ImageBaseType *ref_space) = 0;

  typedef std::vector< std::pair<int, int> > PatchOffsetTable;

  /**
   * Support for random access sampling of patches from the image. This method
   * generates a set of offsets in the image that can be used efficiently to
   * sample patches from the image.
   */
  virtual PatchOffsetTable GetPatchOffsetTable(const SizeType &radius) const = 0;

  /**
   * Sample the image patch around a pixel location. No bounds checking is done,
   * so it is assumed that the location has enough margin to sample from. It is
   * also assumed that the output vector has been allocated already
   */
  virtual void SamplePatchAsDouble(const IndexType &idx, const PatchOffsetTable &offset_table,
                                   double *out_patch) const = 0;

  /** Clear the data associated with storing an image */
  virtual void Reset() = 0;

  /**
   * Get the mapping between the internal data type and the 'native' range,
   * i.e., the range of values shown to the user. This may be a linear mapping
   * or an identity mapping.
   */
  virtual const AbstractNativeIntensityMapping *GetNativeIntensityMapping() const = 0;

  /**
    Export one of the slices as a thumbnail (e.g., PNG file)
    */
  virtual DisplaySlicePointer MakeThumbnail(unsigned int maxdim) = 0;

  /**
   * Access the "IO hints" registry associated with this wrapper. The IO hints
   * are used to help read the image when the filename alone is not sufficient.
   * For example, it may contain the DICOM series ID of the image, or for a raw
   * image the dimensions.
   */
  virtual const Registry &GetIOHints() const = 0;

  /**
   * Set the IO hints
   */
  virtual void SetIOHints(const Registry &io_hints) = 0;

  /**
   * Write the image to disk with the help of the GuidedNativeImageIO object
   */
  virtual void WriteToFile(const char *filename, Registry &hints) = 0;

  /**
   * Check if the image has unsaved changes
   */
  virtual bool HasUnsavedChanges() const = 0;

  /**
   * Check if the time point has unsaved changes
   */
  virtual bool HasUnsavedChanges(unsigned int tp) const = 0;

  /**
   * Save metadata to a Registry file. The metadata are data that are not
   * contained in the image header are need to be restored when the image
   * is reloaded. Currently, this mainly includes the display mapping, but
   * also the transparency, etc.
   */
  virtual void WriteMetaData(Registry &reg) = 0;

  /**
   * Restore metadata from a registry
   */
  virtual void ReadMetaData(Registry &reg) = 0;

  /**
   * Get the meta data accessor object, useful for inspecting metadata
   */
  virtual MetaDataAccessType GetMetaDataAccess() = 0;

  /**
   * This static function constructs a NIFTI matrix from the ITK direction
   * cosines matrix and Spacing and Origin vectors
   */
  static TransformType ConstructNiftiSform(
    vnl_matrix<double> m_dir,
    vnl_vector<double> v_origin,
    vnl_vector<double> v_spacing);

  static TransformType ConstructVTKtoNiftiTransform(
    vnl_matrix<double> m_dir,
    vnl_vector<double> v_origin,
    vnl_vector<double> v_spacing);

  /**
   * Set an ITK transform between this image and a reference image.
   */
  virtual void SetITKTransform(ImageBaseType *referenceSpace, ITKTransformType *transform) = 0;

  /**
   * Set the reference image without changing the transform
   */
  virtual void SetReferenceSpace(ImageBaseType *referenceSpace) = 0;

  /**
   * Get the ITK transform between this image and the reference space
   */
  virtual const ITKTransformType *GetITKTransform() const = 0;

  /**
   * Get the reference space space in which this image is defined
   */
  virtual ImageBaseType* GetReferenceSpace() const = 0;

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
  virtual FloatImageType* CreateCastToFloatPipeline(const char *key, int index = 0) = 0;

  /** Same as CreateCastToFloatPipeline, but for vector images of single dimension */
  virtual FloatVectorImageType* CreateCastToFloatVectorPipeline(const char *key, int index = 0) = 0;

  /** Create a pipeline for casting an image slice to floating point */
  virtual FloatSliceType* CreateCastToFloatSlicePipeline(const char *key, unsigned int slice) = 0;

  /** Create a pipeline for casting an image slice to floating point vector image */
  virtual FloatVectorSliceType* CreateCastToFloatVectorSlicePipeline(const char *key, unsigned int slice) = 0;

  /**
   * Release the filters and images in an internally managed pipeline. Passing -1 for
   * the index will release all the indices for this key
   */
  virtual void ReleaseInternalPipeline(const char *key, int index = -1) = 0;

  /** Get the format of the image for display */
  virtual std::string GetPixelFormatDescription() = 0;

protected:

  /** Write the image to disk with whatever the internal format is */
  virtual void WriteToFileInInternalFormat(const char *filename, Registry &hints) = 0;

  /** Write the image to disk as a floating point image (scalar or vector) */
  virtual void WriteToFileAsFloat(const char *filename, Registry &hints) = 0;

};

class ScalarImageWrapperBase : public virtual ImageWrapperBase
{
public:

  /** A data type representing a pipeline for exporting to VTK */
  struct VTKImporterMiniPipeline
  {
    ImageWrapperBase::MiniPipeline cast_pipeline;
    SmartPtr<itk::ProcessObject> exporter;
    vtkSmartPointer<vtkImageImport> importer;
  };

  /**
   * An enum of export channel types. Export channels are used to present the
   * internal image as an itk::Image of a fixed type. For efficient memory
   * management, there are separate channels for downstream filters that
   * operate on the whole image and filters that generate single-slice previews
   * in the orthogonal slicing directions
   */
  enum ExportChannel {
    WHOLE_IMAGE=0, PREVIEW_X, PREVIEW_Y, PREVIEW_Z, CHANNEL_COUNT
  };

  /**
    Get the maximum possible value of the gradient magnitude. This will
    compute the gradient magnitude of the image (without Gaussian smoothing)
    and return the maximum. The value will be cached so repeated calls to
    this are not expensive.
    */
  virtual double GetImageGradientMagnitudeUpperLimit() = 0;

  /**
    Get the maximum possible value of the gradient magnitude in native units
    */
  virtual double GetImageGradientMagnitudeUpperLimitNative() = 0;

  /**
   * Get the intensity curve used to map raw intensities to color map inputs.
   * The intensity curve is only used by some wrappers (anatomic, speed) and
   * so this method may return NULL for some layers.
   */
  virtual IntensityCurveInterface *GetIntensityCurve() const = 0;

  /**
   * Get the color map used to present image intensities as RGBA.
   */
  virtual ColorMap *GetColorMap() const = 0;

  /**
   * Create a mini-pipeline that can be used to import the image to VTK. Like
   * other pipelines created with Create..., this is meant for temporary use
   * since the pipeline may have to allocate large amounts of memory and we'
   * don't want this memory lingering around when it is not used
   */
  virtual VTKImporterMiniPipeline CreateVTKImporterPipeline() const = 0;

  /** Is volume rendering turned on for this layer */
  virtual bool IsVolumeRenderingEnabled() const = 0;

  /** Turn on volume rendering for this layer */
  virtual void SetVolumeRenderingEnabled(bool state) = 0;
};




/**
 * A class that can be used to iterate over scalar representations.
 * Within some of the scalar representations (for now just SCALAR_REP_COMPONENT)
 * there are multiple indexed scalar components. The iterator iterates over the
 * components before proceeding to the next component.
 */
class ScalarRepresentationIterator
{
public:
  ScalarRepresentationIterator(const VectorImageWrapperBase *wrapper);

  ScalarRepresentationIterator& operator ++();
  bool IsAtEnd() const;

  irisGetMacro(Index, int)

  ScalarRepresentation GetCurrent() const
    { return static_cast<ScalarRepresentation>(m_Current); }

protected:
  int m_Current;
  int m_Index;

  // Depth of each scalar representation
  std::vector<int> m_Depth;

  friend class VectorImageWrapperBase;
};

/**
 * A base class for wrappers around vector-valued images
 */
class VectorImageWrapperBase : public virtual ImageWrapperBase
{
public:

  /**
   * Get a pointer to the given scalar representation of this vector image.
   */
  virtual ScalarImageWrapperBase *GetScalarRepresentation(
      ScalarRepresentation type, int index = 0) = 0;


  /**
   * Access a scalar representation using an iterator
   */
  virtual ScalarImageWrapperBase *GetScalarRepresentation(
      const ScalarRepresentationIterator &it) = 0;

  /**
   * If scalar_rep is a scalar representation of the vector image wrapper, find
   * the type of the representation and the index. Otherwise return false;
   */
  virtual bool FindScalarRepresentation(
      ImageWrapperBase *scalar_rep, ScalarRepresentation &type, int &index) const = 0;

};


#endif // IMAGEWRAPPERBASE_H
