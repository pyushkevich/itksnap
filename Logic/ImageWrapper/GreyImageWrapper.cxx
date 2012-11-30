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
#include "IRISSlicer.h"

#include "ScalarImageHistogram.h"
#include "ColorMap.h"
#include "IntensityCurveVTK.h"
#include "IntensityToColorLookupTableImageFilter.h"
#include "LookupTableIntensityMappingFilter.h"
#include <itkMinimumMaximumImageFilter.h>

template<class TPixel, class TBase>
GreyImageWrapper<TPixel, TBase>
::GreyImageWrapper()
{
  // Initialize the intensity curve
  m_IntensityCurveVTK = IntensityCurveVTK::New();
  m_IntensityCurveVTK->Initialize();

  // Initialize the colormap
  m_ColorMap = ColorMap::New();

  // Initialize the LUT filter
  m_LookupTableFilter = LookupTableFilterType::New();
  m_LookupTableFilter->SetInput(this->m_Image);
  m_LookupTableFilter->SetIntensityCurve(m_IntensityCurveVTK);
  m_LookupTableFilter->SetColorMap(m_ColorMap);
  m_LookupTableFilter->SetImageMinInput(this->m_MinMaxFilter->GetMinimumOutput());
  m_LookupTableFilter->SetImageMaxInput(this->m_MinMaxFilter->GetMaximumOutput());

  // Initialize the filters that apply the LUT
  for(unsigned int i=0; i<3; i++)
    {
    m_IntensityFilter[i] = IntensityFilterType::New();
    m_IntensityFilter[i]->SetInput(this->GetSlice(i));
    m_IntensityFilter[i]->SetLookupTable(m_LookupTableFilter->GetOutput());
    }
}

template<class TPixel, class TBase>
GreyImageWrapper<TPixel, TBase>
::~GreyImageWrapper()
{
}

template<class TPixel, class TBase>
void GreyImageWrapper<TPixel, TBase>
::SetReferenceIntensityRange(double refMin, double refMax)
{
  m_LookupTableFilter->SetReferenceIntensityRange(refMin, refMax);
}

template<class TPixel, class TBase>
void GreyImageWrapper<TPixel, TBase>
::ClearReferenceIntensityRange()
{
  m_LookupTableFilter->RemoveReferenceIntensityRange();
}

template<class TPixel, class TBase>
IntensityCurveInterface*
GreyImageWrapper<TPixel, TBase>
::GetIntensityMapFunction() const
{
  return m_IntensityCurveVTK;
}

template<class TPixel, class TBase>
void
GreyImageWrapper<TPixel, TBase>
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

/*
  TODO: kill this!
template<class TPixel, class TBase>
void
GreyImageWrapper<TPixel, TBase>
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
*/

template<class TPixel, class TBase>
typename GreyImageWrapper<TPixel, TBase>::DisplaySlicePointer
GreyImageWrapper<TPixel, TBase>
::GetDisplaySlice(unsigned int dim)
{
  std::cout << m_IntensityFilter[dim]->GetOutput()->GetBufferedRegion() << std::endl;
  return m_IntensityFilter[dim]->GetOutput();
}

template<class TPixel, class TBase>
ColorMap *
GreyImageWrapper<TPixel, TBase>
::GetColorMap() const
{
  return m_ColorMap;
}

template <class TPixel, class TBase>
void
GreyImageWrapper<TPixel, TBase>
::UpdateImagePointer(ImageType *image)
{
  Superclass::UpdateImagePointer(image);
  m_LookupTableFilter->SetInput(image);
}

template <class TPixel, class TBase>
void
GreyImageWrapper<TPixel, TBase>
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
  for(int i = (int) hist->GetSize() - 1; i >= 0; i--)
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


template class GreyImageWrapper<short>;


