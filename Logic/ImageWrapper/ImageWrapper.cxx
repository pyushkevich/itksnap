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
#include "RLEImageRegionIterator.h"
#include "RLERegionOfInterestImageFilter.h"
#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkNumericTraits.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkIdentityTransform.h"
#include "AdaptiveSlicingPipeline.h"
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
#include "itkConstantBoundaryCondition.h"
#include "IRISException.h"
#include "itkImageAdaptor.h"
#include "itkVectorImageToImageAdaptor.h"
#include "UnaryValueToValueFilter.h"
#include "ScalarImageHistogram.h"
#include "GuidedNativeImageIO.h"
#include "itkTransform.h"
#include "itkExtractImageFilter.h"
#include "AffineTransformHelper.h"


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

  typedef itk::ImageBase<TImage::ImageDimension> ImageBaseType;
  typedef itk::Transform<double, TImage::ImageDimension, TImage::ImageDimension> TransformType;

  static void FillBuffer(ImageType *image, PixelType)
  {
    throw IRISException("FillBuffer unsupported for class %s",
                        image->GetNameOfClass());
  }

  static void Write(ImageType *image, const char *fname, Registry &hints)
  {
    throw IRISException("Write unsupported for class %s",
                        image->GetNameOfClass());
  }

  static void WriteAsFloat(ImageType *image, const char *fname, Registry &hints, double shift, double scale)
  {
    throw IRISException("WriteAsFloat unsupported for class %s",
                        image->GetNameOfClass());
  }

  static SmartPtr<ImageType> CopyRegion(ImageType *image,
                                        ImageBaseType *ref_space,
                                        const TransformType *transform,
                                        const SNAPSegmentationROISettings &roi,
                                        bool force_resampling,
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
  typedef itk::ImageBase<TImage::ImageDimension> ImageBaseType;
  typedef itk::Transform<double, TImage::ImageDimension, TImage::ImageDimension> TransformType;

  static void FillBuffer(ImageType *image, PixelType p)
  {
    image->FillBuffer(p);
  }

  static void Write(ImageType *image, const char *fname, Registry &hints)
  {
    SmartPtr<GuidedNativeImageIO> io = GuidedNativeImageIO::New();
    io->CreateImageIO(fname, hints, false);
    itk::ImageIOBase *base = io->GetIOBase();

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
      ImageBaseType *refspace,
      const TransformType *transform,
      TInterpolateFunction *interp,
      const SNAPSegmentationROISettings &roi,
      bool force_resampling,
      itk::Command *progressCommand)
  {
    // Check if there is a difference in voxel size, i.e., user wants resampling
    Vector3d vOldSpacing = refspace->GetSpacing();
    Vector3d vOldOrigin = refspace->GetOrigin();
    Vector3i vROIIndex(roi.GetROI().GetIndex());
    Vector3ui vROISize(roi.GetROI().GetSize());

    if(force_resampling || roi.IsResampling())
      {
      // Compute the number of voxels in the output
      typedef typename itk::ImageRegion<3> RegionType;
      typedef typename itk::Size<3> SizeType;

      // We need to compute the new spacing and origin of the resampled
      // ROI piece. To do this, we need the direction matrix
      typedef typename ImageType::DirectionType DirectionType;
      const DirectionType &dm = refspace->GetDirection();

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
      fltSample->SetTransform(transform);
      fltSample->SetInterpolator(interp);

      // Set the image sizes and spacing
      fltSample->SetSize(to_itkSize(roi.GetResampleDimensions()));
      fltSample->SetOutputSpacing(vNewSpacing.data_block());
      fltSample->SetOutputOrigin(vNewOrigin.data_block());
      fltSample->SetOutputDirection(refspace->GetDirection());

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
                                        typename Superclass::ImageBaseType *refspace,
                                        const typename Superclass::TransformType *transform,
                                        const SNAPSegmentationROISettings &roi,
                                        bool force_resampling,
                                        itk::Command *progressCommand)
  {
    typedef itk::InterpolateImageFunction<ImageType> Interpolator;
    SmartPtr<Interpolator> interp = NULL;

    // Choose the interpolator
    switch(roi.GetInterpolationMethod())
      {
      case NEAREST_NEIGHBOR :
        typedef itk::NearestNeighborInterpolateImageFunction<ImageType,double> NNInterpolatorType;
        interp = NNInterpolatorType::New().GetPointer();
        break;

      case TRILINEAR :
        typedef itk::LinearInterpolateImageFunction<ImageType,double> LinearInterpolatorType;
        interp = LinearInterpolatorType::New().GetPointer();
        break;

      case TRICUBIC :
        typedef itk::BSplineInterpolateImageFunction<ImageType,double> CubicInterpolatorType;
        interp = CubicInterpolatorType::New().GetPointer();
        break;

      case SINC_WINDOW_05 :
        // More typedefs are needed for the sinc interpolator
        static const unsigned int VRadius = 5;
        typedef itk::Function::HammingWindowFunction<VRadius> WindowFunction;
        typedef itk::ConstantBoundaryCondition<ImageType> Condition;
        typedef itk::WindowedSincInterpolateImageFunction<
          ImageType, VRadius, WindowFunction, Condition, double> SincInterpolatorType;
        interp = SincInterpolatorType::New().GetPointer();
        break;
      };

    return Superclass::template DeepCopyImageRegion<Interpolator>(image,refspace,transform,interp,roi,force_resampling,progressCommand);
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
                                        typename Superclass::ImageBaseType *refspace,
                                        const typename Superclass::TransformType *transform,
                                        const SNAPSegmentationROISettings &roi,
                                        bool force_resampling,
                                        itk::Command *progressCommand)
  {
    typedef itk::InterpolateImageFunction<ImageType> Interpolator;
    SmartPtr<Interpolator> interp = NULL;

    // Choose the interpolator
    switch(roi.GetInterpolationMethod())
      {
      case NEAREST_NEIGHBOR :
        typedef itk::NearestNeighborInterpolateImageFunction<ImageType> NNInterpolatorType;
        interp = NNInterpolatorType::New().GetPointer();
        break;

      case TRILINEAR :
        typedef itk::LinearInterpolateImageFunction<ImageType> LinearInterpolatorType;
        interp = LinearInterpolatorType::New().GetPointer();
        break;

      default:
        throw IRISException("Higher-order interpolation for vector images is unsupported.");
      };

    return Superclass::template DeepCopyImageRegion<Interpolator>(image,refspace,transform,interp,roi,force_resampling,progressCommand);
  }

};


template<class TPixel, unsigned int VDim, class CounterType>
class ImageWrapperPartialSpecializationTraits< RLEImage<TPixel, VDim, CounterType> >
{
public:
  typedef ImageWrapperPartialSpecializationTraits Self;
  typedef RLEImage<TPixel, VDim, CounterType> ImageType;
  typedef itk::Image<TPixel, VDim> UncompressedType;
  typedef typename ImageType::PixelType PixelType;
  typedef itk::ImageBase<VDim> ImageBaseType;
  typedef itk::Transform<double, VDim, VDim> TransformType;

  static void FillBuffer(ImageType *image, PixelType p)
  {
    image->FillBuffer(p);
  }

  static void Write(ImageType *image, const char *fname, Registry &hints)
  {
    //use specialized RoI filter to convert to itk::Image
    typedef itk::RegionOfInterestImageFilter<ImageType, UncompressedType> outConverterType;
    typename outConverterType::Pointer outConv = outConverterType::New();
    outConv->SetInput(image);
    outConv->SetRegionOfInterest(image->GetLargestPossibleRegion());
    outConv->Update();
    typename UncompressedType::Pointer imgUncompressed = outConv->GetOutput();

    SmartPtr<GuidedNativeImageIO> io = GuidedNativeImageIO::New();
    io->CreateImageIO(fname, hints, false);
    itk::ImageIOBase *base = io->GetIOBase();

    typedef itk::ImageFileWriter<UncompressedType> WriterType;
    typename WriterType::Pointer writer = WriterType::New();
    writer->SetFileName(fname);
    if (base)
        writer->SetImageIO(base);
    writer->SetInput(imgUncompressed);
    writer->Update();
  }

  template <class TInterpolateFunction>
  static SmartPtr<ImageType> DeepCopyImageRegion(
      ImageType *image,
      ImageBaseType *ref_space,
      const TransformType *transform,
      TInterpolateFunction *interp,
      const SNAPSegmentationROISettings &roi,
      bool force_resampling,
      itk::Command *progressCommand)
  {
      // Check if there is a difference in voxel size, i.e., user wants resampling
      Vector3d vOldSpacing = image->GetSpacing();
      Vector3d vOldOrigin = image->GetOrigin();
      Vector3i vROIIndex(roi.GetROI().GetIndex());
      Vector3ui vROISize(roi.GetROI().GetSize());

      if (force_resampling || roi.IsResampling())
      {
          // Compute the number of voxels in the output
          typedef typename itk::ImageRegion<3> RegionType;
          typedef typename itk::Size<3> SizeType;

          // We need to compute the new spacing and origin of the resampled
          // ROI piece. To do this, we need the direction matrix
          typedef typename ImageType::DirectionType DirectionType;
          const DirectionType &dm = ref_space->GetDirection();

          // The spacing of the new ROI
          Vector3d vNewSpacing =
              element_quotient(element_product(vOldSpacing, to_double(vROISize)),
              to_double(roi.GetResampleDimensions()));

          // The origin of the new ROI
          Vector3d vNewOrigin =
              vOldOrigin + dm.GetVnlMatrix() * (
              element_product((to_double(vROIIndex) - 0.5), vOldSpacing) +
              vNewSpacing * 0.5);

          //use specialized RoI filter to convert the region to be resampled to itk::Image
          typedef itk::RegionOfInterestImageFilter<ImageType, UncompressedType> outConverterType;
          typename outConverterType::Pointer outConv = outConverterType::New();
          outConv->SetInput(image);
          outConv->SetRegionOfInterest(roi.GetROI());
          outConv->Update();
          typename UncompressedType::Pointer imgUncompressed = outConv->GetOutput();

          // Create a filter for resampling the image
          typedef itk::ResampleImageFilter<UncompressedType, ImageType> ResampleFilterType;
          typename ResampleFilterType::Pointer fltSample = ResampleFilterType::New();

          // Initialize the resampling filter
          fltSample->SetInput(imgUncompressed);
          fltSample->SetTransform(transform);
          fltSample->SetInterpolator(interp);

          // Set the image sizes and spacing
          fltSample->SetSize(to_itkSize(roi.GetResampleDimensions()));
          fltSample->SetOutputSpacing(vNewSpacing.data_block());
          fltSample->SetOutputOrigin(vNewOrigin.data_block());
          fltSample->SetOutputDirection(ref_space->GetDirection());

          // Set the progress bar
          if (progressCommand)
              fltSample->AddObserver(itk::AnyEvent(), progressCommand);

          fltSample->Update();

          return fltSample->GetOutput();
      }
      else
      {
          // The filter used to chop off the region of interest
          typedef itk::RegionOfInterestImageFilter <ImageType, ImageType> ChopFilterType;
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

  static SmartPtr<ImageType> CopyRegion(ImageType *image,
                                        ImageBaseType *refspace,
                                        const TransformType *transform,
                                        const SNAPSegmentationROISettings &roi,
                                        bool force_resampling,
                                        itk::Command *progressCommand)
  {
    //the interpolator will operate on uncompressed region
    typedef itk::InterpolateImageFunction<UncompressedType> Interpolator;
    SmartPtr<Interpolator> interp = NULL;

    // Choose the interpolator
    switch(roi.GetInterpolationMethod())
      {
      case NEAREST_NEIGHBOR :
        typedef itk::NearestNeighborInterpolateImageFunction<UncompressedType, double> NNInterpolatorType;
        interp = NNInterpolatorType::New().GetPointer();
        break;

      case TRILINEAR:
          typedef itk::LinearInterpolateImageFunction<UncompressedType, double> LinearInterpolatorType;
          interp = LinearInterpolatorType::New().GetPointer();
          break;

      case TRICUBIC:
          typedef itk::BSplineInterpolateImageFunction<UncompressedType, double> CubicInterpolatorType;
          interp = CubicInterpolatorType::New().GetPointer();
          break;

      case SINC_WINDOW_05:
          // More typedefs are needed for the sinc interpolator
          static const unsigned int VRadius = 5;
          typedef itk::Function::HammingWindowFunction<VRadius> WindowFunction;
          typedef itk::ConstantBoundaryCondition<UncompressedType> Condition;
          typedef itk::WindowedSincInterpolateImageFunction<
              UncompressedType, VRadius, WindowFunction, Condition, double> SincInterpolatorType;
          interp = SincInterpolatorType::New().GetPointer();
          break;
      };

    return Self::template DeepCopyImageRegion<Interpolator>(image, refspace, transform, interp, roi, force_resampling, progressCommand);
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
  delete m_IOHints;
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::CommonInitialization()
{
  // This is the code that should be called by all constructors

  // Set the unique wrapper id
  m_UniqueId = ++GlobalImageWrapperIndex;

  // Set initial state
  m_Initialized = false;
  m_PipelineReady = false;

  // Create empty IO hints
  m_IOHints = new Registry();

  // Create slicer objects
  m_Slicer[0] = SlicerType::New();
  m_Slicer[1] = SlicerType::New();
  m_Slicer[2] = SlicerType::New();

  // Initialize the display mapping
  m_DisplayMapping = DisplayMapping::New();
  m_DisplayMapping->Initialize(static_cast<typename DisplayMapping::WrapperType *>(this));

  // Set sticky flag
  m_Sticky = TTraits::StickyByDefault;

  // By default, the parent wrapper is NULL. This is overridden for wrappers
  // that are derived from vector wrappers. See VectorImageWrapper::CreateDerivedWrapper
  m_ParentWrapper = NULL;

  // Update the image geometry to default value
  this->UpdateImageGeometry();
}

template<class TTraits, class TBase>
ImageWrapper<TTraits,TBase>
::ImageWrapper(const Self &copy)
{
  CommonInitialization();

  // If the source contains an image, make a copy of that image
  if (copy.IsInitialized() && copy.GetImage())
    {
    typedef itk::RegionOfInterestImageFilter<ImageType, ImageType> roiType;
    typename roiType::Pointer roi = roiType::New();
    roi->SetInput(copy.GetImage());
    roi->Update();
    ImagePointer newImage = roi->GetOutput();
    UpdateImagePointer(newImage);
    }

  // Copy IO hints
  *m_IOHints = copy.GetIOHints();
}

template<class TTraits, class TBase>
const ImageCoordinateTransform *
ImageWrapper<TTraits,TBase>
::GetImageToDisplayTransform(unsigned int iSlice) const
{
  return m_ImageGeometry.GetImageToDisplayTransform(iSlice);
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::SetDisplayGeometry(const IRISDisplayGeometry &dispGeom)
{
  // Set the display geometry
  m_DisplayGeometry = dispGeom;

  // Update the image geometry object and the slicers
  this->UpdateImageGeometry();
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::SetDirectionMatrix(const vnl_matrix<double> &direction)
{
  // Update the direction matrix in the image
  typename ImageType::DirectionType matrix(direction);
  m_ReferenceSpace->SetDirection(matrix);

  // Update the NIFTI/RAS transform
  this->UpdateNiftiTransforms();

  // Update the image geometry
  this->UpdateImageGeometry();
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::CopyImageCoordinateTransform(const ImageWrapperBase *source)
{
  // Better have the image!
  assert(m_Image && source->GetImageBase());

  // Set the new meta-data on the image
  m_Image->SetSpacing(source->GetImageBase()->GetSpacing());
  m_Image->SetOrigin(source->GetImageBase()->GetOrigin());
  m_Image->SetDirection(source->GetImageBase()->GetDirection());

  // Update NIFTI transforms
  this->UpdateNiftiTransforms();

  // Update the image geometry
  this->UpdateImageGeometry();
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
    return (IsPreviewPipelineAttached() && IsPipelineReady())
        || m_Image->GetMTime() > m_ImageAssignTime;
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
::TransformVoxelCIndexToPosition(const Vector3d &iVoxel) const
{
  // Use the ITK method to do this
  itk::ContinuousIndex<double, 3> xIndex;
  for(size_t d = 0; d < 3; d++) xIndex[d] = iVoxel[d];

  itk::Point<double, 3> xPoint;
  m_ReferenceSpace->TransformContinuousIndexToPhysicalPoint(xIndex, xPoint);

  return Vector3d(xPoint);
}

template<class TTraits, class TBase>
Vector3d
ImageWrapper<TTraits,TBase>
::TransformVoxelIndexToPosition(const Vector3i &iVoxel) const
{
  // Use the ITK method to do this
  typename ImageBaseType::IndexType xIndex = to_itkIndex(iVoxel);

  itk::Point<double, 3> xPoint;
  m_ReferenceSpace->TransformIndexToPhysicalPoint(xIndex, xPoint);

  return Vector3d(xPoint);
}

template<class TTraits, class TBase>
Vector3d
ImageWrapper<TTraits,TBase>
::TransformPositionToVoxelCIndex(const Vector3d &vLPS) const
{
  itk::Point<double, 3> xPoint;
  for(size_t d = 0; d < 3; d++) xPoint[d] = vLPS[d];

  // Use the ITK method to do this
  itk::ContinuousIndex<double, 3> xIndex;

  m_ReferenceSpace->TransformPhysicalPointToContinuousIndex(xPoint, xIndex);

  return Vector3d(xIndex);
}

template<class TTraits, class TBase>
Vector3i
ImageWrapper<TTraits,TBase>
::TransformPositionToVoxelIndex(const Vector3d &vLPS) const
{
  itk::Point<double, 3> xPoint;
  for(size_t d = 0; d < 3; d++) xPoint[d] = vLPS[d];

  // Use the ITK method to do this
  typename ImageBaseType::IndexType xIndex;

  m_ReferenceSpace->TransformPhysicalPointToIndex(xPoint, xIndex);

  return Vector3i(xIndex);
}

template<class TTraits, class TBase>
Vector3d
ImageWrapper<TTraits,TBase>
::TransformVoxelCIndexToNIFTICoordinates(const Vector3d &iVoxel) const
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
::TransformNIFTICoordinatesToVoxelCIndex(const Vector3d &vNifti) const
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
bool
ImageWrapper<TTraits,TBase>
::CompareGeometry(
    ImageBaseType *image1,
    ImageBaseType *image2,
    double tol)
{
  // If one of the images is NULL return false
  if(!image1 || !image2)
    return false;

  // Check if the images have same dimensions
  bool same_size = (image1->GetBufferedRegion() == image2->GetBufferedRegion());

  // Now test the 3D geometry of the image to see if it occupies the same space
  bool same_space = true;

  for(int i = 0; i < 3; i++)
    {
    if(fabs(image1->GetOrigin()[i] - image2->GetOrigin()[i]) > tol)
      same_space = false;
    if(fabs(image1->GetSpacing()[i] - image2->GetSpacing()[i]) > tol)
      same_space = false;
    for(int j = 0; j < 3; j++)
      {
      if(fabs(image1->GetDirection()[i][j] - image2->GetDirection()[i][j]) > tol)
        same_space = false;
      }
    }

  return same_size && same_space;
}


template<class TTraits, class TBase>
bool
ImageWrapper<TTraits,TBase>
::CanOrthogonalSlicingBeUsed(
    ImageType *image, ImageBaseType *referenceSpace, ITKTransformType *transform)
{
  // For orthogonal slicing to be usable, two conditions must be met
  //   1. The reference space and the new image must have the same geometry
  //   2. The transform must be identity

  // Check if the images have same dimensions
  double tol = 1e-5;
  bool same_geom = CompareGeometry(image, referenceSpace, tol);

  // Use helper class to check for identity
  bool is_identity = AffineTransformHelper::IsIdentity(transform);


  return same_geom && is_identity;
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::UpdateSlicingPipelines(ImageType *image, ImageBaseType *referenceSpace, ITKTransformType *transform)
{
  /*
  // Can we use orthogonal spacing
  bool ortho = CanOrthogonalSlicingBeUsed(image, referenceSpace, transform);

  // Update each of the slicers
  for(int i = 0; i < 3; i++)
    {
    m_Slicer[i]->SetInput(image);
    m_Slicer[i]->SetPreviewImage(NULL);
    m_Slicer[i]->SetTransform(transform);
    }

  // Set the input of the slicers, depending on whether the image is subject to transformation
  if(ortho)
    {
    // Slicers take their input directly from the new image
    for(int i = 0; i < 3; i++)
      {
      // Set up the basic slicing pipeline
      m_Slicer[i]->SetInput(image);
      m_Slicer[i]->SetPreviewInput(NULL);
      m_Slicer[i]->SetBypassMainInput(false);

      // Drop the advanced slicing pipeline
      m_AdvancedSlicer[i] = NULL;
      m_ResampleFilter[i+3] = NULL;
      }
    }
  else
    {
    // Create a dummy image to serve as the nominal input to the slicers
    // We purposely do not allocate this dummy image!
    SmartPtr<ImageType> dummy = ImageType::New();
    dummy->CopyInformation(referenceSpace);
    dummy->SetLargestPossibleRegion(referenceSpace->GetBufferedRegion());

    // Each slicer is attached to a preview filter
    for(int i = 0; i < 3; i++)
      {
      // Set the input to the dummy image
      m_Slicer[i]->SetInput(dummy);

      // Create an advanced slicer
      m_AdvancedSlicer[i] = NonOrthogonalSlicerType::New();
      m_AdvancedSlicer[i]->SetInput(image);
      m_AdvancedSlicer[i]->SetTransform(transform);
      m_AdvancedSlicer[i]->SetReferenceImage(m_DisplayViewportGeometryReference[i]);

      // Create another set that work with the older slicers - this is temporary
      // TODO: get rid of this
      m_ResampleFilter[i+3] = ResampleFilter::New();
      m_ResampleFilter[i+3]->SetInput(image);
      m_ResampleFilter[i+3]->SetTransform(transform);
      m_ResampleFilter[i+3]->SetOutputParametersFromImage(referenceSpace);
      m_Slicer[i]->SetPreviewInput(m_ResampleFilter[i+3]->GetOutput());
      m_Slicer[i]->SetBypassMainInput(true);
      }
    }
    */
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::UpdateImagePointer(ImageType *newImage, ImageBaseType *referenceSpace, ITKTransformType *transform)
{
  // If there is no reference space, we assume that the reference space is the same as the image
  referenceSpace = referenceSpace ? referenceSpace : newImage;

  // Check if the image size or image direction matrix has changed
  bool isReferenceGeometrySame = CompareGeometry(m_ReferenceSpace, referenceSpace);

  // Update the image
  this->m_ReferenceSpace = referenceSpace;
  this->m_ImageBase = newImage;
  this->m_Image = newImage;

  // Create the transform if it does not exist
  typename ITKTransformType::Pointer tran = transform;
  if(tran.IsNull())
    {
    typedef itk::IdentityTransform<double, 3> IdTransformType;
    typename IdTransformType::Pointer idTran = IdTransformType::New();
    tran = idTran.GetPointer();
    }

  // Which slicer should be used?
  bool ortho = CanOrthogonalSlicingBeUsed(newImage, referenceSpace, tran);

  // Update the slicers
  for(int i = 0; i < 3; i++)
    {
    m_Slicer[i]->SetInput(newImage);
    m_Slicer[i]->SetObliqueTransform(tran);
    m_Slicer[i]->SetPreviewImage(NULL);
    m_Slicer[i]->SetUseOrthogonalSlicing(ortho);
    }

  // Mark the image as Modified to enforce correct sequence of
  // operations with MinMaxCalc
  m_Image->Modified();

  // Update the image in the display mapping
  m_DisplayMapping->UpdateImagePointer(m_Image);

  // Update the image coordinate geometry
  if(!isReferenceGeometrySame)
    {
    // Reset the transform to identity
    this->UpdateImageGeometry();

    // Reset the slice positions to zero
    this->SetSliceIndex(Vector3ui(0,0,0));
    }

  // Update the NIFTI/RAS transform
  this->UpdateNiftiTransforms();

  // We have been initialized
  m_Initialized = true;

  // Store the time when the image was assigned
  m_ImageAssignTime = m_Image->GetTimeStamp();
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::InitializeToWrapper(const ImageWrapperBase *source,
                      ImageType *image, ImageBaseType *refSpace, ITKTransformType *tran)
{
  // Update the display geometry from the source wrapper
  m_DisplayGeometry = source->GetDisplayGeometry();

  // Call the common update method
  UpdateImagePointer(image, refSpace, tran);

  // Update the slice index
  SetSliceIndex(source->GetSliceIndex());
}

template<class TTraits, class TBase>
bool
ImageWrapper<TTraits,TBase>
::IsSlicingOrthogonal() const
{
  return m_Slicer[0]->GetUseOrthogonalSlicing();
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

  // Update the display geometry from the source wrapper
  m_DisplayGeometry = source->GetDisplayGeometry();

  // Call the common update method
  UpdateImagePointer(newImage);

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
::SetImage(ImagePointer newImage, ImageBaseType *refSpace, ITKTransformType *transform)
{
  UpdateImagePointer(newImage, refSpace, transform);
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::SetITKTransform(ImageBaseType *refSpace, ITKTransformType *transform)
{

  // Check if the reference space has changed
  if(m_ReferenceSpace != refSpace)
    {
    // Force a reinitialization of this layer
    this->UpdateImagePointer(m_Image, refSpace, transform);
    }
  else
    {
    bool ortho = CanOrthogonalSlicingBeUsed(m_Image, refSpace, transform);
    for(int i = 0; i < 3; i++)
      {
      m_Slicer[i]->SetObliqueTransform(transform);
      m_Slicer[i]->SetUseOrthogonalSlicing(ortho);
      this->InvokeEvent(WrapperDisplayMappingChangeEvent());

      // m_ResampleFilter[i+3]->SetTransform(transform);
      }
    }
}

template<class TTraits, class TBase>
const typename ImageWrapper<TTraits,TBase>::ITKTransformType *
ImageWrapper<TTraits,TBase>
::GetITKTransform() const
{
  return m_Slicer[0]->GetObliqueTransform();
}


template<class TTraits, class TBase>
typename ImageWrapper<TTraits,TBase>::ImageBaseType *
ImageWrapper<TTraits,TBase>
::GetReferenceSpace() const
{
  return m_ReferenceSpace;
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
  // This code is robust to non-orthogonal slicing
  return this->m_Slicer[0]->LookupIntensityAtReferenceIndex(this->m_ReferenceSpace, index);
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
    // Set the slice using that axis
    m_Slicer[i]->SetSliceIndex(to_itkIndex(cursor));
  }
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::SetDisplayViewportGeometry(unsigned int index,
    const ImageBaseType *viewport_image)
{
  m_Slicer[index]->SetObliqueReferenceImage(viewport_image);
}

template<class TTraits, class TBase>
const typename ImageWrapper<TTraits,TBase>::ImageBaseType*
ImageWrapper<TTraits,TBase>
::GetDisplayViewportGeometry(unsigned int index) const
{
  return m_Slicer[index]->GetObliqueReferenceImage();
}


template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::UpdateImageGeometry()
{
  // This method updates the internally stored image geometry object and
  // the image coordinate transforms in all the slicers based on three
  // pieces of information that are stored in the wrapper:
  //   1. Image size
  //   2. Image direction matrix
  //   3. Display to anatomy transforms (m_DisplayGeometry)
  // This method must be called whenever one of these parameters changes.

  // Create an image coordinate geometry based on the current state
  if(m_ReferenceSpace)
    {
    // Set the geometry based on the current image characteristics
    m_ImageGeometry.SetGeometry(
          m_ReferenceSpace->GetDirection().GetVnlMatrix(),
          m_DisplayGeometry,
          m_ReferenceSpace->GetLargestPossibleRegion().GetSize());

    // Update the geometry for each slice
    for(unsigned int iSlice = 0;iSlice < 3;iSlice ++)
      {
      // Assign the new geometry to the slicer
      m_Slicer[iSlice]->SetOrthogonalTransform(
            m_ImageGeometry.GetImageToDisplayTransform(iSlice));

      // TODO: is this necessary and the right place to do ut?
      // Invalidate the requested region in the display slice. This will
      // cause the RR to reset to largest possible region on next Update
      typename DisplaySliceType::RegionType invalidRegion;
      this->GetDisplaySlice(iSlice)->SetRequestedRegion(invalidRegion);
      }

    // Cause the axis indices in the slicers to be updated due to reorientation
    this->SetSliceIndex(this->GetSliceIndex());
    }
  else
    {
    // Identity matrix
    typename ImageType::DirectionType dirmat;
    dirmat.SetIdentity();

    // Zero size
    typename ImageType::SizeType size;

    // Set the geometry to default values
    m_ImageGeometry.SetGeometry(dirmat.GetVnlMatrix(), m_DisplayGeometry, size);

    // TODO: why are we not updating the slicers?
    // TODO: does this code even get run?
    }
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::UpdateNiftiTransforms()
{
  assert(m_ReferenceSpace);

  // Update the NIFTI/RAS transform
  m_NiftiSform = ImageWrapperBase::ConstructNiftiSform(
    m_ReferenceSpace->GetDirection().GetVnlMatrix(),
    m_ReferenceSpace->GetOrigin().GetVnlVector(),
    m_ReferenceSpace->GetSpacing().GetVnlVector());

  // Compute the inverse transform
  m_NiftiInvSform = vnl_inverse(m_NiftiSform);
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
  // TODO: this is wasteful computing inverse for something that should be cached
  const ImageCoordinateTransform *tran = m_Slicer[iSlice]->GetOrthogonalTransform();
  ImageCoordinateTransform::Pointer traninv = ImageCoordinateTransform::New();
  tran->ComputeInverse(traninv);
  return traninv->GetCoordinateIndexZeroBased(2);
}

template<class TTraits, class TBase>
typename ImageWrapper<TTraits,TBase>::SliceType*
ImageWrapper<TTraits,TBase>
::GetSlice(unsigned int dimension)
{
  return m_Slicer[dimension]->GetOutput();
}

// template<class TTraits, class TBase>
// typename ImageWrapper<TTraits,TBase>::InternalPixelType *
// ImageWrapper<TTraits,TBase>
// ::GetVoxelPointer() const
// {
  // return m_Image->GetBufferPointer();
// }


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
void
ImageWrapper<TTraits, TBase>
::SetFileName(const std::string &name)
{
  m_FileName = name;
  m_FileNameShort = itksys::SystemTools::GetFilenameWithoutExtension(
        itksys::SystemTools::GetFilenameName(name));
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

template<class TTraits, class TBase>
const Registry &
ImageWrapper<TTraits, TBase>
::GetIOHints() const
{
  return *this->m_IOHints;
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits, TBase>
::SetIOHints(const Registry &io_hints)
{
  *this->m_IOHints = io_hints;
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::WriteToFileInInternalFormat(const char *filename, Registry &hints)
{
  typedef ImageWrapperPartialSpecializationTraits<ImageType> Specialization;
  Specialization::Write(m_Image, filename, hints);
}


template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::WriteToFile(const char *filename, Registry &hints)
{
  // What kind of mapping are we using
  if(this->GetNativeMapping().IsIdentity())
    {
    // Do the actual writing
    this->WriteToFileInInternalFormat(filename, hints);
    }
  else
    {
    // The image should be converted to float before writing it
    this->WriteToFileAsFloat(filename, hints);
    }

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
    m_Slicer[i]->SetPreviewImage(filter[i]->GetOutput());

    // Mark the preview filters as modified to ensure that the slicer
    // is going to use it. TODO: is this really needed?
    filter[i]->Modified();
    }

  // This is so that IsDrawable() behaves correctly
  m_ImageAssignTime = m_Image->GetTimeStamp();
}

template<class TTraits, class TBase>
void
ImageWrapper<TTraits,TBase>
::DetachPreviewPipeline()
{
  for(int i = 0; i < 3; i++)
    {
    m_Slicer[i]->SetPreviewImage(NULL);
    }
}

template<class TTraits, class TBase>
bool
ImageWrapper<TTraits,TBase>
::IsPreviewPipelineAttached() const
{
  return m_Slicer[0]->GetPreviewImage() != NULL;
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
typename ImageWrapper<TTraits,TBase>::DisplaySlicePointer
ImageWrapper<TTraits,TBase>
::MakeThumbnail(unsigned int maxdim)
{
  // For images with extreme aspect ratios (greater than 1:2) we
  // choose the direction in which the aspect ratio is closest to
  // one. Otherwise, we choose the axial direction.
  double aspect_ratio[3];
  for(int i = 0; i < 3; i++)
    {
    // Get the slice
    DisplaySliceType *slice = this->GetDisplaySlice(2);

    // The size of the slice
    Vector2ui slice_dim = slice->GetBufferedRegion().GetSize();

    // The physical extents of the slice
    Vector2d slice_extent(slice->GetSpacing()[0] * slice_dim[0],
                          slice->GetSpacing()[1] * slice_dim[1]);

    // The aspect ratio of the slice
    if(slice_extent[0] < slice_extent[1])
      aspect_ratio[i] = slice_extent[0] / slice_extent[1];
    else
      aspect_ratio[i] = slice_extent[1] / slice_extent[0];
    }

  // Choose which aspect ratio to use
  int thumb_axis = -1;
  if(aspect_ratio[2] >= 0.5 || (aspect_ratio[2] > aspect_ratio[0] && aspect_ratio[2] > aspect_ratio[1]))
    thumb_axis = 2;
  else if(aspect_ratio[1] > aspect_ratio[0] && aspect_ratio[1] > aspect_ratio[2])
    thumb_axis = 1;
  else
    thumb_axis = 0;

  // Get the display slice
  // For now, just use the z-axis for exporting the thumbnails
  DisplaySliceType *slice = this->GetDisplaySlice(thumb_axis);
  slice->GetSource()->UpdateLargestPossibleRegion();

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

  // Return the result
  opaquer->Update();
  DisplaySlicePointer result = opaquer->GetOutput();
  return result;
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
  reg["Tags"].PutList(m_Tags);
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
  reg["Tags"].GetList(m_Tags);
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
SmartPtr<ImageWrapperBase>
ImageWrapper<TTraits,TBase>
::ExtractROI(const SNAPSegmentationROISettings &roi,
             itk::Command *progressCommand) const
{
  // Get the ITK image for the ROI
  ImagePointer newImage = this->DeepCopyRegion(roi, progressCommand);

  // Initialize the new wrapper
  typedef typename TTraits::WrapperType WrapperType;
  SmartPtr<WrapperType> newWrapper = WrapperType::New();

  // Copy the display to anatomy geometry to the new wrapper
  IRISDisplayGeometry temp = m_DisplayGeometry;
  newWrapper->SetDisplayGeometry(temp);

  // Assign the new image to the new wrapper
  newWrapper->SetImage(newImage);
  newWrapper->SetNativeMapping(this->GetNativeMapping());

  // Appropriate the default nickname?
  newWrapper->SetDefaultNickname(this->GetDefaultNickname());
  newWrapper->SetAlpha(this->GetAlpha());
  newWrapper->SetSticky(this->IsSticky());

  // We should not copy the user-assigned metadata. It's up to the
  // user what should propagate to the ROI

  // Cast to base class
  SmartPtr<ImageWrapperBase> retptr = newWrapper.GetPointer();
  return retptr;
}


template<class TTraits, class TBase>
typename ImageWrapper<TTraits,TBase>::ImagePointer
ImageWrapper<TTraits,TBase>
::DeepCopyRegion(const SNAPSegmentationROISettings &roi,
                 itk::Command *progressCommand) const
{
  // If the image in this wrapper is not the same as the reference space,
  // we must force resampling to occur
  bool force_resampling = !this->IsSlicingOrthogonal();

  // We use partial template specialization here because region copy is
  // only supported for images that are concrete (Image, VectorImage)
  typedef ImageWrapperPartialSpecializationTraits<ImageType> Specialization;
  return Specialization::CopyRegion(
        m_Image, m_ReferenceSpace,
        this->GetITKTransform(), roi,
        force_resampling, progressCommand);
}












// Allowed types of image wrappers
template class ImageWrapper<SpeedImageWrapperTraits, ScalarImageWrapperBase>;
template class ImageWrapper<LabelImageWrapperTraits, ScalarImageWrapperBase>;
template class ImageWrapper<LevelSetImageWrapperTraits, ScalarImageWrapperBase>;

template class ImageWrapper<AnatomicImageWrapperTraits<GreyType>, VectorImageWrapperBase>;
template class ImageWrapper<AnatomicScalarImageWrapperTraits<GreyType>, ScalarImageWrapperBase>;
template class ImageWrapper<ComponentImageWrapperTraits<GreyType>, ScalarImageWrapperBase>;

typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMagnitudeFunctor> MagTraits;
typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMaxFunctor> MaxTraits;
typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMeanFunctor> MeanTraits;
template class ImageWrapper<MagTraits, ScalarImageWrapperBase>;
template class ImageWrapper<MaxTraits, ScalarImageWrapperBase>;
template class ImageWrapper<MeanTraits, ScalarImageWrapperBase>;
