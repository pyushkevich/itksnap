/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: SignedDistanceFilter.txx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:14 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
using namespace itk;

template<typename TInputImage,typename TOutputImage>
SignedDistanceFilter<TInputImage,TOutputImage>
::SignedDistanceFilter()
{ 
  // Create the inverter
  m_InvertFilter = InvertFilterType::New();
  m_InvertFilter->SetInput(this->GetInput());
  m_InvertFilter->ReleaseDataFlagOn();

  // Create the exterior distance filter 
  m_OutsideDistanceFilter = DistanceFilterType::New();
  m_OutsideDistanceFilter->SetInputIsBinary(true);
  m_OutsideDistanceFilter->SetInput(this->GetInput());
  m_OutsideDistanceFilter->ReleaseDataFlagOn();  
  
  // And the interior filter
  m_InsideDistanceFilter = DistanceFilterType::New();
  m_InsideDistanceFilter->SetInput(m_InvertFilter->GetOutput());
  m_InsideDistanceFilter->SetInputIsBinary(true);
  m_InsideDistanceFilter->ReleaseDataFlagOn();

  // Filter to subtract the inside from the outside
  m_SubtractFilter = SubtractFilterType::New();
  m_SubtractFilter->SetInput1(m_OutsideDistanceFilter->GetDistanceMap());
  m_SubtractFilter->SetInput2(m_InsideDistanceFilter->GetDistanceMap());

  // Create the progress accumulator
  m_ProgressAccumulator = itk::ProgressAccumulator::New();
  m_ProgressAccumulator->SetMiniPipelineFilter(this);

  // Register the filters with the progress accumulator
  m_ProgressAccumulator->RegisterInternalFilter(m_InvertFilter,0.05f);
  m_ProgressAccumulator->RegisterInternalFilter(m_InsideDistanceFilter,0.45f);
  m_ProgressAccumulator->RegisterInternalFilter(m_OutsideDistanceFilter,0.45f);
  m_ProgressAccumulator->RegisterInternalFilter(m_SubtractFilter,0.05f);
}

template<typename TInputImage,typename TOutputImage>
void
SignedDistanceFilter<TInputImage,TOutputImage>
::GenerateData()
{
  // Get the input and output pointers
  const typename InputImageType::ConstPointer inputImage = this->GetInput();
  typename OutputImageType::Pointer outputImage = this->GetOutput();

  // Initialize the progress counter
  m_ProgressAccumulator->ResetProgress();

  // Allocate the output image
  outputImage->SetBufferedRegion(outputImage->GetRequestedRegion());
  outputImage->Allocate();

  // Set the inputs
  m_InvertFilter->SetInput(inputImage);
  m_OutsideDistanceFilter->SetInput(inputImage);

  // Call the filter's GenerateData()
  m_SubtractFilter->GraftOutput(outputImage);
  m_SubtractFilter->Update();

  // graft the mini-pipeline output back onto this filter's output.
  // this is needed to get the appropriate regions passed back.
  GraftOutput( m_SubtractFilter->GetOutput() );
}

template<typename TInputImage,typename TOutputImage>
void
SignedDistanceFilter<TInputImage,TOutputImage>
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os,indent);
}
