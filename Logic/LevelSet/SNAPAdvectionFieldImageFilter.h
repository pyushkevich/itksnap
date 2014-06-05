/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPAdvectionFieldImageFilter.h,v $
  Language:  C++
  Date:      $Date: 2009/01/23 20:09:38 $
  Version:   $Revision: 1.3 $
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
#ifndef __SNAPAdvectionFieldImageFilter_h_
#define __SNAPAdvectionFieldImageFilter_h_

#include "itkCovariantVector.h"
#include "itkImage.h"
#include "itkImageToImageFilter.h"

/**
 * \class SNAPAdvectionFieldImageFilter
 * \brief A filter used to compute the advection field in the SNAP level set
 * equation. 
 */
template <class TInputImage, class TOutputValueType=float>
class SNAPAdvectionFieldImageFilter: 
  public itk::ImageToImageFilter<
    TInputImage,
    itk::Image<itk::CovariantVector<TOutputValueType, TInputImage::ImageDimension>,
      TInputImage::ImageDimension> >
{
public:
  
  /** Input image types */
  typedef TInputImage                                          InputImageType;
  typedef itk::SmartPointer<InputImageType>                 InputImagePointer;
  
  /** Standard class typedefs. */
  typedef SNAPAdvectionFieldImageFilter                                  Self;
  typedef itk::SmartPointer<Self>                                     Pointer;
  typedef itk::SmartPointer<const Self>                          ConstPointer;  
  

  /** Image dimension. */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TInputImage::ImageDimension);    
  

  /** Output image types */
  typedef itk::CovariantVector<
    TOutputValueType, 
    itkGetStaticConstMacro(ImageDimension)>                   VectorType;  
  typedef itk::Image<
    VectorType, 
    itkGetStaticConstMacro(ImageDimension)>              OutputImageType;
  typedef itk::SmartPointer<OutputImageType>               OutputImagePointer;
  
  
  typedef itk::ImageToImageFilter<InputImageType,OutputImageType>  Superclass;
  
  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Set the power of g() by which the gradient is scaled */
  itkSetMacro(Exponent,unsigned int);

  /** Get the power of g() by which the gradient is scaled */
  itkGetMacro(Exponent,unsigned int);
    
protected:

  SNAPAdvectionFieldImageFilter();
  virtual ~SNAPAdvectionFieldImageFilter() {};
  void PrintSelf(std::ostream& os, itk::Indent indent) const;
  
  /** Generate Data */
  void GenerateData( void );

private:

  /** The g-scaling exponent */
  unsigned int m_Exponent;

};

#ifndef ITK_MANUAL_INSTANTIATION
#include "SNAPAdvectionFieldImageFilter.txx"
#endif

#endif // __SNAPAdvectionFieldImageFilter_h_
