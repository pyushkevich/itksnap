/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef __CPUImageToGPUImageFilter_hxx
#define __CPUImageToGPUImageFilter_hxx

#include "CPUImageToGPUImageFilter.h"
#include "itkImageAlgorithm.h"

template< class TGPUOutputImage >
CPUImageToGPUImageFilter< TGPUOutputImage >::CPUImageToGPUImageFilter()
{
  m_GPUKernelManager = itk::GPUKernelManager::New();
}

template< class TGPUOutputImage >
CPUImageToGPUImageFilter< TGPUOutputImage >::~CPUImageToGPUImageFilter()
{
}

template< class TGPUOutputImage >
void
CPUImageToGPUImageFilter< TGPUOutputImage >::PrintSelf(std::ostream & os,
  itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);
  os << indent << "CPUImageToGPUImageFilter " << std::endl;
}

/**
 *
 */
template< class TGPUOutputImage >
void
CPUImageToGPUImageFilter< TGPUOutputImage >
::SetInput(const InputImageType *input)
{
  // Process object is not const-correct so the const_cast is required here
  this->ProcessObject::SetNthInput( 0,
                                    const_cast< InputImageType * >( input ) );
}

template< class TGPUOutputImage >
void
CPUImageToGPUImageFilter< TGPUOutputImage >::GenerateData()
{
  OutputImagePointer output =  dynamic_cast< OutputImageType * >( this->GetOutput() );
  InputImagePointer input = dynamic_cast< InputImageType * >( this->GetPrimaryInput() );

  //output = OutputImageType::New();
  output->CopyInformation( input );

  //output->SetLargestPossibleRegion( input->GetLargestPossibleRegion() );
  //output->SetRequestedRegion( input->GetRequestedRegion() );
  //output->SetBufferedRegion( input->GetBufferedRegion() );
  //output->Allocate();
  //output->OutputImageType::Superclass::Graft(input);

  output->SetBufferedRegion( output->GetRequestedRegion() );
  output->Allocate();
  itk::ImageAlgorithm::Copy(input.GetPointer(), output.GetPointer(), output->GetBufferedRegion(), output->GetBufferedRegion());
}

template< class TGPUOutputImage >
void
CPUImageToGPUImageFilter< TGPUOutputImage >::GraftOutput(itk::DataObject *output)
{
  OutputImagePointer otPtr = dynamic_cast< OutputImageType * >( this->GetOutput() );

  otPtr->Graft( output );
}

template< class TGPUOutputImage >
void
CPUImageToGPUImageFilter< TGPUOutputImage >::GraftOutput(const DataObjectIdentifierType & key, itk::DataObject *output)
{
  OutputImagePointer otPtr = dynamic_cast< OutputImageType * >( this->ProcessObject::GetOutput(key) );

  otPtr->Graft( output );
}

#endif
