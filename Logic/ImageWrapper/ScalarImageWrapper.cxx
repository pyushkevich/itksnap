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
#include "itkImageRegionIterator.h"
#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkNumericTraits.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkIdentityTransform.h"
#include "itkResampleImageFilter.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkBSplineInterpolateImageFunction.h"
#include "itkWindowedSincInterpolateImageFunction.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkVTKImageExport.h"
#include "itkStreamingImageFilter.h"

#include "IRISSlicer.h"
#include "SNAPSegmentationROISettings.h"
#include "itkCommand.h"
#include "itkMinimumMaximumImageFilter.h"
#include "itkGradientMagnitudeImageFilter.h"
#include "itkVectorImageToImageAdaptor.h"
#include "IRISException.h"
#include "VectorImageWrapper.h"
#include "ScalarImageHistogram.h"
#include "ThreadedHistogramImageFilter.h"

#include "vtkImageImport.h"

#include <iostream>

template<class TTraits, class TBase>
ScalarImageWrapper<TTraits,TBase>
::ScalarImageWrapper()
{
  m_MinMaxFilter = MinMaxFilter::New();
  m_HistogramFilter = HistogramFilterType::New();

  m_GradientMagnitudeFilter = GradMagFilter::New();
  m_GradientMagnitudeFilter->ReleaseDataFlagOn();

  // TODO: This filter is a huge waste of memory because it computes the
  // gradient of the image just to obtain a maximum value. Not only that,
  // the filter is applied to the common representation (float) image, so
  // it requires another intermediate volume. The right thing to do would
  // be to implement a filter that computes the maximum possible gradient
  // magnitude of the image using a streaming implementation, and without
  // having to cast to float.
  m_GradientMagnitudeMaximumFilter = GradMagMaxFilter::New();
  m_GradientMagnitudeMaximumFilter->SetInput(m_GradientMagnitudeFilter->GetOutput());

  // Set up VTK export pipeline
  this->SetupVTKImportExport();
}

template<class TTraits, class TBase>
ScalarImageWrapper<TTraits,TBase>
::ScalarImageWrapper(const Self &copy)
{
  Superclass::CommonInitialization();

  // If the source contains an image, make a copy of that image
  if (copy.IsInitialized() && copy.GetImage())
    {
    // Create and allocate the image
    ImagePointer newImage = ImageType::New();
    newImage->SetRegions(copy.GetImage()->GetBufferedRegion());
    newImage->Allocate();

    // Copy the image contents
    typedef typename ImageType::InternalPixelType InternalPixelType;
    InternalPixelType *ptrTarget = newImage->GetBufferPointer();
    InternalPixelType *ptrSource = copy.GetImage()->GetBufferPointer();
    memcpy(ptrTarget,ptrSource,
           sizeof(InternalPixelType) * newImage->GetBufferedRegion().GetNumberOfPixels());
    
    UpdateImagePointer(newImage);
    }
}

template<class TTraits, class TBase>
ScalarImageWrapper<TTraits,TBase>
::~ScalarImageWrapper()
{

}


template<class TTraits, class TBase>
void 
ScalarImageWrapper<TTraits,TBase>
::UpdateImagePointer(ImageType *newImage) 
{
  // Call the parent
  Superclass::UpdateImagePointer(newImage);

  // Update the max-min pipeline once we have one setup
  m_MinMaxFilter->SetInput(newImage);

  // Update the histogram mini-pipeline
  m_HistogramFilter->SetInput(newImage);
  m_HistogramFilter->SetRangeInputs(m_MinMaxFilter->GetMinimumOutput(),
                                    m_MinMaxFilter->GetMaximumOutput());

  // Set the number of bins to default
  m_HistogramFilter->SetNumberOfBins(DEFAULT_HISTOGRAM_BINS);

  // Update the common representation policy
  m_CommonRepresentationPolicy.UpdateInputImage(newImage);

  // Also update the grad max range pipeline
  CommonFormatImageType *imgCommon =
      m_CommonRepresentationPolicy.GetOutput(ScalarImageWrapperBase::WHOLE_IMAGE);

  m_GradientMagnitudeFilter->SetInput(imgCommon);

  m_VTKExporter->SetInput(newImage);
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
typename ScalarImageWrapper<TTraits,TBase>::ComponentTypeObject *
ScalarImageWrapper<TTraits,TBase>
::GetImageMinObject() const
{
  return m_MinMaxFilter->GetMinimumOutput();
}

template<class TTraits, class TBase>
typename ScalarImageWrapper<TTraits,TBase>::ComponentTypeObject *
ScalarImageWrapper<TTraits,TBase>
::GetImageMaxObject() const
{
  return m_MinMaxFilter->GetMaximumOutput();
}

template<class TTraits, class TBase>
double
ScalarImageWrapper<TTraits,TBase>
::GetImageGradientMagnitudeUpperLimit()
{
  m_GradientMagnitudeMaximumFilter->Update();
  return m_GradientMagnitudeMaximumFilter->GetMaximum();
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
double 
ScalarImageWrapper<TTraits,TBase>
::GetImageScaleFactor()
{
  // Make sure min/max are up-to-date
  CheckImageIntensityRange();

  // Return the max or min
  return m_ImageScaleFactor;
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
  // Make sure the display slice is updated
  this->GetDisplaySlice(0)->GetSource()->UpdateLargestPossibleRegion();

  // Find the correct voxel in the space of the first display slice
  Vector3ui idxDisp =
      this->GetImageToDisplayTransform(0).TransformVoxelIndex(this->GetSliceIndex());

  // Get the RGB value
  typename DisplaySliceType::IndexType idx2D = {{idxDisp[0], idxDisp[1]}};
  out_appearance = this->GetDisplaySlice(0)->GetPixel(idx2D);

  // Get the numerical value
  PixelType val_raw = this->GetSlicer(0)->GetOutput()->GetPixel(idx2D);
  out_value.set_size(1);
  out_value[0] = this->m_NativeMapping(val_raw);
}

template<class TTraits, class TBase>
void
ScalarImageWrapper<TTraits,TBase>
::SetupVTKImportExport()
{
  // Initialize the VTK Exporter
  m_VTKExporter = VTKExporter::New();
  m_VTKExporter->ReleaseDataFlagOn();

  // Initialize the VTK Importer
  m_VTKImporter = vtkImageImport::New();
  m_VTKImporter->ReleaseDataFlagOn();

  // Pipe the importer into the exporter (that's a lot of code)
  m_VTKImporter->SetUpdateInformationCallback(
        m_VTKExporter->GetUpdateInformationCallback());
  m_VTKImporter->SetPipelineModifiedCallback(
        m_VTKExporter->GetPipelineModifiedCallback());
  m_VTKImporter->SetWholeExtentCallback(
        m_VTKExporter->GetWholeExtentCallback());
  m_VTKImporter->SetSpacingCallback(
        m_VTKExporter->GetSpacingCallback());
  m_VTKImporter->SetOriginCallback(
        m_VTKExporter->GetOriginCallback());
  m_VTKImporter->SetScalarTypeCallback(
        m_VTKExporter->GetScalarTypeCallback());
  m_VTKImporter->SetNumberOfComponentsCallback(
        m_VTKExporter->GetNumberOfComponentsCallback());
  m_VTKImporter->SetPropagateUpdateExtentCallback(
        m_VTKExporter->GetPropagateUpdateExtentCallback());
  m_VTKImporter->SetUpdateDataCallback(
        m_VTKExporter->GetUpdateDataCallback());
  m_VTKImporter->SetDataExtentCallback(
        m_VTKExporter->GetDataExtentCallback());
  m_VTKImporter->SetBufferPointerCallback(
        m_VTKExporter->GetBufferPointerCallback());
  m_VTKImporter->SetCallbackUserData(
        m_VTKExporter->GetCallbackUserData());
}

template<class TTraits, class TBase>
vtkImageImport *
ScalarImageWrapper<TTraits,TBase>
::GetVTKImporter()
{
  return m_VTKImporter;
}

template<class TTraits, class TBase>
typename ScalarImageWrapper<TTraits, TBase>::CommonFormatImageType *
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


template class ScalarImageWrapper<LabelImageWrapperTraits>;
template class ScalarImageWrapper<SpeedImageWrapperTraits>;
template class ScalarImageWrapper<LevelSetImageWrapperTraits>;
template class ScalarImageWrapper<JsrcImageWrapperTraits>;
template class ScalarImageWrapper< ComponentImageWrapperTraits<GreyType> >;

typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMagnitudeFunctor> MagTraits;
typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMaxFunctor> MaxTraits;
typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMeanFunctor> MeanTraits;
template class ScalarImageWrapper<MagTraits>;
template class ScalarImageWrapper<MaxTraits>;
template class ScalarImageWrapper<MeanTraits>;

