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
#ifndef __ScalarImageWrapper_txx_
#define __ScalarImageWrapper_txx_

#include "ScalarImageWrapper.h"
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

#include <iostream>

template <class TPixel>
ScalarImageWrapper<TPixel>
::ScalarImageWrapper(const ScalarImageWrapper<TPixel> &copy) 
{
  ImageWrapper<TPixel>::CommonInitialization();

  // If the source contains an image, make a copy of that image
  if (copy.IsInitialized() && copy.GetImage())
    {
    // Create and allocate the image
    ImagePointer newImage = ImageType::New();
    newImage->SetRegions(copy.GetImage()->GetBufferedRegion());
    newImage->Allocate();

    // Copy the image contents
    TPixel *ptrTarget = newImage->GetBufferPointer();
    TPixel *ptrSource = copy.GetImage()->GetBufferPointer();
    memcpy(ptrTarget,ptrSource,
           sizeof(TPixel) * newImage->GetBufferedRegion().GetNumberOfPixels());
    
    UpdateImagePointer(newImage);
    }
}

template <class TPixel>
void 
ScalarImageWrapper<TPixel>
::UpdateImagePointer(ImageType *newImage) 
{
  // Update the max-min pipeline once we have one setup
  m_MinMaxCalc = MinMaxCalculatorType::New();
  m_MinMaxCalc->SetImage(newImage);
  
  m_MinMaxCalc->SetRegion(newImage->GetLargestPossibleRegion());

  ImageWrapper<TPixel>::UpdateImagePointer(newImage);
}

template <class TPixel>
typename ScalarImageWrapper<TPixel>::ImagePointer
ScalarImageWrapper<TPixel>
::DeepCopyRegion(const SNAPSegmentationROISettings &roi,
                 itk::Command *progressCommand) const
{
  // The filter used to chop off the region of interest
  typedef itk::RegionOfInterestImageFilter <ImageType,ImageType> ChopFilterType;
  typename ChopFilterType::Pointer fltChop = ChopFilterType::New();

  // Check if there is a difference in voxel size, i.e., user wants resampling
  Vector3ul vOldSize(this->m_Image->GetLargestPossibleRegion().GetSize().GetSize());
  Vector3d vOldSpacing(this->m_Image->GetSpacing().GetDataPointer());
  
  if(roi.GetResampleFlag())
    {
    // Compute the number of voxels in the output
    typedef typename itk::ImageRegion<3> RegionType;
    typedef typename itk::Size<3> SizeType;

    SizeType vNewSize;
    RegionType vNewROI;
    Vector3d vNewSpacing;

    for(unsigned int i = 0; i < 3; i++) 
      {
      double scale = roi.GetVoxelScale()[i];
      vNewSize.SetElement(i, (unsigned long) (vOldSize[i] / scale));
      vNewROI.SetSize(i,(unsigned long) (roi.GetROI().GetSize(i) / scale));
      vNewROI.SetIndex(i,(long) (roi.GetROI().GetIndex(i) / scale));
      vNewSpacing[i] = scale * vOldSpacing[i];
      }

    // Create a filter for resampling the image
    typedef itk::ResampleImageFilter<ImageType,ImageType> ResampleFilterType;
    typename ResampleFilterType::Pointer fltSample = ResampleFilterType::New();

    // Initialize the resampling filter
    fltSample->SetInput(this->m_Image);
    fltSample->SetTransform(itk::IdentityTransform<double,3>::New());

    // Typedefs for interpolators
    typedef itk::NearestNeighborInterpolateImageFunction<
      ImageType,double> NNInterpolatorType;
    typedef itk::LinearInterpolateImageFunction<
      ImageType,double> LinearInterpolatorType;
    typedef itk::BSplineInterpolateImageFunction<
      ImageType,double> CubicInterpolatorType;

    // More typedefs are needed for the sinc interpolator
    static const unsigned int VRadius = 5;
    typedef itk::Function::HammingWindowFunction<VRadius> WindowFunction;
    typedef itk::ConstantBoundaryCondition<ImageType> Condition;
    typedef itk::WindowedSincInterpolateImageFunction<
      ImageType, VRadius, 
      WindowFunction, Condition, double> SincInterpolatorType;

    // Choose the interpolator
    switch(roi.GetInterpolationMethod())
      {
      case SNAPSegmentationROISettings::NEAREST_NEIGHBOR :
        fltSample->SetInterpolator(NNInterpolatorType::New());
        break;

      case SNAPSegmentationROISettings::TRILINEAR : 
        fltSample->SetInterpolator(LinearInterpolatorType::New());
        break;

      case SNAPSegmentationROISettings::TRICUBIC :
        fltSample->SetInterpolator(CubicInterpolatorType::New());
        break;  

      case SNAPSegmentationROISettings::SINC_WINDOW_05 :
        fltSample->SetInterpolator(SincInterpolatorType::New());
        break;  
      };

    // Set the image sizes and spacing
    fltSample->SetSize(vNewSize);
    fltSample->SetOutputSpacing(vNewSpacing.data_block());
    fltSample->SetOutputOrigin(this->m_Image->GetOrigin());
    fltSample->SetOutputDirection(this->m_Image->GetDirection());

    // Set the progress bar
    if(progressCommand)
      fltSample->AddObserver(itk::AnyEvent(),progressCommand);

    // Perform resampling
    fltSample->GetOutput()->SetRequestedRegion(vNewROI);
    fltSample->Update();  

    // Pipe into the chopper
    fltChop->SetInput(fltSample->GetOutput());

    // Update the region of interest
    fltChop->SetRegionOfInterest(vNewROI);
    }
  else
    {
    // Pipe image into the chopper
    fltChop->SetInput(this->m_Image);    
    
    // Set the region of interest
    fltChop->SetRegionOfInterest(roi.GetROI());
    }

  // Update the pipeline
  fltChop->Update();

  // Return the resulting image
  return fltChop->GetOutput();
}

template <class TPixel> 
void 
ScalarImageWrapper<TPixel>
::CheckImageIntensityRange() 
{
  // Image should be loaded
  assert(this->m_Image && m_MinMaxCalc);

  // Check if the image has been updated since the last time that
  // the min/max has been computed
  if (this->m_Image->GetMTime() > m_MinMaxCalc->GetMTime())
    {
    m_MinMaxCalc->Compute();
    m_MinMaxCalc->Modified();
    m_ImageScaleFactor = 1.0 / (m_MinMaxCalc->GetMaximum() - m_MinMaxCalc->GetMinimum());
    }
}

template <class TPixel> 
TPixel 
ScalarImageWrapper<TPixel>
::GetImageMin() 
{
  // Make sure min/max are up-to-date
  CheckImageIntensityRange();

  // Return the max or min
  return m_MinMaxCalc->GetMinimum();
}

template <class TPixel> 
TPixel 
ScalarImageWrapper<TPixel>
::GetImageMax()
{
  // Make sure min/max are up-to-date
  CheckImageIntensityRange();

  // Return the max or min
  return m_MinMaxCalc->GetMaximum();
}

template <class TPixel>
double 
ScalarImageWrapper<TPixel>
::GetImageScaleFactor()
{
  // Make sure min/max are up-to-date
  CheckImageIntensityRange();

  // Return the max or min
  return m_ImageScaleFactor;    
}

template <class TPixel>    
void 
ScalarImageWrapper<TPixel>
::RemapIntensityToRange(TPixel min, TPixel max)
{
  typedef itk::RescaleIntensityImageFilter<ImageType> FilterType;
  typedef typename FilterType::Pointer FilterPointer;

  // Create a filter to remap the intensities
  FilterPointer filter = FilterType::New();
  filter->SetInput(this->m_Image);
  filter->SetOutputMinimum(min);
  filter->SetOutputMaximum(max);

  // Run the filter
  filter->Update();

  // Store the output and point everything to it
  UpdateImagePointer(filter->GetOutput());
}

template <class TPixel>    
void 
ScalarImageWrapper<TPixel>
::RemapIntensityToMaximumRange()
{
  RemapIntensityToRange(itk::NumericTraits<TPixel>::min(),
                        itk::NumericTraits<TPixel>::max());
}

#endif // __ScalarImageWrapper_txx_
