/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SignedDistanceFilter.h,v $
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
#ifndef __SignedDistanceFilter_h_
#define __SignedDistanceFilter_h_

#include "itkCommand.h"
#include "itkProgressAccumulator.h"
#include "itkUnaryFunctorImageFilter.h"
#include "itkDanielssonDistanceMapImageFilter.h"
#include "itkSubtractImageFilter.h"

/**
 * \class SignedDistanceFilter
 * \brief This filter computes an inside/outside signed distance image 
 * given a binary image of the 'inside'
 */
template <typename TInputImage,typename TOutputImage>
class SignedDistanceFilter: 
  public itk::ImageToImageFilter<TInputImage,TOutputImage>
{
public:
  
  /** Standard class typedefs. */
  typedef SignedDistanceFilter                                     Self;
  typedef itk::ImageToImageFilter<TInputImage,TOutputImage>  Superclass;
  typedef itk::SmartPointer<Self>                               Pointer;
  typedef itk::SmartPointer<const Self>                    ConstPointer;  
  
  /** Pixel Type of the input image */
  typedef TInputImage                                    InputImageType;
  typedef typename InputImageType::PixelType             InputPixelType;
  typedef itk::SmartPointer<InputImageType>           InputImagePointer;

  /** Pixel Type of the output image */
  typedef TOutputImage                                  OutputImageType;
  typedef typename OutputImageType::PixelType           OutputPixelType;
  typedef itk::SmartPointer<OutputImageType>         OutputImagePointer;

  /** Type used for internal calculations */
  typedef float                                                RealType;
  typedef itk::Image<RealType,3>                      InternalImageType;
  typedef itk::SmartPointer<InternalImageType>     InternalImagePointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)
    
  /** Image dimension. */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TInputImage::ImageDimension);
protected:

  SignedDistanceFilter();
  virtual ~SignedDistanceFilter() {};
  void PrintSelf(std::ostream& os, itk::Indent indent) const;
  
  /** Generate Data */
  void GenerateData( void );

private:

  /** A functor used to invert the input image */
  class InvertFunctor {
  public:
    InputPixelType operator()(InputPixelType input) 
    { 
      return input == itk::NumericTraits<InputPixelType>::Zero ? 
        itk::NumericTraits<InputPixelType>::One : 
        itk::NumericTraits<InputPixelType>::Zero; 
    }  
  };

  // Types used in the internal pipeline
  typedef itk::UnaryFunctorImageFilter<InputImageType,
    InputImageType,InvertFunctor>                       InvertFilterType;
    
  typedef itk::DanielssonDistanceMapImageFilter<
    InputImageType,OutputImageType>                   DistanceFilterType;

  typedef itk::SubtractImageFilter<OutputImageType,
    OutputImageType,OutputImageType>                  SubtractFilterType;

  // Progress accumulator object
  typedef itk::ProgressAccumulator::Pointer           AccumulatorPointer;

  // Define the actual filters
  typename InvertFilterType::Pointer   m_InvertFilter;
  typename DistanceFilterType::Pointer m_InsideDistanceFilter;
  typename DistanceFilterType::Pointer m_OutsideDistanceFilter;  
  typename SubtractFilterType::Pointer m_SubtractFilter;

  /** Progress tracking object */
  AccumulatorPointer        m_ProgressAccumulator;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "SignedDistanceFilter.txx"
#endif

#endif // __SignedDistanceFilter_h_
