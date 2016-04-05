/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPAdvectionFieldImageFilter.txx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:14 $
  Version:   $Revision: 1.2 $
  Copyright (c) 2007 Paul A. Yushkevich
  
  This file is part of ITK-SNAP 

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#include "itkGradientImageFilter.h"
#include "itkMultiplyImageFilter.h"

template<class TInputImage, class TOutputValueType>
SNAPAdvectionFieldImageFilter<TInputImage,TOutputValueType>
::SNAPAdvectionFieldImageFilter()
{
  m_Exponent = 0;
}

template<class TInputImage, class TOutputValueType>
void
SNAPAdvectionFieldImageFilter<TInputImage,TOutputValueType>
::GenerateData()
{
  // Get the input and output pointers
  typename InputImageType::ConstPointer imgInput = this->GetInput();
  typename OutputImageType::Pointer imgOutput = this->GetOutput();

  // Allocate the output image
  imgOutput->SetBufferedRegion(imgOutput->GetRequestedRegion());
  imgOutput->Allocate();

  // Create a new gradient filter
  typedef itk::GradientImageFilter<
    InputImageType,TOutputValueType,TOutputValueType> GradientFilter;
  typename GradientFilter::Pointer fltGradient = GradientFilter::New();
  fltGradient->SetInput(imgInput);
  fltGradient->ReleaseDataFlagOn();

  // A pointer to the pipeline tail
  typename itk::ImageSource<OutputImageType>::Pointer 
    fltPipeEnd = fltGradient.GetPointer();
  
  // Attach the appropriate number of multiplicative filters
  typedef itk::MultiplyImageFilter<
    OutputImageType,InputImageType,OutputImageType> MultiplyFilter;
  
  for(unsigned int i=0;i<m_Exponent;i++)
    {
    typename MultiplyFilter::Pointer fltMulti = MultiplyFilter::New();
    fltMulti->SetInput1(fltPipeEnd->GetOutput());
    fltMulti->SetInput2(imgInput);
    fltMulti->ReleaseDataFlagOn();
    fltPipeEnd = fltMulti;
    }
  
  // Call the filter's GenerateData()
  fltPipeEnd->GraftOutput(imgOutput);
  fltPipeEnd->Update();

  // graft the mini-pipeline output back onto this filter's output.
  // this is needed to get the appropriate regions passed back.
  this->GraftOutput( fltPipeEnd->GetOutput() );
}

template<class TInputImage, class TOutputValueType>
void
SNAPAdvectionFieldImageFilter<TInputImage,TOutputValueType>
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf(os,indent);
}
