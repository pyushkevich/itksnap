/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: SNAPAdvectionFieldImageFilter.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:14 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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
    itk::Image<
      itk::CovariantVector<TOutputValueType,
        ::itk::GetImageDimension<TInputImage>::ImageDimension>,
      ::itk::GetImageDimension<TInputImage>::ImageDimension> >
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
