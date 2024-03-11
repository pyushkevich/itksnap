/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: ScalarImageWrapper.txx,v $
  Language:  C++
  Date:      $Date: 2009/01/24 01:50:21 $
  Version:   $Revision: 1.4 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/

#include "ScalarImageWrapper.h"
#include "ImageWrapperTraits.h"
#include "RLEImageRegionIterator.h"
#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkNumericTraits.h"
#include "RLERegionOfInterestImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkIdentityTransform.h"
#include "itkResampleImageFilter.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkBSplineInterpolateImageFunction.h"
#include "itkWindowedSincInterpolateImageFunction.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkVTKImageExport.h"
#include "itkStreamingImageFilter.h"

#include "AdaptiveSlicingPipeline.h"
#include "SNAPSegmentationROISettings.h"
#include "itkCommand.h"
#include "itkMinimumMaximumImageFilter.h"
#include "itkVectorImageToImageAdaptor.h"
#include "itkCastImageFilter.h"
#include "IRISException.h"
#include "VectorImageWrapper.h"
#include "GuidedNativeImageIO.h"
#include "itkImageFileWriter.h"

#include "vtkImageImport.h"
#include "SNAPExportITKToVTK.h"

#include <iostream>
#include <type_traits>

template<class TTraits>
ScalarImageWrapper<TTraits>
::ScalarImageWrapper()
{
}

template<class TTraits>
ScalarImageWrapper<TTraits>
::ScalarImageWrapper(const Self &copy)
  : Superclass(copy)
{
}

template<class TTraits>
ScalarImageWrapper<TTraits>
::~ScalarImageWrapper()
{
}

template<class TTraits>
double
ScalarImageWrapper<TTraits>
::GetImageGradientMagnitudeUpperLimit()
{
  // Paul - 12/1/2014: I have modified this code to not compute the gradient magnitude
  // on the whole image, but instead, to compute just the difference between the max and
  // min values. I think this is much more efficient, and this removes some problems with
  // wrappers that wrap around ImageAdapter objects.
  //
  // I hope this does not cause too much trouble...
  return this->GetImageMaxAsDouble() - this->GetImageMinAsDouble();
}

template<class TTraits>
double
ScalarImageWrapper<TTraits>
::GetImageGradientMagnitudeUpperLimitNative()
{
  return this->m_NativeMapping.MapGradientMagnitudeToNative(
        this->GetImageGradientMagnitudeUpperLimit());
}

#ifdef __DELETE_THIS_CODE_
template <class TInputImage, class TOutputImage, class TFunctor>
class UnaryFunctorImageToSingleComponentVectorImageFilter
    : public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  typedef UnaryFunctorImageToSingleComponentVectorImageFilter<TInputImage, TOutputImage, TFunctor> Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputImage> Superclass;
  typedef itk::SmartPointer<Self> Pointer;
  typedef itk::SmartPointer< const Self >  ConstPointer;

  typedef TInputImage InputImageType;
  typedef TOutputImage OutputImageType;
  typedef TFunctor FunctorType;

  typedef typename Superclass::OutputImageRegionType OutputImageRegionType;

  /** Run-time type information (and related methods). */
  itkTypeMacro(UnaryFunctorImageToSingleComponentVectorImageFilter, ImageToImageFilter)
  itkNewMacro(Self)

  /** ImageDimension constants */
  itkStaticConstMacro(InputImageDimension, unsigned int,
                      TInputImage::ImageDimension);
  itkStaticConstMacro(OutputImageDimension, unsigned int,
                      TOutputImage::ImageDimension);

  void SetFunctor(const FunctorType &functor)
  {
    if(m_Functor != functor)
      {
      m_Functor = functor;
      this->Modified();
      }
  }

  itkGetConstReferenceMacro(Functor, FunctorType)

  void DynamicThreadedGenerateData(const OutputImageRegionType & outputRegionForThread) ITK_OVERRIDE;


protected:

  UnaryFunctorImageToSingleComponentVectorImageFilter() {}
  virtual ~UnaryFunctorImageToSingleComponentVectorImageFilter() {}

  FunctorType m_Functor;

};

#include "ImageRegionConstIteratorWithIndexOverride.h"

template <class TInputImage, class TOutputImage, class TFunctor>
void
UnaryFunctorImageToSingleComponentVectorImageFilter<TInputImage, TOutputImage, TFunctor>
::DynamicThreadedGenerateData(const OutputImageRegionType &outputRegionForThread)
{
  // Use our fast iterators for vector images
  typedef itk::ImageLinearIteratorWithIndex<OutputImageType> IterBase;
  typedef IteratorExtender<IterBase> IterType;

  typedef typename OutputImageType::InternalPixelType OutputComponentType;
  typedef typename InputImageType::InternalPixelType InputComponentType;

  // Define the iterators
  IterType outputIt(this->GetOutput(), outputRegionForThread);
  int line_len = outputRegionForThread.GetSize(0);

  // Using a generic ITK iterator for the input because it supports RLE images and adaptors
  itk::ImageScanlineConstIterator< InputImageType > inputIt(this->GetInput(), outputRegionForThread);

  while ( !inputIt.IsAtEnd() )
    {
    // Get the pointer to the input and output pixel lines
    OutputComponentType *out = outputIt.GetPixelPointer(this->GetOutput());

    for(int i = 0; i < line_len; i++, ++inputIt)
      {
      out[i] = m_Functor(inputIt.Get());
      }

    outputIt.NextLine();
    inputIt.NextLine();
    }
}

#endif // __DELETE_THIS_CODE_

/** Compute statistics over a run of voxels in the image starting at the index
 * startIdx. Appends the statistics to a running sum and sum of squared. The
 * statistics are returned in internal (not native mapped) format */
template<class TTraits>
void
ScalarImageWrapper<TTraits>
::GetRunLengthIntensityStatistics(
    const itk::ImageRegion<3> &region,
    const itk::Index<3> &startIdx, long runlength,
    double *out_nvalid, double *out_sum, double *out_sumsq) const
{
  if(this->IsSlicingOrthogonal())
    {
    ConstIterator it(this->m_Image, region);
    it.SetIndex(startIdx);

    // Perform the integration
    for(long q = 0; q < runlength; q++, ++it)
      {
      double p = (double) it.Get();
      if(!std::isnan(p))
        {
        *out_nvalid += 1.0;
        *out_sum += p;
        *out_sumsq += p * p;
        }
      }
    }
  else
    {
    // TODO: implement non-orthogonal statistics
    *out_sum += nan("");
    *out_sumsq += nan("");
    }
}

/**
  Get the RGBA apperance of the voxel at the intersection of the three
  display slices.
  */
template<class TTraits>
void
ScalarImageWrapper<TTraits>
::GetVoxelUnderCursorDisplayedValueAndAppearance(
    vnl_vector<double> &out_value, DisplayPixelType &out_appearance)
{
  // Sample the intensity under the cursor for the current time point
  this->SampleIntensityAtReferenceIndex(
        this->m_SliceIndex, this->GetTimePointIndex(), true, out_value);

  // Use the display mapping to map to display pixel
  out_appearance = this->m_DisplayMapping->MapPixel(
                     this->m_IntensitySamplingArray.data_block() +
        this->GetTimePointIndex() * this->GetNumberOfComponents());
}


template<class TTraits>
typename ScalarImageWrapper<TTraits>::DisplaySlicePointer
ScalarImageWrapper<TTraits>
::SampleArbitraryDisplaySlice(const ImageBaseType *ref_space)
{
  // Create a non-orthogonal slicer for this task - we don't want to interfere with the
  // main slicing pipeline
  using ThumbSlicer = typename SlicerType::NonOrthogonalSlicerType;
  typename ThumbSlicer::Pointer thumb_slicer = ThumbSlicer::New();
  thumb_slicer->SetReferenceImage(ref_space);
  thumb_slicer->SetInput(this->GetImage());

  // The affine transform is set to identity
  typedef itk::IdentityTransform<double, 3> IdTransformType;
  typename IdTransformType::Pointer idTran = IdTransformType::New();
  thumb_slicer->SetTransform(idTran);

  // Perform slicing
  thumb_slicer->Update();
  auto *raw_thumb = thumb_slicer->GetOutput();

  // Allocate the output image
  typename Superclass::DisplaySlicePointer thumb_image = DisplaySliceType::New();
  typename DisplaySliceType::RegionType thumb_region;
  thumb_region.SetSize(0, ref_space->GetBufferedRegion().GetSize()[0]);
  thumb_region.SetSize(1, ref_space->GetBufferedRegion().GetSize()[1]);
  thumb_image->SetRegions(thumb_region);
  thumb_image->Allocate();

  // Now, put this through the display pipeline
  auto *src_pix = raw_thumb->GetBufferPointer(), *p = src_pix;
  auto *dst_pix = thumb_image->GetBufferPointer(), *q = dst_pix;
  unsigned int n_pix = thumb_region.GetNumberOfPixels();
  unsigned int k = raw_thumb->GetNumberOfComponentsPerPixel();
  for(; q < dst_pix + n_pix; p += k, q++)
    *q = this->GetDisplayMapping()->MapPixel(p);

  return thumb_image;
}


template<class TTraits>
typename ScalarImageWrapper<TTraits>::VTKImporterMiniPipeline
ScalarImageWrapper<TTraits>::CreateVTKImporterPipeline() const
{
  // The cast pipeline
  auto cast_pipeline = this->CreateCastToConcreteImagePipeline();

  // This filter is the exporter
  typedef typename Superclass::ConcreteImageType ConcreteImageType;
  typedef itk::VTKImageExport<ConcreteImageType> Exporter;
  SmartPtr<Exporter> exporter = Exporter::New();
  exporter->SetInput(cast_pipeline.second);

  // This is the importer
  vtkNew<vtkImageImport> importer;
  ConnectITKExporterToVTKImporter(exporter.GetPointer(), importer, true, true, false);

  // Create return struct
  VTKImporterMiniPipeline pip;
  pip.cast_pipeline = cast_pipeline.first;
  pip.exporter = exporter.GetPointer();
  pip.importer = importer;

  return pip;
  }

#include "itkRepresentImageAsVectorImageFilter.h"

template<class TTraits>
ImageWrapperBase::FloatVectorImageType *
ScalarImageWrapper<TTraits>::CreateCastToFloatVectorPipeline(const char *key, int index)
{
  // Cast to a float scalar image
  auto *scalar = this->CreateCastToFloatPipeline(key, index);

  // Retrieve the stored mini-pipeline
  auto &mp = this->m_ManagedPipelines[std::string(key)][index];

  // Add a filter that represents this as a vector image
  typedef itk::RepresentImageAsVectorImageFilter<
      typename ImageWrapperBase::FloatImageType,
      typename ImageWrapperBase::FloatVectorImageType> VectorMasquerader;

  typename VectorMasquerader::Pointer vm = VectorMasquerader::New();
  vm->SetInput(scalar);

  // Update the mini-pipeline
  mp.filters.push_back(vm.GetPointer());
  mp.output = vm->GetOutput();

  return vm->GetOutput();
}

template<class TTraits>
IntensityCurveInterface *
ScalarImageWrapper<TTraits>
::GetIntensityCurve() const
{
  return this->m_DisplayMapping->GetIntensityCurve();
}

template<class TTraits>
ColorMap *
ScalarImageWrapper<TTraits>
::GetColorMap() const
{
  return this->m_DisplayMapping->GetColorMap();
}


template<class TTraits>
void
ScalarImageWrapper<TTraits>
::WriteToFileAsFloat(const char *fname, Registry &hints)
{
  SmartPtr<GuidedNativeImageIO> io = GuidedNativeImageIO::New();
  io->CreateImageIO(fname, hints, false);
  itk::ImageIOBase *base = io->GetIOBase();

  // Create a pipeline that casts the image to floating type
  auto *float_img = this->CreateCastToFloatPipeline("WriteToFileAsFloat");

  typedef typename ImageWrapperBase::FloatImageType FloatImageType;
  typedef itk::ImageFileWriter<FloatImageType> WriterType;
  SmartPtr<WriterType> writer = WriterType::New();
  writer->SetFileName(fname);
  if(base)
    writer->SetImageIO(base);
  writer->SetInput(float_img);
  writer->Update();

  // Release the pipeline (what a pain)
  this->ReleaseInternalPipeline("WriteToFileAsFloat");
}

// --------------------------------------------
// Explicit template instantiation
#define ScalarImageWrapperInstantiateMacro(type) \
  template class ScalarImageWrapper<typename ImageWrapperTraits<type>::ScalarTraits>; \
  template class ScalarImageWrapper<typename ImageWrapperTraits<type>::ComponentTraits>; \
  template class ScalarImageWrapper<typename ImageWrapperTraits<type>::MagnitudeTraits>; \
  template class ScalarImageWrapper<typename ImageWrapperTraits<type>::MaxTraits>; \
  template class ScalarImageWrapper<typename ImageWrapperTraits<type>::MeanTraits>;

ScalarImageWrapperInstantiateMacro(unsigned char)
ScalarImageWrapperInstantiateMacro(char)
ScalarImageWrapperInstantiateMacro(unsigned short)
ScalarImageWrapperInstantiateMacro(short)
ScalarImageWrapperInstantiateMacro(float)
ScalarImageWrapperInstantiateMacro(double)

template class ScalarImageWrapper<LabelImageWrapperTraits>;
template class ScalarImageWrapper<SpeedImageWrapperTraits>;
template class ScalarImageWrapper<LevelSetImageWrapperTraits>;
