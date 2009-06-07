/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GreyImageWrapper.cxx,v $
  Language:  C++
  Date:      $Date: 2009/06/07 22:55:48 $
  Version:   $Revision: 1.6 $
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
#include "GreyImageWrapper.h"
#include "UnaryFunctorCache.h"
#include "ImageWrapper.txx"
#include "ScalarImageWrapper.txx"

#include "itkFunctionBase.h"
#include "itkUnaryFunctorImageFilter.h"

using namespace itk;

// Create an instance of ImageWrapper of appropriate type
template class ImageWrapper<GreyType>;
template class ScalarImageWrapper<GreyType>;

GreyImageWrapper
::GreyImageWrapper()
: ScalarImageWrapper<GreyType> ()
{
  // Initialize the intensity functor
  m_IntensityFunctor.m_Alpha = 255;

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

  // By default, reference range is not used
  m_FlagUseReferenceIntensityRange = false;
}

GreyImageWrapper
::~GreyImageWrapper()
{
}

void GreyImageWrapper
::SetReferenceIntensityRange(GreyType refMin, GreyType refMax)
{
  m_FlagUseReferenceIntensityRange = true;
  m_ReferenceIntensityMin = refMin;
  m_ReferenceIntensityMax = refMax;  
}

void GreyImageWrapper
::ClearReferenceIntensityRange()
{
  m_FlagUseReferenceIntensityRange = false;
}

void GreyImageWrapper
::SetIntensityMapFunction(
  IntensityMapType *curve) 
{
  // Store the curve pointer in the functor
  m_IntensityFunctor.m_IntensityMap = curve;

  // Get the range of the image
  GreyType iMin = GetImageMin();
  GreyType iMax = GetImageMax();

  // Set the input range of the functor
  if(m_FlagUseReferenceIntensityRange)
    {
    m_IntensityFunctor.SetInputRange(
      m_ReferenceIntensityMin,
      m_ReferenceIntensityMax);
    }
  else
    {
    m_IntensityFunctor.SetInputRange(iMin, iMax);
    }
    
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
GreyImageWrapper
::SetAlpha(unsigned char alpha)
{
  m_IntensityFunctor.m_Alpha = alpha;
}

void 
GreyImageWrapper::IntensityFunctor
::SetInputRange(GreyType intensityMin, GreyType intensityMax) 
{
  m_IntensityMin = intensityMin;
  m_IntensityFactor = 1.0f / (intensityMax-intensityMin);
}

GreyImageWrapper::DisplayPixelType
GreyImageWrapper::IntensityFunctor
::operator()(const GreyType &in) const 
{
  // Map the input value to range of 0 to 1
  float inZeroOne = (in - m_IntensityMin) * m_IntensityFactor;
  
  // Compute the mapping
  float outZeroOne = m_IntensityMap->Evaluate(inZeroOne);

  // Map the output to a RGBA pixel
  DisplayPixelType pixel;
  const unsigned char tmp = (unsigned char)(255.0f * outZeroOne);
  pixel[0] = tmp;
  pixel[1] = tmp;
  pixel[2] = tmp;
  pixel[3] = m_Alpha;

  return pixel;
}


