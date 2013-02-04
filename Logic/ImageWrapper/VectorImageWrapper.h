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
  typedef typename Superclass::ImageType                             ImageType;
  typedef typename Superclass::ImagePointer                       ImagePointer;

  // Pixel type
  typedef typename Superclass::PixelType                             PixelType;
  typedef typename ImageType::InternalPixelType              InternalPixelType;

  // Slice image type
  typedef typename Superclass::SliceType                             SliceType;
  typedef typename Superclass::SlicerPointer                      SlicePointer;

  // Slicer type
  typedef typename Superclass::SlicerType                           SlicerType;
  typedef typename Superclass::SlicerPointer                     SlicerPointer;

  // Iterator types
  typedef typename Superclass::Iterator                               Iterator;
  typedef typename Superclass::ConstIterator                     ConstIterator;

  typedef typename Superclass::NativeIntensityMapping   NativeIntensityMapping;

  // Access to the components
  typedef typename TTraits::ComponentWrapperType          ComponentWrapperType;

  // Pipeline objects wrapped around values
  typedef typename Superclass::ComponentTypeObject         ComponentTypeObject;

  virtual bool IsScalar() const { return false; }

  /**
   * This function returns whatever scalar representation is current. The
   * current representation depends on the display mode of the wrapper. If the
   * display mode is RGB, the default scalar representation in MaximumComponent,
   * which is somewhat arbitrary. If the components are being animated, there
   * is also some ambiguity as to what the default scalar component should be.
   * @see ImageWrapperBase::GetScalarRepresentation
   */
  ScalarImageWrapperBase *GetDefaultScalarRepresentation();

  ScalarImageWrapperBase *GetScalarRepresentation(
      VectorImageWrapperBase::ScalarRepresentation type,
      int index = 0);

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
                              itk::Command *progressCommand = NULL) const;


  /** This image type has only one component */
  virtual size_t GetNumberOfComponents() const
  {
    return this->m_Image->GetNumberOfComponentsPerPixel();
  }

  /** Voxel access */
  // virtual double GetVoxelAsDouble(const itk::Index<3> &idx) const;

  /** Voxel access */
  // virtual double GetVoxelAsDouble(const Vector3ui &v) const;

  virtual void GetVoxelAsDouble(const Vector3ui &x, double *out) const
  {
    const PixelType &p = Superclass::GetVoxel(x);
    for(unsigned int i = 0; i < this->GetNumberOfComponents(); i++)
      out[i] = p[i];
  }

  virtual void GetVoxelAsDouble(const itk::Index<3> &idx, double *out) const
  {
    const PixelType &p = Superclass::GetVoxel(idx);
    for(unsigned int i = 0; i < this->GetNumberOfComponents(); i++)
      out[i] = p[i];
  }

  virtual void GetVoxelMappedToNative(const Vector3ui &x, double *out) const
  {
    const PixelType &p = Superclass::GetVoxel(x);
    for(unsigned int i = 0; i < this->GetNumberOfComponents(); i++)
      out[i] = this->m_NativeMapping(p[i]);
  }

  virtual void GetVoxelMappedToNative(const itk::Index<3> &idx, double *out) const
  {
    const PixelType &p = Superclass::GetVoxel(idx);
    for(unsigned int i = 0; i < this->GetNumberOfComponents(); i++)
      out[i] = this->m_NativeMapping(p[i]);
  }

  /**
   * Get the component-wise minimum and maximum of the image in native format
   */
  virtual ComponentTypeObject *GetImageMinObject() const;
  virtual ComponentTypeObject *GetImageMaxObject() const;

  /**
   * This method returns a vector of values for the voxel under the cursor.
   * This is the natural value or set of values that should be displayed to
   * the user. The value depends on the current display mode. For scalar
   * images, it's just the value of the voxel, but for multi-component images,
   * it's the value of the selected component (if there is one) or the value
   * of the multiple components when the mode is RGB.
   */
  virtual vnl_vector<double> GetVoxelUnderCursorDisplayedValue();

  virtual void SetNativeMapping(NativeIntensityMapping mapping);

  virtual void SetSliceIndex(const Vector3ui &cursor);

  virtual void SetImageToDisplayTransform(
    unsigned int iSlice,const ImageCoordinateTransform &transform);

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

  virtual void UpdateImagePointer(ImageType *newImage);

  /** Destructor */
  virtual ~VectorImageWrapper();

  /** Create a derived wrapper of a certain type */
  template <class TFunctor>
  SmartPtr<ScalarImageWrapperBase> CreateDerivedWrapper(ImageType *image);

  template <class TFunctor>
  void SetNativeMappingInDerivedWrapper(
      ScalarImageWrapperBase *w, NativeIntensityMapping &mapping);

  // Array of derived quantities
  typedef SmartPtr<ScalarImageWrapperBase> ScalarWrapperPointer;
  typedef typename VectorImageWrapperBase::ScalarRepresentation ScalarRepresentation;
  typedef std::pair<ScalarRepresentation, int> ScalarRepIndex;

  typedef std::map<ScalarRepIndex, ScalarWrapperPointer> ScalarRepMap;
  typedef typename ScalarRepMap::iterator ScalarRepIterator;
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

  // Other derived wrappers
  typedef VectorToScalarMagnitudeFunctor<InternalPixelType,float> MagnitudeFunctor;
  typedef VectorToScalarMaxFunctor<InternalPixelType, float> MaxFunctor;
  typedef VectorToScalarMeanFunctor<InternalPixelType,float> MeanFunctor;

  virtual void AddSamplesToHistogram();
};

#endif // __VectorImageWrapper_h_
