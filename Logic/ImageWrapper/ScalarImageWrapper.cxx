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
#include "IRISSlicer.h"
#include "SNAPSegmentationROISettings.h"
#include "itkCommand.h"
#include "itkMinimumMaximumImageFilter.h"
#include "itkGradientMagnitudeImageFilter.h"
#include "itkVectorImageToImageAdaptor.h"
#include "IRISException.h"
#include "VectorImageWrapper.h"

#include "ScalarImageHistogram.h"
#include "itkStreamingImageFilter.h"

#include <iostream>

template<class TTraits, class TBase>
ScalarImageWrapper<TTraits,TBase>
::ScalarImageWrapper()
{
  m_Histogram = ScalarImageHistogram::New();
  m_MinMaxFilter = MinMaxFilter::New();

  m_GradientMagnitudeFilter = GradMagFilter::New();
  m_GradientMagnitudeFilter->ReleaseDataFlagOn();

  // Set up a streamer to avoid extra memory use when using the gradient mag
  // filter. Ideally, there would be a streaming MaximumMinimum filter available
  typedef itk::StreamingImageFilter<FloatImageType, FloatImageType> StreamerType;
  typename StreamerType::Pointer streamer = StreamerType::New();
  streamer->SetInput(m_GradientMagnitudeFilter->GetOutput());
  streamer->SetNumberOfStreamDivisions(16);

  m_GradientMagnitudeMaximumFilter = GradMagMaxFilter::New();
  m_GradientMagnitudeMaximumFilter->SetInput(streamer->GetOutput());
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

  // Update the common representation policy
  m_CommonRepresentationPolicy.UpdateInputImage(newImage);

  // Also update the grad max range pipeline
  CommonFormatImageType *imgCommon =
      m_CommonRepresentationPolicy.GetOutput(ScalarImageWrapperBase::WHOLE_IMAGE);

  m_GradientMagnitudeFilter->SetInput(imgCommon);


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

template<class TTraits, class TBase>
vnl_vector<double>
ScalarImageWrapper<TTraits,TBase>
::GetVoxelUnderCursorDisplayedValue()
{
  vnl_vector<double> v(1);
  v[0] = this->GetVoxelMappedToNative(this->m_SliceIndex);
  return v;
}



//#include "itkListSampleToHistogramGenerator.h"
//#include "itkImageToListAdaptor.h"

template<class TTraits, class TBase>
const ScalarImageHistogram *
ScalarImageWrapper<TTraits,TBase>
::GetHistogram(size_t nBins)
{
  // Zero parameter means we want to reuse the current histogram size
  if(nBins == 0)
    nBins = m_Histogram->GetSize();
  if(nBins == 0)
    nBins = 128;

  // First check if an update is needed
  if(m_Histogram->GetMTime() < this->m_Image->GetMTime() ||
     m_Histogram->GetSize() != nBins)
    {
    // Create the histogram
    m_Histogram->Initialize(this->GetImageMinNative(),
                            this->GetImageMaxNative(),
                            nBins);

    // Add all points as samples
    for(ConstIterator it = this->GetImageConstIterator(); !it.IsAtEnd(); ++it)
      {
      m_Histogram->AddSample(this->m_NativeMapping(it.Get()));
      }

    // Set the m-time
    m_Histogram->Modified();
    }

  return m_Histogram;
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



template class ScalarImageWrapper<LabelImageWrapperTraits>;
template class ScalarImageWrapper<SpeedImageWrapperTraits>;
template class ScalarImageWrapper<LevelSetImageWrapperTraits>;
template class ScalarImageWrapper< ComponentImageWrapperTraits<GreyType> >;

typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMagnitudeFunctor> MagTraits;
typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMaxFunctor> MaxTraits;
typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMeanFunctor> MeanTraits;
template class ScalarImageWrapper<MagTraits>;
template class ScalarImageWrapper<MaxTraits>;
template class ScalarImageWrapper<MeanTraits>;

