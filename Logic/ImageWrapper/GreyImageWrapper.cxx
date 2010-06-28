/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GreyImageWrapper.cxx,v $
  Language:  C++
  Date:      $Date: 2010/06/28 18:45:08 $
  Version:   $Revision: 1.19 $
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


// Create an instance of ImageWrapper of appropriate type
template class ImageWrapper<GreyType>;
template class ScalarImageWrapper<GreyType>;

GreyImageWrapper
::GreyImageWrapper()
: ScalarImageWrapper<GreyType> ()
{
  // Initialize the intensity curve
  m_IntensityCurveVTK = IntensityCurveVTK::New();
  m_IntensityCurveVTK->Initialize();

  // Initialize the intensity functor
  m_IntensityFunctor.m_IntensityMap = m_IntensityCurveVTK;

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
  for (size_t i = 0; i < 3; ++i)
    {
    m_IntensityFilter[i] = NULL;
    }
  m_IntensityMapCache = NULL;
  m_IntensityCurveVTK = NULL;
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

IntensityCurveInterface*
GreyImageWrapper
::GetIntensityMapFunction()
{
  return m_IntensityCurveVTK;
}

void 
GreyImageWrapper
::CopyIntensityMap(const GreyImageWrapper &s)
{
  m_IntensityCurveVTK->Initialize(
    s.m_IntensityCurveVTK->GetControlPointCount());
  for(size_t i = 0; i < m_IntensityCurveVTK->GetControlPointCount(); i++)
    {
    float t, x;
    s.m_IntensityCurveVTK->GetControlPoint(i, t, x);
    m_IntensityCurveVTK->UpdateControlPoint(i, t, x);
    }
}

void GreyImageWrapper
::UpdateIntensityMapFunction()
{
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

  // Dirty the intensity filters
  for(unsigned int i=0;i<3;i++)
    m_IntensityFilter[i]->Modified();
}

GreyImageWrapper::DisplaySlicePointer
GreyImageWrapper
::GetDisplaySlice(unsigned int dim) const
{
  return m_IntensityFilter[dim]->GetOutput();
}

ColorMap
GreyImageWrapper
::GetColorMap() const
{
  return m_IntensityFunctor.m_Colormap;
}

void
GreyImageWrapper
::SetColorMap(const ColorMap& colormap)
{
  m_IntensityFunctor.m_Colormap = colormap;
}

void
GreyImageWrapper
::Update()
{
  // Dirty the intensity filters
  for(unsigned int i=0;i<3;i++)
    m_IntensityFilter[i]->Modified();
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
  double inZeroOne = (in - m_IntensityMin) * m_IntensityFactor;
  
  // Compute the intensity mapping
  double outZeroOne = m_IntensityMap->Evaluate(inZeroOne);

  // Map the output to a RGBA pixel
  return m_Colormap.MapIndexToRGBA(outZeroOne);
}


