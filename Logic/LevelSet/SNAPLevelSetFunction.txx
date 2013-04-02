/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPLevelSetFunction.txx,v $
  Language:  C++
  Date:      $Date: 2009/09/19 14:00:16 $
  Version:   $Revision: 1.5 $
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
#include "itkGradientImageFilter.h"
#include "itkUnaryFunctorImageFilter.h"
#include "itkMultiplyImageFilter.h"
#include "itkImageRegionIterator.h"
#include "itkNumericTraits.h"
#include "itkSimpleFastMutexLock.h"
#include "itkMutexLockHolder.h"

#include <map>

template <class TSpeedImageType, class TImageType>
SNAPLevelSetFunction<TSpeedImageType,TImageType>
::SNAPLevelSetFunction()
: Superclass()
{
  m_TimeStepFactor = 1.0f;
  
  m_CurvatureSpeedExponent = 0;
  m_AdvectionSpeedExponent = 0;
  m_PropagationSpeedExponent = 0;
  m_LaplacianSmoothingSpeedExponent = 0;
  m_UseExternalAdvectionField = false;
  m_SpeedScaleFactor = 1.0;

  m_SpeedInterpolator = SpeedImageInterpolatorType::New();
  m_AdvectionFieldInterpolator = VectorInterpolatorType::New();
  
  m_AdvectionFilter = AdvectionFilterType::New();
}

template <class TSpeedImageType, class TImageType>
SNAPLevelSetFunction<TSpeedImageType,TImageType>
::~SNAPLevelSetFunction()
{

}

template <class TSpeedImageType, class TImageType>
void 
SNAPLevelSetFunction<TSpeedImageType,TImageType>
::SetSpeedImage(SpeedImageType *pointer)
{
  m_SpeedImage = pointer;
  m_SpeedInterpolator->SetInputImage(m_SpeedImage);
  m_AdvectionFilter->SetInput(m_SpeedImage);
}

template <class TSpeedImageType, class TImageType>
void
SNAPLevelSetFunction<TSpeedImageType,TImageType>
::CalculateInternalImages()
{
  
  // There is still the business of the advection image to attend to
  // Compute \f$ \nabla g() \f$ (will be cached from run to run)
  if(!m_UseExternalAdvectionField)
    {
    assert(m_AdvectionSpeedExponent >= 0);
    m_AdvectionFilter->SetExponent((unsigned int)m_AdvectionSpeedExponent);
    m_AdvectionFilter->Update();
    m_AdvectionField = 
      reinterpret_cast< VectorImageType* > (
        m_AdvectionFilter->GetOutput());
    }

  // Set up the advection interpolator
  // if(m_AdvectionSpeedExponent != 0)
  m_AdvectionFieldInterpolator->SetInputImage(m_AdvectionField);
}

template <class TSpeedImageType, class TImageType>
typename SNAPLevelSetFunction<TSpeedImageType,TImageType>::VectorType
SNAPLevelSetFunction<TSpeedImageType,TImageType>
::AdvectionField(const NeighborhoodType &neighborhood,
                 const FloatOffsetType &offset,
                 GlobalDataStruct *) const
{
  IndexType idx = neighborhood.GetIndex();
  typedef typename VectorInterpolatorType::ContinuousIndexType VectorContinuousIndexType;
  VectorContinuousIndexType cdx;
  typename SNAPLevelSetFunction<TSpeedImageType,TImageType>::VectorType avec;
  for (unsigned i = 0; i < ImageDimension; ++i)
    {
    cdx[i] = static_cast<double>(idx[i]) - offset[i];
    }
  if ( m_AdvectionFieldInterpolator->IsInsideBuffer(cdx) )
    {
    avec = m_VectorCast(m_AdvectionFieldInterpolator->EvaluateAtContinuousIndex(cdx));
    }
  else
    {
    avec = m_AdvectionField->GetPixel(idx);
    }

  for(unsigned i = 0; i < ImageDimension; i++)
    avec[i] *= m_SpeedScaleFactor;

  return avec;
}

template <class TSpeedImageType, class TImageType>
void
SNAPLevelSetFunction<TSpeedImageType, TImageType>
::PrintSelf(std::ostream &os, itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

template <class TSpeedImageType, class TImageType>
typename SNAPLevelSetFunction<TSpeedImageType, TImageType>::PixelType
SNAPLevelSetFunction<TSpeedImageType, TImageType>
::ComputeUpdate(
    const NeighborhoodType &neighborhood,
    void *globalData, const FloatOffsetType &offset)
{

  // Interpolate the speed value at this location. This way, we don't need to
  // perform interpolation each time the GetXXXSpeed() function is called.
  IndexType idx = neighborhood.GetIndex();
  ContinuousIndexType cdx;
  for (unsigned i = 0; i < ImageDimension; ++i)
    cdx[i] = static_cast<double>(idx[i]) - offset[i];

  // Store the speed in thread-specific memory
  m_CachedSpeed =
      m_SpeedScaleFactor * static_cast<ScalarValueType>(
        m_SpeedInterpolator->IsInsideBuffer(cdx)
        ? m_SpeedInterpolator->EvaluateAtContinuousIndex(cdx)
        : m_SpeedImage->GetPixel(idx));

  // Call the parent method
  return Superclass::ComputeUpdate(neighborhood, globalData, offset);
}

template <class TSpeedImageType, class TImageType>
typename SNAPLevelSetFunction<TSpeedImageType, TImageType>::ScalarValueType
SNAPLevelSetFunction<TSpeedImageType, TImageType>
::GetSpeedWithExponent(int exponent,
                       const NeighborhoodType &neighbourhood,
                       const FloatOffsetType &offset,
                       GlobalDataStruct *) const
{
  // Get the speed value from thread-specific cache
  ScalarValueType speed = m_CachedSpeed;

  switch(exponent)
    {
    case 0 : return itk::NumericTraits<ScalarValueType>::One;
    case 1 : return speed;
    case 2 : return speed * speed;
    case 3 : return speed * speed * speed;
    default : return pow(speed, exponent);
    }
}

