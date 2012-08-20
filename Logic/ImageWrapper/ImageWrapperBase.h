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

/**
 \class ImageWrapper
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
  typedef itk::RGBAPixel<unsigned char> DisplayPixelType;
  typedef itk::Image<DisplayPixelType,2> DisplaySliceType;
  typedef SmartPtr<DisplaySliceType> DisplaySlicePointer;

  // Image base
  typedef itk::ImageBase<3> ImageBaseType;

  // Transform matrices
  typedef vnl_matrix_fixed<double, 4, 4> TransformType;

  // Image wrappers can fire certain events
  FIRES(AppearanceUpdateEvent)

  virtual ~ImageWrapperBase() { }


  /** Get the coordinate transform for each display slice */
  virtual const ImageCoordinateTransform &GetImageToDisplayTransform(
    unsigned int) const = 0;

  /**
   * Set the trasforms from image space to one of the three display slices (be
   * sure to set all three, or you'll get weird looking slices!
   */
  virtual void SetImageToDisplayTransform(
    unsigned int, const ImageCoordinateTransform &) = 0;

  /**
   * Use a default image-slice transformation, the first slice is along z,
   * the second along y, the third, along x, all directions of traversal are
   * positive.
   */
  virtual void SetImageToDisplayTransformsToDefault() = 0;

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

  /** Is this image of scalar type? */
  virtual bool IsScalar() const = 0;

  /**
   * Get the size of the image
   */
  virtual Vector3ui GetSize() const = 0;

  /** Get layer transparency */
  irisVirtualSetMacro(Alpha, unsigned char)

  /** Set layer transparency */
  irisVirtualGetMacro(Alpha, unsigned char)

  /** Switch on/off visibility */
  virtual void ToggleVisibility() = 0;

  /**
   * Get the buffered region of the image
   */
  virtual itk::ImageRegion<3> GetBufferedRegion() const = 0;

  /** Transform a voxel index into a spatial position */
  virtual Vector3d TransformVoxelIndexToPosition(const Vector3ui &iVoxel) const = 0;

  /** Transform a voxel index into NIFTI coordinates (RAS) */
  virtual Vector3d TransformVoxelIndexToNIFTICoordinates(const Vector3d &iVoxel) const = 0;

  /** Transform NIFTI coordinates to a continuous voxel index */
  virtual Vector3d TransformNIFTICoordinatesToVoxelIndex(const Vector3d &vNifti) const = 0;

  /** Get the NIFTI s-form matrix for this image */
  irisVirtualGetMacro(NiftiSform, TransformType)

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

  /**
    Get the RGBA apperance of the voxel at the intersection of the three
    display slices.
    */
  virtual void GetVoxelUnderCursorAppearance(DisplayPixelType &out) = 0;

  /** Get the voxel array, as void pointer */
  virtual void *GetVoxelVoidPointer() const = 0;

  /** Clear the data associated with storing an image */
  virtual void Reset() = 0;

  // Access the filename
  irisVirtualGetStringMacro(FileName)
  irisVirtualSetStringMacro(FileName)

  // Access the nickname
  irisVirtualGetStringMacro(Nickname)
  irisVirtualSetStringMacro(Nickname)

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

protected:

};

class ScalarImageWrapperBase : public virtual ImageWrapperBase
{
public:

  /**
   * Get the scaling factor used to convert between intensities stored
   * in this image and the 'true' image intensities
   */
  virtual double GetImageScaleFactor() = 0;

  /**
   * Remap the intensity range of the image to a given range
   */
  virtual void RemapIntensityToRange(double min, double max) = 0;

  /** Get voxel at index as a single double value */
  virtual double GetVoxelAsDouble(const Vector3ui &x) const = 0;

  /** Get voxel at index as a single double value */
  virtual double GetVoxelAsDouble(const itk::Index<3> &idx) const = 0;

  /** Get voxel intensity in native space */
  virtual double GetVoxelMappedToNative(const Vector3ui &vec) const = 0;

  /** Get the min/max intensity (internal representation) */
  virtual double GetImageMaxAsDouble() = 0;
  virtual double GetImageMinAsDouble() = 0;

  /** Get min/max voxel intensity in native space */
  virtual double GetImageMinNative() = 0;
  virtual double GetImageMaxNative() = 0;

  /**
    There may be a linear mapping between internal values stored in the
    image and the values stored in the native image format.
    */
  irisVirtualGetMacro(NativeMapping, InternalToNativeFunctor)
  irisVirtualSetMacro(NativeMapping, InternalToNativeFunctor)

  /**
    Compute the histogram of the image and store it in the ITK
    histogram object.
    */
  virtual const ScalarImageHistogram *GetHistogram(size_t nBins) = 0;

};

/**
  This type of image wrapper is meant to represent a continuous range of values
  as opposed to a discrete set of labels. The wrapper contains a color map
  which is used to map from intensity ranges to display pixels
  */
class ContinuousScalarImageWrapperBase : public virtual ScalarImageWrapperBase
{
public:

  /** Set the reference to the color map object */
  virtual ColorMap* GetColorMap() const = 0;


};

class GreyImageWrapperBase : public virtual ContinuousScalarImageWrapperBase
{
public:

  /**
   * Get the intensity curve to be used for mapping image intensities
   * from GreyType to DisplayType. The curve is defined on the domain
   * [0, 1]. By default, the entire intensity range of the image is
   * mapped to the domain of the curve. However, in some situations
   * (e.g., when the image is a subregion of another image with respect
   * to which the curve was created), the domain of the curve should
   * correspond to a different intensity range. That can be specified
   * using the SetReferenceIntensityRange() function
   */
  virtual IntensityCurveInterface* GetIntensityMapFunction() const = 0;

  /**
   * Copy the intensity curve information from another grey image wrapper
   */
  virtual void CopyIntensityMap(const GreyImageWrapperBase &source) = 0;

  /**
    Automatically rescale the intensity range based on image histogram
    quantiles.
    */
  virtual void AutoFitContrast() = 0;

  /**
   * Set the reference intensity range - a range of intensity that
   * is mapped to the domain of the intensity curve
   * @see GetIntensityMapFunction
   */
  virtual void SetReferenceIntensityRange(double refMin, double refMax) = 0;
  virtual void ClearReferenceIntensityRange() = 0;

};

class VectorImageWrapperBase : public virtual ImageWrapperBase
{
public:

};

class RGBImageWrapperBase : public virtual VectorImageWrapperBase
{
public:

};

#endif // IMAGEWRAPPERBASE_H
