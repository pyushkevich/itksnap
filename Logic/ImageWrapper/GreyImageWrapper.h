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
#include "ColorMap.h"
#include "IntensityCurveVTK.h"
// #include "UnaryFunctorCache.h"

// Forward references
namespace itk {
  template<class TInput,class TOutput> class FunctionBase;
  template<class TInput,class TOutput,class TFunctor> 
    class UnaryFunctorImageFilter;
};
template <class TInput, class TOutput, class TFunctor> 
  class UnaryFunctorCache;
template <class TInput, class TOutput, class TFunctor> 
  class CachingUnaryFunctor;

/**
 * \class GreyImageWrapper
 * \brief Image wrapper used for greyscale images in IRIS/SNAP.
 * 
 * Adds ability to remap intensity from short to byte using an
 * arbitrary function when outputing slices.
 */
class GreyImageWrapper : public ScalarImageWrapper<GreyType>
{
public:

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
  IntensityCurveInterface* GetIntensityMapFunction();

  /**
   * Copy the intensity curve information from another grey image wrapper
   */
  void CopyIntensityMap(const GreyImageWrapper &source);

  void UpdateIntensityMapFunction();

  /**
   * Set the reference intensity range - a range of intensity that 
   * is mapped to the domain of the intensity curve
   * @see GetIntensityMapFunction
   */
  void SetReferenceIntensityRange(GreyType refMin, GreyType refMax);
  void ClearReferenceIntensityRange();

  /**
   * Set the transformation to native intensity space
   */
  irisSetMacro(NativeMapping, GreyTypeToNativeFunctor);
  irisGetMacro(NativeMapping, GreyTypeToNativeFunctor);


  /**
   * Get voxel intensity in native space
   */
  double GetVoxelMappedToNative(const Vector3ui &vec)
    { return m_NativeMapping(this->GetVoxel(vec)); }

  /**
   * Get min/max voxel intensity in native space
   */
  double GetImageMinNative()
    { return m_NativeMapping(this->GetImageMin()); }
  double GetImageMaxNative()
    { return m_NativeMapping(this->GetImageMax()); }

  /**
   * Get the display slice in a given direction.  To change the
   * display slice, call parent's MoveToSlice() method
   */
  DisplaySlicePointer GetDisplaySlice(unsigned int dim) const;

  /**
   * Get/Set the colormap
   */
  ColorMap GetColorMap () const;
  void SetColorMap (const ColorMap& colormap);

  void Update();

  /** Constructor initializes mappers */
  GreyImageWrapper();

  /** Destructor */
  ~GreyImageWrapper();

private:

  /**
   * This object is passed on to the cache for intensity mapping
   */
  class IntensityFunctor {
  public:

    /** Map a grey value */
    DisplayPixelType operator()(const GreyType &value) const;

    // The storage for the float->float intensity map
    IntensityCurveInterface *m_IntensityMap;

    // Intensity mapping factors
    GreyType m_IntensityMin;
    float m_IntensityFactor;
 
    // Color map
    ColorMap m_Colormap;

    // Equality operators required, if variables defined!!!
    bool operator == (const IntensityFunctor &z) const 
      { 
      return 
        m_IntensityMap == z.m_IntensityMap &&
        m_IntensityFactor == z.m_IntensityFactor &&
        m_IntensityMin == z.m_IntensityMin &&
        m_Colormap == z.m_Colormap;
      }

    bool operator != (const IntensityFunctor &z) const 
      { return !(*this == z); }

    /**
     * Set the range over which the input data is mapped to output data
     */
    void SetInputRange(GreyType intensityMin,GreyType intensityMax);
  };

  // Type of intensity function used to map 3D volume intensity into
  // 2D slice intensities
  typedef UnaryFunctorCache<GreyType,DisplayPixelType,IntensityFunctor> CacheType;
  typedef itk::SmartPointer<CacheType> CachePointer;  
  typedef CachingUnaryFunctor<GreyType,DisplayPixelType,IntensityFunctor>
     CacheFunctor;

  // Filters applied to slices
  typedef itk::Image<GreyType,2> GreySliceType;
  typedef itk::UnaryFunctorImageFilter<
    GreySliceType,DisplaySliceType,CacheFunctor> IntensityFilterType;
  typedef itk::SmartPointer<IntensityFilterType> IntensityFilterPointer;

  /**
   * Reference intensity range. This is used for images that are subregions
   * of larger images. When evaluating the intensity of these images, the 
   * intensity curve needs to be applied to the intensity range of the larger 
   * image, not that of the region.
   */
  GreyType m_ReferenceIntensityMin, m_ReferenceIntensityMax;
  bool m_FlagUseReferenceIntensityRange;

  /**
   * An instance of the private intensity mapper (this mapper wraps the
   * passed in float->float function to a new function that is 
   * [min..max]->uchar)
   */
  IntensityFunctor m_IntensityFunctor;

  /**
   * A cache used for the intensity mapping function
   */
  CachePointer m_IntensityMapCache;

  /**
   *
   */
  IntensityCurveVTK::Pointer m_IntensityCurveVTK;

  /**
   * Filters used to remap the intensity of the slices in this image
   * into unsigned char images
   */
  IntensityFilterPointer m_IntensityFilter[3];

  /** 
   * The grey image is an image of shorts. But the real image on disk may
   * be an image of floats or doubles. So we store a transformation from
   * internal intensity values to 'native' intensity values
   */
  GreyTypeToNativeFunctor m_NativeMapping;
};

#endif // __GreyImageWrapper_h_
