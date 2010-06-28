/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SignedDistanceFilter.txx,v $
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
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf(os,indent);
}
