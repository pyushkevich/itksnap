/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: itkTustisonBSplineKernelFunction.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:12 $
  Version:   $Revision: 1.2 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  
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
#ifndef _itkTustisonBSplineKernelFunction_h
#define _itkTustisonBSplineKernelFunction_h

#include "itkKernelFunction.h"
#include "vnl/vnl_math.h"
#include "vnl/vnl_real_polynomial.h"
#include "vnl/vnl_matrix.h"

namespace itk
{

/** \class TustisonBSplineKernelFunction
 * \brief BSpline kernel used for density estimation and nonparameteric
 *  regression.
 *
 * This class enscapsulates BSpline kernel for
 * density estimation or nonparameteric regression.
 * See documentation for KernelFunction for more details.
 *
 * This class is templated over the spline order to cohere with
 * the previous incarnation of this class. One can change the
 * order during an instantiation's existence.  Note that 
 * other authors have defined the B-spline order as being the
 * degree of spline + 1.  In the ITK context (e.g. in this 
 * class), the spline order is equivalent to the degree of 
 * the spline.
 *
 * \sa KernelFunction
 *
 * \ingroup Functions
 */
template <unsigned int VSplineOrder = 3>
class ITK_EXPORT TustisonBSplineKernelFunction 
: public KernelFunction
{
public:
  /** Standard class typedefs. */
  typedef TustisonBSplineKernelFunction Self;
  typedef KernelFunction Superclass;
  typedef SmartPointer<Self>  Pointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self); 

  /** Run-time type information (and related methods). */
  itkTypeMacro( TustisonBSplineKernelFunction, KernelFunction ); 

  typedef double                       RealType;
  typedef vnl_vector<RealType>         VectorType;
  typedef vnl_real_polynomial          PolynomialType;  
  typedef vnl_matrix<RealType>         MatrixType;

  /** Get/Sets the Spline Order */
  void SetSplineOrder( unsigned int ); 
  itkGetMacro( SplineOrder, unsigned int );

  /** Evaluate the function. */
  inline RealType Evaluate( const RealType & u ) const
    {
    RealType absValue = vnl_math_abs( u );  
    int which;
    if ( this->m_SplineOrder % 2 == 0 )
      {
      which = static_cast<unsigned int>( absValue+0.5 );
      }        
    else
      {
      which = static_cast<unsigned int>( absValue );
      }
    if ( which < this->m_BSplineShapeFunctions.rows() )
      {
      return PolynomialType( m_BSplineShapeFunctions.get_row( which ) ).evaluate( absValue );
      }
    else
      {
      return NumericTraits<RealType>::Zero;
      }
    }

  /** Evaluate the derivative. */
  inline RealType EvaluateDerivative( const double & u ) const
    {
    RealType absValue = vnl_math_abs( u );  
    int which;
    if ( this->m_SplineOrder % 2 == 0 )
      {
      which = static_cast<unsigned int>( absValue+0.5 );
      }        
    else
      {
      which = static_cast<unsigned int>( absValue );
      }
    if ( which < this->m_BSplineShapeFunctions.rows() )
      {
      RealType der = PolynomialType( this->m_BSplineShapeFunctions.get_row( which ) ).devaluate( absValue );
      if ( u < NumericTraits<RealType>::Zero )
        {
        return -der;
	       }
      else
        {
	       return der;
	       }
      }
    else
      {
      return NumericTraits<RealType>::Zero;
      }
    }

  /**
   * For a specific order, return the (m_SplineOrder+1) pieces of
   * the single basis function centered at zero.
   */  
  MatrixType GetShapeFunctions();

  /**
   * For a specific order, generate and return the (m_SplineOrder+1) 
   * pieces of the different basis functions in the [0, 1] interval.
   */  
  MatrixType GetShapeFunctionsInZeroToOneInterval();

protected:
  TustisonBSplineKernelFunction();
  ~TustisonBSplineKernelFunction();
  void PrintSelf( std::ostream& os, Indent indent ) const;

private:
  TustisonBSplineKernelFunction( const Self& ); //purposely not implemented
  void operator=( const Self& ); //purposely not implemented
  
  /**
   * For a specific order, generate the (m_SplineOrder+1) pieces of
   * the single basis function centered at zero.
   */  
  void GenerateBSplineShapeFunctions( unsigned int );
  
  /**
   * Use the CoxDeBoor recursion relation to generate the piecewise
   * polynomials which compose the basis function.
   */  
  PolynomialType CoxDeBoor( unsigned short, VectorType, unsigned int, unsigned int );
  
  MatrixType    m_BSplineShapeFunctions;  
  unsigned int  m_SplineOrder;

};

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkTustisonBSplineKernelFunction.txx"
#endif

#endif
