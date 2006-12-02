/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: GreyImageWrapper.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:11 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "GreyImageWrapper.h"
#include "UnaryFunctorCache.h"
#include "ImageWrapper.txx"

#include "itkFunctionBase.h"
#include "itkUnaryFunctorImageFilter.h"

using namespace itk;

// Create an instance of ImageWrapper of appropriate type
template class ImageWrapper<GreyType>;

GreyImageWrapper
::GreyImageWrapper()
: ImageWrapper<GreyType> ()
{
  // Instantiate the cache
  m_IntensityMapCache = CacheType::New();

  // Set the target of the cache
  m_IntensityMapCache->SetInputFunctor(&m_IntensityFunctor);

  // Instantiate the filters
  for(unsigned int i=0;i<3;i++) 
  {
    m_IntensityFilter[i] = IntensityFilterType::New();
    m_IntensityFilter[i]->SetFunctor(m_IntensityMapCache->GetCachingFunctor());
    m_IntensityFilter[i]->SetInput(m_Slicer[i]->GetOutput());
  }
}

GreyImageWrapper
::~GreyImageWrapper()
{
}

void GreyImageWrapper
::SetIntensityMapFunction(IntensityMapType *curve) 
{
  // Store the curve pointer in the functor
  m_IntensityFunctor.m_IntensityMap = curve;

  // Get the range of the image
  GreyType iMin = GetImageMin();
  GreyType iMax = GetImageMax();

  // Set the input range of the functor
  m_IntensityFunctor.SetInputRange(iMin,iMax);
    
  // Set the active range of the cache
  m_IntensityMapCache->SetEvaluationRange(iMin,iMax);

  // Compute the cache
  m_IntensityMapCache->ComputeCache();

  // Dirty the intensity filters
  for(unsigned int i=0;i<3;i++)
    m_IntensityFilter[i]->Modified();  
}

GreyImageWrapper::DisplaySlicePointer
GreyImageWrapper
::GetDisplaySlice(unsigned int dim)
{
  return m_IntensityFilter[dim]->GetOutput();
}


void 
GreyImageWrapper::IntensityFunctor
::SetInputRange(GreyType intensityMin, GreyType intensityMax) 
{
  m_IntensityMin = intensityMin;
  m_IntensityFactor = 1.0f / (intensityMax-intensityMin);
}

unsigned char
GreyImageWrapper::IntensityFunctor
::operator()(const GreyType &in) const 
{
  // Map the input value to range of 0 to 1
  float inZeroOne = (in - m_IntensityMin) * m_IntensityFactor;
  
  // Compute the mapping
  float outZeroOne = m_IntensityMap->Evaluate(inZeroOne);

  // Map the output to a byte
  return (unsigned char)(255.0f * outZeroOne);
}


