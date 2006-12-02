/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: EdgePreprocessingImageFilter.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:15 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __EdgePreprocessingFilter_h_
#define __EdgePreprocessingFilter_h_

#include "itkCommand.h"
#include "itkCastImageFilter.h"
#include "itkDiscreteGaussianImageFilter.h"
#include "itkGradientMagnitudeImageFilter.h"
#include "itkImageAdaptor.h"
#include "itkMinimumMaximumImageCalculator.h"
#include "itkProgressAccumulator.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkUnaryFunctorImageFilter.h"
#include "EdgePreprocessingSettings.h"

/**
 * The g() function that remaps the gradient magnitude image to the 
 * range 0 to 1.  
 */
template<class TInput, class TOutput>
class EdgeRemappingFunctor
  {
public:
  typedef EdgeRemappingFunctor<TInput, TOutput> Self;

  void SetParameters(float intensityMin, float intensityMax,
                     float exponent, float kappa)
  {
    m_KappaFactor = 1.0f / kappa;
    m_Exponent = exponent;
    m_IntensityBase = intensityMin;
    m_IntensityScale = 1.0f / (intensityMax - intensityMin);
  }

  inline TOutput operator()(const TInput &x)
  {
    float xNorm = (static_cast<float>(x)-m_IntensityBase)*m_IntensityScale;
    float y = 1.0 / (1.0 + pow(xNorm * m_KappaFactor,m_Exponent));
    return static_cast<TOutput> (y);
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
template <typename TInputImage,typename TOutputImage = TInputImage>
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
  typedef typename OutputImageType::PixelType           OutputPixelType;
  typedef itk::SmartPointer<OutputImageType>         OutputImagePointer;

  /** Type used for internal calculations */
  typedef float                                                RealType;
  typedef itk::Image<RealType,3>                      InternalImageType;
  typedef itk::SmartPointer<InternalImageType>     InternalImagePointer;

  /** Functor type used for thresholding */
  typedef EdgeRemappingFunctor<RealType,OutputPixelType>    FunctorType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)
    
  /** Image dimension. */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TInputImage::ImageDimension);

  /** Assign new edge processing settings */
  void SetEdgePreprocessingSettings(const EdgePreprocessingSettings &settings);

protected:

  EdgePreprocessingImageFilter();
  virtual ~EdgePreprocessingImageFilter() {};
  void PrintSelf(std::ostream& os, itk::Indent indent) const;
  
  /** Generate Data */
  void GenerateData( void );

  /** 
   * This method maps an input region to an output region.  It's necessary to
   * reflect the way this filter pads the requested region
   */
  void GenerateInputRequestedRegion();

private:

  /** The unary functor filter type used for remapping */
  typedef itk::UnaryFunctorImageFilter<
    InternalImageType,TOutputImage,FunctorType>      RemappingFilterType;
  typedef typename RemappingFilterType::Pointer   RemappingFilterPointer;
  
  /** The min / max calculator used to compute gradient range */
  typedef itk::MinimumMaximumImageCalculator<
    InternalImageType>                                    CalculatorType;
  typedef typename CalculatorType::Pointer             CalculatorPointer;

  /** Adaptor used to cast to float */
  typedef itk::CastImageFilter<
    InputImageType,InternalImageType>                     CastFilterType;
  typedef typename CastFilterType::Pointer             CastFilterPointer;
  
  /** Gaussian smoothing filter */
  typedef itk::DiscreteGaussianImageFilter<
    InternalImageType,InternalImageType>              GaussianFilterType;
  typedef typename GaussianFilterType::Pointer     GaussianFilterPointer;
  
  /** Gradient magnitude filter */
  typedef itk::GradientMagnitudeImageFilter<
    InternalImageType,InternalImageType>              GradientFilterType;
  typedef typename GradientFilterType::Pointer     GradientFilterPointer;

  /** Intensity rescaling filter */
  typedef itk::RescaleIntensityImageFilter<
    InternalImageType,InternalImageType>               RescaleFilterType;
  typedef typename RescaleFilterType::Pointer       RescaleFilterPointer;
  
  /** Progress accumulator object */
  typedef itk::ProgressAccumulator::Pointer           AccumulatorPointer;

  CastFilterPointer         m_CastFilter;
  RemappingFilterPointer    m_RemappingFilter;
  GaussianFilterPointer     m_GaussianFilter;
  GradientFilterPointer     m_GradientFilter;
  CalculatorPointer         m_Calculator;
  RescaleFilterPointer      m_RescaleFilter;

  EdgePreprocessingSettings m_EdgePreprocessingSettings;

  /** Progress tracking object */
  AccumulatorPointer        m_ProgressAccumulator;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "EdgePreprocessingImageFilter.txx"
#endif

#endif // __EdgePreprocessingFilter_h_
