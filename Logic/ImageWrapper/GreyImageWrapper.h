/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GreyImageWrapper.h,v $
  Language:  C++
  Date:      $Date: 2009/09/19 08:15:09 $
  Version:   $Revision: 1.17 $
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
#ifndef __GreyImageWrapper_h_
#define __GreyImageWrapper_h_

#include "ScalarImageWrapper.h"

// Forward references
template <class TIn, class TLUT> class IntensityToColorLookupTableImageFilter;
template <class TIn, class TOut> class LookupTableIntensityMappingFilter;
class IntensityCurveVTK;
class ColorMap;

/**
 * \class GreyImageWrapper
 * \brief Image wrapper used for greyscale images in IRIS/SNAP.
 * 
 * Adds ability to remap intensity from short to byte using an
 * arbitrary function when outputing slices.
 */
template <class TPixel, class TBase = GreyImageWrapperBase>
class GreyImageWrapper : public ScalarImageWrapper<TPixel, TBase>
{
public:

  // Standard ITK business
  typedef GreyImageWrapper<TPixel, TBase>                                 Self;
  typedef ScalarImageWrapper<TPixel, TBase>                         Superclass;
  typedef SmartPtr<Self>                                               Pointer;
  typedef SmartPtr<const Self>                                    ConstPointer;
  itkTypeMacro(GreyImageWrapper, ScalarImageWrapper)
  itkNewMacro(Self)

  // Types inherited from parent
  typedef typename Superclass::ImageType                             ImageType;
  typedef typename Superclass::SliceType                             SliceType;
  typedef typename Superclass::DisplaySliceType               DisplaySliceType;
  typedef typename Superclass::DisplaySlicePointer         DisplaySlicePointer;

    
  /**
   * Get the intensity curve to be used for mapping image intensities 
   * from GreyType to DisplayType. The curve is defined on the domain
   * [0, 1]. By default, the entire intensity range of the image is
   * mapped to the domain of the curve. However, in some situations 
   * (e.g., when the image is a subregion of another image with respect
   * to which the curve was created), the domain of the curve should 
   * correspond to a different intensity range. That can be specified
   * using the SetReferenceIntensityRange() function
   */
  IntensityCurveInterface* GetIntensityMapFunction() const;

  /**
   * Copy the intensity curve information from another grey image wrapper
   */
  void CopyIntensityMap(const GreyImageWrapperBase &source);

  // void UpdateIntensityMapFunction();

  /**
   * Set the reference intensity range - a range of intensity that 
   * is mapped to the domain of the intensity curve
   * @see GetIntensityMapFunction
   */
  void SetReferenceIntensityRange(double refMin, double refMax);
  void ClearReferenceIntensityRange();

  /**
   * Get the display slice in a given direction.  To change the
   * display slice, call parent's MoveToSlice() method
   */
  DisplaySlicePointer GetDisplaySlice(unsigned int dim);

  /**
    Get a reference to the colormap
    */
  ColorMap *GetColorMap () const;

  /**
    Automatically rescale the intensity range based on image histogram
    quantiles.
    */
  void AutoFitContrast();


protected:

  /** Constructor initializes mappers */
  GreyImageWrapper();

  /** Destructor */
  ~GreyImageWrapper();

  // Lookup table
  typedef typename Superclass::DisplayPixelType              DisplayPixelType;
  typedef itk::Image<DisplayPixelType, 1>                     LookupTableType;

  // Filter that generates the lookup table
  typedef IntensityToColorLookupTableImageFilter<
              ImageType, LookupTableType>               LookupTableFilterType;

  // Filter that applies the lookup table to slices
  typedef LookupTableIntensityMappingFilter<
                SliceType, DisplaySliceType>              IntensityFilterType;


  // LUT generator
  SmartPtr<LookupTableFilterType> m_LookupTableFilter;

  // Filters for the three slice directions
  SmartPtr<IntensityFilterType> m_IntensityFilter[3];

  /**
   * Implementation of the intensity curve funcitonality. The intensity map
   * transforms input intensity values into the range [0 1], which serves as
   * the input into the color map transform.
   */
  SmartPtr<IntensityCurveVTK> m_IntensityCurveVTK;

  /**
    * Color map, which maps intensity values normalized to the range [0 1]
    * to output RGB values
    */
  SmartPtr<ColorMap> m_ColorMap;

  // Handle image pointer changes
  virtual void UpdateImagePointer(ImageType *image);

};

#endif // __GreyImageWrapper_h_
