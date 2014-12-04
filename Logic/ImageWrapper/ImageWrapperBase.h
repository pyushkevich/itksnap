#ifndef IMAGEWRAPPERBASE_H
#define IMAGEWRAPPERBASE_H

#include "SNAPCommon.h"
#include "ImageCoordinateTransform.h"
#include "itkImageRegion.h"
#include "itkObject.h"
#include "SNAPEvents.h"

namespace itk {
  template <unsigned int VDim> class ImageBase;
  template <class TPixel, unsigned int VDim> class Image;
  template <class TPixel> class RGBAPixel;
  template <class TOutputImage> class ImageSource;
  template <class TScalar, unsigned int V1, unsigned int V2> class Transform;

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
class ImageWrapperBase : public itk::Object
{
public:

  // Definition for the display slice type
  typedef itk::RGBAPixel<unsigned char>                       DisplayPixelType;
  typedef itk::Image<DisplayPixelType,2>                      DisplaySliceType;
  typedef SmartPtr<DisplaySliceType>                       DisplaySlicePointer;

  // Image base
  typedef itk::ImageBase<3> ImageBaseType;

  // Transform matrices
  typedef vnl_matrix_fixed<double, 4, 4>                         TransformType;

  // ITK's coordinate transform (rigid, affine, etc)
  typedef itk::Transform<double, 3, 3>                        ITKTransformType;

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
    Get a unique id for this wrapper. All wrappers ever created have
    different ids.
    */
  virtual unsigned long GetUniqueId() const = 0;

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
  virtual const ImageCoordinateTransform &GetImageToDisplayTransform(
    unsigned int) const = 0;

  /**
   * Set the coordinate transformation between the display coordinates and
   * the anatomical coordinates. This affects the behavior of the slicers
   */
  virtual void SetDisplayGeometry(IRISDisplayGeometry &dispGeom) = 0;

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
  virtual const ImageCoordinateGeometry &GetImageGeometry() const = 0;

  /** Get the current slice index */
  irisVirtualGetMacro(SliceIndex, Vector3ui)

  /**
   * Set the current slice index in all three dimensions.  The index should
   * be specified in the image coordinates, the slices will be generated
   * in accordance with the transforms that are specified
   */
  virtual void SetSliceIndex(const Vector3ui &) = 0;

  /** Return some image info independently of pixel type */
  irisVirtualGetMacro(ImageBase, ImageBaseType *)

  /**
   * Is the image initialized?
   */
  irisVirtualIsMacro(Initialized)

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

  /** Get layer transparency */
  irisVirtualSetMacro(Alpha, double)

  /** Set layer transparency */
  irisVirtualGetMacro(Alpha, double)

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
   * Get the buffered region of the image
   */
  virtual itk::ImageRegion<3> GetBufferedRegion() const = 0;

  /**
   * Extract a region of interest from the image wrapper, as a new wrapper of
   * the same type
   */
  virtual SmartPtr<ImageWrapperBase> ExtractROI(
      const SNAPSegmentationROISettings &roi, itk::Command *progressCommand) const = 0;

  /** Transform a voxel index into a spatial position */
  virtual Vector3d TransformVoxelIndexToPosition(const Vector3ui &iVoxel) const = 0;

  /** Transform a voxel index into NIFTI coordinates (RAS) */
  virtual Vector3d TransformVoxelIndexToNIFTICoordinates(const Vector3d &iVoxel) const = 0;

  /** Transform NIFTI coordinates to a continuous voxel index */
  virtual Vector3d TransformNIFTICoordinatesToVoxelIndex(const Vector3d &vNifti) const = 0;

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

  /** Get voxel at index as an array of double components */
  virtual void GetVoxelAsDouble(const Vector3ui &x, double *out) const = 0;

  /** Get voxel at index as an array of double components */
  virtual void GetVoxelAsDouble(const itk::Index<3> &idx, double *out) const = 0;

  /** Get voxel intensity in native space. These methods are not recommended
      for iterating over the entire image, since there is a virutal method
      being resolved at each iteration. */
  virtual void GetVoxelMappedToNative(const Vector3ui &vec, double *out) const = 0;
  virtual void GetVoxelMappedToNative(const itk::Index<3> &idx, double *out) const = 0;

  /** Return componentwise minimum cast to double, without mapping to native range */
  virtual double GetImageMinAsDouble() = 0;

  /** Return componentwise maximum cast to double, without mapping to native range */
  virtual double GetImageMaxAsDouble() = 0;

  /** Return componentwise minimum cast to double, after mapping to native range */
  virtual double GetImageMinNative() = 0;

  /** Return componentwise maximum cast to double, after mapping to native range */
  virtual double GetImageMaxNative() = 0;

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
  virtual const ScalarImageHistogram *GetHistogram(size_t nBins) = 0;

  /** Compute statistics over a run of voxels in the image starting at the index
   * startIdx. Appends the statistics to a running sum and sum of squared. The
   * statistics are returned in internal (not native mapped) format */
  virtual void GetRunLengthIntensityStatistics(
      const itk::ImageRegion<3> &region,
      const itk::Index<3> &startIdx, long runlength,
      double *out_sum, double *out_sumsq) const = 0;

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

  /** Get the voxel array, as void pointer */
  virtual void *GetVoxelVoidPointer() const = 0;

  /** Clear the data associated with storing an image */
  virtual void Reset() = 0;

  /**
   * Get the mapping between the internal data type and the 'native' range,
   * i.e., the range of values shown to the user. This may be a linear mapping
   * or an identity mapping.
   */
  virtual const AbstractNativeIntensityMapping *GetNativeIntensityMapping() const = 0;

  /**
   * Get the display mapping policy. This policy differs from wrapper to wrapper
   * and may involve using color labels or color maps.
   */
  virtual AbstractDisplayMappingPolicy *GetDisplayMapping() = 0;

  // Access the filename
  irisVirtualGetStringMacro(FileName)
  irisVirtualSetStringMacro(FileName)

  // Access the nickname - which may be a custom nickname or derived from the
  // filename if there is no custom nickname
  irisVirtualGetMacro(Nickname, const std::string &)

  // Set the custom nickname - precedence over the filename
  irisVirtualGetMacro(CustomNickname, const std::string &)
  irisVirtualSetMacro(CustomNickname, const std::string &)

  // Fallback nickname - shown if no filename and no custom nickname set.
  irisVirtualGetMacro(DefaultNickname, const std::string &)
  irisVirtualSetMacro(DefaultNickname, const std::string &)

  /**
    Export one of the slices as a thumbnail (e.g., PNG file)
    */
  virtual void WriteThumbnail(const char *filename, unsigned int maxdim) = 0;

  /**
   * Write the image to disk with the help of the GuidedNativeImageIO object
   */
  virtual void WriteToFile(const char *filename, Registry &hints) = 0;

  /**
   * Check if the image has unsaved changes
   */
  virtual bool HasUnsavedChanges() const = 0;

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

  typedef itk::Image<short, 3> ShortImageType;

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
  virtual void SetUserData(const std::string &role, itk::Object *data) = 0;

  /**
   * Get the user data associated with this wrapper for a specific role. If
   * no association exists, NULL is returned.
   */
  virtual itk::Object* GetUserData(const std::string &role) const = 0;

  //

  /**
   * Set an ITK transform between this image and a reference image.
   */
  virtual void SetITKTransform(ImageBaseType *referenceSpace, ITKTransformType *transform) = 0;

protected:

};

class ScalarImageWrapperBase : public virtual ImageWrapperBase
{
public:

  // A common image format to which the contents of the scalar image wrapper
  // may be cast for downstream processing
  typedef itk::Image<GreyType, 3>                      CommonFormatImageType;
  typedef itk::Image<float, 3>                                FloatImageType;
  typedef itk::ImageSource<FloatImageType>                  FloatImageSource;

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
   * Get the scaling factor used to convert between intensities stored
   * in this image and the 'true' image intensities
   */
  virtual double GetImageScaleFactor() = 0;

  /** Get voxel at index as a single double value */
  virtual double GetVoxelAsDouble(const Vector3ui &x) const = 0;

  /** Get voxel at index as a single double value */
  virtual double GetVoxelAsDouble(const itk::Index<3> &idx) const = 0;

  /** Get voxel intensity in native space. These methods are not recommended
      for iterating over the entire image, since there is a virutal method
      being resolved at each iteration. */
  virtual double GetVoxelMappedToNative(const Vector3ui &vec) const = 0;
  virtual double GetVoxelMappedToNative(const itk::Index<3> &idx) const = 0;

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
   * Extract a GreyType representation from the image wrapper. Note that
   * internally, the scalar image wrapper can be of many itk types, e.g.,
   * it could be a component of a vector image computed dynamically. In
   * order to use the scalar image in downstream filters, we must have a
   * way to map it to some common datatype. If not, we would have to template
   * the downstream filter on the type of the image in the wrapper, which would
   * lead to an exponential explosion of types.
   *
   * There are actually four representations for each image wrapper, one of
   * which is intended for pipelines that act on entire image volumes and the
   * other three intended for use in preview-capable pipelines, which generate
   * output for just one slice. Since ITK only allocates the requested image
   * region, these four representations should not really use much extra memory.
   *
   * However, it is very important that downstream filters use the itk streaming
   * image filter to break up operations into pieces. Without that, there would
   * be unnecessary large memory allocation.
   */
  virtual CommonFormatImageType* GetCommonFormatImage(
      ExportChannel channel = WHOLE_IMAGE) = 0;

  /**
    Cast the internally stored image to a floating point image. The returned
    image is connected to the internally stored image by a mini-pipeline that
    may include a cast filter or a scale/shift filter, depending on the internal
    format of the image and the internal-to-native intensity mapping. This mini
    pipeline is not memory managed by the wrapper, and as soon as the returned
    image smartpointer goes out of scope, the mini-pipeline is deallocated.

    The method is intended for use with external pipelines that don't know what
    the internal data representation is for the image. There is a cost with using
    this method in terms of memory, so the recommended use is in conjunction with
    streaming filters, so that the cast mini-pipeline does not allocate the whole
    floating point image all at once.

    The mini-pipeline should not be kept around in memory after it's used. This would
    result in unnecessary duplication of memory.
    */
  virtual SmartPtr<FloatImageSource> CreateCastToFloatPipeline() const = 0;

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

  /** Get a version of this image that is usable in VTK pipelines */
  virtual vtkImageImport *GetVTKImporter() = 0;
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
