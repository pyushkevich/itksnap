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

#include <map>

template<class TImageType>
SNAPLevelSetFunction<TImageType>
::SNAPLevelSetFunction()
: Superclass()
{
  m_TimeStepFactor = 1.0f;
  
  m_CurvatureSpeedExponent = 0;
  m_AdvectionSpeedExponent = 0;
  m_PropagationSpeedExponent = 0;
  m_LaplacianSmoothingSpeedExponent = 0;
  m_UseExternalAdvectionField = false;

  m_PropagationSpeedInterpolator = ImageInterpolatorType::New();
  m_CurvatureSpeedInterpolator = ImageInterpolatorType::New();
  m_LaplacianSmoothingSpeedInterpolator = ImageInterpolatorType::New();
  m_AdvectionFieldInterpolator = VectorInterpolatorType::New();
  
  m_AdvectionFilter = AdvectionFilterType::New();
}

template<class TImageType>
SNAPLevelSetFunction<TImageType>
::~SNAPLevelSetFunction()
{

}

template<class TImageType>
void 
SNAPLevelSetFunction<TImageType>
::SetSpeedImage(ImageType *pointer)
{
  m_SpeedImage = pointer;
  m_AdvectionFilter->SetInput(m_SpeedImage);
}
/*
template<class TImageType>
void
SNAPLevelSetFunction<TImageType>
::SetAdvectionField(VectorImageType *pointer)
{
  m_UseExternalAdvectionField = true;
  m_AdvectionField = pointer;
}
*/


template<class TImageType>
void
SNAPLevelSetFunction<TImageType>
::CalculateInternalImages()
{
  // Create a map of integers to image pointers.  This map will cache the 
  // different powers of g() that must be computed (hopefully none!)
  typedef std::map<int,ImagePointer> PowerMapType;
  PowerMapType powerMap;

  // Initialize the map for the default powers
  powerMap[0] = NULL;
  powerMap[1] = m_SpeedImage;

  // Create a list of the required powers
  int powers[4] = {
    this->GetPropagationSpeedExponent(),
    this->GetAdvectionSpeedExponent(),
    this->GetCurvatureSpeedExponent(),
    this->GetLaplacianSmoothingSpeedExponent()
  };

  // What equation are we solving?
  // std::cout << "Solving Equation :  = " << std::endl;
  // std::cout << "  P-W = " << this->GetPropagationWeight() << std::endl;
  // std::cout << "  P-E = " << this->GetPropagationSpeedExponent() << std::endl;
  // std::cout << "  C-W = " << this->GetCurvatureWeight() << std::endl;
  // std::cout << "  C-E = " << this->GetCurvatureSpeedExponent() << std::endl;
  // std::cout << "  A-W = " << this->GetAdvectionWeight() << std::endl;
  // std::cout << "  A-E = " << this->GetAdvectionSpeedExponent() << std::endl;
  // std::cout << "  L-W = " << this->GetLaplacianSmoothingWeight() << std::endl;
  // std::cout << "  L-E = " << this->GetLaplacianSmoothingSpeedExponent() << std::endl;

  // Create an image for each of these powers, if needed
  for(unsigned int iPower=0; iPower < 4; iPower++)
    {
    // For powers of 0 and 1, which are by far the most common, there is
    // nothing to compute
    if(powers[iPower] == 0 || powers[iPower] == 1) 
      {
      continue;
      }
    
    // For power 2, we square the speed image.  Since pow() is a dog, 
    // let's handle this case explicitly
    else if(powers[iPower] == 2)
      {
      // Create a filter that will square the image
      typedef itk::UnaryFunctorImageFilter<ImageType,ImageType,SquareFunctor>
        ExponentFilterType;

      // Run the filter
      typename ExponentFilterType::Pointer filter = 
        ExponentFilterType::New();
      filter->SetInput(m_SpeedImage);
      filter->Update();
      
      // Stick the filter's output into the map
      powerMap[2] = filter->GetOutput();
      }

    // For powers other than 3, let the user suffer through the pow()!
    else
      {
      // Create a filter that will square the image
      typedef itk::UnaryFunctorImageFilter<ImageType,ImageType,PowFunctor>
        ExponentFilterType;

      typename ExponentFilterType::Pointer filter = 
        ExponentFilterType::New();
      filter->SetInput(m_SpeedImage);
      
      // Create a functor with specified power
      PowFunctor functor;
      functor.power = powers[iPower];
      filter->SetFunctor(functor);

      // Run the filter
      filter->Update();
      
      // Stick the filter's output into the map
      powerMap[powers[iPower]] = filter->GetOutput();
      }
    } // For all powers

  // Now that we have the powers, we can assign the speed images
  m_PropagationSpeedImage = powerMap[m_PropagationSpeedExponent];
  m_CurvatureSpeedImage = powerMap[m_CurvatureSpeedExponent];
  m_LaplacianSmoothingSpeedImage = powerMap[m_LaplacianSmoothingSpeedExponent];
  
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

  // Set up the image interpolators to point to the generated images
  if(m_PropagationSpeedExponent != 0)
    m_PropagationSpeedInterpolator->SetInputImage(m_PropagationSpeedImage);
  if(m_CurvatureSpeedExponent != 0)
    m_CurvatureSpeedInterpolator->SetInputImage(m_CurvatureSpeedImage);
  if(m_LaplacianSmoothingSpeedExponent != 0)
    m_LaplacianSmoothingSpeedInterpolator->SetInputImage(
      m_LaplacianSmoothingSpeedImage);

  // Set up the advection interpolator
  // if(m_AdvectionSpeedExponent != 0)
  m_AdvectionFieldInterpolator->SetInputImage(m_AdvectionField);
}


template<class TImageType>
typename SNAPLevelSetFunction<TImageType>::ScalarValueType
SNAPLevelSetFunction<TImageType>
::CurvatureSpeed(const NeighborhoodType &neighborhood, 
                 const FloatOffsetType &offset,
                 GlobalDataStruct *) const 
{
  // If the exponent is zero, there is nothing to return
  if(m_CurvatureSpeedExponent == 0)
    return itk::NumericTraits<ScalarValueType>::One; 
  
  // Otherwise, perform interpolation on the image
  IndexType idx = neighborhood.GetIndex();
  ContinuousIndexType cdx;
  for (unsigned i = 0; i < ImageDimension; ++i)
    {
    cdx[i] = static_cast<double>(idx[i]) - offset[i];
    }
  if ( m_CurvatureSpeedInterpolator->IsInsideBuffer(cdx) )
    {
    return (static_cast<ScalarValueType>(
        m_CurvatureSpeedInterpolator->EvaluateAtContinuousIndex(cdx)));
    }
  else return ( static_cast<ScalarValueType>(m_CurvatureSpeedImage->GetPixel(idx)) );
}

#include "itkFastMutexLock.h"

template<class TImageType>
typename SNAPLevelSetFunction<TImageType>::ScalarValueType
SNAPLevelSetFunction<TImageType>
::PropagationSpeed(const NeighborhoodType &neighborhood, 
                   const FloatOffsetType &offset,
                   GlobalDataStruct *) const 
{
  // If the exponent is zero, there is nothing to return
  if(m_PropagationSpeedExponent == 0)
    return itk::NumericTraits<ScalarValueType>::One; 
  
  // Otherwise, perform interpolation on the image
  IndexType idx = neighborhood.GetIndex();
  ContinuousIndexType cdx;

  ScalarValueType v;
  for (unsigned i = 0; i < ImageDimension; ++i)
    {
    cdx[i] = static_cast<double>(idx[i]) - offset[i];
    }
  if ( m_PropagationSpeedInterpolator->IsInsideBuffer(cdx) )
    {
    v = (static_cast<ScalarValueType>(
        m_PropagationSpeedInterpolator->EvaluateAtContinuousIndex(cdx)));
    }
  else v = ( static_cast<ScalarValueType>(m_PropagationSpeedImage->GetPixel(idx)) );

  return v;
}

template<class TImageType>
typename SNAPLevelSetFunction<TImageType>::ScalarValueType
SNAPLevelSetFunction<TImageType>
::LaplacianSmoothingSpeed(const NeighborhoodType &neighborhood, 
                          const FloatOffsetType &offset,
                          GlobalDataStruct *) const 
{
  // If the exponent is zero, there is nothing to return
  if(m_LaplacianSmoothingSpeedExponent == 0)
    return itk::NumericTraits<ScalarValueType>::One; 
  
  // Otherwise, perform interpolation on the image
  IndexType idx = neighborhood.GetIndex();
  ContinuousIndexType cdx;
  for (unsigned i = 0; i < ImageDimension; ++i)
    {
    cdx[i] = static_cast<double>(idx[i]) - offset[i];
    }
  if ( m_LaplacianSmoothingSpeedInterpolator->IsInsideBuffer(cdx) )
    {
    return (static_cast<ScalarValueType>(
        m_LaplacianSmoothingSpeedInterpolator->EvaluateAtContinuousIndex(cdx)));
    }
  else return ( static_cast<ScalarValueType>(m_LaplacianSmoothingSpeedImage->GetPixel(idx)) );
}

template <class TImageType>
typename SNAPLevelSetFunction<TImageType>::VectorType
SNAPLevelSetFunction<TImageType>
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

template <class TImageType>
void
SNAPLevelSetFunction<TImageType>
::PrintSelf(std::ostream &os, itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

