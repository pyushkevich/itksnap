/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SmoothBinaryThresholdImageFilter.txx,v $
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
SmoothBinaryThresholdImageFilter<TInputImage,TOutputImage>
::SmoothBinaryThresholdImageFilter()
{
  // Construct the mini-pipeline
  m_Calculator = CalculatorType::New();
  m_ThresholdFilter = ThresholdFilterType::New();
  
  // Initialize the progress tracker
  m_ProgressAccumulator = itk::ProgressAccumulator::New();
  m_ProgressAccumulator->SetMiniPipelineFilter(this);
  
  // Add the filters to the progress accumulator (just one filter)
  m_ProgressAccumulator->RegisterInternalFilter(m_ThresholdFilter,1.0f);
}

template<typename TInputImage,typename TOutputImage>
void
SmoothBinaryThresholdImageFilter<TInputImage,TOutputImage>
::GenerateData()
{
  // Get the input and output pointers
  const typename InputImageType::ConstPointer inputImage = this->GetInput();
  typename OutputImageType::Pointer outputImage = this->GetOutput();

  // Allocate the output image
  outputImage->SetBufferedRegion(outputImage->GetRequestedRegion());
  outputImage->Allocate();

  // Reset the progress
  m_ProgressAccumulator->ResetProgress();

  // Compute the min/max of the image
  m_Calculator->SetImage(inputImage);
  m_Calculator->Compute();
  
  // Construct the functor
  FunctorType functor;
  functor.SetParameters(m_Calculator->GetMinimum(),
                        m_Calculator->GetMaximum(),
                        m_ThresholdSettings);

  // Assign the functor to the filter
  m_ThresholdFilter->SetInput(inputImage);
  m_ThresholdFilter->SetFunctor(functor);

  // Call the filter's GenerateData()
  m_ThresholdFilter->GraftOutput(outputImage);
  m_ThresholdFilter->Update();

  // graft the mini-pipeline output back onto this filter's output.
  // this is needed to get the appropriate regions passed back.
  this->GraftOutput( m_ThresholdFilter->GetOutput() );
}

template<typename TInputImage,typename TOutputImage>
void
SmoothBinaryThresholdImageFilter<TInputImage,TOutputImage>
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf(os,indent);
}

template<typename TInputImage,typename TOutputImage>
void 
SmoothBinaryThresholdImageFilter<TInputImage,TOutputImage>
::SetThresholdSettings(const ThresholdSettings &settings)
{
  if(!(settings == m_ThresholdSettings))
    {
    m_ThresholdSettings = settings;
    this->Modified();
    }
}
