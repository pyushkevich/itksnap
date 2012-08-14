/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SpeedImageWrapper.h,v $
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
#ifndef __SpeedImageWrapper_h_
#define __SpeedImageWrapper_h_

#include "GreyImageWrapper.h"

// #include "SpeedColorMap.h"

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
 * \class SpeedImageWrapper
 * \brief Image wraper for speed images in SNAP
 *
 * This wrapper remaps floating point slices to byte slices differently
 * depending on if it's in InOut snake mode (some speed values are negative) or
 * in Edge snake mode (speed values are nonnegative).
 *
 * \sa ImageWrapper
 */
class SpeedImageWrapper : public GreyImageWrapper<short>
{
public:
  // Basics
  typedef SpeedImageWrapper Self;
  typedef ScalarImageWrapper<short> Superclass;
  typedef Superclass::ImageType ImageType;

  /**
   * Set the preview source for the slices.  This means that the slices
   * will be generated not from the internal image but from external 
   * images (outputs of some preprocessing filters)
   */
  void SetSliceSourceForPreview(unsigned int slice,ImageType *source);

  /** 
   * Unset the preview sources for all slices.  The slices will be now
   * generated from the internal image 
   */
  void RemoveSliceSourcesForPreview();

  /** Get a 'preview' voxel, i.e., a voxel from the previewing slices.  For
   * the results to be valid, the voxel has to be on one of the previewing
   * slices, and this method is intended for getting the voxel at the
   * cross-hairs position */
  float GetPreviewVoxel(const Vector3ui &point) const;

  /**
   * Indicate that this image is a In/Out speed image that has a 
   * range of -1 to +1.  
   */
  void SetModeToInsideOutsideSnake()
    {
    m_IsModeInsideOutside = true;
    }
  
  /**
   * Indicate that this image is a Edge speed image that has a 
   * range of 0 to 1.  
   */
  void SetModeToEdgeSnake()
    {
    m_IsModeInsideOutside = false;
    }

  /**
   * Check if the image is in the Inside/Outside or Edge mode
   */
  bool IsModeInsideOutsideSnake() const
    {
    return m_IsModeInsideOutside;
    }
    
  /**
   * Check if the image is in the Inside/Outside or Edge mode
   */
  bool IsModeEdgeOutsideSnake() const
    {
    return !m_IsModeInsideOutside;
    }

  typedef itk::ImageSource<ImageType> PreviewFilterType;
  typedef itk::SmartPointer<PreviewFilterType> PreviewFilterPointer;
  
  /** Constructor initializes mappers */
  SpeedImageWrapper();

  /** Destructor */
  ~SpeedImageWrapper();

protected:
  /** We override this method in order to maintain the preview sources 
   * when the image gets changed */
  void UpdateImagePointer(ImageType *newImage);

  /** Whether or not the image is in edge or in-out mode */
  bool m_IsModeInsideOutside;

  /** Preview sources */
  ImagePointer m_PreviewSource[3];
};

#endif // __SpeedImageWrapper_h_
