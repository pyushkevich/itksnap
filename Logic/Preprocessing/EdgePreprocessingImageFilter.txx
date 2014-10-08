/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: EdgePreprocessingImageFilter.txx,v $
  Language:  C++
  Date:      $Date: 2010/06/28 18:45:08 $
  Version:   $Revision: 1.3 $
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

=========================================================================*/

#include <EdgePreprocessingSettings.h>
#include <itkProgressAccumulator.h>

#include <itkCastImageFilter.h>
#include <itkDiscreteGaussianImageFilter.h>
#include <itkGradientMagnitudeImageFilter.h>
#include <itkUnaryFunctorImageFilter.h>
#include <IRISException.h>

template<typename TInputImage,typename TOutputImage>
EdgePreprocessingImageFilter<TInputImage,TOutputImage>
::EdgePreprocessingImageFilter()
{  
  // Set the number of inputs to two (second is the parameters)
  this->SetNumberOfIndexedInputs(2);

  // Set the gradient magnitude to default value
  m_InputImageMaximumGradientMagnitude = 0.0;

  // Initialize the mini-pipeline
  m_CastFilter = CastFilter::New();
  m_CastFilter->ReleaseDataFlagOn();

#ifndef SNAP_USE_GPU
  m_BlurFilter = BlurFilter::New();
  m_BlurFilter->SetInput(m_CastFilter->GetOutput());
  m_BlurFilter->ReleaseDataFlagOn();

  // Prevent streaming inside the Gaussian filter because we will be streaming
  // anyway. Too much streaming increases execution time unnecessarilty
  m_BlurFilter->SetInternalNumberOfStreamDivisions(1);
  m_BlurFilter->SetMaximumError(0.1);

  m_GradMagFilter = GradMagFilter::New();
  m_GradMagFilter->SetInput(m_BlurFilter->GetOutput());
  m_GradMagFilter->ReleaseDataFlagOn();
#else
  m_GPUImageSource = GPUImageSource::New();
  m_GPUImageSource->SetInput(m_CastFilter->GetOutput());

  m_GPUBlurFilter = GPUBlurFilter::New();
  m_GPUBlurFilter->SetInput(m_GPUImageSource->GetOutput());
  m_GPUBlurFilter->ReleaseDataFlagOn();

  // Prevent streaming inside the Gaussian filter because we will be streaming
  // anyway. Too much streaming increases execution time unnecessarilty
  m_GPUBlurFilter->SetInternalNumberOfStreamDivisions(1);
  m_GPUBlurFilter->SetMaximumError(0.1);
  //m_ROIFilter = ROIFilter::New();
  //m_ROIFilter->SetInput(m_GPUBlurFilter->GetOutput());

  m_GradMagFilter = GradMagFilter::New();
  m_GradMagFilter->SetInput(m_GPUBlurFilter->GetOutput());
  //m_GradMagFilter->SetInput(m_ROIFilter->GetOutput());
  m_GradMagFilter->ReleaseDataFlagOn();
#endif

  m_RemapFilter = RemapFilter::New();
  m_RemapFilter->SetInput(m_GradMagFilter->GetOutput());
}

template<typename TInputImage,typename TOutputImage>
void
EdgePreprocessingImageFilter<TInputImage,TOutputImage>
::GenerateData()
{
  // Get the input and output pointers
  const typename InputImageType::ConstPointer inputImage = this->GetInput();
  typename OutputImageType::Pointer outputImage = this->GetOutput();

  // Get the settings
  EdgePreprocessingSettings *settings = this->GetParameters();

  // Settings must be set!
  if(!settings)
    throw IRISException("Parameters not set in EdgePreprocessingImageFilter");

  itk::ProgressAccumulator::Pointer pac = itk::ProgressAccumulator::New();
  pac->SetMiniPipelineFilter(this);

#ifndef SNAP_USE_GPU
  pac->RegisterInternalFilter(m_BlurFilter, 0.8);
#else
  pac->RegisterInternalFilter(m_GPUBlurFilter, 0.8);
#endif

    pac->RegisterInternalFilter(m_RemapFilter, 0.2);

  // Configure the pipeline
  m_CastFilter->SetInput(inputImage);

  // Configure the Gaussian
#ifndef SNAP_USE_GPU
  m_BlurFilter->SetUseImageSpacingOff();
  m_BlurFilter->SetVariance(
        settings->GetGaussianBlurScale() * settings->GetGaussianBlurScale());
#else
  m_GPUBlurFilter->SetUseImageSpacingOff();
  m_GPUBlurFilter->SetVariance(
        settings->GetGaussianBlurScale() * settings->GetGaussianBlurScale());
#endif

  // Construct the functor
  // TODO: fixme!
  FunctorType functor;
  functor.SetParameters(0.0, m_InputImageMaximumGradientMagnitude,
                        settings->GetRemappingExponent(),
                        settings->GetRemappingSteepness());

  // Configure the remapping filter
  m_RemapFilter->SetFunctor(functor);

  // Graft outputs and update the filter
  m_RemapFilter->GraftOutput(outputImage);
  m_RemapFilter->Update();
  this->GraftOutput(m_RemapFilter->GetOutput());
}

template<typename TInputImage,typename TOutputImage>
void
EdgePreprocessingImageFilter<TInputImage,TOutputImage>
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf(os,indent);
}

template<typename TInputImage,typename TOutputImage>
void 
EdgePreprocessingImageFilter<TInputImage,TOutputImage>
::SetParameters(EdgePreprocessingSettings *settings)
{
  if(settings != GetParameters())
    {
    this->SetNthInput(1, settings);
    }
}

template<typename TInputImage,typename TOutputImage>
EdgePreprocessingSettings *
EdgePreprocessingImageFilter<TInputImage,TOutputImage>
::GetParameters()
{
  if(this->GetNumberOfInputs() > 1)
    {
    return static_cast<EdgePreprocessingSettings *>(
          this->GetInputs()[1].GetPointer());
    }
  else return NULL;
}


template<typename TInputImage,typename TOutputImage>
void 
EdgePreprocessingImageFilter<TInputImage,TOutputImage>
::GenerateInputRequestedRegion()
{
  // Make sure we call the parent's implementation
  Superclass::GenerateInputRequestedRegion();

  // Get pointers to the input and output
  InputImagePointer  inputPtr = 
    const_cast< TInputImage * >( this->GetInput() );
  OutputImagePointer outputPtr = this->GetOutput();

  // Use the largest possible region (hack)
  inputPtr->SetRequestedRegion(
        inputPtr->GetLargestPossibleRegion());
}

