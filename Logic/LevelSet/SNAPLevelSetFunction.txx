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
  for (unsigned i = 0; i < ImageDimension; ++i)
    {
    cdx[i] = static_cast<double>(idx[i]) - offset[i];
    }
  if ( m_AdvectionFieldInterpolator->IsInsideBuffer(cdx) )
    {
    return (
      m_VectorCast(
       m_AdvectionFieldInterpolator->EvaluateAtContinuousIndex(cdx)));
    }
  else return ( m_AdvectionField->GetPixel(idx) );
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
  // Create a mutex lock so that this whole section of code is executed
  // in a single thread. Otherwise our stored speed is not going to be
  // useful
  itk::SimpleFastMutexLock mx;
  mx.Lock();

  // Interpolate the speed value at this location. This way, we don't need to
  // perform interpolation each time the GetXXXSpeed() function is called.
  IndexType idx = neighborhood.GetIndex();
  ContinuousIndexType cdx;
  for (unsigned i = 0; i < ImageDimension; ++i)
    {
    cdx[i] = static_cast<double>(idx[i]) - offset[i];
    }
  if ( m_SpeedInterpolator->IsInsideBuffer(cdx) )
    {
    m_SpeedValue = static_cast<ScalarValueType>(
        m_SpeedInterpolator->EvaluateAtContinuousIndex(cdx));
    }
  else
    {
    m_SpeedValue = static_cast<ScalarValueType>(m_SpeedImage->GetPixel(idx));
    }

  // Scale the speed value
  m_SpeedValue *= m_SpeedScaleFactor;

  // Call the parent method
  PixelType rv = Superclass::ComputeUpdate(neighborhood, globalData, offset);

  // Remove the thread lock
  mx.Unlock();

  // Return the RV
  return rv;
}

template <class TSpeedImageType, class TImageType>
typename SNAPLevelSetFunction<TSpeedImageType, TImageType>::ScalarValueType
SNAPLevelSetFunction<TSpeedImageType, TImageType>
::GetSpeedWithExponent(int exponent) const
{
  switch(exponent)
    {
    case 0 : return itk::NumericTraits<ScalarValueType>::One;
    case 1 : return m_SpeedValue;
    case 2 : return m_SpeedValue * m_SpeedValue;
    case 3 : return m_SpeedValue * m_SpeedValue * m_SpeedValue;
    default : return pow(m_SpeedValue, exponent);
    }
}

