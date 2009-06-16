/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: RGBImageWrapper.h,v $
  Language:  C++
  Date:      $Date: 2009/06/16 04:55:45 $
  Version:   $Revision: 1.5 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __RGBImageWrapper_h_
#define __RGBImageWrapper_h_

#include "VectorImageWrapper.h"
#include "itkRGBAPixel.h"

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
class RGBImageWrapper : public VectorImageWrapper<RGBType>
{
public:
  // Basics
  typedef RGBImageWrapper Self;
  typedef VectorImageWrapper<RGBType> Superclass;
  typedef Superclass::ImageType ImageType;

  // The type definition for the image used to display speed slices
  typedef itk::RGBAPixel<unsigned char> DisplayPixelType;
  typedef itk::Image<DisplayPixelType,2> DisplaySliceType;
  typedef itk::SmartPointer<DisplaySliceType> DisplaySlicePointer;

  /**
   * Get the display slice in a given direction.  To change the
   * display slice, call parent's MoveToSlice() method
   */
  DisplaySliceType *GetDisplaySlice(unsigned int dim);

  /** Constructor initializes mappers */
  RGBImageWrapper();

  /** Destructor */
  ~RGBImageWrapper();

private:
  
  class IntensityFunctor {
  public:
    /** The operator that maps label to color */
    DisplayPixelType operator()(const RGBType &x) const;

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
  typedef itk::Image<RGBType,2> RGBSliceType;
  typedef itk::UnaryFunctorImageFilter<
    RGBSliceType,DisplaySliceType,IntensityFunctor> 
    IntensityFilterType;
  typedef itk::SmartPointer<IntensityFilterType> IntensityFilterPointer;

  IntensityFilterPointer m_DisplayFilter[3];

  IntensityFunctor m_IntensityFunctor;

};

#endif // __RGBImageWrapper_h_
