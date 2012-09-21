/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: RGBImageWrapper.h,v $
  Language:  C++
  Date:      $Date: 2009/08/25 21:38:16 $
  Version:   $Revision: 1.6 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __RGBImageWrapper_h_
#define __RGBImageWrapper_h_

#include "VectorImageWrapper.h"

// Forward references to ITK
namespace itk {
  template <class TInput,class TOutput,class TFunctor> 
    class UnaryFunctorImageFilter;
  template <class TOutput> class ImageSource;
};

// Disable 'inheritance by dominance' warining in VC6
#if defined(_WIN32) && defined(_MSC_VER)
  #pragma warning (disable: 4250)
#endif

/**
 * \class RGBImageWrapper
 * \brief Image wrapper for RGB images in SNAP
 *
 * \sa ImageWrapper
 */
template <class TComponent, class TBase = RGBImageWrapperBase>
class RGBImageWrapper :
    public VectorImageWrapper< itk::RGBPixel<TComponent>, TBase >
{
public:

  // Standard ITK business
  typedef itk::RGBPixel<TComponent>                                  PixelType;
  typedef RGBImageWrapper<TComponent, TBase>                              Self;
  typedef VectorImageWrapper<PixelType, TBase>                      Superclass;
  typedef SmartPtr<Self>                                               Pointer;
  typedef SmartPtr<const Self>                                    ConstPointer;
  itkTypeMacro(RGBImageWrapper, VectorImageWrapper)
  itkNewMacro(Self)

  // Basics
  typedef typename Superclass::ImageType                             ImageType;
  typedef typename Superclass::DisplaySliceType               DisplaySliceType;
  typedef typename Superclass::DisplaySlicePointer         DisplaySlicePointer;
  typedef typename Superclass::DisplayPixelType               DisplayPixelType;

  /**
   * Get the display slice in a given direction.  To change the
   * display slice, call parent's MoveToSlice() method
   */
  DisplaySlicePointer GetDisplaySlice(unsigned int dim);

protected:

  /** Constructor initializes mappers */
  RGBImageWrapper();

  /** Destructor */
  ~RGBImageWrapper();


  class IntensityFunctor {
  public:
    /** The operator that maps label to color */
    DisplayPixelType operator()(const PixelType &x) const;

    // Equality operators required, if variables defined!!!
    bool operator == (const IntensityFunctor &z) const
      {
      return true;
      }
    bool operator != (const IntensityFunctor &z) const
      {
      return !(*this == z);
      }
  };

  // Type of the display intensity mapping filter used when the 
  // input is a in-out image
  typedef itk::Image<PixelType,2> RGBSliceType;
  typedef itk::UnaryFunctorImageFilter<
    RGBSliceType,DisplaySliceType,IntensityFunctor> 
    IntensityFilterType;
  typedef itk::SmartPointer<IntensityFilterType> IntensityFilterPointer;

  IntensityFilterPointer m_DisplayFilter[3];

  IntensityFunctor m_IntensityFunctor;

};

#endif // __RGBImageWrapper_h_
