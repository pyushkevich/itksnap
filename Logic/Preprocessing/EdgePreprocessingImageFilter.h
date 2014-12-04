/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: EdgePreprocessingImageFilter.h,v $
  Language:  C++
  Date:      $Date: 2009/01/24 01:50:21 $
  Version:   $Revision: 1.4 $
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
#ifndef __EdgePreprocessingFilter_h_
#define __EdgePreprocessingFilter_h_

#include "itkCommand.h"
#include "itkImageToImageFilter.h"
#include "EdgePreprocessingSettings.h"

#include "GPUSettings.h"
#ifdef SNAP_USE_GPU
#include "CPUImageToGPUImageFilter.h"
#include <itkGPUDiscreteGaussianImageFilter.h>
#endif

namespace itk {
  template <class TIn, class TOut> class DiscreteGaussianImageFilter;
  template <class TIn, class TOut> class GradientMagnitudeImageFilter;
  template <class TIn, class TOut, class Fun> class UnaryFunctorImageFilter;
  template <class TIn, class TOut> class StreamingImageFilter;
  template <class TIn, class TOut> class CastImageFilter;
}


/**
 * The g() function that remaps the gradient magnitude image to the 
 * range 0 to 1.  
 */
template<class TInput>
class EdgeRemappingFunctor
  {
public:
  typedef EdgeRemappingFunctor<TInput> Self;

  void SetParameters(float intensityMin, float intensityMax,
                     float exponent, float kappa)
  {
    m_KappaFactor = 1.0f / kappa;
    m_Exponent = exponent;
    m_IntensityBase = intensityMin;
    m_IntensityScale = 1.0f / (intensityMax - intensityMin);
  }

  // This operator maps the input value into the range of short integers
  inline short operator()(const TInput &x)
  {
    float xNorm = (static_cast<float>(x)-m_IntensityBase)*m_IntensityScale;
    float y = 1.0 / (1.0 + pow(xNorm * m_KappaFactor,m_Exponent));
    return static_cast<short> (y * 0x7fff);
  }

  bool operator ==(const Self &z)
    { 
    return 
      m_KappaFactor == z.m_KappaFactor &&
      m_IntensityBase == z.m_IntensityBase &&
      m_IntensityScale == z.m_IntensityScale &&
      m_Exponent == z.m_Exponent;
    }

  bool operator !=(const Self &z)
    { return !(*this == z); }

private:
  
  float m_KappaFactor;
  float m_IntensityBase;
  float m_IntensityScale;
  float m_Exponent;
};

/**
 * \class EdgePreprocessingImageFilter
 * \brief A filter used for edge preprocessing of images in the IRIS application.
 * 
 * This functor implements a Gaussian blur, followed by a gradient magnitude
 * operator, followed by a 'contrast enhancement' intensity remapping filter.
 */
template <typename TInputImage,typename TOutputImage>
class EdgePreprocessingImageFilter: 
  public itk::ImageToImageFilter<TInputImage,TOutputImage>
{
public:
  
  /** Standard class typedefs. */
  typedef EdgePreprocessingImageFilter                             Self;
  typedef itk::ImageToImageFilter<TInputImage,TOutputImage>  Superclass;
  typedef itk::SmartPointer<Self>                               Pointer;
  typedef itk::SmartPointer<const Self>                    ConstPointer;  
  
  /** Pixel Type of the input image */
  typedef TInputImage                                    InputImageType;
  typedef typename InputImageType::PixelType             InputPixelType;
  typedef itk::SmartPointer<InputImageType>           InputImagePointer;

  /** Pixel Type of the output image */
  typedef TOutputImage                                  OutputImageType;
  typedef itk::SmartPointer<OutputImageType>         OutputImagePointer;
  typedef typename Superclass::OutputImageRegionType
                                                  OutputImageRegionType;

  /** Type used for internal calculations */
  typedef float                                                RealType;
  typedef itk::Image<RealType,3>                      InternalImageType;
  typedef itk::SmartPointer<InternalImageType>     InternalImagePointer;

#ifdef SNAP_USE_GPU
  typedef typename itk::GPUTraits<InternalImageType>::Type
                                                   GPUInternalImageType;
  typedef itk::SmartPointer<GPUInternalImageType> 
                                                GPUInternalImagePointer;
#endif

  /** Functor type used for thresholding */
  typedef EdgeRemappingFunctor<RealType>                    FunctorType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)
    
  /** Image dimension. */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TInputImage::ImageDimension);

  /** Assign new edge processing settings */
  void SetParameters(EdgePreprocessingSettings *parameters);

  /** Set the maximum possible value of the gradient magnitude of the input
    image. This should be computed by applying a gradient magnitude filter
    to the input image. Must be provided for the filter to work. */
  itkSetMacro(InputImageMaximumGradientMagnitude, double)

  /** Get the parameters pointer */
  EdgePreprocessingSettings *GetParameters();

protected:

  EdgePreprocessingImageFilter();
  virtual ~EdgePreprocessingImageFilter() {}
  void PrintSelf(std::ostream& os, itk::Indent indent) const;
  
  /** Generate Data */
  void GenerateData();

  /** 
   * This method maps an input region to an output region.  It's necessary to
   * reflect the way this filter pads the requested region
   */
  void GenerateInputRequestedRegion();

private:

  double m_InputImageMaximumGradientMagnitude;

  typedef itk::CastImageFilter<InputImageType, InternalImageType>   CastFilter;

  typedef itk::DiscreteGaussianImageFilter<InternalImageType,
                                           InternalImageType>       BlurFilter;

#ifdef SNAP_USE_GPU
  typedef CPUImageToGPUImageFilter<GPUInternalImageType>        GPUImageSource;
  typedef itk::GPUDiscreteGaussianImageFilter<GPUInternalImageType,
                                              GPUInternalImageType>
                                                                 GPUBlurFilter;
#endif

  typedef itk::GradientMagnitudeImageFilter<InternalImageType,
                                            InternalImageType>   GradMagFilter;

  typedef itk::UnaryFunctorImageFilter<InternalImageType,
                                       OutputImageType,
                                       FunctorType>                RemapFilter;

  SmartPtr<CastFilter> m_CastFilter;
  SmartPtr<BlurFilter> m_BlurFilter;
  SmartPtr<GradMagFilter> m_GradMagFilter;
  SmartPtr<RemapFilter> m_RemapFilter;

#ifdef SNAP_USE_GPU
  SmartPtr<GPUImageSource> m_GPUImageSource;
  SmartPtr<GPUBlurFilter>  m_GPUBlurFilter;
#endif

};

#ifndef ITK_MANUAL_INSTANTIATION
#include "EdgePreprocessingImageFilter.txx"
#endif

#endif // __EdgePreprocessingFilter_h_
