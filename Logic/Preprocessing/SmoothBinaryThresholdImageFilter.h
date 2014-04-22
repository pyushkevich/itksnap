/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SmoothBinaryThresholdImageFilter.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:15 $
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
#ifndef __SmoothBinaryThresholdImageFilter_h_
#define __SmoothBinaryThresholdImageFilter_h_

#include "itkUnaryFunctorImageFilter.h"
#include "ThresholdSettings.h"

/**
 * A functor used for the smooth threshold operation on images.  
 * Used in conjuction with itk::UnaryFunctorImageFilter.
 */
template<class TInput>
class SmoothBinaryThresholdFunctor 
{
public:
  typedef SmoothBinaryThresholdFunctor<TInput> Self;

  /** Initialize the function */
  void SetParameters(ThresholdSettings *settings, double imin, double imax);

  /** Apply the function to image intensity */
  inline short operator()(const TInput &x);

  /** Compare two functor objects */
  bool operator ==(const Self &z);
  bool operator !=(const Self &x);

private:
  // The lower threshold in intensity units
  float m_LowerThreshold;

  // The upper threshold in intensity units
  float m_UpperThreshold;

  // The scaling factor that incorporates the smoothness parameter and the
  // intensity range of the input image
  float m_ScalingFactor;

  // The multiplier applied to the left/right threshold.  This can be set to 
  // 0 and 1 depending on the other threshold
  float m_FactorLower;
  float m_FactorUpper;

  // The shift amount added to the sum of the left and right 
  // thresholded intensities to force them into [-1,1]
  float m_Shift;
};

/**
 * \class SmoothBinaryThresholdFunctor
 * \brief A filter used to perform binary thresholding to produce SNAP speed images.
 * 
 * This filter uses a sigmoid function as a smooth threshold
 */
template <typename TInputImage,typename TOutputImage>
class SmoothBinaryThresholdImageFilter: 
  public itk::UnaryFunctorImageFilter<TInputImage,TOutputImage,
    SmoothBinaryThresholdFunctor<typename TInputImage::PixelType> >
{
public:

  /** Pixel Type of the input image */
  typedef TInputImage                                    InputImageType;
  typedef typename InputImageType::PixelType             InputPixelType;

  /** Pixel Type of the output image */
  typedef TOutputImage                                  OutputImageType;
  typedef typename OutputImageType::PixelType           OutputPixelType;

  /** The functor type */
  typedef SmoothBinaryThresholdFunctor<InputPixelType>      FunctorType;

  /** Standard class typedefs. */
  typedef SmoothBinaryThresholdImageFilter                         Self;
  typedef itk::UnaryFunctorImageFilter<InputImageType,
                                       OutputImageType,
                                       FunctorType>          Superclass;
  typedef itk::SmartPointer<Self>                               Pointer;
  typedef itk::SmartPointer<const Self>                    ConstPointer;  


  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Image dimension. */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TInputImage::ImageDimension);

  /** Set the minimum value of the input image. Must be provided for the
    filter to work */
  itkSetMacro(InputImageMinimum, double)

  /** Set the maximum value of the input image. Must be provided for the
    filter to work */
  itkSetMacro(InputImageMaximum, double)

  /** Assign threshold settings */
  void SetParameters(ThresholdSettings *settings);

  /** Access the threshold settings */
  ThresholdSettings *GetParameters();
  
protected:

  SmoothBinaryThresholdImageFilter();
  virtual ~SmoothBinaryThresholdImageFilter() {}
  void PrintSelf(std::ostream& os, itk::Indent indent) const;

  void GenerateData();

  double m_InputImageMinimum, m_InputImageMaximum;
  
  SmartPtr<ThresholdSettings> m_Parameters;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "SmoothBinaryThresholdImageFilter.txx"
#endif

#endif // __SmoothBinaryThresholdImageFilter_h_
