/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GreyImageWrapper.cxx,v $
  Language:  C++
  Date:      $Date: 2009/06/09 05:35:12 $
  Version:   $Revision: 1.9 $
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
  // Initialize the intensity curve
  m_IntensityCurveVTK = IntensityCurveVTK::New();
  m_IntensityCurveVTK->Initialize(3);

  // Initialize the intensity functor
  m_IntensityFunctor.m_Alpha = 255;
  m_IntensityFunctor.m_Colormap = COLORMAP_GREY;
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
GreyImageWrapper
::SetColorMap(ColorMapType colormap)
{
  m_IntensityFunctor.m_Colormap = colormap;
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
  float tmp;
  switch (m_Colormap)
    {
    default:
    case COLORMAP_GREY:
	    tmp = 255.0*outZeroOne;
         pixel[0] = (unsigned char)(tmp);
         pixel[1] = pixel[0];
         pixel[2] = pixel[0];
	    break;
    case COLORMAP_RED:
         pixel[0] = (unsigned char)(255.0*outZeroOne);
         pixel[1] = 0;
         pixel[2] = 0;
	    break;
    case COLORMAP_GREEN:
         pixel[0] = 0;
         pixel[1] = (unsigned char)(255.0*outZeroOne);
         pixel[2] = 0;
	    break;
    case COLORMAP_BLUE:
         pixel[0] = 0;
         pixel[1] = 0;
         pixel[2] = (unsigned char)(255.0*outZeroOne);
	    break;
    case COLORMAP_HOT:
	    tmp = 63.0/26.0*outZeroOne - 1.0/13.0;
	    tmp = (tmp < 1.0) ? tmp : 1.0;
	    tmp = (tmp > 0.0) ? tmp : 0.0;
         pixel[0] = (unsigned char)(255.0*tmp);
	    tmp = 63.0/26.0*outZeroOne - 11.0/13.0;
	    tmp = (tmp < 1.0) ? tmp : 1.0;
	    tmp = (tmp > 0.0) ? tmp : 0.0;
         pixel[1] = (unsigned char)(255.0*tmp);
	    tmp = 4.5*outZeroOne - 3.5;
	    tmp = (tmp < 1.0) ? tmp : 1.0;
	    tmp = (tmp > 0.0) ? tmp : 0.0;
         pixel[2] = (unsigned char)(255.0*tmp);
	    break;
    case COLORMAP_COOL:
         pixel[0] = (unsigned char)(255.0*outZeroOne);
         pixel[1] = 255 - pixel[0];
         pixel[2] = 255;
	    break;
    case COLORMAP_SPRING:
         pixel[0] = 255;
         pixel[1] = (unsigned char)(255.0*outZeroOne);
         pixel[2] = 255 - pixel[1];
	    break;
    case COLORMAP_SUMMER:
         pixel[0] = (unsigned char)(255.0*outZeroOne);
         pixel[1] = (unsigned char)(127.5*outZeroOne + 127.5);
         pixel[2] = 102;
	    break;
    case COLORMAP_AUTUMN:
         pixel[0] = 255;
         pixel[1] = (unsigned char)(255.0*outZeroOne);
         pixel[2] = 0;
	    break;
    case COLORMAP_WINTER:
         pixel[0] = 0;
         pixel[1] = (unsigned char)(255.0*outZeroOne);
         pixel[2] = (unsigned char)(255.0 - 127.5*outZeroOne);
	    break;
    case COLORMAP_COPPER:
	    tmp = 306.0*outZeroOne;
         pixel[0] = (unsigned char)((255.0 < tmp) ? 255.0 : tmp);
         pixel[1] = (unsigned char)(204.0*outZeroOne);
         pixel[2] = (unsigned char)(127.5*outZeroOne);
	    break;
    case COLORMAP_HSV:
	    tmp = fabs(5.0*outZeroOne - 2.5) - 5.0/6.0;
	    tmp = (tmp < 1.0) ? tmp : 1.0;
	    tmp = (tmp > 0.0) ? tmp : 0.0;
         pixel[0] = (unsigned char)(255.0*tmp);
	    tmp = 11.0/6.0 - fabs(5.0*outZeroOne - 11.0/6.0);
	    tmp = (tmp < 1.0) ? tmp : 1.0;
	    tmp = (tmp > 0.0) ? tmp : 0.0;
         pixel[1] = (unsigned char)(255.0*tmp);
	    tmp = 11.0/6.0 - fabs(5.0*outZeroOne - 19.0/6.0);
	    tmp = (tmp < 1.0) ? tmp : 1.0;
	    tmp = (tmp > 0.0) ? tmp : 0.0;
         pixel[2] = (unsigned char)(255.0*tmp);
	    break;
    case COLORMAP_JET:
	    tmp = 1.625 - fabs(3.75*outZeroOne - 2.8125);
	    tmp = (tmp < 1.0) ? tmp : 1.0;
	    tmp = (tmp > 0.0) ? tmp : 0.0;
         pixel[0] = (unsigned char)(255.0*tmp);
	    tmp = 1.625 - fabs(3.75*outZeroOne - 1.875);
	    tmp = (tmp < 1.0) ? tmp : 1.0;
	    tmp = (tmp > 0.0) ? tmp : 0.0;
         pixel[1] = (unsigned char)(255.0*tmp);
	    tmp = 1.625 - fabs(3.75*outZeroOne - 0.9375);
	    tmp = (tmp < 1.0) ? tmp : 1.0;
	    tmp = (tmp > 0.0) ? tmp : 0.0;
         pixel[2] = (unsigned char)(255.0*tmp);
	    break;
    case COLORMAP_OVERUNDER:
	    if (outZeroOne == 0.0)
		 {
           pixel[0] = 0;
           pixel[1] = 0;
           pixel[2] = 255;
		 }
         else if (outZeroOne == 1.0)
		 {
           pixel[0] = 255;
           pixel[1] = 0;
           pixel[2] = 0;
		 }
         else
		 {
           tmp = (unsigned char)(255.0*outZeroOne);
           pixel[0] = tmp;
           pixel[1] = tmp;
           pixel[2] = tmp;
		 }
	    break;
    }
  pixel[3] = m_Alpha;

  return pixel;
}


