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
#include "ImageWrapperTraits.h"
#include "AdaptiveSlicingPipeline.h"
#include "itkRegionOfInterestImageFilter.h"
#include "SNAPSegmentationROISettings.h"
#include "itkCommand.h"
#include "ImageCoordinateGeometry.h"
#include <itkImageFileWriter.h>
#include <itkResampleImageFilter.h>
#include <itkIdentityTransform.h>
#include <itkFlipImageFilter.h>
#include <itkUnaryFunctorImageFilter.h>
#include "UnaryFunctorVectorImageFilter.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkBSplineInterpolateImageFunction.h"
#include "itkWindowedSincInterpolateImageFunction.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkConstantBoundaryCondition.h"
#include "IRISException.h"
#include "itkImageAdaptor.h"
#include "itkVectorImageToImageAdaptor.h"
#include "GuidedNativeImageIO.h"
#include "itkMatrixOffsetTransformBase.h"
#include "AffineTransformHelper.h"
#include "InputSelectionImageFilter.h"
#include "MetaDataAccess.h"
#include "itkCastImageFilter.h"
#include "RLEImageRegionConstIterator.h"
#include "TDigestImageFilter.h"
#include "AllPurposeProgressAccumulator.h"

#include <vnl/vnl_inverse.h>
#include <iostream>
#include <cassert>

#include <itksys/SystemTools.hxx>

template <class TPixel>
class SimpleCastToDoubleFunctor
{
public:
  typedef TPixel InputType;
  typedef double OutputType;
  double operator()(TPixel input) { return static_cast<double>(input); }
};


/**
 This method copies image information from a 4D image to a 3D image. This includes region, spacing, origin, direction
 and the number of components.
 */
template <class Image4DType, class ImageType>
void CopyInformationFrom4DToTimePoint(Image4DType *image_4d, ImageType *image_tp)
{
  typename ImageType::RegionType region;
  typename ImageType::SpacingType spacing;
  typename ImageType::PointType origin;
  typename ImageType::DirectionType dir;
  for(unsigned int j = 0; j < 3; j++)
    {
    region.SetSize(j, image_4d->GetBufferedRegion().GetSize()[j]);
    region.SetIndex(j, image_4d->GetBufferedRegion().GetIndex()[j]);
    spacing[j] = image_4d->GetSpacing()[j];
    origin[j] = image_4d->GetOrigin()[j];
    for(unsigned int k = 0; k < 3; k++)
      dir(j,k) = image_4d->GetDirection()(j,k);
    }

  // All of the information from the 4D image is propagaged to the 3D timepoints
  image_tp->SetRegions(region);
  image_tp->SetSpacing(spacing);
  image_tp->SetOrigin(origin);
  image_tp->SetDirection(dir);
  image_tp->SetNumberOfComponentsPerPixel(image_4d->GetNumberOfComponentsPerPixel());
}


/* ================================================================================
 * IMAGE-LEVEL PARTIAL SPECIALIZATION CODE
 * ================================================================================ */

/**
 * Image-level template specialization code: things we need
 * to obtain from images when we don't know their exact type
 */
template <class TImage>
class ImagePartialSpecialization
{
public:
  typedef typename TImage::PixelType PixelType;
  typedef typename TImage::InternalPixelType InternalPixelType;
  typedef itk::VariableLengthVector<InternalPixelType> VectorPixelType;

  static int GetNumberOfComponents(TImage *image) { return 1; }
  typedef std::is_base_of<VectorPixelType, PixelType> IsVector;
};

template <class TPixel, unsigned int VDim>
class ImagePartialSpecialization<itk::VectorImage<TPixel, VDim> >
{
public:
  typedef itk::VectorImage<TPixel, VDim> TImage;
  typedef std::true_type IsVector;
  static int GetNumberOfComponents(TImage *image) { return image->GetNumberOfComponentsPerPixel(); }
};



/**
 * Some functions in the image wrapper are only defined for 'concrete' image
 * wrappers, i.e., those that store an image or a vectorimage. These functions
 * involve copying subregions, filling the buffer, IO, etc. To handle this
 * differential availability of functionality, we use partial template
 * specialization below.
 */
template <class TImage, class TImage4D>
class ImageWrapperPartialSpecializationTraitsBase
{
public:
  typedef TImage ImageType;
  typedef typename TImage::PixelType PixelType;

  typedef itk::ImageBase<TImage::ImageDimension> ImageBaseType;
  typedef itk::Transform<double, TImage::ImageDimension, TImage::ImageDimension> TransformType;
  typedef TImage4D Image4DType;
  typedef std::vector<std::pair<int, int> > PatchOffsetTable;

  static void FillBuffer(ImageType *image, PixelType itkNotUsed(value))
  {
    throw IRISException("FillBuffer unsupported for class %s",
                        image->GetNameOfClass());
  }

  static void FillBuffer(Image4DType *image, PixelType itkNotUsed(value))
  {
    throw IRISException("FillBuffer unsupported for class %s",
                        image->GetNameOfClass());
  }

  // This image will either save or 3D or 4D image
  template <class TSavedImage> static void Write(TSavedImage *image,
                                                 const char *itkNotUsed(fname),
                                                 Registry &itkNotUsed(hints))
  {
    throw IRISException("Write unsupported for class %s",
                        image->GetNameOfClass());
  }

  static void WriteAsFloat(ImageType *image,
                           const char *itkNotUsed(fname),
                           Registry &itkNotUsed(hints),
                           double itkNotUsed(shift),
                           double itkNotUsed(scale))
  {
    throw IRISException("WriteAsFloat unsupported for class %s",
                        image->GetNameOfClass());
  }

  static SmartPtr<ImageType> CopyRegion(ImageType *image,
                                        ImageBaseType *itkNotUsed(ref_space),
                                        const TransformType *itkNotUsed(transform),
                                        const SNAPSegmentationROISettings &itkNotUsed(roi),
                                        bool itkNotUsed(force_resampling),
                                        itk::Command *itkNotUsed(progressCommand))
  {
    throw IRISException("CopyRegion unsupported for class %s",
                        image->GetNameOfClass());
    return NULL;
  }

  static void ConfigureTimePointImageFromImage4D(Image4DType *image_4d,
                                                 ImageType *itkNotUsed(image_tp),
                                                 unsigned int itkNotUsed(tp))
  {
    throw IRISException("ConfigureTimePointImageFromImage4D unsupported for class %s",
                        image_4d->GetNameOfClass());
  }

  static void AssignPixelContainerFromTimePointTo4D(Image4DType *image_4d,
                                                    ImageType *itkNotUsed(image_tp))
  {
    throw IRISException("AssignPixelContainerFromTimePointTo4D unsupported for class %s",
                        image_4d->GetNameOfClass());
  }

  static void SetSourceNativeMapping(Image4DType *image_4d, double itkNotUsed(scale), double itkNotUsed(shift))
  {
    throw IRISException("SetSourceNativeMapping unsupported for class %s",
                        image_4d->GetNameOfClass());
  }

  static void UpdatePixelContainer(Image4DType *image_4d,
                                   typename Image4DType::PixelContainer *itkNotUsed(container))
  {
    throw IRISException("UpdatePixelContainer unsupported for class %s",
                        image_4d->GetNameOfClass());
  }

  static PatchOffsetTable GetPatchOffsetTable(TImage *image, const itk::Size<3> &)
  {
    throw IRISException("GetPatchOffsetTable unsupported for class %s", image->GetNameOfClass());
    return PatchOffsetTable();
  }

  static void SamplePatchAsDouble(TImage *image, const itk::Index<3> &idx,
                                  const PatchOffsetTable &offset_table,
                                  double *out)
  {
    throw IRISException("SamplePatchAsDouble unsupported for class %s", image->GetNameOfClass());

  }

  /*
  template <typename TPixel>
  static void UpdateImportPointer(Image4DType *image_4d,
                                  TPixel *itkNotUsed(ptr),
                                  unsigned long itkNotUsed(size))
  {
    throw IRISException("UpdateImportPointer unsupported for class %s",
                        image_4d->GetNameOfClass());
  }
  */
};


/**
 * This is a common implementation of the specialization traits for image-like objects
 * (Image, VectorImage, RLEImage)
 */
template <class TImage, class TImage4D>
class ImageWrapperPartialSpecializationTraitsCommon :
    public ImageWrapperPartialSpecializationTraitsBase<TImage, TImage4D>
{
public:
  typedef ImageWrapperPartialSpecializationTraitsBase<TImage, TImage4D> Superclass;
  typedef TImage ImageType;
  typedef TImage4D Image4DType;
  typedef typename TImage::PixelType PixelType;
  typedef typename TImage::InternalPixelType InternalPixelType;
  typedef itk::ImageBase<TImage::ImageDimension> ImageBaseType;
  typedef itk::Transform<double, TImage::ImageDimension, TImage::ImageDimension> TransformType;
  typedef typename Superclass::PatchOffsetTable PatchOffsetTable;

  static void FillBuffer(ImageType *image, PixelType p)
  {
    image->FillBuffer(p);
  }

  static void FillBuffer(Image4DType *image, PixelType p)
  {
    image->FillBuffer(p);
  }

  template <class TSavedImage> static void Write(TSavedImage *image, const char *fname, Registry &hints)
  {
    SmartPtr<GuidedNativeImageIO> io = GuidedNativeImageIO::New();
    io->CreateImageIO(fname, hints, false);
    itk::ImageIOBase *base = io->GetIOBase();

    typedef itk::ImageFileWriter<TSavedImage> WriterType;
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

  static void ConfigureTimePointImageFromImage4D(Image4DType *image_4d,
                                                 ImageType *image_tp,
                                                 unsigned int tp)
  {
    unsigned int nt = image_4d->GetBufferedRegion().GetSize()[TImage::ImageDimension];
    unsigned int bytes_per_volume = image_4d->GetPixelContainer()->Size() / nt;
  
    // Copy the information from 4D image to 3D image
    CopyInformationFrom4DToTimePoint(image_4d, image_tp);

    // Set the buffer pointer
    image_tp->GetPixelContainer()->SetImportPointer(
          image_4d->GetBufferPointer() + bytes_per_volume * tp,
          bytes_per_volume);
  }

  static void AssignPixelContainerFromTimePointTo4D(Image4DType *image_4d,
                                                    ImageType *image_tp)
  {
    image_4d->SetPixelContainer(image_tp->GetPixelContainer());
  }

  static void UpdatePixelContainer(Image4DType *image_4d,
                                   typename Image4DType::PixelContainer *container)
  {
    image_4d->SetPixelContainer(container);
  }

  static PatchOffsetTable GetPatchOffsetTable(TImage *image, const itk::Size<3> &radius)
  {
    // Create an iterator over the output image
    typedef itk::ImageLinearIteratorWithIndex<ImageType> IterBase;
    typedef IteratorExtender<IterBase> IterType;
    PatchOffsetTable offset_table;

    // Create the region
    itk::ImageRegion<3> region;
    itk::Index<3> center;
    for(unsigned int i = 0; i < 3; i++)
      {
      region.SetIndex(i, 0);
      region.SetSize(i, 2*radius[i]+1);
      center[i] = radius[i];
      }

    // Get the offset of the center
    IterType it(image, region);
    it.SetIndex(center);
    const auto *p_center = it.GetPixelPointer(image);

    // Iterate over lines in the region
    for(it.GoToBegin(); !it.IsAtEnd(); it.NextLine())
      {
      // The offset at the beginning of the line
      int offset_in_bytes_begin = (int)(it.GetPixelPointer(image) - p_center);

      // The offset at the end of line
      it.GoToEndOfLine();
      int offset_in_bytes_end = (int)(it.GetPixelPointer(image) - p_center);

      // Representation of the line
      offset_table.push_back(std::make_pair(offset_in_bytes_begin, offset_in_bytes_end));
      }

    return offset_table;
  }

  static void SamplePatchAsDouble(TImage *image, const itk::Index<3> &idx,
                                  const PatchOffsetTable &offset_table,
                                  double *out)
  {

    // Compute the buffer offset for the center pixel
    typedef itk::ImageHelper<3, 3> Helper;
    typename Helper::OffsetValueType offset = 0;
    Helper::ComputeOffset(image->GetBufferedRegion().GetIndex(), idx, image->GetOffsetTable(), offset);
    auto *buffer = image->GetBufferPointer();

    // The offset may need to be scaled, using std::is_base_of should make the compiler
    // ignore the multiplication for non-vector types
    typedef ImagePartialSpecialization<TImage> Specializaton;
    if(Specializaton::IsVector::value)
      offset *= Specializaton::GetNumberOfComponents(image);

    // Sample around the center pixel
    int i = 0;
    for(auto &p : offset_table)
      for(int k = p.first + offset; k < p.second + offset; k++)
        out[i++] = (double) buffer[k];
  }



  /*
  template <class TPixel>
  static void UpdateImportPointer(Image4DType *image_4d,
                                  TPixel *ptr,
                                  unsigned long size)
  {
    image_4d->GetPixelContainer()->SetImportPointer(ptr, size);
  }
  */
};


/**
 * The default specialization class inherits from the base class, in which
 * all specializable methods simply throw an exception
 */
template <class TImage, class TImage4D>
class ImageWrapperPartialSpecializationTraits
    : public ImageWrapperPartialSpecializationTraitsBase<TImage, TImage4D>
{
};


template<class TPixel, unsigned int VDim>
class ImageWrapperPartialSpecializationTraits<
    itk::Image<TPixel, VDim>,
    itk::Image<TPixel, VDim+1> >
    : public ImageWrapperPartialSpecializationTraitsCommon<
    itk::Image<TPixel, VDim>,
    itk::Image<TPixel, VDim+1> >
{
public:
  typedef itk::Image<TPixel, VDim> ImageType;
  typedef itk::Image<TPixel, VDim+1> Image4DType;
  typedef typename ImageType::PixelType PixelType;
  typedef ImageWrapperPartialSpecializationTraitsCommon<ImageType, Image4DType> Superclass;

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
class ImageWrapperPartialSpecializationTraits<
    itk::VectorImage<TPixel, VDim>,
    itk::VectorImage<TPixel, VDim+1> >
    : public ImageWrapperPartialSpecializationTraitsCommon<
    itk::VectorImage<TPixel, VDim>,
    itk::VectorImage<TPixel, VDim+1> >
{
public:
  typedef itk::VectorImage<TPixel, VDim> ImageType;
  typedef itk::VectorImage<TPixel, VDim+1> Image4DType;
  typedef typename ImageType::PixelType PixelType;
  typedef ImageWrapperPartialSpecializationTraitsCommon<ImageType,Image4DType> Superclass;

  typedef itk::Image<float, 3> FloatImageType;
  typedef itk::ImageSource<FloatImageType> FloatImageSource;
  typedef itk::VectorImage<float, 3> FloatVectorImageType;
  typedef itk::ImageSource<FloatVectorImageType> FloatVectorImageSource;

  static void FillBuffer(ImageType *image, PixelType p)
  {
    image->FillBuffer(p);
  }

  static void FillBuffer(Image4DType *image, PixelType p)
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
class ImageWrapperPartialSpecializationTraits<
    RLEImage<TPixel, VDim, CounterType>,
    RLEImage<TPixel, VDim+1, CounterType> >
    : public ImageWrapperPartialSpecializationTraitsBase<
    RLEImage<TPixel, VDim, CounterType>,
    RLEImage<TPixel, VDim+1, CounterType> >
{
public:
  typedef ImageWrapperPartialSpecializationTraits Self;
  typedef RLEImage<TPixel, VDim, CounterType> ImageType;
  typedef RLEImage<TPixel, VDim+1, CounterType> Image4DType;
  typedef itk::Image<TPixel, VDim> UncompressedType;
  typedef typename ImageType::PixelType PixelType;
  typedef itk::ImageBase<VDim> ImageBaseType;
  typedef itk::Transform<double, VDim, VDim> TransformType;
  typedef ImageWrapperPartialSpecializationTraitsBase<ImageType,Image4DType> Superclass;

  static void FillBuffer(ImageType *image, PixelType p)
  {
    image->FillBuffer(p);
  }

  static void FillBuffer(Image4DType *image, PixelType p)
  {
    image->FillBuffer(p);
  }

  template <class TSavedImage> static void Write(TSavedImage *image, const char *fname, Registry &hints)
  {
    //use specialized RoI filter to convert to itk::Image
    typedef itk::Image<TPixel, TSavedImage::ImageDimension> UncompressedType;
    typedef itk::RegionOfInterestImageFilter<TSavedImage, UncompressedType> outConverterType;
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

  static void ConfigureTimePointImageFromImage4D(Image4DType *image_4d,
                                                 ImageType *image_tp,
                                                 unsigned int tp)
  {
    unsigned int nt = image_4d->GetBufferedRegion().GetSize()[VDim];
    unsigned int bytes_per_volume = image_4d->GetBuffer()->GetPixelContainer()->Size() / nt;
  
    // Copy the information
    CopyInformationFrom4DToTimePoint(image_4d, image_tp);

    // Set the buffer pointer
    image_tp->GetBuffer()->GetPixelContainer()->SetImportPointer(
          image_4d->GetBuffer()->GetBufferPointer() + bytes_per_volume * tp,
          bytes_per_volume);
  }


  static void AssignPixelContainerFromTimePointTo4D(Image4DType *image_4d,
                                                    ImageType *image_tp)
  {
    image_4d->GetBuffer()->SetPixelContainer(image_tp->GetBuffer()->GetPixelContainer());
  }  
};

/**
 * This templated code is shared by the different ImageAdapter specializations below
 */
template <class TImageAdaptor, class TImageAdaptor4D>
class ImageWrapperPartialSpecializationTraitsImageAdaptorCommon
: public ImageWrapperPartialSpecializationTraitsBase<TImageAdaptor, TImageAdaptor4D>
{
public:
  typedef typename TImageAdaptor::InternalImageType InternalImageType;
  typedef ImageWrapperPartialSpecializationTraitsBase<TImageAdaptor, TImageAdaptor4D> Superclass;

  static void ConfigureTimePointImageFromImage4D(TImageAdaptor4D *image_4d,
                                                 TImageAdaptor *image_tp,
                                                 unsigned int tp)
 {
   unsigned int nt = image_4d->GetBufferedRegion().GetSize()[TImageAdaptor::ImageDimension];
   unsigned int bytes_per_volume = image_4d->GetPixelContainer()->Size() / nt;

   // Set up a new image for the internals of this timepoint
   typename InternalImageType::Pointer tp_internals = InternalImageType::New();
 
   // Set the information in this internal image from the 4D image
   CopyInformationFrom4DToTimePoint(image_4d, tp_internals.GetPointer());
 
   // Figure out the number of components in the raw image (this is not handled by the call above)
   tp_internals->SetNumberOfComponentsPerPixel(image_4d->GetPixelAccessor().GetVectorLength());

   // Set the pixel container of the internal image
   tp_internals->GetPixelContainer()->SetImportPointer(
         image_4d->GetBufferPointer() + bytes_per_volume * tp, bytes_per_volume);

   // Set the pixel accessor
   image_tp->CopyInformation(tp_internals);
   image_tp->SetImage(tp_internals);
 
   // Update the pixel accessor
   image_tp->SetPixelAccessor(image_4d->GetPixelAccessor());
 }

  static void AssignPixelContainerFromTimePointTo4D(TImageAdaptor4D *image_4d,
                                                    TImageAdaptor *image_tp)
  {
    image_4d->SetPixelContainer(image_tp->GetPixelContainer());
  }

};


template<class TPixel, unsigned int VDim, class TAdaptor>
class ImageWrapperPartialSpecializationTraits<
    itk::ImageAdaptor<itk::VectorImage<TPixel, VDim>, TAdaptor>,
    itk::ImageAdaptor<itk::VectorImage<TPixel, VDim+1>, TAdaptor > >
    : public ImageWrapperPartialSpecializationTraitsImageAdaptorCommon<
    itk::ImageAdaptor<itk::VectorImage<TPixel, VDim>, TAdaptor>,
    itk::ImageAdaptor<itk::VectorImage<TPixel, VDim+1>, TAdaptor > >
{
public:
  typedef itk::ImageAdaptor<itk::VectorImage<TPixel, VDim+1>, TAdaptor > ImageAdaptor4DType;

  static void SetSourceNativeMapping(ImageAdaptor4DType *img, double scale, double shift)
  {
    img->GetPixelAccessor().SetSourceNativeMapping(scale, shift);
  }
};


template<class TPixel, unsigned int VDim>
class ImageWrapperPartialSpecializationTraits<
    itk::VectorImageToImageAdaptor<TPixel, VDim>,
    itk::VectorImageToImageAdaptor<TPixel, VDim+1> >
    : public ImageWrapperPartialSpecializationTraitsImageAdaptorCommon<
    itk::VectorImageToImageAdaptor<TPixel, VDim>,
    itk::VectorImageToImageAdaptor<TPixel, VDim+1> >
{
};


/* ================================================================================
 * PIXEL-LEVEL PARTIAL SPECIALIZATION CODE
 * ================================================================================ */

template <class TPixel, class TComponent>
struct ImageWrapperPixelPartialSpecializationTraits
{
  static void ExportToComponentArray(const TPixel &p, unsigned int itkNotUsed(ncomp), TComponent *arr)
  {
    *arr = p;
  }
};

template <class TComponent>
struct ImageWrapperPixelPartialSpecializationTraits< itk::VariableLengthVector<TComponent>, TComponent >
{
  typedef itk::VariableLengthVector<TComponent> PixelType;
  static void ExportToComponentArray(const PixelType &p, unsigned int ncomp,  TComponent *arr)
  {
    for(unsigned int c = 0; c < ncomp; c++)
      arr[c] = p[c];
  }
};


/* ================================================================================
 * IMAGE WRAPPER IMPLEMENTATION BEGINS
 * ================================================================================ */
template<class TTraits>
ImageWrapper<TTraits>
::ImageWrapper()
{
  // Set initial state
  m_Initialized = false;
  m_PipelineReady = false;
  m_ReferenceSpace = nullptr;

  // Create empty IO hints
  m_IOHints = new Registry();

  // Create the slicers
  for(unsigned int i = 0; i < 3; i++)
    m_Slicers[i] = SlicerType::New();

  // Initialize the display mapping
  m_DisplayMapping = DisplayMapping::New();
  m_DisplayMapping->Initialize(static_cast<typename DisplayMapping::WrapperType *>(this));

  // Set sticky flag
  m_Sticky = TTraits::StickyByDefault;
  m_DisplayMapping->SetSticky(m_Sticky);

  // By default, the parent wrapper is NULL. This is overridden for wrappers
  // that are derived from vector wrappers. See VectorImageWrapper::CreateDerivedWrapper
  m_ParentWrapper = NULL;

  // Initialize the t-digest filter
  m_TDigestFilter = TDigestFilterType::New();

  // Update the image geometry to default value
  this->UpdateImageGeometry();
}

template<class TTraits>
ImageWrapper<TTraits>
::~ImageWrapper()
{
  Reset();
  delete m_IOHints;
}

template<class TTraits>
ImageWrapper<TTraits>
::ImageWrapper(const Self &copy)
  : ImageWrapper()
{
  m_ReferenceSpace = nullptr;

  // If the source contains an image, make a copy of that image
  if (copy.IsInitialized() && copy.GetImage())
    {
    typedef itk::RegionOfInterestImageFilter<Image4DType, Image4DType> roiType;
    typename roiType::Pointer roi = roiType::New();
    roi->SetInput(copy.m_Image4D);
    roi->Update();
    Image4DPointer newImage = roi->GetOutput();
    UpdateWrappedImages(newImage);
    }

  // Copy IO hints
  *m_IOHints = copy.GetIOHints();
}

template<class TTraits>
const ImageCoordinateTransform *
ImageWrapper<TTraits>
::GetImageToDisplayTransform(unsigned int iSlice) const
{
  return m_ImageGeometry->GetImageToDisplayTransform(iSlice);
}

template<class TTraits>
void
ImageWrapper<TTraits>
::SetDisplayGeometry(const IRISDisplayGeometry &dispGeom)
{
  // Set the display geometry
  m_DisplayGeometry = dispGeom;

  // Update the image geometry object and the slicers
  this->UpdateImageGeometry();
}

template<class TTraits>
void
ImageWrapper<TTraits>
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

template<class TTraits>
void
ImageWrapper<TTraits>
::CopyImageCoordinateTransform(const ImageWrapperBase *source)
{
  // Better have the image!
  itkAssertOrThrowMacro(
        m_Image && source->GetImageBase(),
        "Both target and source must have images in ImageWrapper::CopyImageCoordinateTransform")

  // Set the new meta-data on the image, applying to all time points
  for(ImagePointer img : m_ImageTimePoints)
    {
    img->SetSpacing(source->GetImageBase()->GetSpacing());
    img->SetOrigin(source->GetImageBase()->GetOrigin());
    img->SetDirection(source->GetImageBase()->GetDirection());
    }

  // Also update the transforms on the 4D image
  m_Image4D->SetSpacing(source->GetImage4DBase()->GetSpacing());
  m_Image4D->SetOrigin(source->GetImage4DBase()->GetOrigin());
  m_Image4D->SetDirection(source->GetImage4DBase()->GetDirection());

  // Update NIFTI transforms
  this->UpdateNiftiTransforms();

  // Update the image geometry
  this->UpdateImageGeometry();
}

template<class TTraits>
typename ImageWrapper<TTraits>::ImageBaseType *
ImageWrapper<TTraits>
::GetImageBase() const
{
  m_TimePointSelectFilter->Update();
  return m_Image;
}

template<class TTraits>
const typename ImageWrapper<TTraits>::ImageType *
ImageWrapper<TTraits>
::GetImage() const
{
  m_TimePointSelectFilter->Update();
  return m_Image;
}

template<class TTraits>
Vector3ui
ImageWrapper<TTraits>
::GetSize() const
{
  // Cast the size to our vector format
  itk::Size<3> size = m_Image->GetLargestPossibleRegion().GetSize();
  return Vector3ui(
        (unsigned int) size[0],
        (unsigned int) size[1],
      (unsigned int) size[2]);
  }

template<class TTraits>
void ImageWrapper<TTraits>::SetSticky(bool value)
{
  m_Sticky = value;
  m_DisplayMapping->SetSticky(value);
  this->Modified();
  this->InvokeEvent(WrapperVisibilityChangeEvent());
  this->InvokeEvent(WrapperDisplayMappingChangeEvent());
}

template<class TTraits>
bool
ImageWrapper<TTraits>
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
        || m_Image4D->GetMTime() > m_ImageAssignTime;
    }

  // Otherwise, it's drawable
  return true;
}


template<class TTraits>
itk::ImageRegion<3>
ImageWrapper<TTraits>
::GetBufferedRegion() const
{
  return m_ImageBase->GetBufferedRegion();
}

template<class TTraits>
size_t
ImageWrapper<TTraits>
::GetNumberOfVoxels() const
{
  return m_ImageBase->GetBufferedRegion().GetNumberOfPixels();
}

template<class TTraits>
Vector3d
ImageWrapper<TTraits>
::TransformVoxelCIndexToLPSCoordinates(const Vector3d &iVoxel) const
{
  // Use the ITK method to do this
  itk::ContinuousIndex<double, 3> xIndex;
  for(size_t d = 0; d < 3; d++) xIndex[d] = iVoxel[d];

  itk::Point<double, 3> xPoint;
  m_ReferenceSpace->TransformContinuousIndexToPhysicalPoint(xIndex, xPoint);

  return Vector3d(xPoint);
}

template<class TTraits>
Vector3d
ImageWrapper<TTraits>
::TransformVoxelIndexToLPSCoordinates(const Vector3i &iVoxel) const
{
  // Use the ITK method to do this
  typename ImageBaseType::IndexType xIndex = to_itkIndex(iVoxel);

  itk::Point<double, 3> xPoint;
  m_ReferenceSpace->TransformIndexToPhysicalPoint(xIndex, xPoint);

  return Vector3d(xPoint);
}

template<class TTraits>
Vector3d
ImageWrapper<TTraits>
::TransformLPSCoordinatesToVoxelCIndex(const Vector3d &vLPS) const
{
  itk::Point<double, 3> xPoint;
  for(size_t d = 0; d < 3; d++) xPoint[d] = vLPS[d];

  // Use the ITK method to do this
  itk::ContinuousIndex<double, 3> xIndex;

  m_ReferenceSpace->TransformPhysicalPointToContinuousIndex(xPoint, xIndex);

  return Vector3d(xIndex);
}

template<class TTraits>
Vector3i
ImageWrapper<TTraits>
::TransformLPSCoordinatesToVoxelIndex(const Vector3d &vLPS) const
{
  itk::Point<double, 3> xPoint;
  for(size_t d = 0; d < 3; d++) xPoint[d] = vLPS[d];

  // Use the ITK method to do this
  typename ImageBaseType::IndexType xIndex;

  m_ReferenceSpace->TransformPhysicalPointToIndex(xPoint, xIndex);

  return Vector3i(xIndex);
}

template<class TTraits>
Vector3d
ImageWrapper<TTraits>
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

template<class TTraits>
Vector3d
ImageWrapper<TTraits>
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

template<class TTraits>
void
ImageWrapper<TTraits>
::TransformReferenceCIndexToWrappedImageCIndex(
    const itk::ContinuousIndex<double, 3> &ref_index, itk::ContinuousIndex<double, 3> &img_index) const
{
  // There appears to be a bug in ITK, where in itk::ImageAdaptor, the methods
  // TransformPhysicalPointToContinuousIndex, etc. are not marked as override
  // and so virtual calls to them from base pointer do not work. To overcome
  // this we rely on the parent method when available.
  if(m_ParentWrapper)
    {
    m_ParentWrapper->TransformReferenceCIndexToWrappedImageCIndex(ref_index, img_index);
    }
  else
    {
    if(m_ImageSpaceMatchesReferenceSpace)
      {
      // If the reference space matches the wrapped image space, the transformation
      // is going to be identity
      for(unsigned int i = 0; i < 3; i++)
        img_index[i] = ref_index[i];
      }
    else
      {
      itk::Point<double, 3> x_phys_ref, x_phys_img;

      // Map the index to the physical space of the reference image
      m_ReferenceSpace->TransformContinuousIndexToPhysicalPoint(ref_index, x_phys_ref);

      // Apply the transform to the reference point
      x_phys_img = m_AffineTransform->TransformPoint(x_phys_ref);

      // Map the coordinate into image voxel space
      m_ImageBase->TransformPhysicalPointToContinuousIndex(x_phys_img, img_index);
      }
    }
}

template<class TTraits>
bool ImageWrapper<TTraits>
::ImageSpaceMatchesReferenceSpace() const
{
  return m_ImageSpaceMatchesReferenceSpace;
}


template<class TTraits>
void
ImageWrapper<TTraits>
::PrintDebugInformation()
{
  std::cout << "=== Image Properties ===" << std::endl;
  std::cout << "   Dimensions         : " << m_Image->GetLargestPossibleRegion().GetSize() << std::endl;
  std::cout << "   Origin             : " << m_Image->GetOrigin() << std::endl;
  std::cout << "   Spacing            : " << m_Image->GetSpacing() << std::endl;
  std::cout << "   Time Points        : " << m_ImageTimePoints.size() << std::endl;
  std::cout << "------------------------" << std::endl;
}


template<class TTraits>
bool
ImageWrapper<TTraits>
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

template<class TTraits>
bool
ImageWrapper<TTraits>
::CanOrthogonalSlicingBeUsed(
    ImageType *image, ImageBaseType *referenceSpace, ITKTransformType *transform)
{
  // For orthogonal slicing to be usable, two conditions must be met
  //   1. The reference space and the new image must have the same geometry
  //   2. The transform must be identity

  // Additionally, orthogonal slicing becomes quite expensive for very large images
  // because the slice extracted is much larger that the screen region onto which
  // it is then mapped. So we can heuristically set a maximum size after which we
  // do not use orthogonal slicing
  const int max_ortho_dim = 1024;

  // Check if the images have same dimensions
  double tol = 1e-5;
  bool same_geom = CompareGeometry(image, referenceSpace, tol);

  // Use helper class to check for identity
  bool is_identity = AffineTransformHelper::IsIdentity(transform);

  // Check if any of the image dimensions are above the max
  bool is_large = false;
  for(unsigned int d = 0; d < 3; d++)
    if(image->GetBufferedRegion().GetSize()[d] > max_ortho_dim)
      is_large = true;

  return same_geom && is_identity && (!is_large);
}

template<class TTraits>
void
ImageWrapper<TTraits>
::UpdateWrappedImages(
    Image4DType *image_4d,
    ImageBaseType *referenceSpace,
    ITKTransformType *transform)
{
  // Assign the pointer to the 4D image
  m_Image4D = image_4d;

  // The time dimension is the last dimension
  unsigned int nt = image_4d->GetBufferedRegion().GetSize()[3];

  // Assign these 3D volumes as inputs to a timepoint selector
  m_TimePointSelectFilter = TimePointSelectFilter::New();

  // Just create the images
  m_ImageTimePoints.clear();
  for(unsigned int i = 0; i < nt; i++)
    {
    ImagePointer ip = ImageType::New();

    // Set the buffer pointer
    typedef ImageWrapperPartialSpecializationTraits<ImageType, Image4DType> Specialization;
    Specialization::ConfigureTimePointImageFromImage4D(image_4d, ip.GetPointer(), i);

    // Append image to the array
    m_ImageTimePoints.push_back(ip);

    // Add timepoint to the selector filter
    m_TimePointSelectFilter->AddSelectableInput(i, ip);
    }

  // Update the selected time point in the selector
  m_TimePointSelectFilter->SetSelectedInput(m_TimePointIndex);

  // The current logic has the reference space point to the image itself if the image
  // it its own reference. This is not smart because we have to worry about zombie
  // pointers. Here to preserve this logic, we clear m_ReferenceSpace if matches m_ImageBase
  if(m_ReferenceSpace == m_ImageBase)
    m_ReferenceSpace = nullptr;

  // Assign the current timepoint pointers
  m_Image = m_TimePointSelectFilter->GetOutput();
  m_ImageBase = m_Image;

  // Set up the slicers
  for(unsigned int i = 0; i < 3; i++)
    {
    m_Slicers[i]->SetInput(m_Image);
    m_Slicers[i]->SetPreviewImage(nullptr);
    }

  // Mark the image as Modified to enforce correct sequence of
  // operations with MinMaxCalc
  m_Image4D->Modified();

  // Set the image as the input to the TDigest
  m_TDigestFilter->SetInput(m_Image4D);

  // Set the sampling rate in the TDigest. For large images it is too computationally
  // expensive to digest the whole image, so instead we can digest a subset of the pixels.
  // The values here restrict sampling to a value between 500000 and 1000000.
  auto n_values = m_Image4D->GetBufferedRegion().GetNumberOfPixels() * m_Image4D->GetNumberOfComponentsPerPixel();
  double x_oversampling = n_values * 1.0e-6;
  int digest_sampling_rate_log2 = x_oversampling > 1.0 ? (int) std::log2(x_oversampling * 2.0) : 0;
  m_TDigestFilter->SetLog2SamplingRate(digest_sampling_rate_log2);

  // Update the image in the display mapping
  m_DisplayMapping->UpdateImagePointer(m_Image);

  // Update the time point select filter, so that m_Image and m_ImageBase have the right
  // spatial information. This has to be done before the call to SetITKTransform()
  m_TimePointSelectFilter->Update();

  // Update the reference space and transform
  this->SetITKTransform(referenceSpace, transform);

  // Store the time when the image was assigned
  m_ImageAssignTime = m_ImageSaveTime = m_Image4D->GetTimeStamp();

  // We have been initialized
  m_Initialized = true;

  // Update MTime so downstream users can update accordingly
  this->Modified();

}

template<class TTraits>
void
ImageWrapper<TTraits>
::InitializeToWrapper(const ImageWrapperBase *source, Image4DType *image_4d,
                      ImageBaseType *referenceSpace, ITKTransformType *transform)
{
  // Update the display geometry from the source wrapper
  m_DisplayGeometry = source->GetDisplayGeometry();

  // Call the common update method
  UpdateWrappedImages(image_4d, referenceSpace, transform);

  // Update the slice index
  SetSliceIndex(source->GetSliceIndex());
}

template<class TTraits>
void
ImageWrapper<TTraits>
::InitializeToWrapper(const ImageWrapperBase *source, Image4DType *image_4d)
{
  // Create a copy of the transform if it exists
  typedef itk::MatrixOffsetTransformBase<double, 3, 3> AffineTransform;
  const AffineTransform *src_tran =
      dynamic_cast<const AffineTransform *>(source->GetITKTransform());
  SmartPtr<AffineTransform> tran = nullptr;
  if(src_tran)
    {
    tran = AffineTransform::New();
    tran->SetMatrix(src_tran->GetMatrix());
    tran->SetOffset(src_tran->GetOffset());
    }

  // Call the common update method
  this->InitializeToWrapper(source, image_4d, source->GetReferenceSpace(), tran);
}

template<class TTraits>
bool
ImageWrapper<TTraits>
::IsSlicingOrthogonal() const
{
  return m_Slicers[0]->GetUseOrthogonalSlicing();
}

template<class TTraits>
void
ImageWrapper<TTraits>
::InitializeToWrapper(const ImageWrapperBase *source, const PixelType &value)
{
  typedef ImageWrapperPartialSpecializationTraits<ImageType, Image4DType> Specialization;

  // Allocate an empty 4D image to match the source
  Image4DPointer img_new = Image4DType::New();
  img_new->SetRegions(source->GetImage4DBase()->GetBufferedRegion().GetSize());
  img_new->SetSpacing(source->GetImage4DBase()->GetSpacing());
  img_new->SetOrigin(source->GetImage4DBase()->GetOrigin());
  img_new->SetDirection(source->GetImage4DBase()->GetDirection());
  img_new->Allocate();

  // Use specialization to fill the buffer
  Specialization::FillBuffer(img_new.GetPointer(), value);

  // Update the display geometry from the source wrapper
  m_DisplayGeometry = source->GetDisplayGeometry();

  // Call the common update method
  UpdateWrappedImages(img_new);

  // Update the slice index
  SetSliceIndex(source->GetSliceIndex());

  // Update the time point
  SetTimePointIndex(source->GetTimePointIndex());
}

template<class TTraits>
void
ImageWrapper<TTraits>
::SetImage4D(Image4DType *image_4d)
{
  UpdateWrappedImages(image_4d);
}

template<class TTraits>
void
ImageWrapper<TTraits>
::SetImage4D(Image4DType *image_4d, ImageBaseType *refSpace, ITKTransformType *transform)
{
  UpdateWrappedImages(image_4d, refSpace, transform);
}

template<class TTraits>
void
ImageWrapper<TTraits>
::UpdateTimePoint(ImageType *image, int time_point)
{
  itkAssertOrThrowMacro(
        time_point < (int) m_ImageTimePoints.size(),
        "Time point out of range in ImageWrapper::UpdateTimePoint")

  // Get the referenced time point
  if(time_point < 0)
    time_point = m_TimePointIndex;
  ImageType *idest = m_ImageTimePoints[time_point];

  itkAssertOrThrowMacro(
        idest->GetBufferedRegion() == image->GetBufferedRegion(),
        "Source/Destination region mismatch in ImageWrapper::UpdateTimePoint")

  // Use iterators to perform update
  ConstIterator it_src(image, image->GetBufferedRegion());
  Iterator it_dest(m_ImageTimePoints[time_point], image->GetBufferedRegion());
  while(!it_src.IsAtEnd())
    {
    it_dest.Set(it_src.Get());
    ++it_src;
    ++it_dest;
    }

  // Set modification (we are not keeping track of number of updated voxels because of
  // potential added overhead
  PixelsModified();
}

template<class TTraits>
void
ImageWrapper<TTraits>
::SetITKTransform(ImageBaseType *refSpace, ITKTransformType *transform)
{
  // If nullptr passed as the reference space, make the reference be the image itself
  if(!refSpace)
    {
    refSpace = m_ImageBase;
    }

  // If the transform is nullptr, set it to identity
  if(transform)
    {
    m_AffineTransform = transform;
    }
  else
    {
    typedef itk::IdentityTransform<double, 3> IdTransformType;
    typename IdTransformType::Pointer idTran = IdTransformType::New();
    m_AffineTransform = idTran.GetPointer();
    }

  // Check if the reference geometry has changed
  if(m_ReferenceSpace != refSpace)
    {
    // Check if the geometry actually changed, we need to update geometry and reset the index
    if(!CompareGeometry(m_ReferenceSpace, refSpace))
      {
      // Store the reference space
      m_ReferenceSpace = refSpace;

      // Reset the transform to identity
      this->UpdateImageGeometry();

      // Reset the slice positions to zero
      this->SetSliceIndex(IndexType({{0,0,0}}));
      }
    else
      {
      // Store the reference space
      m_ReferenceSpace = refSpace;
      }

    // Update the NIFTI/RAS transform
    this->UpdateNiftiTransforms();
    }

  // Update the reference space identity flag
  m_ImageSpaceMatchesReferenceSpace =
      CanOrthogonalSlicingBeUsed(m_Image, m_ReferenceSpace, m_AffineTransform);

  // Update the transform
  for(int i = 0; i < 3; i++)
    {
    m_Slicers[i]->SetObliqueTransform(m_AffineTransform);
    m_Slicers[i]->SetUseOrthogonalSlicing(m_ImageSpaceMatchesReferenceSpace);
    m_Slicers[i]->SetOrthogonalTransform(m_ImageGeometry->GetImageToDisplayTransform(i));
    }

  // Fire an update event
  this->InvokeEvent(WrapperDisplayMappingChangeEvent());
}

template<class TTraits>
void
ImageWrapper<TTraits>
::SetReferenceSpace(ImageBaseType *referenceSpace)
{
  this->SetITKTransform(referenceSpace, m_AffineTransform);
}

template<class TTraits>
const typename ImageWrapper<TTraits>::ITKTransformType *
ImageWrapper<TTraits>
::GetITKTransform() const
{
  return m_Slicers[0]->GetObliqueTransform();
}

template<class TTraits>
typename ImageWrapper<TTraits>::ImageBaseType *
ImageWrapper<TTraits>
::GetReferenceSpace() const
{
  return m_ReferenceSpace;
}

template<class TTraits>
void 
ImageWrapper<TTraits>
::Reset()
{
  if (m_Initialized)
    {
    for(ImagePointer img : m_ImageTimePoints)
      img->ReleaseData();

    m_ImageTimePoints.clear();
    if(m_ReferenceSpace == m_ImageBase)
      m_ReferenceSpace = nullptr;
    m_ImageBase = nullptr;
    m_Image = nullptr;
    }
  m_Initialized = false;

  m_Alpha = 0.5;
}


template<class TTraits>
void
ImageWrapper<TTraits>
::SetVoxel(const Vector3ui &index, const PixelType &value)
{
  this->SetVoxel(to_itkIndex(index), value);
}

template<class TTraits>
void
ImageWrapper<TTraits>
::SetVoxel(const itk::Index<3> &index, const PixelType &value)
{
  // Verify that the pixel is contained by the image at debug time
  itkAssertOrThrowMacro(
        m_Image && m_Image->GetLargestPossibleRegion().IsInside(index),
        "Voxel index outside of range")

  // Update the pixel
  m_Image->SetPixel(index, value);

  // The 4D image must receive the modified event
  m_Image4D->Modified();
}

template<class TTraits>
inline typename ImageWrapper<TTraits>::PixelType
ImageWrapper<TTraits>
::GetVoxel(const itk::Index<3> &index, int time_point) const
{
  itkAssertOrThrowMacro(
        time_point < (int) m_ImageTimePoints.size(),
        "Requested time point out of range")

  if(time_point < 0)
    time_point = m_TimePointIndex;

  // Simply use ITK's GetPixel method
  return m_ImageTimePoints[time_point]->GetPixel(index);
  }

template<class TTraits>
typename ImageWrapper<TTraits>::PatchOffsetTable
ImageWrapper<TTraits>
::GetPatchOffsetTable(const SizeType &radius) const
{
  typedef ImageWrapperPartialSpecializationTraits<ImageType, Image4DType> Specialization;
  return Specialization::GetPatchOffsetTable(m_Image, radius);
}

template<class TTraits>
void
ImageWrapper<TTraits>
::SamplePatchAsDouble(
    const IndexType &idx, const PatchOffsetTable &offset_table, double *out_patch) const
{
  typedef ImageWrapperPartialSpecializationTraits<ImageType, Image4DType> Specialization;
  Specialization::SamplePatchAsDouble(m_Image, idx, offset_table, out_patch);
}

template<class TTraits>
void
ImageWrapper<TTraits>
::SetSlicingInterpolationMode(InterpolationMode mode)
{
    for(unsigned int i = 0; i < 3; i++)
        m_Slicers[i]->SetUseNearestNeighbor(mode == Superclass::NEAREST);
}

template<class TTraits>
typename ImageWrapper<TTraits>::InterpolationMode
ImageWrapper<TTraits>
::GetSlicingInterpolationMode() const
{
    return m_Slicers[0]->GetUseNearestNeighbor() ? Superclass::NEAREST : Superclass::LINEAR;
}

template<class TTraits>
void
ImageWrapper<TTraits>
::SampleIntensityAtReferenceIndex(
    const itk::Index<3> &index, int time_point,
    bool map_to_native, vnl_vector<double> &out) const
{
  // Compute and allocate output dimensions
  unsigned int nc = this->GetNumberOfComponents();
  unsigned int nt = this->GetNumberOfTimePoints();
  unsigned int odim = nc * (time_point < 0 ? nt : 1);

  // Allocate the output array if necessary
  if(out.size() != odim)
    out.set_size(odim);

  // Determine the range of timepoints to sample
  unsigned int tp_begin = time_point < 0 ? 0 : (unsigned int) time_point;
  unsigned int tp_end = time_point < 0 ? this->GetNumberOfTimePoints() : (unsigned int) time_point+1;

  // Call the internal method to do the sampling
  this->SampleIntensityAtReferenceIndexInternal(index, tp_begin, tp_end);

  // Remap the intensities to double or native format
  ComponentType *p_begin = m_IntensitySamplingArray.data_block() + nc * tp_begin;
  if(map_to_native)
    for(unsigned int k = 0; k < odim; k++)
      out[k] = m_NativeMapping(p_begin[k]);
  else
    for(unsigned int k = 0; k < odim; k++)
      out[k] = static_cast<double>(p_begin[k]);
}


template<class TTraits>
void
ImageWrapper<TTraits>
::SampleIntensityAtReferenceIndexInternal(
    const itk::Index<3> &index, unsigned int tp_begin, unsigned int tp_end) const
{
  // Compute and allocate output dimensions
  unsigned int nc = this->GetNumberOfComponents();
  unsigned int nt = this->GetNumberOfTimePoints();

  // Make sure the sampling array has been allocated
  if(m_IntensitySamplingArray.size() != nc * nt)
    m_IntensitySamplingArray.set_size(nc * nt);

  // Create a specialization for actual sampling
  using Specialization = ImageWrapperPixelPartialSpecializationTraits<PixelType,ComponentType>;
  using InterpolateWorker = DefaultNonOrthogonalSlicerWorkerTraits<ImageType, SliceType>;

  // Get the raw pixels to write to
  ComponentType *arr = m_IntensitySamplingArray.data_block() + tp_begin * nc;

  // Do we need interpolation?
  if(m_ImageSpaceMatchesReferenceSpace)
    {
    // If the preview pipeline is being used, we need to sample from it
    if(m_Slicers[0]->GetPreviewImage())
      {
      // The index has to be the same as in the slicers, otherwise the preview image lookup
      // will not be valid
      itkAssertOrThrowMacro(m_Slicers[0]->GetSliceIndex() == index,
          "SampleIntensityAtReferenceIndexInternal called with an index that does not match Slicer index")

      // The slicer needs to be updated, so that the preview image is updated in the requested region
      m_Slicers[0]->Update();

      // Lookup the pixel from the preview image
      PixelType p = m_Slicers[0]->GetPreviewImage()->GetPixel(index);
      Specialization::ExportToComponentArray(p, nc, arr);
      }
    else
      {
      // The simple case when no interpolation is required
      for(unsigned int tp = tp_begin; tp < tp_end; tp++, arr+=nc)
        {
        PixelType p = m_ImageTimePoints[tp]->GetPixel(index);
        Specialization::ExportToComponentArray(p, nc, arr);
        }
      }
    }
  else
    {
    // The index at which to sample (will be reused in the loop below)
    itk::ContinuousIndex<double, 3> cidx;
    this->TransformReferenceCIndexToWrappedImageCIndex(index, cidx);
    bool is_nn = this->GetSlicingInterpolationMode() == ImageWrapperBase::NEAREST;

    // Sample all time points
    for(unsigned int tp = tp_begin; tp < tp_end; tp++)
      {
      // Use an interpolator to do the work
      // TODO: too much being initialized here for a single lookup operation!
      InterpolateWorker iw(m_ImageTimePoints[tp]);

      // Process the voxel, arr will be updated by the function
      iw.ProcessVoxel(cidx.GetDataPointer(), is_nn, &arr);
      }
    }
  }

template<class TTraits>
void
ImageWrapper<TTraits>
::SetNativeMapping(NativeIntensityMapping nim)
{
  m_NativeMapping = nim;

  // Propagate the mapping to the tdigest
  m_TDigestFilter->SetIntensityTransform(nim.GetScale(), nim.GetShift());
}

template<class TTraits>
typename ImageWrapper<TTraits>::SlicerType *
ImageWrapper<TTraits>::GetSlicer(unsigned int iDirection) const
{
  return m_Slicers[iDirection];
}

template<class TTraits>
typename ImageWrapper<TTraits>::ConstIterator
ImageWrapper<TTraits>
::GetImageConstIterator() const
{
  ConstIterator it(m_Image,m_Image->GetLargestPossibleRegion());
  it.GoToBegin();
  return it;
}

template<class TTraits>
typename ImageWrapper<TTraits>::Iterator
ImageWrapper<TTraits>
::GetImageIterator()
{
  Iterator it(m_Image,m_Image->GetLargestPossibleRegion());
  it.GoToBegin();
  return it;
}

template<class TTraits>
void
ImageWrapper<TTraits>
::SetSliceIndex(const IndexType &cursor)
{
  // Save the cursor position
  m_SliceIndex = cursor;

  // Select the appropriate slice for each slicer
  for(unsigned int i = 0; i < 3; i++)
    m_Slicers[i]->SetSliceIndex(cursor);
}

template<class TTraits>
void
ImageWrapper<TTraits>
::SetTimePointIndex(unsigned int index)
{
  itkAssertOrThrowMacro(
        index < m_ImageTimePoints.size(),
        "Requested time point out of range")

  // Set the current time index
  if(index != m_TimePointIndex)
    {
    m_TimePointIndex = index;

    // Update the image selector
    m_TimePointSelectFilter->SetSelectedInput(index);
    m_TimePointSelectFilter->Update();
    }
}

template<class TTraits>
const typename ImageWrapper<TTraits>::ImagePointer
ImageWrapper<TTraits>::GetImageByTimePoint(unsigned int timepoint) const
{
  itkAssertOrThrowMacro(
        timepoint < m_ImageTimePoints.size(),
        "Requested time point out of range")

  return m_ImageTimePoints[timepoint];
}

template<class TTraits>
void
ImageWrapper<TTraits>
::SetDisplayViewportGeometry(unsigned int index,
    const ImageBaseType *viewport_image)
{
  m_Slicers[index]->SetObliqueReferenceImage(viewport_image);
}

template<class TTraits>
TDigestDataObject *
ImageWrapper<TTraits>::GetTDigest()
{
  return m_TDigestFilter->GetTDigest();
}

template<class TTraits>
const typename ImageWrapper<TTraits>::MinMaxObjectType *
ImageWrapper<TTraits>::GetImageMinObject()
{
  return m_TDigestFilter->GetImageMin();
}

template<class TTraits>
const typename ImageWrapper<TTraits>::MinMaxObjectType *
ImageWrapper<TTraits>::GetImageMaxObject()
{
  return m_TDigestFilter->GetImageMax();
}



template<class TTraits>
const typename ImageWrapper<TTraits>::ImageBaseType*
ImageWrapper<TTraits>
::GetDisplayViewportGeometry(unsigned int index) const
{
  return m_Slicers[index]->GetObliqueReferenceImage();
}


template<class TTraits>
void
ImageWrapper<TTraits>
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
    // Create an new image geometry object
    SmartPtr<ImageCoordinateGeometry> p = ImageCoordinateGeometry::New();
    m_ImageGeometry = p;

    // Set the geometry based on the current image characteristics
    m_ImageGeometry->SetGeometry(
          m_ReferenceSpace->GetDirection().GetVnlMatrix().as_matrix(),
          m_DisplayGeometry,
          m_ReferenceSpace->GetLargestPossibleRegion().GetSize());

    // Update the geometry for each slice
    for(unsigned int iSlice = 0;iSlice < 3;iSlice ++)
      {
      // Assign the new geometry to the slicers
      m_Slicers[iSlice]->SetOrthogonalTransform(m_ImageGeometry->GetImageToDisplayTransform(iSlice));

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
    m_ImageGeometry = nullptr;
    }
}

template<class TTraits>
void
ImageWrapper<TTraits>
::UpdateNiftiTransforms()
{
  itkAssertOrThrowMacro(
        m_ReferenceSpace,
        "Calling ImageWrapper::UpdateNiftiTransforms on image wrapper with no reference image");

  // Update the NIFTI/RAS transform
  m_NiftiSform = ImageWrapperBase::ConstructNiftiSform(
    m_ReferenceSpace->GetDirection().GetVnlMatrix().as_matrix(),
    m_ReferenceSpace->GetOrigin().GetVnlVector(),
    m_ReferenceSpace->GetSpacing().GetVnlVector());

  // Compute the inverse transform
  m_NiftiInvSform = vnl_inverse(m_NiftiSform);
}

template<class TTraits>
void
ImageWrapper<TTraits>
::PixelsModified()
{
  // Update the 4D image
  m_Image4D->Modified();

  // Update the current time point. Note that we don't update m_Image,
  // which is the output of the time point selection pipeline and thus
  // is not necessarily input to downstream filters.
  m_ImageTimePoints[m_TimePointIndex]->Modified();
  }

template<class TTraits>
void ImageWrapper<TTraits>
::SetPixelContainer(typename ImageType::PixelContainer *container)
{
  itkAssertOrThrowMacro(
        container->Size() == m_Image4D->GetPixelContainer()->Size(),
        "Source array size does not match target array size in SetPixelContainer");

  typedef ImageWrapperPartialSpecializationTraits<ImageType, Image4DType> Specialization;
  Specialization::UpdatePixelContainer(m_Image4D, container);
  for(unsigned int tp = 0; tp < m_ImageTimePoints.size(); tp++)
    Specialization::ConfigureTimePointImageFromImage4D(m_Image4D, m_ImageTimePoints[tp], tp);
  m_TimePointSelectFilter->Update();

  this->PixelsModified();
}

template<class TTraits>
inline double
ImageWrapper<TTraits>
::GetImageMinAsDouble()
{
  m_TDigestFilter->GetTDigest()->Update();
  return (double) m_TDigestFilter->GetTDigest()->GetImageMinimum();
}

template<class TTraits>
inline double
ImageWrapper<TTraits>
::GetImageMaxAsDouble()
{
  m_TDigestFilter->GetTDigest()->Update();
  return (double) m_TDigestFilter->GetTDigest()->GetImageMaximum();
}

template<class TTraits>
inline double
ImageWrapper<TTraits>
::GetImageMinNative()
{
  m_TDigestFilter->GetTDigest()->Update();
  return (double) m_TDigestFilter->GetTDigest()->GetImageMinimum();
}

template<class TTraits>
inline double
ImageWrapper<TTraits>
::GetImageMaxNative()
{
  m_TDigestFilter->GetTDigest()->Update();
  return (double) m_TDigestFilter->GetTDigest()->GetImageMaximum();
}

/** For each slicer, find out which image dimension does is slice along */
template<class TTraits>
unsigned int
ImageWrapper<TTraits>
::GetDisplaySliceImageAxis(unsigned int iSlice)
{
  // TODO: this is wasteful computing inverse for something that should be cached
  const ImageCoordinateTransform *tran = m_Slicers[iSlice]->GetOrthogonalTransform();
  ImageCoordinateTransform::Pointer traninv = ImageCoordinateTransform::New();
  tran->ComputeInverse(traninv);
  return traninv->GetCoordinateIndexZeroBased(2);
}

template<class TTraits>
typename ImageWrapper<TTraits>::SliceType*
ImageWrapper<TTraits>
::GetSlice(unsigned int dimension)
{
  return m_Slicers[dimension]->GetOutput();
}

// TODO: this should take advantage of an in-place filter!
template<class TTraits>
unsigned int
ImageWrapper<TTraits>
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
    PixelsModified();

  // Return the number of replacements
  return nReplaced;
}

template<class TTraits>
unsigned int
ImageWrapper<TTraits>
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
    PixelsModified();

  // Return the number of replacements
  return nReplaced;
}

template<class TTraits>
typename ImageWrapper<TTraits>::DisplaySlicePointer
ImageWrapper<TTraits>::GetDisplaySlice(unsigned int dim)
{
  return m_DisplayMapping->GetDisplaySlice(dim);
}

template<class TTraits>
void
ImageWrapper<TTraits>
::SetFileName(const std::string &name)
{
  m_FileName = name;
  m_FileNameShort = itksys::SystemTools::GetFilenameWithoutExtension(
        itksys::SystemTools::GetFilenameName(name));
  this->InvokeEvent(WrapperMetadataChangeEvent());
}

template<class TTraits>
const std::string &
ImageWrapper<TTraits>
::GetNickname() const
{
  if(m_CustomNickname.length())
    return m_CustomNickname;

  else if(m_FileName.length())
    return m_FileNameShort;

  else return m_DefaultNickname;
}

template<class TTraits>
void
ImageWrapper<TTraits>
::SetCustomNickname(const std::string &nickname)
{
  // Make sure the nickname is real
  if(nickname == m_FileNameShort)
    m_CustomNickname.clear();
  else
    m_CustomNickname = nickname;

  this->InvokeEvent(WrapperMetadataChangeEvent());
}

template<class TTraits>
const Registry &
ImageWrapper<TTraits>
::GetIOHints() const
{
  return *this->m_IOHints;
}

template<class TTraits>
void
ImageWrapper<TTraits>
::SetIOHints(const Registry &io_hints)
{
  *this->m_IOHints = io_hints;
}

template<class TTraits>
void
ImageWrapper<TTraits>
::WriteToFileInInternalFormat(const char *filename, Registry &hints)
{
  typedef ImageWrapperPartialSpecializationTraits<ImageType, Image4DType> Specialization;

  // Write either in 4D or in 3D
  if(this->GetNumberOfTimePoints() > 1)
    Specialization::Write(m_Image4D.GetPointer(), filename, hints);
  else
    Specialization::Write(m_Image, filename, hints);
}

template<class TTraits>
void
ImageWrapper<TTraits>
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
  for(ImagePointer img : m_ImageTimePoints)
    if(m_ImageSaveTime < img->GetTimeStamp())
      m_ImageSaveTime = img->GetTimeStamp();

  // Also check global 4d image time stamp
  if (m_ImageSaveTime < m_Image4D->GetTimeStamp())
    m_ImageSaveTime = m_Image4D->GetTimeStamp();
}

template<class TTraits>
void
ImageWrapper<TTraits>
::WriteCurrentTPImageToFile(const char *filename)
{
  typedef ImageWrapperPartialSpecializationTraits<ImageType, Image4DType> Specialization;
  Registry reg;
  Specialization::Write(m_Image, filename, reg);
  }

template<class TTraits>
void
ImageWrapper<TTraits>
::AttachPreviewPipeline(
    PreviewFilterType *f0, PreviewFilterType *f1, PreviewFilterType *f2)
{
  // Preview pipelines are not supported for image wrappers containing 4D images
  itkAssertOrThrowMacro(
        m_ImageTimePoints.size() == 1,
        "Only single time point images support ImageWrapper::AttachPreviewPipeline")

  std::array<PreviewFilterType *, 3> filter = {{f0, f1, f2}};
  for(int i = 0; i < 3; i++)
    {
    // Update the preview inputs to the slicers
    m_Slicers[i]->SetPreviewImage(filter[i]->GetOutput());

    // Mark the preview filters as modified to ensure that the slicer
    // is going to use it. TODO: is this really needed?
    filter[i]->Modified();
    }

  // This is so that IsDrawable() behaves correctly
  m_ImageAssignTime = m_Image->GetTimeStamp();
}

template<class TTraits>
void
ImageWrapper<TTraits>
::DetachPreviewPipeline()
{
  // Preview pipelines are not supported for image wrappers containing 4D images
  itkAssertOrThrowMacro(
        m_ImageTimePoints.size() == 1,
        "Only single time point images support ImageWrapper::DetachPreviewPipeline")

  for(int i = 0; i < 3; i++)
    {
    m_Slicers[i]->SetPreviewImage(NULL);
    }
}

template<class TTraits>
bool
ImageWrapper<TTraits>
::IsPreviewPipelineAttached() const
{
  // Preview pipelines are not supported for image wrappers containing 4D images
  itkAssertOrThrowMacro(
        m_ImageTimePoints.size() == 1,
        "Only single time point images support ImageWrapper::IsPreviewPipelineAttached")

  return m_Slicers[0]->GetPreviewImage() != NULL;
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

template<class TTraits>
typename ImageWrapper<TTraits>::DisplaySlicePointer
ImageWrapper<TTraits>
::MakeThumbnail(unsigned int maxdim)
{
  // Determine which axis to use for thumbnail generation. Each axis is assigned
  // a penalty based on the following
  //   - Type 1 penalty is 1 if one of the slice dimensions is 1, 0 otherwise
  //   - Type 2 penalty is 0 if slice aspect ratio is < 2.0, otherwise equal to the aspect ratio
  //   - Type 3 penalty is the angle with the axial direction
  using penalty = std::tuple<bool, double, double>;
  using axis_info = std::tuple<penalty, int>;
  std::vector<axis_info> axis_score;

  auto direction = this->GetImageBase()->GetDirection().GetVnlMatrix();
  for(int i = 0; i < 3; i++)
    {
    int j = (i + 1) % 3, k = (i + 2) % 3;
    auto sz_j = this->GetSize()[j], sz_k = this->GetSize()[k];
    double ext_j = sz_j * this->GetImageBase()->GetSpacing()[j];
    double ext_k = sz_k * this->GetImageBase()->GetSpacing()[k];

    // Is the slice 1D?
    bool is_one_d = (sz_j == 1) || (sz_k == 1);

    // Is the aspect ratio off?
    double aspect_ratio = ext_j < ext_k ? ext_j / ext_k : ext_k / ext_j;
    double ar_penalty = (aspect_ratio > 0.5) ? 0.0 : (0.5 - aspect_ratio);

    // Angle with the axial direction
    auto y = direction.get_column(i);
    y.normalize();
    double cos_alpha = y[2];
    double sin_alpha = sqrt(1 - cos_alpha * cos_alpha);

    // Create penalty
    axis_score.push_back(std::make_tuple(std::make_tuple(is_one_d, ar_penalty, sin_alpha), i));
    }

  // Sort based on penalty
  std::sort(axis_score.begin(), axis_score.end());
  int thumb_z_axis = std::get<1>(axis_score[0]);

  // Now that we have sorted this out, we need to find the display axis that best matches
  // the selected direction and use the geometry of that display axis to set up the thumbnail
  // plane. This will make the thumbnail more consistent with what is viewed on the screen
  int display_axis = -1;
  for(int i = 0; i < 3; i++)
    {
    auto *d_to_i = this->GetImageGeometry()->GetDisplayToImageTransform(i);
    unsigned int z_coord_for_display_axis = d_to_i->GetCoordinateIndexZeroBased(2);
    if(z_coord_for_display_axis == thumb_z_axis)
      {
      display_axis = i;
      break;
      }
    }
  auto d_to_i = this->GetImageGeometry()->GetDisplayToImageTransform(display_axis);

  // Now that we have done this, we need to create a reference image that matches the slice
  // direction. We already know the axis in image space of the slicing direction, but now
  // we need to determine how the x and y axes of the thumbnail will map to the other
  // image axes
  int thumb_x_axis = d_to_i->GetCoordinateIndexZeroBased(0);
  int thumb_y_axis = d_to_i->GetCoordinateIndexZeroBased(1);
  double thumb_x_dir = d_to_i->GetCoordinateOrientation(0);
  double thumb_y_dir = d_to_i->GetCoordinateOrientation(1);

  // Compute the spacing of the referene slice
  double spc_x = this->GetSize()[thumb_x_axis] * this->GetImageBase()->GetSpacing()[thumb_x_axis] / maxdim;
  double spc_y = this->GetSize()[thumb_y_axis] * this->GetImageBase()->GetSpacing()[thumb_y_axis] / maxdim;
  double spc_max = std::max(spc_x, spc_y);
  typename ImageBaseType::SpacingType ref_spacing;
  ref_spacing[0] = spc_max;
  ref_spacing[1] = spc_max;
  ref_spacing[2] = this->GetImageBase()->GetSpacing()[thumb_z_axis];

  // Compute the direction matrix of the reference slice. The direction matrix should be the
  // corresponding column from the image direction matrix, but the sign may be flipped.
  auto ref_direction = direction;
  ref_direction.set_identity();
  ref_direction.set_column(0, direction.get_column(thumb_x_axis) * thumb_x_dir);
  ref_direction.set_column(1, direction.get_column(thumb_y_axis) * thumb_y_dir);
  ref_direction.set_column(2, direction.get_column(thumb_z_axis));

  // Compute the origin of the reference slice. Here we want the center of the thumbnail
  // to match the center of the image.
  auto origin_img = this->GetImageBase()->GetOrigin().GetVnlVector();
  Vector3d offset_ctr;
  for(unsigned int d = 0; d < 3; d++)
    offset_ctr[d] = 0.5 * this->GetImageBase()->GetSpacing()[d] * (this->GetSize()[d] - 1);
  Vector3d center_img = origin_img + direction * offset_ctr;

  // Compute the origin for the thumb
  Vector3d ref_offset_ctr;
  ref_offset_ctr[0] = 0.5 * ref_spacing[0] * (maxdim - 1);
  ref_offset_ctr[1] = 0.5 * ref_spacing[1] * (maxdim - 1);
  ref_offset_ctr[2] = 0;
  Vector3d ref_origin = center_img - ref_direction * ref_offset_ctr;

  // Create the reference space
  using RefType = itk::Image<unsigned char, 3>;
  typename RefType::Pointer ref_slice = RefType::New();
  ref_slice->SetSpacing(ref_spacing);
  ref_slice->SetOrigin(to_itkPoint(ref_origin));

  typename ImageBaseType::DirectionType ref_direction_itk;
  ref_direction_itk = ref_direction;
  ref_slice->SetDirection(ref_direction_itk);

  // The size of the viewport is fairly easy
  typename ImageBaseType::RegionType ref_region;
  ref_region.SetSize(0, maxdim); ref_region.SetSize(1, maxdim); ref_region.SetSize(2, 1);
  ref_slice->SetRegions(ref_region);

  // Sample the display slice
  DisplaySlicePointer thumb_image = this->SampleArbitraryDisplaySlice(ref_slice);

  // Background color for thumbnails
  unsigned char defrgb[] = {0,0,0,255};

  // For thumbnails, the image needs to be flipped
  typedef itk::FlipImageFilter<DisplaySliceType> FlipFilter;
  SmartPtr<FlipFilter> flipper = FlipFilter::New();
  flipper->SetInput(thumb_image);
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

/*


template<class TTraits>
typename ImageWrapper<TTraits>::DisplaySlicePointer
ImageWrapper<TTraits>
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

*/

template<class TTraits>
void
ImageWrapper<TTraits>
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

template<class TTraits>
void
ImageWrapper<TTraits>
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

template<class TTraits>
typename ImageWrapper<TTraits>::MetaDataAccessType
ImageWrapper<TTraits>
::GetMetaDataAccess()
{
  return MetaDataAccessType(m_Image4D);
}

template<class TTraits>
bool
ImageWrapper<TTraits>
::HasUnsavedChanges() const
{
  // Check each of the timepoint images
  for(ImagePointer img : m_ImageTimePoints)
    if(img->GetTimeStamp() > m_ImageSaveTime)
      return true;

  // Check the 4D image
  return m_Image4D->GetTimeStamp() > m_ImageSaveTime;
}

template<class TTraits>
bool
ImageWrapper<TTraits>
::HasUnsavedChanges(unsigned int tp) const
{
  if (m_ImageTimePoints.size() <= tp)
    return false;

  auto img = m_ImageTimePoints[tp];
  return img->GetTimeStamp() > m_ImageSaveTime;
}


template<class TTraits>
void
ImageWrapper<TTraits>
::SetSourceNativeMapping(double scale, double shift)
{
  typedef ImageWrapperPartialSpecializationTraits<ImageType, Image4DType> Specialization;
  Specialization::SetSourceNativeMapping(m_Image4D, scale, shift);
  for(unsigned int j = 0; j < m_ImageTimePoints.size(); j++)
    Specialization::ConfigureTimePointImageFromImage4D(m_Image4D, m_ImageTimePoints[j], j);
}

template<class TTraits>
SmartPtr<ImageWrapperBase>
ImageWrapper<TTraits>
::ExtractROI(const SNAPSegmentationROISettings &roi,
             itk::Command *progressCommand) const
{
  // Get the ITK image for the ROI (it will be a single time point image)
  ImagePointer newImage = this->DeepCopyRegion(roi, progressCommand);

  // Dress this image up as a 4D image.
  Image4DPointer newImage4D = Image4DType::New();

  // Initialize the region
  typename Image4DType::RegionType region_4d;
  region_4d.SetIndex(3, 0); region_4d.SetSize(3, 1);

  // Set the spacing, origin, direction for the last coordinate
  auto spacing_4d = m_Image4D->GetSpacing();
  auto origin_4d  = m_Image4D->GetOrigin();
  auto dir_4d     = m_Image4D->GetDirection();

  for(unsigned int j = 0; j < 3; j++)
    {
    region_4d.SetIndex(j, newImage->GetBufferedRegion().GetIndex(j));
    region_4d.SetSize(j, newImage->GetBufferedRegion().GetSize(j));
    spacing_4d[j] = newImage->GetSpacing()[j];
    origin_4d[j] = newImage->GetOrigin()[j];
    for(unsigned int k = 0; k < 3; k++)
      dir_4d(j,k) = newImage->GetDirection() (j,k);
    }

  newImage4D->SetRegions(region_4d);
  newImage4D->SetSpacing(spacing_4d);
  newImage4D->SetOrigin(origin_4d);
  newImage4D->SetDirection(dir_4d);
  newImage4D->SetNumberOfComponentsPerPixel(newImage->GetNumberOfComponentsPerPixel());

  // Take the 3D image's container into the 4D image
  typedef ImageWrapperPartialSpecializationTraits<ImageType, Image4DType> Specialization;
  Specialization::AssignPixelContainerFromTimePointTo4D(newImage4D, newImage);

  // Initialize the new wrapper
  typedef typename TTraits::WrapperType WrapperType;
  SmartPtr<WrapperType> newWrapper = WrapperType::New();

  // Copy the display to anatomy geometry to the new wrapper
  IRISDisplayGeometry temp = m_DisplayGeometry;
  newWrapper->SetDisplayGeometry(temp);

  // Assign the new image to the new wrapper
  newWrapper->SetImage4D(newImage4D);
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

template<class TTraits>
SmartPtr<ImageWrapperBase>
ImageWrapper<TTraits>
::ExtractROI4D(const SNAPSegmentationROISettings &roi,
             itk::Command *progressCommand) const
{
  Image4DPointer newImage = this->DeepCopyRegion4D(roi, progressCommand);

  // Initialize the new wrapper
  typedef typename TTraits::WrapperType WrapperType;
  SmartPtr<WrapperType> newWrapper = WrapperType::New();

  // Copy the display to anatomy geometry to the new wrapper
  IRISDisplayGeometry temp = m_DisplayGeometry;
  newWrapper->SetDisplayGeometry(temp);

  // Assign the new image to the new wrapper
  newWrapper->SetImage4D(newImage);
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


template<class TTraits>
typename ImageWrapper<TTraits>::ImagePointer
ImageWrapper<TTraits>
::DeepCopyRegion(const SNAPSegmentationROISettings &roi,
                 itk::Command *progressCommand) const
{
  // If the image in this wrapper is not the same as the reference space,
  // we must force resampling to occur
  bool force_resampling = !this->IsSlicingOrthogonal();

  // We use partial template specialization here because region copy is
  // only supported for images that are concrete (Image, VectorImage)
  typedef ImageWrapperPartialSpecializationTraits<ImageType, Image4DType> Specialization;
  return Specialization::CopyRegion(
        m_Image, m_ReferenceSpace,
        this->GetITKTransform(), roi,
        force_resampling, progressCommand);
}

template<class TTraits>
typename ImageWrapper<TTraits>::Image4DPointer
ImageWrapper<TTraits>
::DeepCopyRegion4D(const SNAPSegmentationROISettings &roi,
                 itk::Command *progressCommand) const
{
  // If the image in this wrapper is not the same as the reference space,
  // we must force resampling to occur
  bool force_resampling = !this->IsSlicingOrthogonal();

  Image4DPointer outImg = Image4DType::New();
  const unsigned int nT = this->GetNumberOfTimePoints();

  // Set the spacing, origin, direction for the last coordinate
  auto spacing_4d = m_Image4D->GetSpacing();
  auto origin_4d = m_Image4D->GetOrigin();
  auto dir_4d = m_Image4D->GetDirection();
  auto region_4d = m_Image4D->GetBufferedRegion();

  // Create a 4d buffer
  typedef typename Image4DType::PixelContainer::Element ElementType;
  typedef typename Image4DType::PixelContainer::ElementIdentifier ElementIdType;
  auto size_3d = roi.GetROI().GetSize();
  auto nC = m_Image4D->GetNumberOfComponentsPerPixel();
  ElementIdType buffer3dSize = size_3d[0] * size_3d[1] * size_3d[2] * nC;
  ElementIdType buffer3dSizeInBytes = buffer3dSize * sizeof(ElementType);
  ElementIdType buffer4dSize = buffer3dSize * nT;
  ElementType *buffer4d = new ElementType[buffer4dSize];
  ElementType *pCrntTPStart = buffer4d; // starting mem location of the current tp

  typedef ImageWrapperPartialSpecializationTraits<ImageType, Image4DType> Specialization;

  // Prepare progress accumulation
  SmartPtr<AllPurposeProgressAccumulator> progress = AllPurposeProgressAccumulator::New();
  progress->AddObserver(itk::ProgressEvent(), progressCommand);
  typedef AllPurposeProgressAccumulator::CommandPointer CmdPtr;
  std::vector<CmdPtr> TPCommand;

  for (unsigned int t = 0u; t < nT; ++t)
    {
    TPCommand.push_back(progress->RegisterITKSourceViaCommand(1));
    }

  typename ImageType::Pointer tpResliced, tpImg;

  for (unsigned int t = 0u; t < nT; ++t)
    {
    tpImg = this->GetImageByTimePoint(t);
    tpResliced = Specialization::CopyRegion(tpImg, m_ReferenceSpace, this->GetITKTransform(),
                                         roi, force_resampling, TPCommand[t]);

    auto buffer3d = tpResliced->GetPixelContainer()->GetBufferPointer();
    memcpy(pCrntTPStart, buffer3d, buffer3dSizeInBytes);
    pCrntTPStart += buffer3dSize; // Increment pointer to next tp start
    }

  // use the last resliced tp image to configure the output origin/spacing/direction/region
  // -- this only configures the first three dimensions
  for (int j = 0; j < 3; ++j)
    {
    region_4d.SetIndex(j, tpResliced->GetBufferedRegion().GetIndex(j));
    region_4d.SetSize(j, tpResliced->GetBufferedRegion().GetSize(j));
    spacing_4d[j] = tpResliced->GetSpacing()[j];
    origin_4d[j] = tpResliced->GetOrigin()[j];
    for(unsigned int k = 0; k < 3; k++)
      dir_4d(j,k) = tpResliced->GetDirection() (j,k);
    }

  outImg->SetRegions(region_4d);
  outImg->SetSpacing(spacing_4d);
  outImg->SetOrigin(origin_4d);
  outImg->SetDirection(dir_4d);
  outImg->SetNumberOfComponentsPerPixel(tpResliced->GetNumberOfComponentsPerPixel());

  // set the constructed 4d buffer to output's pixel container
  outImg->GetPixelContainer()->SetImportPointer(buffer4d, buffer4dSize, true);
  return outImg;
}


/**
 * A cast to target pipeline implementation that uses the cast image filter
 */
template <class TInputImage, class TOutputImage, class TNativeMapping>
class CreateCastToTargetTypePipelineUsingCastFilter
{
public:
  typedef std::pair<ImageWrapperBase::MiniPipeline, SmartPtr<TOutputImage> > ReturnType;
  static ReturnType CreatePipeline(TInputImage *image, TNativeMapping native_mapping)
    {
    // Create a casting filter
    typedef itk::CastImageFilter<TInputImage, TOutputImage> CastFilter;
    typename CastFilter::Pointer filter = CastFilter::New();
    filter->SetInput(image);

    // Populate the mini-pipeline with the filter
    ImageWrapperBase::MiniPipeline mp;
    mp.filters.push_back(filter.GetPointer());

    // Return the pipeline and its output
    return std::make_pair(mp, filter->GetOutput());
    }
};

/**
 * A cast to target pipeline implementation that just returns the input image
 */
template <class TInputImage, class TNativeMapping>
class CreateCastToTargetTypePipelineUsingImageCopy
{
public:
  typedef std::pair<ImageWrapperBase::MiniPipeline, SmartPtr<TInputImage> > ReturnType;
  static ReturnType CreatePipeline(TInputImage *image, TNativeMapping native_mapping)
    {
    // No pipeline needed, just return the image itself
    return std::make_pair(ImageWrapperBase::MiniPipeline(), image);
    }
};

/**
 * A functor used to apply linear mapping to each image channel
 */
template <class TInputPixel, class TOutputComponent, class TScalarFunctor>
class MultiComponentChannelWiseFunctor
{
public:
  typedef MultiComponentChannelWiseFunctor<TInputPixel,TOutputComponent,TScalarFunctor> Self;

  void SetNumberOfComponentsPerPixel(unsigned int n) { m_Components = n; }
  unsigned int GetNumberOfComponentsPerPixel() const { return m_Components; }

  void SetFunctor(const TScalarFunctor &functor) { m_Functor = functor; }

  void operator() (const TInputPixel &x, TOutputComponent *y) const
  {
    for(unsigned int k = 0; k < m_Components; k++)
      y[k] = m_Functor(x[k]);
  }

  bool operator != (const Self &other) const
  {
    return m_Components != other.m_Components || m_Functor != other.m_Functor;
  }

protected:
  unsigned int m_Components;
  TScalarFunctor m_Functor;
};


/**
 * CreateCastToTargetTypePipelinePartialSpecializationTraits is a helper class that
 * can create a mini-pipeline between the internally stored image in ImageWrapper and
 * an image of desired output type, with optional linear mapping. This implementation
 * throws an error; actual work is done in the various specializations below
 */
template <class TInputImage, class TOutputImage, class TNativeMapping, bool LinearMapping, bool Compatible>
class CreateCastToTargetTypePipelinePartialSpecializationTraits
{
public:
  typedef std::pair<ImageWrapperBase::MiniPipeline, SmartPtr<TOutputImage> > ReturnType;
  static ReturnType CreatePipeline(TInputImage *image, TNativeMapping native_mapping)
    {
    throw IRISException("CreatePipeline not implemented for these input types: %s, %s, %s, %d, %d. "
                        "(ImageWrapper::CreateCastToTargetTypePipelinePartialSpecializationTraits)",
                        typeid(TInputImage).name(), typeid(TOutputImage).name(), typeid(TNativeMapping).name(),
                        static_cast<int>(LinearMapping), static_cast<int>(Compatible));
    return std::make_pair(ImageWrapperBase::MiniPipeline(), SmartPtr<TOutputImage>());
    }
};

/**
 * This specialization is for when the input image and output image are not the same
 * class, but there is no intensity mapping, so it is possible to use ITK's cast filter
 */
template <class TInputImage, class TOutputImage>
class CreateCastToTargetTypePipelinePartialSpecializationTraits<
    TInputImage, TOutputImage, IdentityInternalToNativeIntensityMapping, false, true>
    : public CreateCastToTargetTypePipelineUsingCastFilter<
      TInputImage, TOutputImage, IdentityInternalToNativeIntensityMapping>
{
};


/**
 * This specialization is for when the input image and output image are VectorImages
 * and a linear mapping is used
 */
template <class TSourcePixel, class TTargetPixel, unsigned int VDim, class TLinearMapping>
class CreateCastToTargetTypePipelinePartialSpecializationTraits<
    itk::VectorImage<TSourcePixel, VDim>, itk::VectorImage<TTargetPixel, VDim>,
    TLinearMapping, true, true>
{
public:
  typedef itk::VectorImage<TSourcePixel, VDim> TInputImage;
  typedef itk::VectorImage<TTargetPixel, VDim> TOutputImage;
  typedef std::pair<ImageWrapperBase::MiniPipeline, SmartPtr<TOutputImage> > ReturnType;

  static ReturnType CreatePipeline(TInputImage *image, TLinearMapping native_mapping)
    {
    typedef MultiComponentChannelWiseFunctor<TSourcePixel, TTargetPixel, TLinearMapping> FunctorType;
    typedef UnaryFunctorVectorImageFilter<TInputImage, TOutputImage, FunctorType> FilterType;
    FunctorType functor;
    functor.SetNumberOfComponentsPerPixel(image->GetNumberOfComponentsPerPixel());
    functor.SetFunctor(native_mapping);
    SmartPtr<FilterType> filter = FilterType::New();
    filter->SetInput(image);
    filter->SetFunctor(functor);

    // Populate the mini-pipeline with the filter
    ImageWrapperBase::MiniPipeline mp;
    mp.filters.push_back(filter.GetPointer());

    // Return the pipeline and its output
    return std::make_pair(mp, filter->GetOutput());
    }
};

/**
 * This specialization is for when a linear mapping is used and the output is not
 * a vector image
 */
template <class TInputImage, class TTargetPixel, unsigned int VDim, class TLinearMapping>
class CreateCastToTargetTypePipelinePartialSpecializationTraits<
    TInputImage, itk::Image<TTargetPixel, VDim>, TLinearMapping, true, true>
{
public:
  typedef itk::Image<TTargetPixel, VDim> TOutputImage;
  typedef std::pair<ImageWrapperBase::MiniPipeline, SmartPtr<TOutputImage> > ReturnType;

  static ReturnType CreatePipeline(TInputImage *image, TLinearMapping native_mapping)
    {
    typedef itk::UnaryFunctorImageFilter<TInputImage, TOutputImage, TLinearMapping> FilterType;
    SmartPtr<FilterType> filter = FilterType::New();
    filter->SetInput(image);
    filter->SetFunctor(native_mapping);

    // Populate the mini-pipeline with the filter
    ImageWrapperBase::MiniPipeline mp;
    mp.filters.push_back(filter.GetPointer());

    // Return the pipeline and its output
    return std::make_pair(mp, filter->GetOutput());
    }
};


/**
 * This specialization is for when the input image and output images are the same
 * class and there is no intensity mapping, so we can just return the input instead
 * of casting to another type
 */
template <class TImage>
class CreateCastToTargetTypePipelinePartialSpecializationTraits<
    TImage, TImage,
    IdentityInternalToNativeIntensityMapping, false, true>
    : public CreateCastToTargetTypePipelineUsingImageCopy<TImage, IdentityInternalToNativeIntensityMapping>
{
};


template<class TTraits>
typename ImageWrapper<TTraits>::FloatImageType *
ImageWrapper<TTraits>
::CreateCastToFloatPipeline(const char *key, int index)
{
  typedef typename std::is_base_of<LinearInternalToNativeIntensityMapping, NativeIntensityMapping> IsLinear;
  typedef typename std::is_base_of<itk::VectorImage<ComponentType, 3>, ImageType> IsVector;

  typedef CreateCastToTargetTypePipelinePartialSpecializationTraits<
      ImageType, FloatImageType, NativeIntensityMapping, IsLinear::value, !IsVector::value> Specialization;

  auto p = Specialization::CreatePipeline(this->m_Image, this->m_NativeMapping);

  if(p.second)
    this->AddInternalPipeline(p.first, key, index);

  return p.second;
}

template<class TTraits>
typename ImageWrapper<TTraits>::FloatVectorImageType *
ImageWrapper<TTraits>
::CreateCastToFloatVectorPipeline(const char *key, int index)
{
  typedef typename std::is_base_of<LinearInternalToNativeIntensityMapping, NativeIntensityMapping> IsLinear;

  // Create a pipeline that maps us to the matching image
  typedef CreateCastToTargetTypePipelinePartialSpecializationTraits<
      ImageType, FloatVectorImageType, NativeIntensityMapping, IsLinear::value, IsVector::value> Specialization;
  auto p = Specialization::CreatePipeline(this->m_Image, this->m_NativeMapping);

  // Now, if MatchingFloatImage is not a FloatVectorImageType, we have to create a filter that
  // will disguise it as one

  if(p.second)
    this->AddInternalPipeline(p.first, key, index);

  return p.second;
}

template<class TTraits>
typename ImageWrapper<TTraits>::FloatSliceType *
ImageWrapper<TTraits>::CreateCastToFloatSlicePipeline(const char *key, unsigned int slice)
{
  typedef typename std::is_base_of<LinearInternalToNativeIntensityMapping, NativeIntensityMapping> IsLinear;

  typedef CreateCastToTargetTypePipelinePartialSpecializationTraits<
      SliceType, FloatSliceType, NativeIntensityMapping, IsLinear::value, !IsVector::value> Specialization;

  auto p = Specialization::CreatePipeline(this->GetSlice(slice), this->m_NativeMapping);

  if(p.second)
    this->AddInternalPipeline(p.first, key, slice);

  return p.second;
}

template<class TTraits>
typename ImageWrapper<TTraits>::FloatVectorSliceType *
ImageWrapper<TTraits>::CreateCastToFloatVectorSlicePipeline(const char *key, unsigned int slice)
{
  typedef typename std::is_base_of<LinearInternalToNativeIntensityMapping, NativeIntensityMapping> IsLinear;
  typedef typename std::is_base_of<itk::VectorImage<ComponentType, 3>, ImageType> IsVector;

  typedef CreateCastToTargetTypePipelinePartialSpecializationTraits<
      SliceType, FloatVectorSliceType, NativeIntensityMapping, IsLinear::value, IsVector::value> Specialization;

  auto p = Specialization::CreatePipeline(this->GetSlice(slice), this->m_NativeMapping);

  if(p.second)
    this->AddInternalPipeline(p.first, key, slice);

  return p.second;
  }


template<class TTraits>
void ImageWrapper<TTraits>::AddInternalPipeline(const MiniPipeline &mp, const char *key, int index)
{
  m_ManagedPipelines[std::string(key)][index] = mp;
}

template<class TTraits>
void ImageWrapper<TTraits>::ReleaseInternalPipeline(const char *key, int index)
{
  std::string k(key);
  if(index < 0)
    {
    m_ManagedPipelines.erase(k);
    }
  else
    {
    m_ManagedPipelines[k].erase(index);
    if(m_ManagedPipelines[k].size() == 0)
      m_ManagedPipelines.erase(k);
    }
  }


template<class TTraits>
string ImageWrapper<TTraits>::GetPixelFormatDescription()
{
  bool is_signed = std::is_signed<InternalPixelType>::value;
  bool is_integer = std::is_integral<InternalPixelType>::value;
  bool is_float = std::is_floating_point<InternalPixelType>::value;
  bool is_vector = std::is_base_of<itk::VariableLengthVector<InternalPixelType>, PixelType>::value;

  std::string component_type =
      is_float ? "float" : (is_integer ? (is_signed ? "int" : "uint") : "unknown");

  int n_bits = sizeof(InternalPixelType) * 8;

  std::ostringstream oss;
  if(is_vector)
    oss << this->GetNumberOfComponents() << " x " << component_type << n_bits;
  else
    oss << component_type << n_bits;

  return oss.str();
}


template<class TTraits>
std::pair<typename ImageWrapper<TTraits>::MiniPipeline, typename ImageWrapper<TTraits>::ConcreteImageType*>
ImageWrapper<TTraits>::CreateCastToConcreteImagePipeline() const
{
  typedef CreateCastToTargetTypePipelinePartialSpecializationTraits<
      ImageType, ConcreteImageType, IdentityInternalToNativeIntensityMapping, false, true> Specialization;
  IdentityInternalToNativeIntensityMapping dummy_mapping;
  return Specialization::CreatePipeline(this->m_Image, dummy_mapping);
}

// --------------------------------------------
// Explicit template instantiation
#define ImageWrapperInstantiateMacro(type) \
  template class ImageWrapper<typename ImageWrapperTraits<type>::ScalarTraits>; \
  template class ImageWrapper<typename ImageWrapperTraits<type>::VectorTraits>; \
  template class ImageWrapper<typename ImageWrapperTraits<type>::ComponentTraits>; \
  template class ImageWrapper<typename ImageWrapperTraits<type>::MagnitudeTraits>; \
  template class ImageWrapper<typename ImageWrapperTraits<type>::MaxTraits>; \
  template class ImageWrapper<typename ImageWrapperTraits<type>::MeanTraits>;

ImageWrapperInstantiateMacro(unsigned char)
ImageWrapperInstantiateMacro(char)
ImageWrapperInstantiateMacro(unsigned short)
ImageWrapperInstantiateMacro(short)
ImageWrapperInstantiateMacro(float)
ImageWrapperInstantiateMacro(double)

template class ImageWrapper<SpeedImageWrapperTraits>;
template class ImageWrapper<LabelImageWrapperTraits>;
template class ImageWrapper<LevelSetImageWrapperTraits>;
