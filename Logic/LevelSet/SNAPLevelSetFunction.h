/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPLevelSetFunction.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:14 $
  Version:   $Revision: 1.2 $
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
#ifndef __SNAPLevelSetFunction_h_
#define __SNAPLevelSetFunction_h_

#if ITK_VERSION_MAJOR >= 4
#define ITK_TYPENAME typename
#endif

//#include "itkLevelSetFunction.h"
#include "itkSegmentationLevelSetFunction.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkVectorLinearInterpolateImageFunction.h"
#include "itkVectorCastImageFilter.h"
// #include "itkGradientImageFilter.h"

#include "SNAPAdvectionFieldImageFilter.h"

/**
  \class SNAPLevelSetFunction
  \brief A level set function that implements the generic level set equation
  decribed in [Ho et al., 2003] and used by the SnAP Application.
 
  This class defines a level set equation that is similar to the Geodesic Active
  Contours implementation (see GeodesicActiveContoursLevelSetFunction).  However, 
  it includes an additional Laplacian smoothing (i.e., diffusion) term and it
  modulates each of the terms by a speed function, which is an integer power of
  the speed function passed in by the user.

  Unlike the SegmentationLevelSetFunction, this class requires that the user pass
  in a speed function \f$ g({\mathbb x}) \f$.  This is done because in the SnAP
  application the computation of the speed function is supervised by the user.
  
  The equation implemented by this function is of the form

  \f[

  \phi_t 
    = A g^a |\nabla \phi| 
    + B g^b \nabla g \cdot \nabla \phi
    + C g^c \kappa \nabla | \phi |
    + D g^d \nabla^2 \phi \ .

  \f

  The right-hand-side consists, in order, of the propagation term, the advection
  term, the curvature term and the Laplacian smoothing term.  Each of these terms
  has a constant weight associated with it.  Each term is scaled by a power of the
  speed function \f$ g({\mathbb x}) \f$.  Often the constants \f$ a, b, c, d \f$
  are equal to 0, in which case there is no modulation of the terms by the speed 
  function.  Modulation helps slow down the snake at edges in the image.

  Before passing an instance of this class to a FiniteDifferenceImageFilter, 
  set the weights and speed exponents for each of the four terms using the
  SetXXXWeight() and SetXXXSpeedExponent() methods. Then  pass in a speed image 
  using SetSpeedImage() and then compute the internal images by 
  calling CalculateInternalImages().  
 */
template <class TImageType>
class ITK_EXPORT SNAPLevelSetFunction
  : public itk::SegmentationLevelSetFunction<TImageType>
{
public:
  /** Standard class typedefs. */
  typedef SNAPLevelSetFunction Self;
  typedef itk::SegmentationLevelSetFunction<TImageType> Superclass;
  typedef itk::SmartPointer<Self> Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;
                                                                                
  /** Method for creation through the object factory. */
  itkNewMacro(Self);
                                                                                
  /** Run-time type information (and related methods) */
  itkTypeMacro( SNAPLevelSetFunction, itk::LevelSetFunction );
                                                                                
                                                                                
  /** Extract some parameters from the superclass. */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      Superclass::ImageDimension);

  /** Extract some parameters from the superclass. */
  typedef typename Superclass::ImageType ImageType;
  typedef typename ImageType::Pointer ImagePointer;
  typedef typename Superclass::NeighborhoodType NeighborhoodType;
  typedef typename Superclass::ScalarValueType ScalarValueType;
  typedef typename Superclass::RadiusType RadiusType;
  typedef typename Superclass::FloatOffsetType FloatOffsetType;
  
  typedef typename Superclass::VectorType VectorType;
  typedef itk::Image<VectorType, ImageDimension> VectorImageType;
  typedef typename VectorImageType::Pointer VectorImagePointer;

  typedef typename Superclass::TimeStepType TimeStepType;
  typedef typename Superclass::GlobalDataStruct GlobalDataStruct;

  /** Interpolators used to access the speed images */
  typedef itk::LinearInterpolateImageFunction<
    ImageType> ImageInterpolatorType;
  typedef itk::VectorLinearInterpolateImageFunction<
    VectorImageType,float> VectorInterpolatorType;

  typedef typename ImageType::IndexType IndexType;
  typedef typename ImageInterpolatorType::ContinuousIndexType 
    ContinuousIndexType;

  
  /** Set the speed image (a.k.a. function g()) on which the 
      computation of the variuous internal speed images is
      based.  The function g() should be near zero at edges
      of structures in the image and near one at flat regions */
  virtual void SetSpeedImage(ImageType *pointer);
      
  /** Get the speed image g() */
  virtual ImageType *GetSpeedImage()
    {
    return m_SpeedImage;
    }

  /** Set the external advection image (optional). This is only 
   * needed if your advection image is not the gradient of the
   * speed image. We use this for DTI segmentation */
  virtual void SetAdvectionField(VectorImageType *pointer)
    {
    m_UseExternalAdvectionField = true;
    m_AdvectionField = pointer;
    }

  /** Compute speed and advection images from feature image. */
  virtual void CalculateInternalImages();
                                                                                
  /** Local multiplier for the curvature term */
  virtual ScalarValueType CurvatureSpeed(
    const NeighborhoodType &neighbourhood,
    const FloatOffsetType &offset, 
    GlobalDataStruct * = 0 ) const;

  /** Local multiplier for the laplacian smoothing term */
  virtual ScalarValueType LaplacianSmoothingSpeed(
    const NeighborhoodType &neighbourhood,
    const FloatOffsetType &offset, 
    GlobalDataStruct * = 0 ) const;

  /** Local multiplier for the propagation term */
  virtual ScalarValueType PropagationSpeed(
    const NeighborhoodType &neighbourhood,
    const FloatOffsetType &offset, 
    GlobalDataStruct * = 0 ) const;

  /** Local multiplier for the advection term */
  virtual VectorType AdvectionField(
    const NeighborhoodType &neighbourhood,
    const FloatOffsetType &offset, 
    GlobalDataStruct * = 0 ) const;

  /** Set the exponent to which the speed image g() is taken
      when converted to the curvature speed */
  void SetCurvatureSpeedExponent(int exponent) 
    {
    m_CurvatureSpeedExponent = exponent;
    }
 
  /** Get the exponent to which the speed image g() is taken
      when converted to the curvature speed */
  int GetCurvatureSpeedExponent() const 
    {
    return m_CurvatureSpeedExponent;
    }
  
  /** Set the exponent to which the speed image g() is taken
      when computing the advection field */
  void SetAdvectionSpeedExponent(int exponent) 
    {
    m_AdvectionSpeedExponent = exponent;
    }
 
  /** Get the exponent to which the speed image g() is taken
      when computing the advection field */
  int GetAdvectionSpeedExponent() const 
    {
    return m_AdvectionSpeedExponent;
    }
  
  /** Set the exponent to which the speed image g() is taken
      when converted to the propagation speed */
  void SetPropagationSpeedExponent(int exponent) 
    {
    m_PropagationSpeedExponent = exponent;
    }
 
  /** Get the exponent to which the speed image g() is taken
      when converted to the propagation speed */
  int GetPropagationSpeedExponent() const 
    {
    return m_PropagationSpeedExponent;
    }
  
  /** Set the exponent to which the speed image g() is taken
      when converted to the laplacian smoothing speed */
  void SetLaplacianSmoothingSpeedExponent(int exponent) 
    {
    m_LaplacianSmoothingSpeedExponent = exponent;
    }
 
  /** Get the exponent to which the speed image g() is taken
      when converted to the laplacian smoothing speed */
  int GetLaplacianSmoothingSpeedExponent() const 
    {
    return m_LaplacianSmoothingSpeedExponent;
    }
  
  /** Set/Get the time step. For this filter the time-step is supplied 
      by the user and remains fixed for all updates. */
  void SetTimeStepFactor(const TimeStepType &t)
    { m_TimeStepFactor = t; }
  const TimeStepType &GetTimeStepFactor() const
    { return m_TimeStepFactor; }

  /** Returns the time step supplied by the user.  If the time step value
      passed on to this filter is equal to zero, this method will use the
      automatic time step calculation from the parent class.  If the value
      is non-zero, the fixed time step will be returned. */
  virtual TimeStepType ComputeGlobalTimeStep(void *GlobalData) const
    { 
    return m_TimeStepFactor == 0
      ? Superclass::ComputeGlobalTimeStep(GlobalData)
      : m_TimeStepFactor * Superclass::ComputeGlobalTimeStep(GlobalData); 
    }

protected:

  SNAPLevelSetFunction();
  ~SNAPLevelSetFunction();
  void PrintSelf(std::ostream &s, itk::Indent indent) const;

private:

  /** The exponent to which speed image g() is taken to compute the 
      curvature speed */
  int m_CurvatureSpeedExponent;
  
  /** The exponent to which speed image g() is taken to compute the 
      advection field */
  int m_AdvectionSpeedExponent;
  
  /** The exponent to which speed image g() is taken to compute the 
      propagation speed */
  int m_PropagationSpeedExponent;
  
  /** The exponent to which speed image g() is taken to compute the 
      Laplacian smoothing speed */
  int m_LaplacianSmoothingSpeedExponent;
  
  /** The speed image g() computed externally with user interaction */
  ImagePointer m_SpeedImage;

  /** The curvature image derived from the speed image g() */
  ImagePointer m_CurvatureSpeedImage;

  /** The propagation speed derived from the speed image g() */
  ImagePointer m_PropagationSpeedImage;

  /** The smoothing speed derived from the speed image g() */
  ImagePointer m_LaplacianSmoothingSpeedImage;

  /** The advection field (possibly scaled by speed image g() */
  VectorImagePointer m_AdvectionField;

  /** Flag, specifyting that the advection image is loaded externally */
  bool m_UseExternalAdvectionField;

  /** Gradient filter used to produce the advection field */
  typedef SNAPAdvectionFieldImageFilter<TImageType,float> AdvectionFilterType;
  typename AdvectionFilterType::Pointer m_AdvectionFilter;

  /** Instances of the interpolators */
  typename ImageInterpolatorType::Pointer m_PropagationSpeedInterpolator;
  typename ImageInterpolatorType::Pointer m_CurvatureSpeedInterpolator;
  typename ImageInterpolatorType::Pointer m_LaplacianSmoothingSpeedInterpolator;
  typename VectorInterpolatorType::Pointer m_AdvectionFieldInterpolator;

  /** The constant time step */
  TimeStepType m_TimeStepFactor;
  
  /** A trivial functor to square the g() image */
  class SquareFunctor
    {
    public:
      inline ScalarValueType operator()(ScalarValueType x) 
        { return x * x; }
    
      // Dummy equality operators, since there is no data here
      bool operator == (const SquareFunctor &) const { return true; }
      bool operator != (const SquareFunctor &) const { return false; }
    };
  
  /** A trivial functor to take the g() image to any power */
  class PowFunctor
    {
    public: 
      inline ScalarValueType operator()(ScalarValueType x) 
        { return vcl_pow(x,power); }

      // Dummy equality operators, since there is no data here
      bool operator == (const PowFunctor &z) const 
        { return power == z.power; }
      
      bool operator != (const PowFunctor &z) const 
        { return !(*this == z); }

      int power;
    };

  /** A casting functor to convert between vector types.  */
  itk::Functor::VectorCast< 
    ITK_TYPENAME VectorInterpolatorType::OutputType,
    VectorType > m_VectorCast;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "SNAPLevelSetFunction.txx"
#endif

#endif
