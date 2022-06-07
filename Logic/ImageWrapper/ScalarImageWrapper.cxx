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
#include "ScalarImageHistogram.h"
#include "ThreadedHistogramImageFilter.h"
#include "GuidedNativeImageIO.h"
#include "itkImageFileWriter.h"

#include "vtkImageImport.h"
#include "SNAPExportITKToVTK.h"

#include <iostream>

template<class TTraits, class TBase>
ScalarImageWrapper<TTraits,TBase>
::ScalarImageWrapper()
{
  m_MinMaxFilter = MinMaxFilter::New();
  m_HistogramFilter = HistogramFilterType::New();
}

template<class TTraits, class TBase>
ScalarImageWrapper<TTraits,TBase>
::ScalarImageWrapper(const Self &copy)
  : Superclass(copy)
{
}

template<class TTraits, class TBase>
ScalarImageWrapper<TTraits,TBase>
::~ScalarImageWrapper()
{
}


template<class TTraits, class TBase>
void 
ScalarImageWrapper<TTraits,TBase>
::UpdateWrappedImages(Image4DType *image_4d, ImageBaseType *referenceSpace, ITKTransformType *transform)
{
  // Call the parent
  Superclass::UpdateWrappedImages(image_4d, referenceSpace, transform);

  // Update the max-min pipeline once we have one setup
  m_MinMaxFilter->SetInput(image_4d);

  // Update the histogram mini-pipeline
  m_HistogramFilter->SetInput(image_4d);
  m_HistogramFilter->SetRangeInputs(m_MinMaxFilter->GetMinimumOutput(),
                                    m_MinMaxFilter->GetMaximumOutput());

  // Set the number of bins to default
  m_HistogramFilter->SetNumberOfBins(DEFAULT_HISTOGRAM_BINS);

  // Update the common representation policy
  m_CommonRepresentationPolicy.UpdateInputImage(this->GetImage());
}

template <class TTraits, class TBase>
void
ScalarImageWrapper<TTraits,TBase>
::SetNativeMapping(NativeIntensityMapping mapping)
{
  Superclass::SetNativeMapping(mapping);

  // Propagate the mapping to the histogram
  m_HistogramFilter->SetIntensityTransform(mapping.GetScale(), mapping.GetShift());
}


template<class TTraits, class TBase>
void 
ScalarImageWrapper<TTraits,TBase>
::CheckImageIntensityRange() 
{
  // Image should be loaded
  assert(this->m_Image);

  // Check if the image has been updated since the last time that
  // the min/max has been computed
  m_MinMaxFilter->Update();
  m_ImageScaleFactor = 1.0 / (m_MinMaxFilter->GetMaximum() - m_MinMaxFilter->GetMinimum());
}

template<class TTraits, class TBase>
const typename ScalarImageWrapper<TTraits, TBase>::ComponentTypeObject *
ScalarImageWrapper<TTraits, TBase>::GetImageMinObject() const
{
  return m_MinMaxFilter->GetMinimumOutput();
}

template<class TTraits, class TBase>
const typename ScalarImageWrapper<TTraits, TBase>::ComponentTypeObject *
ScalarImageWrapper<TTraits, TBase>::GetImageMaxObject() const
{
  return m_MinMaxFilter->GetMaximumOutput();
}

template<class TTraits, class TBase>
double
ScalarImageWrapper<TTraits,TBase>
::GetImageGradientMagnitudeUpperLimit()
{
  // Paul - 12/1/2014: I have modified this code to not compute the gradient magnitude
  // on the whole image, but instead, to compute just the difference between the max and
  // min values. I think this is much more efficient, and this removes some problems with
  // wrappers that wrap around ImageAdapter objects.
  //
  // I hope this does not cause too much trouble...
  return m_MinMaxFilter->GetMaximum() - m_MinMaxFilter->GetMinimum();
}

template<class TTraits, class TBase>
double
ScalarImageWrapper<TTraits,TBase>
::GetImageGradientMagnitudeUpperLimitNative()
{
  return this->m_NativeMapping.MapGradientMagnitudeToNative(
        this->GetImageGradientMagnitudeUpperLimit());
}


template<class TTraits, class TBase>
SmartPtr<typename ScalarImageWrapper<TTraits, TBase>::FloatImageSource>
ScalarImageWrapper<TTraits, TBase>::CreateCastToFloatPipeline() const
{
  // The kind of mini-pipeline that is created here depends on whether the
  // internal image is a floating point image or not, and whether the native
  // to intensity mapping is identity or not. We use template specialization to
  // select the right behavior
  typedef itk::UnaryFunctorImageFilter<ImageType, FloatImageType, NativeIntensityMapping> FilterType;
  SmartPtr<FilterType> filter = FilterType::New();
  filter->SetInput(this->m_Image);
  filter->SetFunctor(this->m_NativeMapping);

  SmartPtr<FloatImageSource> output = filter.GetPointer();
  return output;
}

template<class TTraits, class TBase>
SmartPtr<typename ScalarImageWrapper<TTraits, TBase>::DoubleImageSource>
ScalarImageWrapper<TTraits, TBase>::CreateCastToDoublePipeline() const
{
  // The kind of mini-pipeline that is created here depends on whether the
  // internal image is a floating point image or not, and whether the native
  // to intensity mapping is identity or not. We use template specialization to
  // select the right behavior
  typedef itk::UnaryFunctorImageFilter<ImageType, DoubleImageType, NativeIntensityMapping> FilterType;
  SmartPtr<FilterType> filter = FilterType::New();
  filter->SetInput(this->m_Image);
  filter->SetFunctor(this->m_NativeMapping);

  SmartPtr<DoubleImageSource> output = filter.GetPointer();
  return output;
}

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

template<class TTraits, class TBase>
SmartPtr<typename ScalarImageWrapper<TTraits, TBase>::FloatVectorImageSource>
ScalarImageWrapper<TTraits, TBase>::CreateCastToFloatVectorPipeline() const
{
  typedef UnaryFunctorImageToSingleComponentVectorImageFilter<
      ImageType, FloatVectorImageType, NativeIntensityMapping> FilterType;
  SmartPtr<FilterType> filter = FilterType::New();
  filter->SetInput(this->m_Image);
  filter->SetFunctor(this->m_NativeMapping);

  SmartPtr<FloatVectorImageSource> output = filter.GetPointer();
  return output;
}

template<class TTraits, class TBase>
SmartPtr<typename ScalarImageWrapper<TTraits, TBase>::DoubleVectorImageSource>
ScalarImageWrapper<TTraits, TBase>::CreateCastToDoubleVectorPipeline() const
{
  typedef UnaryFunctorImageToSingleComponentVectorImageFilter<
      ImageType, DoubleVectorImageType, NativeIntensityMapping> FilterType;
  SmartPtr<FilterType> filter = FilterType::New();
  filter->SetInput(this->m_Image);
  filter->SetFunctor(this->m_NativeMapping);

  SmartPtr<DoubleVectorImageSource> output = filter.GetPointer();
  return output;
}

template<class TTraits, class TBase>
double 
ScalarImageWrapper<TTraits,TBase>
::GetImageScaleFactor()
{
  // Make sure min/max are up-to-date
  CheckImageIntensityRange();

  // Return the max or min
  return m_ImageScaleFactor;
}

/** Compute statistics over a run of voxels in the image starting at the index
 * startIdx. Appends the statistics to a running sum and sum of squared. The
 * statistics are returned in internal (not native mapped) format */
template<class TTraits, class TBase>
void
ScalarImageWrapper<TTraits, TBase>
::GetRunLengthIntensityStatistics(
    const itk::ImageRegion<3> &region,
    const itk::Index<3> &startIdx, long runlength,
    double *out_sum, double *out_sumsq) const
{
  if(this->IsSlicingOrthogonal())
    {
    ConstIterator it(this->m_Image, region);
    it.SetIndex(startIdx);

    // Perform the integration
    for(long q = 0; q < runlength; q++, ++it)
      {
      double p = (double) it.Get();
      *out_sum += p;
      *out_sumsq += p * p;
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
template<class TTraits, class TBase>
void
ScalarImageWrapper<TTraits,TBase>
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

template<class TTraits, class TBase>
vtkImageImport *
ScalarImageWrapper<TTraits,TBase>
::GetVTKImporter()
{
  // Create an importer on demand
  if(!m_VTKImporter)
    {
    // TODO: using the common format image may cause unnecessary memory allocation!
    m_VTKExporter = VTKExportType::New();
    m_VTKImporter = vtkSmartPointer<vtkImageImport>::New();
    m_VTKExporter->SetInput(this->GetCommonFormatImage());
    ConnectITKExporterToVTKImporter(m_VTKExporter.GetPointer(), m_VTKImporter, true, true, false);
    }
  return m_VTKImporter;
}

template<class TTraits, class TBase>
const typename ScalarImageWrapper<TTraits, TBase>::CommonFormatImageType *
ScalarImageWrapper<TTraits, TBase>
::GetCommonFormatImage(ExportChannel channel)
{
  return m_CommonRepresentationPolicy.GetOutput(channel);
}

template<class TTraits, class TBase>
IntensityCurveInterface *
ScalarImageWrapper<TTraits, TBase>
::GetIntensityCurve() const
{
  return this->m_DisplayMapping->GetIntensityCurve();
}

template<class TTraits, class TBase>
ColorMap *
ScalarImageWrapper<TTraits, TBase>
::GetColorMap() const
{
  return this->m_DisplayMapping->GetColorMap();
}

template<class TTraits, class TBase>
const ScalarImageHistogram *
ScalarImageWrapper<TTraits,TBase>
::GetHistogram(size_t nBins)
{
  // If the user passes in a non-zero number of bins, we pass that as a
  // parameter to the filter
  if(nBins > 0)
    m_HistogramFilter->SetNumberOfBins(nBins);

  m_HistogramFilter->Update();
  return m_HistogramFilter->GetHistogramOutput();
}

template<class TTraits, class TBase>
void
ScalarImageWrapper<TTraits, TBase>
::WriteToFileAsFloat(const char *fname, Registry &hints)
{
  SmartPtr<GuidedNativeImageIO> io = GuidedNativeImageIO::New();
  io->CreateImageIO(fname, hints, false);
  itk::ImageIOBase *base = io->GetIOBase();

  SmartPtr<FloatImageSource> pipeline = this->CreateCastToFloatPipeline();

  typedef itk::ImageFileWriter<FloatImageType> WriterType;
  SmartPtr<WriterType> writer = WriterType::New();
  writer->SetFileName(fname);
  if(base)
    writer->SetImageIO(base);
  writer->SetInput(pipeline->GetOutput());
  writer->Update();
}



template class ScalarImageWrapper<LabelImageWrapperTraits>;
template class ScalarImageWrapper<SpeedImageWrapperTraits>;
template class ScalarImageWrapper<LevelSetImageWrapperTraits>;
template class ScalarImageWrapper< ComponentImageWrapperTraits<GreyType> >;
template class ScalarImageWrapper< AnatomicScalarImageWrapperTraits<GreyType> >;

typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMagnitudeFunctor> MagTraits;
typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMaxFunctor> MaxTraits;
typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMeanFunctor> MeanTraits;
template class ScalarImageWrapper<MagTraits>;
template class ScalarImageWrapper<MaxTraits>;
template class ScalarImageWrapper<MeanTraits>;

