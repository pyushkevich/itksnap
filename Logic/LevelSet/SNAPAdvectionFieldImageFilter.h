/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPAdvectionFieldImageFilter.h,v $
  Language:  C++
  Date:      $Date: 2009/01/23 20:09:38 $
  Version:   $Revision: 1.3 $
  Copyright (c) 2007 Paul A. Yushkevich
  
  This file is part of ITK-SNAP 

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
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
  void PrintSelf(std::ostream& os, itk::Indent indent) const ITK_OVERRIDE;
  
  /** Generate Data */
  void GenerateData( void ) ITK_OVERRIDE;

private:

  /** The g-scaling exponent */
  unsigned int m_Exponent;

};

#ifndef ITK_MANUAL_INSTANTIATION
#include "SNAPAdvectionFieldImageFilter.txx"
#endif

#endif // __SNAPAdvectionFieldImageFilter_h_
