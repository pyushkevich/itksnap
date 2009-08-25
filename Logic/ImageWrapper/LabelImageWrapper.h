/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LabelImageWrapper.h,v $
  Language:  C++
  Date:      $Date: 2009/08/25 21:38:16 $
  Version:   $Revision: 1.5 $
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
#ifndef __LabelImageWrapper_h_
#define __LabelImageWrapper_h_

#include "ScalarImageWrapper.h"
#include "UnaryFunctorCache.h"
#include "LabelToRGBAFilter.h"



// Forward references
namespace itk {
  template<class TInput, class TOutput, class TFunctor> 
    class UnaryFunctorImageFilter;
}

class ColorLabel;
class ColorLabelTable;

/**
 * \class LabelImageWrapper
 * \brief ImageWrapper for segmentation images in SNAP/IRIS.
 * 
 * An extension of the ImageWrapper class for dealing with segmentation
 * images.  Using a table of color labels, it is possible to get RGBA 
 * slices from these images.
 *
 * \sa ImageWrapper
 */
class LabelImageWrapper : public ScalarImageWrapper<LabelType>
{
public:

  /**
   * Set the table of color labels used to produce color slice images
   */  
  void SetLabelColorTable(ColorLabelTable *labels);

  /**
   * Get the color label table
   */
  ColorLabelTable *GetLabelColorTable() const;

  /**
   * Tell the object to update it's color mapping cache
   * TODO: Implement this with ModifiedTime stuff
   */
  void UpdateColorMappingCache();

  /**
   * Get a color slice for display purposes
   */
  DisplaySlicePointer GetDisplaySlice(unsigned int dim) const;

  /** Constructor initializes mapper */
  LabelImageWrapper();

  /** Constructor that copies another wrapper */
  LabelImageWrapper(const LabelImageWrapper &source);

  /** Destructor */
  ~LabelImageWrapper();  

private:
  /**
   * Functor used for display caching.  This class keeps a pointer to 
   * the table of colors and maps colors to RGBA Pixels
   */
  class IntensityFunctor {
  public:    
      /** The pointer to the label table */
      ColorLabelTable *m_ColorLabelTable;

      /** The operator that maps label to color */
      DisplayPixelType operator()(const LabelType &x) const;

      // Dummy equality operators, since there is no data here
      bool operator == (const IntensityFunctor &) const { return true; }
      bool operator != (const IntensityFunctor &) const { return false; }
  };

  // Type of intensity function used to map 3D volume intensity into
  // 2D slice intensities
  // typedef 
  //  UnaryFunctorCache<LabelType,DisplayPixelType,IntensityFunctor> CacheType;  
  // typedef itk::SmartPointer<CacheType> CachePointer;
  // typedef CacheType::CachingFunctor CacheFunctor;

  // Filter applied to slices
  typedef itk::Image<LabelType,2> LabelSliceType;


  typedef LabelToRGBAFilter RGBAFilterType;
  typedef RGBAFilterType::Pointer RGBAFilterPointer;

  RGBAFilterPointer m_RGBAFilter[3];

  // typedef 
  //  itk::UnaryFunctorImageFilter<LabelSliceType,DisplaySliceType,CacheFunctor>
  //  IntensityFilterType;
  // typedef itk::SmartPointer<IntensityFilterType> IntensityFilterPointer;

  /**
   * An instance of the private intensity mapper (this mapper wraps the passed
   * in list of labels
   */
  // IntensityFunctor m_IntensityFunctor;

  /**
   * A cache used for the intensity mapping function
   */
  // CachePointer m_IntensityMapCache;

  /**
   * Filters used to remap the intensity of the slices in this image
   * into unsigned char images
   */
  // IntensityFilterPointer m_IntensityFilter[3];
};

#endif // __LabelImageWrapper_h_
