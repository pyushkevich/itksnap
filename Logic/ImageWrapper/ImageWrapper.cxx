/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ImageWrapper.txx,v $
  Language:  C++
  Date:      $Date: 2010/10/14 16:21:04 $
  Version:   $Revision: 1.11 $
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

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/

#include "ImageWrapper.h"
#include "itkImageRegionIterator.h"
#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkNumericTraits.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkIdentityTransform.h"
#include "IRISSlicer.h"
#include "SNAPSegmentationROISettings.h"
#include "itkCommand.h"
#include "ImageCoordinateGeometry.h"
#include <itkImageFileWriter.h>
#include <itkResampleImageFilter.h>
#include <itkIdentityTransform.h>
#include <itkFlipImageFilter.h>
#include <itkUnaryFunctorImageFilter.h>
#include "ImageWrapperTraits.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkBSplineInterpolateImageFunction.h"
#include "itkWindowedSincInterpolateImageFunction.h"
#include "itkLinearInterpolateImageFunction.h"
#include "IRISException.h"
#include "itkImageAdaptor.h"
#include "itkVectorImageToImageAdaptor.h"
#include "UnaryValueToValueFilter.h"
#include "ScalarImageHistogram.h"
#include "GuidedNativeImageIO.h"


#include <vnl/vnl_inverse.h>
#include <iostream>

#include <itksys/SystemTools.hxx>

unsigned long GlobalImageWrapperIndex = 0;


template <class TPixel>
class SimpleCastToDoubleFunctor
{
public:
  typedef TPixel InputType;
  typedef double OutputType;
  double operator()(TPixel input) { return static_cast<double>(input); }
};


/**
 * Some functions in the image wrapper are only defined for 'concrete' image
 * wrappers, i.e., those that store an image or a vectorimage. These functions
 * involve copying subregions, filling the buffer, IO, etc. To handle this
 * differential availability of functionality, we use partial template
 * specialization below.
 */
template <class TImage>
class ImageWrapperPartialSpecializationTraits
{
public:
  typedef TImage ImageType;
  typedef typename TImage::PixelType PixelType;

  static void FillBuffer(ImageType *image, PixelType)
  {
    throw IRISException("FillBuffer unsupported for class %s",
                        image->GetNameOfClass());
  }

  static void Write(ImageType *image, const char *fname, Registry &hints)
  {
    throw IRISException("FillBuffer unsupported for class %s",
                        image->GetNameOfClass());
  }

  static SmartPtr<ImageType> CopyRegion(ImageType *image,
                                        const SNAPSegmentationROISettings &roi,
                                        itk::Command *progressCommand)
  {
    throw IRISException("CopyRegion unsupported for class %s",
                        image->GetNameOfClass());
    return NULL;
  }
};

template <class TImage>
class ImageWrapperPartialSpecializationTraitsCommon
{
public:
  typedef TImage ImageType;
  typedef typename TImage::PixelType PixelType;

  static void FillBuffer(ImageType *image, PixelType p)
  {
    image->FillBuffer(p);
  }

  static void Write(ImageType *image, const char *fname, Registry &hints)
  {
    GuidedNativeImageIO io;
    io.CreateImageIO(fname, hints, false);
    itk::ImageIOBase *base = io.GetIOBase();

    typedef itk::ImageFileWriter<TImage> WriterType;
    typename WriterType::Pointer writer = WriterType::New();
    writer->SetFileName(fname);
    if(base)
      writer->SetImageIO(base);
    writer->SetInput(image);
    writer->Update();
  }

  template <class TInterpolateFunction>
  static SmartPtr<ImageType> DeepCopyImageRegion(
      ImageType *image,
      TInterpolateFunction *interp,
      const SNAPSegmentationROISettings &roi,
      itk::Command *progressCommand)
  {
    // Check if there is a difference in voxel size, i.e., user wants resampling
    Vector3d vOldSpacing = image->GetSpacing();
    Vector3d vOldOrigin = image->GetOrigin();
    Vector3i vROIIndex(roi.GetROI().GetIndex());
    Vector3ui vROISize(roi.GetROI().GetSize());

    if(roi.IsResampling())
      {
      // Compute the number of voxels in the output
      typedef typename itk::ImageRegion<3> RegionType;
      typedef typename itk::Size<3> SizeType;

      // We need to compute the new spacing and origin of the resampled
      // ROI piece. To do this, we need the direction matrix
      typedef typename ImageType::DirectionType DirectionType;
      const DirectionType &dm = image->GetDirection();

      // The spacing of the new ROI
      Vector3d vNewSpacing =
          element_quotient(element_product(vOldSpacing, to_double(vROISize)),
                           to_double(roi.GetResampleDimensions()));

      // The origin of the new ROI
      Vector3d vNewOrigin =
          vOldOrigin + dm.GetVnlMatrix() * (
            element_product((to_double(vROIIndex) - 0.5), vOldSpacing) +
            vNewSpacing * 0.5);

      // Create a filter for resampling the image
      typedef itk::ResampleImageFilter<ImageType,ImageType> ResampleFilterType;
      typename ResampleFilterType::Pointer fltSample = ResampleFilterType::New();

      // Initialize the resampling filter
      fltSample->SetInput(image);
      fltSample->SetTransform(itk::IdentityTransform<double,3>::New());
      fltSample->SetInterpolator(interp);

      // Set the image sizes and spacing
      fltSample->SetSize(to_itkSize(roi.GetResampleDimensions()));
      fltSample->SetOutputSpacing(vNewSpacing.data_block());
      fltSample->SetOutputOrigin(vNewOrigin.data_block());
      fltSample->SetOutputDirection(image->GetDirection());

      // Set the progress bar
      if(progressCommand)
        fltSample->AddObserver(itk::AnyEvent(),progressCommand);

      fltSample->Update();

      return fltSample->GetOutput();
      }
    else
      {
      // The filter used to chop off the region of interest
      typedef itk::RegionOfInterestImageFilter <ImageType,ImageType> ChopFilterType;
      typename ChopFilterType::Pointer fltChop = ChopFilterType::New();

      // Pipe image into the chopper
      fltChop->SetInput(image);

      // Set the region of interest
      fltChop->SetRegionOfInterest(roi.GetROI());

      // Update the pipeline
      fltChop->Update();

      // Return the resulting image
      return fltChop->GetOutput();
      }
  }

};


template<class TPixel, unsigned int VDim>
class ImageWrapperPartialSpecializationTraits< itk::Image<TPixel, VDim> >
    : public ImageWrapperPartialSpecializationTraitsCommon< itk::Image<TPixel, VDim> >
{
public:
  typedef itk::Image<TPixel, VDim> ImageType;
  typedef typename ImageType::PixelType PixelType;
  typedef ImageWrapperPartialSpecializationTraitsCommon<ImageType> Superclass;

  static SmartPtr<ImageType> CopyRegion(ImageType *image,
                                        const SNAPSegmentationROISettings &roi,
                                        itk::Command *progressCommand)
  {
    typedef itk::InterpolateImageFunction<ImageType> Interpolator;
    SmartPtr<Interpolator> interp = NULL;

    // Choose the interpolator
    switch(roi.GetInterpolationMethod())
      {
      case SNAPSegmentationROISettings::NEAREST_NEIGHBOR :
        typedef itk::NearestNeighborInterpolateImageFunction<ImageType,double> NNInterpolatorType;
        interp = NNInterpolatorType::New().GetPointer();
        break;

      case SNAPSegmentationROISettings::TRILINEAR :
        typedef itk::LinearInterpolateImageFunction<ImageType,double> LinearInterpolatorType;
        interp = LinearInterpolatorType::New().GetPointer();
        break;

      case SNAPSegmentationROISettings::TRICUBIC :
        typedef itk::BSplineInterpolateImageFunction<ImageType,double> CubicInterpolatorType;
        interp = CubicInterpolatorType::New().GetPointer();
        break;

      case SNAPSegmentationROISettings::SINC_WINDOW_05 :
        // More typedefs are needed for the sinc interpolator
        static const unsigned int VRadius = 5;
        typedef itk::Function::HammingWindowFunction<VRadius> WindowFunction;
        typedef itk::ConstantBoundaryCondition<ImageType> Condition;
        typedef itk::WindowedSincInterpolateImageFunction<
          ImageType, VRadius, WindowFunction, Condition, double> SincInterpolatorType;
        interp = SincInterpolatorType::New().GetPointer();
        break;
      };

    return Superclass::template DeepCopyImageRegion<Interpolator>(image,interp,roi,progressCommand);
  }
};


template<class TPixel, unsigned int VDim>
class ImageWrapperPartialSpecializationTraits< itk::VectorImage<TPixel, VDim> >
   : public ImageWrapperPartialSpecializationTraitsCommon< itk::VectorImage<TPixel, VDim> >
{
public:
  typedef itk::VectorImage<TPixel, VDim> ImageType;
  typedef typename ImageType::PixelType PixelType;
  typedef ImageWrapperPartialSpecializationTraitsCommon<ImageType> Superclass;

  static void FillBuffer(ImageType *image, PixelType p)
  {
    image->FillBuffer(p);
  }

  static SmartPtr<ImageType> CopyRegion(ImageType *image,
                                        const SNAPSegmentationROISettings &roi,
                                        itk::Command *progressCommand)
  {
    typedef itk::InterpolateImageFunction<ImageType> Interpolator;
    SmartPtr<Interpolator> interp = NULL;

    // Choose the interpolator
    switch(roi.GetInterpolationMethod())
      {
      case SNAPSegmentationROISettings::NEAREST_NEIGHBOR :
        typedef itk::NearestNeighborInterpolateImageFunction<ImageType> NNInterpolatorType;
        interp = NNInterpolatorType::New().GetPointer();
        break;

      case SNAPSegmentationROISettings::TRILINEAR :
        typedef itk::LinearInterpolateImageFunction<ImageType> LinearInterpolatorType;
        interp = LinearInterpolatorType::New().GetPointer();
        break;

      default:
        throw IRISException("Higher-order interpolation for vector images is unsupported.");
      };

    return Superclass::template DeepCopyImageRegion<Interpolator>(image,interp,roi,progressCommand);
  }

};


template<class TTraits, class TBase>
ImageWrapper<TTraits,TBase>
::ImageWrapper() 
{
  CommonInitialization();
}

template<class TTraits, class TBase>
ImageWrapper<TTraits,TBase>
::~ImageWrapper() 
{
  Reset();
}

template<class TTraits, class TBase>
void 
ImageWrapper<TTraits,TBase>
::CommonInitialization()
{
  // Set the unique wrapper id
  m_UniqueId = ++GlobalImageWrapperIndex;

  // Set initial state    
  m_Initialized = false;

  // Create slicer objects
  m_Slicer[0] = SlicerType::New();
  m_Slicer[1] = SlicerType::New();
  m_Slicer[2] = SlicerType::New();

  // Initialize the display mapping
  m_DisplayMapping = DisplayMapping::New();
  m_DisplayMapping->Initialize(
        static_cast<typename TTraits::WrapperType *>(this));

  // Set sticky flag
  m_Sticky = TTraits::StickyByDefault;

  // By default, the parent wrapper is NULL. This is overridden for wrappers
  // that are derived from vector wrappers. See VectorImageWrapper::CreateDerivedWrapper
  m_ParentWrapper = NULL;

  // Set the transform to identity, which will initialize the directions of the
  // slicers
  this->SetImageToDisplayTransformsToDefault();
}

template<class TTraits, class TBase>
ImageWrapper<TTraits,TBase>
::ImageWrapper(const Self &copy)
{
  CommonInitialization();

  // If the source contains an image, make a copy of that image
  if (copy.IsInitialized() && copy.GetImage())
    {
    // Create and allocate the image
    ImagePointer newImage = ImageType::New();
    newImage->SetRegions(copy.GetImage()->GetBufferedRegion());
    newImage->Allocate();

    // Copy the image contents
    InternalPixelType *ptrTarget = newImage->GetBufferPointer();
    InternalPixelType *ptrSource = copy.GetImage()->GetBufferPointer();
    memcpy(ptrTarget,ptrSource,
           sizeof(PixelType) * newImage->GetPixelContainer()->Size());

    UpdateImagePointer(newImage);
    }
}

template<class TTraits, class TBase>
const ImageCoordinateTransform &
ImageWrapper<TTraits,TBase>
::GetImageToDisplayTransform(unsigned int iSlice) const
{
  return m_ImageToDisplayTransform[iSlice];
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::SetImageToDisplayTransformsToDefault()
{
  ImageCoordinateTransform id[3];
  id[0].SetTransform(Vector3i(1,2,3),Vector3ui(0,0,0));
  id[1].SetTransform(Vector3i(1,3,2),Vector3ui(0,0,0));
  id[2].SetTransform(Vector3i(2,3,1),Vector3ui(0,0,0));
  SetImageToDisplayTransform(0,id[0]);
  SetImageToDisplayTransform(1,id[1]);
  SetImageToDisplayTransform(2,id[2]);
}

template<class TTraits, class TBase>
Vector3ui
ImageWrapper<TTraits,TBase>
::GetSize() const
{
  // Cast the size to our vector format
  itk::Size<3> size = m_Image->GetLargestPossibleRegion().GetSize();
  return Vector3ui(
    (unsigned int) size[0],
    (unsigned int) size[1],
        (unsigned int) size[2]);
}

template<class TTraits, class TBase>
bool
ImageWrapper<TTraits,TBase>
::IsDrawable() const
{
  // If not initialized, the layer is not drawable
  if(!this->IsInitialized())
    return false;

  // If the image is a pipeline output, then it is displayable either if
  // there is a preview pipeline in place, or if the image volume itself has
  // been modified.
  if(TTraits::PipelineOutput)
    {
    return IsPreviewPipelineAttached() || m_Image->GetMTime() > m_ImageAssignTime;
    }

  // Otherwise, it's drawable
  return true;
}


template<class TTraits, class TBase>
itk::ImageRegion<3>
ImageWrapper<TTraits,TBase>
::GetBufferedRegion() const
{
  return m_ImageBase->GetBufferedRegion();
}

template<class TTraits, class TBase>
size_t
ImageWrapper<TTraits,TBase>
::GetNumberOfVoxels() const
{
  return m_ImageBase->GetBufferedRegion().GetNumberOfPixels();
}

template<class TTraits, class TBase>
Vector3d
ImageWrapper<TTraits,TBase>
::TransformVoxelIndexToPosition(const Vector3ui &iVoxel) const
{
  // Use the ITK method to do this
  typename ImageBaseType::IndexType xIndex;
  for(size_t d = 0; d < 3; d++) xIndex[d] = iVoxel[d];

  itk::Point<double, 3> xPoint;
  m_ImageBase->TransformIndexToPhysicalPoint(xIndex, xPoint);

  Vector3d xOut;
  for(unsigned int q = 0; q < 3; q++) xOut[q] = xPoint[q];

  return xOut;
}

template<class TTraits, class TBase>
Vector3d
ImageWrapper<TTraits,TBase>
::TransformVoxelIndexToNIFTICoordinates(const Vector3d &iVoxel) const
{
  // Create homogeneous vector
  vnl_vector_fixed<double, 4> x;
  for(size_t d = 0; d < 3; d++)
    x[d] = (double) iVoxel[d];
  x[3] = 1.0;

  // Transform to NIFTI coords
  vnl_vector_fixed<double, 4> p = m_NiftiSform * x;

  // Return the component
  return Vector3d(p[0], p[1], p[2]);
}

template<class TTraits, class TBase>
Vector3d
ImageWrapper<TTraits,TBase>
::TransformNIFTICoordinatesToVoxelIndex(const Vector3d &vNifti) const
{
  // Create homogeneous vector
  vnl_vector_fixed<double, 4> x;
  for(size_t d = 0; d < 3; d++)
    x[d] = (double) vNifti[d];
  x[3] = 1.0;

  // Transform to NIFTI coords
  vnl_vector_fixed<double, 4> p = m_NiftiInvSform * x;

  // Return the component
  return Vector3d(p[0], p[1], p[2]);
}


template<class TTraits, class TBase>
void 
ImageWrapper<TTraits,TBase>
::PrintDebugInformation() 
{
  std::cout << "=== Image Properties ===" << std::endl;
  std::cout << "   Dimensions         : " << m_Image->GetLargestPossibleRegion().GetSize() << std::endl;
  std::cout << "   Origin             : " << m_Image->GetOrigin() << std::endl;
  std::cout << "   Spacing            : " << m_Image->GetSpacing() << std::endl;
  std::cout << "------------------------" << std::endl;
}

template<class TTraits, class TBase>
void 
ImageWrapper<TTraits,TBase>
::UpdateImagePointer(ImageType *newImage) 
{
  // Check if the image size has changed
  bool hasSizeChanged = 
    (!m_Image) || 
    (newImage->GetLargestPossibleRegion().GetSize() !=
     m_Image->GetLargestPossibleRegion().GetSize());
  
  // Change the input of the slicers 
  m_Slicer[0]->SetInput(newImage);
  m_Slicer[1]->SetInput(newImage);
  m_Slicer[2]->SetInput(newImage);

  // Update the image
  this->m_ImageBase = newImage;
  m_Image = newImage;

  // If so, the coordinate transform needs to be reinitialized to identity
  if(hasSizeChanged)
    {
    // Reset the transform to identity
    this->SetImageToDisplayTransformsToDefault();

    // Reset the slice positions to zero
    this->SetSliceIndex(Vector3ui(0,0,0));
    }

  // Mark the image as Modified to enforce correct sequence of
  // operations with MinMaxCalc
  m_Image->Modified();

  // Set the NIFTI/RAS transform
  m_NiftiSform = ImageWrapperBase::ConstructNiftiSform(
    m_Image->GetDirection().GetVnlMatrix(),
    m_Image->GetOrigin().GetVnlVector(),
    m_Image->GetSpacing().GetVnlVector());
  m_NiftiInvSform = vnl_inverse(m_NiftiSform);

  // Update the image in the display mapping
  m_DisplayMapping->UpdateImagePointer(m_Image);

  // We have been initialized
  m_Initialized = true;

  // Store the time when the image was assigned
  m_ImageAssignTime = m_Image->GetTimeStamp();
}

template<class TTraits, class TBase>
void 
ImageWrapper<TTraits,TBase>
::InitializeToWrapper(const ImageWrapperBase *source, ImageType *image) 
{
  // Call the common update method
  UpdateImagePointer(image);

  // Update the image-display transforms
  for(unsigned int d=0;d<3;d++)
    SetImageToDisplayTransform(d,source->GetImageToDisplayTransform(d));

  // Update the slice index
  SetSliceIndex(source->GetSliceIndex());
}



template<class TTraits, class TBase>
void 
ImageWrapper<TTraits,TBase>
::InitializeToWrapper(const ImageWrapperBase *source, const PixelType &value)
{
  typedef ImageWrapperPartialSpecializationTraits<ImageType> Specialization;

  // Allocate the image
  ImagePointer newImage = ImageType::New();
  newImage->SetRegions(source->GetImageBase()->GetBufferedRegion().GetSize());
  newImage->Allocate();
  Specialization::FillBuffer(newImage.GetPointer(), value);
  newImage->SetOrigin(source->GetImageBase()->GetOrigin());
  newImage->SetSpacing(source->GetImageBase()->GetSpacing());
  newImage->SetDirection(source->GetImageBase()->GetDirection());

  // Call the common update method
  UpdateImagePointer(newImage);

  // Update the image-display transforms
  for(unsigned int d=0;d<3;d++)
    SetImageToDisplayTransform(d,source->GetImageToDisplayTransform(d));

  // Update the slice index
  SetSliceIndex(source->GetSliceIndex());
}

template<class TTraits, class TBase>
void 
ImageWrapper<TTraits,TBase>
::SetImage(ImagePointer newImage) 
{
  UpdateImagePointer(newImage);
}


template<class TTraits, class TBase>
void 
ImageWrapper<TTraits,TBase>
::Reset() 
{
  if (m_Initialized)
    {
    m_Image->ReleaseData();
    m_Image = NULL;
    }
  m_Initialized = false;

  m_Alpha = 0.5;
}


template<class TTraits, class TBase>
inline typename ImageWrapper<TTraits,TBase>::PixelType
ImageWrapper<TTraits,TBase>
::GetVoxel(const Vector3ui &index) const 
{
  return GetVoxel(index[0],index[1],index[2]);
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits, TBase>
::SetVoxel(const Vector3ui &index, const PixelType &value)
{
  this->SetVoxel(to_itkIndex(index), value);
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::SetVoxel(const itk::Index<3> &index, const PixelType &value)
{
  // Verify that the pixel is contained by the image at debug time
  assert(m_Image && m_Image->GetLargestPossibleRegion().IsInside(index));

  // Return the pixel
  m_Image->SetPixel(index, value);
}

template<class TTraits, class TBase>
inline typename ImageWrapper<TTraits,TBase>::PixelType
ImageWrapper<TTraits,TBase>
::GetVoxel(unsigned int x, unsigned int y, unsigned int z) const
{
  itk::Index<3> index;
  index[0] = x;
  index[1] = y;
  index[2] = z;

  return GetVoxel(index);
}

template<class TTraits, class TBase>
inline typename ImageWrapper<TTraits,TBase>::PixelType
ImageWrapper<TTraits,TBase>
::GetVoxel(const itk::Index<3> &index) const
{
  // Verify that the pixel is contained by the image at debug time
  assert(m_Image && m_Image->GetLargestPossibleRegion().IsInside(index));

  // Return the pixel
  return m_Image->GetPixel(index);
}

template<class TTraits, class TBase>
typename ImageWrapper<TTraits,TBase>::ConstIterator
ImageWrapper<TTraits,TBase>
::GetImageConstIterator() const 
{
  ConstIterator it(m_Image,m_Image->GetLargestPossibleRegion());
  it.GoToBegin();
  return it;
}

template<class TTraits, class TBase>
typename ImageWrapper<TTraits,TBase>::Iterator
ImageWrapper<TTraits,TBase>
::GetImageIterator() 
{
  Iterator it(m_Image,m_Image->GetLargestPossibleRegion());
  it.GoToBegin();
  return it;
}

template<class TTraits, class TBase>
void 
ImageWrapper<TTraits,TBase>
::SetSliceIndex(const Vector3ui &cursor)
{
  // Save the cursor position
  m_SliceIndex = cursor;

  // Select the appropriate slice for each slicer
  for(unsigned int i=0;i<3;i++)
  {
    // Which axis does this slicer slice?
    unsigned int axis = m_Slicer[i]->GetSliceDirectionImageAxis();

    // Set the slice using that axis
    m_Slicer[i]->SetSliceIndex(cursor[axis]);
  }
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::SetImageGeometry(const ImageCoordinateGeometry &geom)
{
  // Set the direction matrix in image
  ImageCoordinateTransform tran[] = {
    geom.GetImageToDisplayTransform(0),
    geom.GetImageToDisplayTransform(1),
    geom.GetImageToDisplayTransform(2) };

  this->SetImageGeometry(geom.GetImageDirectionCosineMatrix(), tran);
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::SetImageGeometry(const itk::Matrix<double,3,3> &directionMatrix,
                   ImageCoordinateTransform imageToDisplayTransform[3])
{
  // Set the direction matrix in image
  this->GetImageBase()->SetDirection(directionMatrix);

  // Set the NIFTI/RAS transform
  m_NiftiSform = ImageWrapperBase::ConstructNiftiSform(
    m_Image->GetDirection().GetVnlMatrix(),
    m_Image->GetOrigin().GetVnlVector(),
    m_Image->GetSpacing().GetVnlVector());
  m_NiftiInvSform = vnl_inverse(m_NiftiSform);

  // Update the geometry for each slice
  for(unsigned int iSlice = 0;iSlice < 3;iSlice ++)
    {
    // Set the display transform (modifies the slicer axes)
    this->SetImageToDisplayTransform(
          iSlice, imageToDisplayTransform[iSlice]);

    // Invalidate the requested region in the display slice. This will
    // cause the RR to reset to largest possible region on next Update
    typename DisplaySliceType::RegionType invalidRegion;
    this->GetDisplaySlice(iSlice)->SetRequestedRegion(invalidRegion);
    }

  // Cause the axis indices in the slicers to be updated due to reorientation
  this->SetSliceIndex(this->GetSliceIndex());
}



template<class TTraits, class TBase>
void 
ImageWrapper<TTraits,TBase>
::SetImageToDisplayTransform(unsigned int iSlice,
                             const ImageCoordinateTransform &transform)
{
  // Get the transform and its inverse
  m_ImageToDisplayTransform[iSlice] = transform;
  m_DisplayToImageTransform[iSlice] = transform.Inverse();

  // Tell slicer in which directions to slice
  m_Slicer[iSlice]->SetSliceDirectionImageAxis(
    m_DisplayToImageTransform[iSlice].GetCoordinateIndexZeroBased(2));
  
  m_Slicer[iSlice]->SetLineDirectionImageAxis(
    m_DisplayToImageTransform[iSlice].GetCoordinateIndexZeroBased(1));

  m_Slicer[iSlice]->SetPixelDirectionImageAxis(
    m_DisplayToImageTransform[iSlice].GetCoordinateIndexZeroBased(0));

  m_Slicer[iSlice]->SetPixelTraverseForward(
    m_DisplayToImageTransform[iSlice].GetCoordinateOrientation(0) > 0);

  m_Slicer[iSlice]->SetLineTraverseForward(
        m_DisplayToImageTransform[iSlice].GetCoordinateOrientation(1) > 0);
}

template<class TTraits, class TBase>
inline double
ImageWrapper<TTraits,TBase>
::GetImageMinAsDouble()
{
  this->GetImageMinObject()->Update();
  return static_cast<double>(this->GetImageMinObject()->Get());
}

template<class TTraits, class TBase>
inline double
ImageWrapper<TTraits,TBase>
::GetImageMaxAsDouble()
{
  this->GetImageMaxObject()->Update();
  return static_cast<double>(this->GetImageMaxObject()->Get());
}

template<class TTraits, class TBase>
inline double
ImageWrapper<TTraits,TBase>
::GetImageMinNative()
{
  this->GetImageMinObject()->Update();
  return m_NativeMapping(this->GetImageMinObject()->Get());
}

template<class TTraits, class TBase>
inline double
ImageWrapper<TTraits,TBase>
::GetImageMaxNative()
{
  this->GetImageMaxObject()->Update();
  return m_NativeMapping(this->GetImageMaxObject()->Get());
}



/** For each slicer, find out which image dimension does is slice along */
template<class TTraits, class TBase>
unsigned int 
ImageWrapper<TTraits,TBase>
::GetDisplaySliceImageAxis(unsigned int iSlice)
{
  return m_Slicer[iSlice]->GetSliceDirectionImageAxis();
}

template<class TTraits, class TBase>
typename ImageWrapper<TTraits,TBase>::SliceType*
ImageWrapper<TTraits,TBase>
::GetSlice(unsigned int dimension)
{
  return m_Slicer[dimension]->GetOutput();
}

template<class TTraits, class TBase>
typename ImageWrapper<TTraits,TBase>::InternalPixelType *
ImageWrapper<TTraits,TBase>
::GetVoxelPointer() const
{
  return m_Image->GetBufferPointer();
}


// TODO: this should take advantage of an in-place filter!
template<class TTraits, class TBase>
unsigned int 
ImageWrapper<TTraits,TBase>
::ReplaceIntensity(PixelType iOld, PixelType iNew)
{
  // Counter for the number of replaced voxels
  unsigned int nReplaced = 0;

  // Replace the voxels
  for(Iterator it = GetImageIterator(); !it.IsAtEnd(); ++it)
    if(it.Get() == iOld)
      {
      it.Set(iNew);
      ++nReplaced;
      }

  // Flag that changes have been made
  if(nReplaced > 0)
    m_Image->Modified();

  // Return the number of replacements
  return nReplaced;
}

template<class TTraits, class TBase>
unsigned int 
ImageWrapper<TTraits,TBase>
::SwapIntensities(PixelType iFirst, PixelType iSecond)
{
  // Counter for the number of replaced voxels
  unsigned int nReplaced = 0;

  // Replace the voxels
  for(Iterator it = GetImageIterator(); !it.IsAtEnd(); ++it)
    {
    PixelType iCurrent = it.Get();
    if(iCurrent == iFirst)
      {
      it.Set(iSecond);
      ++nReplaced;
      }
    else if(iCurrent == iSecond)
      {
      it.Set(iFirst);
      ++nReplaced;
      }
    }

  // Flag that changes have been made
  if(nReplaced > 0)
    m_Image->Modified();

  // Return the number of replacements
  return nReplaced;
}

template<class TTraits, class TBase>
typename ImageWrapper<TTraits,TBase>::DisplaySlicePointer
ImageWrapper<TTraits,TBase>::GetDisplaySlice(unsigned int dim)
{
  return m_DisplayMapping->GetDisplaySlice(dim);
}

template<class TTraits, class TBase>
void *
ImageWrapper<TTraits,TBase>
::GetVoxelVoidPointer() const
{
  return (void *) m_Image->GetBufferPointer();
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits, TBase>
::SetFileName(const std::string &name)
{
  m_FileName = name;
  m_FileNameShort = itksys::SystemTools::GetFilenameName(name);
  this->InvokeEvent(WrapperMetadataChangeEvent());
}

template<class TTraits, class TBase>
const std::string &
ImageWrapper<TTraits, TBase>
::GetNickname() const
{
  if(m_CustomNickname.length())
    return m_CustomNickname;

  else if(m_FileName.length())
    return m_FileNameShort;

  else return m_DefaultNickname;
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits, TBase>
::SetCustomNickname(const std::string &nickname)
{
  // Make sure the nickname is real
  if(nickname == m_FileNameShort)
    m_CustomNickname.clear();
  else
    m_CustomNickname = nickname;

  this->InvokeEvent(WrapperMetadataChangeEvent());
}


// The method that can be called for some wrappers, not others
template <class TImage>
static void DoWriteImage(TImage *image, const char *fname, Registry &hints)
{
  GuidedNativeImageIO io;
  io.CreateImageIO(fname, hints, false);
  itk::ImageIOBase *base = io.GetIOBase();

  typedef itk::ImageFileWriter<TImage> WriterType;
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetFileName(fname);
  if(base)
    writer->SetImageIO(base);
  writer->SetInput(image);
  writer->Update();
}

template<class TImage>
class ImageWrapperWriteTraits
{
public:
  static void Write(TImage *image, const char *fname, Registry &hints)
  {
    throw IRISException("FillBuffer unsupported for class %s",
                        image->GetNameOfClass());
  }
};

template<class TPixel, unsigned int VDim>
class ImageWrapperWriteTraits< itk::Image<TPixel, VDim> >
{
public:
  typedef itk::Image<TPixel, VDim> ImageType;
  static void Write(ImageType *image, const char *fname, Registry &hints)
  {
    DoWriteImage(image, fname, hints);
  }
};

template<class TPixel, unsigned int VDim>
class ImageWrapperWriteTraits< itk::VectorImage<TPixel, VDim> >
{
public:
  typedef itk::VectorImage<TPixel, VDim> ImageType;
  static void Write(ImageType *image, const char *fname, Registry &hints)
  {
    DoWriteImage(image, fname, hints);
  }
};


template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::WriteToFile(const char *filename, Registry &hints)
{
  // Do the actual writing
  typedef ImageWrapperPartialSpecializationTraits<ImageType> Specialization;
  Specialization::Write(m_Image, filename, hints);

  // Store the filename
  m_FileName = itksys::SystemTools::GetFilenamePath(filename);

  // Store the timestamp when the filename was written
  m_ImageSaveTime = m_Image->GetTimeStamp();

}


template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::AttachPreviewPipeline(
    PreviewFilterType *f0, PreviewFilterType *f1, PreviewFilterType *f2)
{
  PreviewFilterType *filter[] = {f0, f1, f2};
  for(int i = 0; i < 3; i++)
    {
    // Update the preview inputs to the slicers
    m_Slicer[i]->SetPreviewInput(filter[i]->GetOutput());

    // Mark the preview filters as modified to ensure that the slicer
    // is going to use it. TODO: is this really needed?
    filter[i]->Modified();
    }
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::DetachPreviewPipeline()
{
  for(int i = 0; i < 3; i++)
    {
    m_Slicer[i]->SetPreviewInput(NULL);
    }
}

template<class TTraits, class TBase>
bool
ImageWrapper<TTraits,TBase>
::IsPreviewPipelineAttached() const
{
  return m_Slicer[0]->GetPreviewInput() != NULL;
}


struct RemoveTransparencyFunctor
{
  typedef ImageWrapperBase::DisplayPixelType PixelType;
  PixelType operator()(const PixelType &p)
  {
    PixelType pnew = p;
    pnew[3] = 255;
    return pnew;
  }
};

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::WriteThumbnail(const char *file, unsigned int maxdim)
{
  // Get the display slice
  // For now, just use the z-axis for exporting the thumbnails
  DisplaySliceType *slice = this->GetDisplaySlice(2);
  slice->Update();

  // The size of the slice
  Vector2ui slice_dim = slice->GetBufferedRegion().GetSize();

  // The physical extents of the slice
  Vector2d slice_extent(slice->GetSpacing()[0] * slice_dim[0],
                        slice->GetSpacing()[1] * slice_dim[1]);

  // The output thumbnail will have the extents as the slice, but its size
  // must be at max maxdim
  double slice_extent_max = slice_extent.max_value();

  // Create a simple square thumbnail
  Vector2ui thumb_size(maxdim, maxdim);

  // Spacing is such that the slice extent fits into the thumbnail
  Vector2d thumb_spacing(slice_extent_max / maxdim,
                         slice_extent_max / maxdim);

  // The origin of the thumbnail is such that the centers coincide
  Vector2d thumb_origin(0.5 * (slice_extent[0] - slice_extent_max),
                        0.5 * (slice_extent[1] - slice_extent_max));

  typedef typename itk::IdentityTransform<double, 2> TransformType;
  TransformType::Pointer transform = TransformType::New();

  typedef typename itk::ResampleImageFilter<
      DisplaySliceType, DisplaySliceType> ResampleFilter;

  // Background color for thumbnails
  unsigned char defrgb[] = {0,0,0,255};

  SmartPtr<ResampleFilter> filter = ResampleFilter::New();
  filter->SetInput(slice);
  filter->SetTransform(transform);
  filter->SetSize(to_itkSize(thumb_size));
  filter->SetOutputSpacing(thumb_spacing.data_block());
  filter->SetOutputOrigin(thumb_origin.data_block());
  filter->SetDefaultPixelValue(DisplayPixelType(defrgb));

  // For thumbnails, the image needs to be flipped
  typedef itk::FlipImageFilter<DisplaySliceType> FlipFilter;
  SmartPtr<FlipFilter> flipper = FlipFilter::New();
  flipper->SetInput(filter->GetOutput());
  typename FlipFilter::FlipAxesArrayType flipaxes;
  flipaxes[0] = false; flipaxes[1] = true;
  flipper->SetFlipAxes(flipaxes);

  // We also need to replace the transparency
  typedef itk::UnaryFunctorImageFilter<
      DisplaySliceType, DisplaySliceType, RemoveTransparencyFunctor> OpaqueFilter;
  SmartPtr<OpaqueFilter> opaquer = OpaqueFilter::New();
  opaquer->SetInput(flipper->GetOutput());

  // Write a PNG file
  typedef typename itk::ImageFileWriter<DisplaySliceType> WriterType;
  SmartPtr<WriterType> writer = WriterType::New();
  writer->SetInput(opaquer->GetOutput());
  writer->SetFileName(file);
  writer->Update();
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::WriteMetaData(Registry &reg)
{
  // Save the display mapping
  m_DisplayMapping->Save(reg.Folder("DisplayMapping"));

  // Save the alpha and the stickiness
  reg["Alpha"] << m_Alpha;
  reg["Sticky"] << m_Sticky;
  reg["CustomNickName"] << m_CustomNickname;
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::ReadMetaData(Registry &reg)
{
  // Load the display mapping
  m_DisplayMapping->Restore(reg.Folder("DisplayMapping"));

  // Load the alpha and the stickiness
  this->SetAlpha(reg["Alpha"][m_Alpha]);
  this->SetSticky(reg["Sticky"][m_Sticky]);
  this->SetCustomNickname(reg["CustomNickName"][m_CustomNickname]);
}

template<class TTraits, class TBase>
bool
ImageWrapper<TTraits,TBase>
::HasUnsavedChanges() const
{
  itk::TimeStamp tsNow = m_Image->GetTimeStamp();
  return (tsNow > m_ImageAssignTime && tsNow > m_ImageSaveTime);
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::SetUserData(const std::string &role, itk::Object *data)
{
  m_UserDataMap[role] = data;
}

template<class TTraits, class TBase>
itk::Object *
ImageWrapper<TTraits,TBase>
::GetUserData(const std::string &role) const
{
  UserDataMapType::const_iterator it = m_UserDataMap.find(role);
  if(it == m_UserDataMap.end())
    return NULL;
  else return it->second;
}

template<class TTraits, class TBase>
typename ImageWrapper<TTraits,TBase>::ImagePointer
ImageWrapper<TTraits,TBase>
::DeepCopyRegion(const SNAPSegmentationROISettings &roi,
                 itk::Command *progressCommand) const
{

  // We use partial template specialization here because region copy is
  // only supported for images that are concrete (Image, VectorImage)
  typedef ImageWrapperPartialSpecializationTraits<ImageType> Specialization;
  return Specialization::CopyRegion(m_Image, roi, progressCommand);
}












// Allowed types of image wrappers
template class ImageWrapper<SpeedImageWrapperTraits, ScalarImageWrapperBase>;
template class ImageWrapper<LabelImageWrapperTraits, ScalarImageWrapperBase>;
template class ImageWrapper<LevelSetImageWrapperTraits, ScalarImageWrapperBase>;
template class ImageWrapper<JsrcImageWrapperTraits, ScalarImageWrapperBase>;

template class ImageWrapper<AnatomicImageWrapperTraits<GreyType>, VectorImageWrapperBase>;
template class ImageWrapper<ComponentImageWrapperTraits<GreyType>, ScalarImageWrapperBase>;

typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMagnitudeFunctor> MagTraits;
typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMaxFunctor> MaxTraits;
typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMeanFunctor> MeanTraits;
template class ImageWrapper<MagTraits, ScalarImageWrapperBase>;
template class ImageWrapper<MaxTraits, ScalarImageWrapperBase>;
template class ImageWrapper<MeanTraits, ScalarImageWrapperBase>;
