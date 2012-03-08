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
#include "IRISSlicer.h"

#include "itkFunctionBase.h"
#include "itkUnaryFunctorImageFilter.h"

#include "ScalarImageHistogram.h"

template<class TPixel>
GreyImageWrapper<TPixel>
::GreyImageWrapper()
: ScalarImageWrapper<TPixel> ()
{
  // Initialize the intensity curve
  m_IntensityCurveVTK = IntensityCurveVTK::New();
  m_IntensityCurveVTK->Initialize();

  // Initialize the colormap
  m_ColorMap = ColorMap::New();

  // Initialize the intensity functor
  m_IntensityFunctor.m_Parent = this;

  // Instantiate the cache
  m_IntensityMapCache = CacheType::New();

  // Set the target of the cache
  m_IntensityMapCache->SetInputFunctor(&m_IntensityFunctor);

  // Instantiate the filters
  for(unsigned int i=0;i<3;i++) 
  {
    m_IntensityFilter[i] = IntensityFilterType::New();
    m_IntensityFilter[i]->SetFunctor(m_IntensityMapCache->GetCachingFunctor());
    m_IntensityFilter[i]->SetInput(this->m_Slicer[i]->GetOutput());
  }

  // By default, reference range is not used
  m_FlagUseReferenceIntensityRange = false;
}

template<class TPixel>
GreyImageWrapper<TPixel>
::~GreyImageWrapper()
{
  for (size_t i = 0; i < 3; ++i)
    {
    m_IntensityFilter[i] = NULL;
    }
  m_IntensityMapCache = NULL;
  m_IntensityCurveVTK = NULL;
}

template<class TPixel>
void GreyImageWrapper<TPixel>
::SetReferenceIntensityRange(double refMin, double refMax)
{
  m_FlagUseReferenceIntensityRange = true;
  m_ReferenceIntensityMin = refMin;
  m_ReferenceIntensityMax = refMax;  
}

template<class TPixel>
void GreyImageWrapper<TPixel>
::ClearReferenceIntensityRange()
{
  m_FlagUseReferenceIntensityRange = false;
}

template<class TPixel>
IntensityCurveInterface*
GreyImageWrapper<TPixel>
::GetIntensityMapFunction() const
{
  return m_IntensityCurveVTK;
}

template<class TPixel>
void
GreyImageWrapper<TPixel>
::CopyIntensityMap(const GreyImageWrapperBase &s)
{
  const IntensityCurveInterface *ici = s.GetIntensityMapFunction();
  m_IntensityCurveVTK->Initialize(ici->GetControlPointCount());
  for(size_t i = 0; i < m_IntensityCurveVTK->GetControlPointCount(); i++)
    {
    float t, x;
    ici->GetControlPoint(i, t, x);
    m_IntensityCurveVTK->UpdateControlPoint(i, t, x);
    }
}

template<class TPixel>
void
GreyImageWrapper<TPixel>
::UpdateIntensityMapFunction()
{
  // Get the range of the image
  TPixel iMin = this->GetImageMin();
  TPixel iMax = this->GetImageMax();

  // Set the input range of the functor
  if(m_FlagUseReferenceIntensityRange)
    {
    m_IntensityFunctor.SetInputRange(
      (TPixel) m_ReferenceIntensityMin,
      (TPixel) m_ReferenceIntensityMax);
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

template<class TPixel>
typename GreyImageWrapper<TPixel>::DisplaySlicePointer
GreyImageWrapper<TPixel>
::GetDisplaySlice(unsigned int dim)
{
  unsigned long t_filter = m_IntensityFilter[dim]->GetMTime();
  unsigned long t_curve = m_IntensityCurveVTK->GetMTime();
  unsigned long t_colormap = m_ColorMap->GetMTime();

  if(t_curve >= t_filter || t_colormap >= t_filter)
    UpdateIntensityMapFunction();

  return m_IntensityFilter[dim]->GetOutput();
}

template<class TPixel>
ColorMap *
GreyImageWrapper<TPixel>
::GetColorMap() const
{
  return m_ColorMap;
}


template<class TPixel>
void
GreyImageWrapper<TPixel>
::Update()
{
  // Dirty the intensity filters
  for(unsigned int i=0;i<3;i++)
    m_IntensityFilter[i]->Modified();
}

template<class TPixel>
void
GreyImageWrapper<TPixel>::IntensityFunctor
::SetInputRange(TPixel intensityMin, TPixel intensityMax)
{
  m_IntensityMin = intensityMin;
  m_IntensityFactor = 1.0f / (intensityMax-intensityMin);
}

template <class TPixel>
void
GreyImageWrapper<TPixel>
::AutoFitContrast()
{
  // Get the histogram
  const ScalarImageHistogram *hist = this->GetHistogram();

  // Integrate the histogram until reaching 0.1%
  GreyType imin = hist->GetBinMin(0);
  GreyType ilow = imin;
  size_t accum = 0;
  size_t accum_goal = this->GetNumberOfVoxels() / 1000;
  for(size_t i = 0; i < hist->GetSize(); i++)
    {
    if(accum + hist->GetFrequency(i) < accum_goal)
      {
      accum += hist->GetFrequency(i);
      ilow = hist->GetBinMax(i);
      }
    else break;
    }

  // Same, but from above
  GreyType imax = hist->GetBinMax(hist->GetSize() - 1);
  GreyType ihigh = imax;
  accum = 0;
  for(size_t i = hist->GetSize() - 1; i >= 0; i--)
    {
    if(accum + hist->GetFrequency(i) < accum_goal)
      {
      accum += hist->GetFrequency(i);
      ihigh = hist->GetBinMin(i);
      }
    else break;
    }

  // If for some reason the window is off, we set everything to max/min
  if(ilow >= ihigh)
    { ilow = imin; ihigh = imax; }

  // Compute the unit coordinate values that correspond to min and max
  double iAbsMax = this->GetImageMaxAsDouble();
  double iAbsMin = this->GetImageMinAsDouble();
  double factor = 1.0 / (iAbsMax - iAbsMin);
  double t0 = factor * (ilow - iAbsMin);
  double t1 = factor * (ihigh - iAbsMin);

  // Set the window and level
  m_IntensityCurveVTK->ScaleControlPointsToWindow(
        (float) t0, (float) t1);
}



template<class TPixel>
typename GreyImageWrapper<TPixel>::DisplayPixelType
GreyImageWrapper<TPixel>::IntensityFunctor
::operator()(const TPixel &in) const
{
  // Map the input value to range of 0 to 1
  double inZeroOne = (in - m_IntensityMin) * m_IntensityFactor;
  
  // Compute the intensity mapping
  double outZeroOne = m_Parent->m_IntensityCurveVTK->Evaluate(inZeroOne);

  // Map the output to a RGBA pixel
  return m_Parent->m_ColorMap->MapIndexToRGBA(outZeroOne);
}


template class GreyImageWrapper<short>;
