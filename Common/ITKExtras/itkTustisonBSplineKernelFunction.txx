/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: itkTustisonBSplineKernelFunction.txx,v $
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
#ifndef _itkTustisonBSplineKernelFunction_txx
#define _itkTustisonBSplineKernelFunction_txx

#include "itkTustisonBSplineKernelFunction.h"

namespace itk
{

template <unsigned int VSplineOrder>
TustisonBSplineKernelFunction<VSplineOrder>
::TustisonBSplineKernelFunction() 
{ 
  m_SplineOrder = VSplineOrder;
  this->GenerateBSplineShapeFunctions( m_SplineOrder+1 );
}

template <unsigned int VSplineOrder>
TustisonBSplineKernelFunction<VSplineOrder>
::~TustisonBSplineKernelFunction() 
{ 
}

template <unsigned int VSplineOrder>
void
TustisonBSplineKernelFunction<VSplineOrder>
::SetSplineOrder( unsigned int order )
{
  if ( order != this->m_SplineOrder )
    {
    m_SplineOrder = order;
    this->GenerateBSplineShapeFunctions( this->m_SplineOrder+1 );
    this->Modified();
    } 	
}

template <unsigned int VSplineOrder>
void
TustisonBSplineKernelFunction<VSplineOrder>
::GenerateBSplineShapeFunctions( unsigned int order )
{
  unsigned int NumberOfPieces = static_cast<unsigned int>( 0.5*( order+1 ) );
  this->m_BSplineShapeFunctions.set_size( NumberOfPieces, order );

  VectorType knots( order+1 );
  for ( unsigned int i = 0; i < knots.size(); i++)
    {
    knots[i] = -0.5*static_cast<RealType>( order ) + static_cast<RealType>( i );
    }				

  for ( unsigned int i = 0; i < NumberOfPieces; i++ )
    {
    PolynomialType poly = this->CoxDeBoor(order, knots, 
             0, static_cast<unsigned int>( 0.5*( order ) ) + i );
    this->m_BSplineShapeFunctions.set_row( i, poly.coefficients() );
    }   
}

template <unsigned int VSplineOrder>
typename TustisonBSplineKernelFunction<VSplineOrder>::PolynomialType 
TustisonBSplineKernelFunction<VSplineOrder>
::CoxDeBoor( unsigned short order, VectorType knots, 
             unsigned int whichBasisFunction, unsigned int whichPiece )
{
  VectorType tmp(2);
  PolynomialType poly1(0.0), poly2(0.0);
  RealType den;
  unsigned short p = order-1;
  unsigned short i = whichBasisFunction;   

  if ( p == 0 && whichBasisFunction == whichPiece )
    {
    PolynomialType poly(1.0);
    return poly;
    }          

  // Term 1
  den = knots(i+p)-knots(i);
  if ( den == NumericTraits<RealType>::Zero )  
    {
    PolynomialType poly(0.0);
    poly1 = poly;
    }
  else
    {
    tmp(0) = 1.0;
    tmp(1) = -knots(i);
    tmp /= den;
    poly1 = PolynomialType(tmp) * this->CoxDeBoor( order-1, knots, i, whichPiece );   
    }

  // Term 2
  den = knots(i+p+1)-knots(i+1);
  if ( den == NumericTraits<RealType>::Zero )  
    {
    PolynomialType poly(0.0);
    poly2 = poly;
    }
  else
    {
    tmp(0) = -1.0;
    tmp(1) = knots(i+p+1);
    tmp /= den;
    poly2 = PolynomialType(tmp) * this->CoxDeBoor( order-1, knots, i+1, whichPiece );
    }    
  return ( poly1 + poly2 );
}

template <unsigned int VSplineOrder>
typename TustisonBSplineKernelFunction<VSplineOrder>::MatrixType 
TustisonBSplineKernelFunction<VSplineOrder>
::GetShapeFunctionsInZeroToOneInterval()
{
  int order = m_SplineOrder+1;
  unsigned int NumberOfPieces = static_cast<unsigned int>( order );
  MatrixType ShapeFunctions( NumberOfPieces, order );

  VectorType knots(2*order);
  for ( unsigned int i = 0; i < knots.size(); i++ )
  {
    knots[i] = -static_cast<RealType>( this->m_SplineOrder ) + static_cast<RealType>( i );
  }			

  for ( int i = 0; i < NumberOfPieces; i++ )
  {
    PolynomialType poly = this->CoxDeBoor( order, knots, i, order-1 );
    ShapeFunctions.set_row( i, poly.coefficients() );	
  }   
  return ShapeFunctions;
}

template <unsigned int VSplineOrder>
void
TustisonBSplineKernelFunction<VSplineOrder>
::PrintSelf( std::ostream& os, Indent indent ) const
{ 
  Superclass::PrintSelf( os, indent ); 
  os << indent  << "Spline Order: " << m_SplineOrder << std::endl;
  os << indent  << "Piecewise Polynomial Pieces: " << std::endl;  
  RealType a, b;
  for ( unsigned int i = 0; i < m_BSplineShapeFunctions.rows(); i++ )
    {
    os << indent << indent;
    PolynomialType( this->m_BSplineShapeFunctions.get_row( i ) ).print( os );
    if (i == 0)
      {
      a = 0.0;
      if ( this->m_SplineOrder % 2 == 0 )
        {
       	b = 0.5;
	       }
      else
        {
								b = 1.0;
								}
      }
    else
      {
      a = b;
      b += 1.0;
      }  
    os << ",  X \\in [" << a << ", " << b << "]" << std::endl;
    }  
}  


} // namespace itk

#endif
