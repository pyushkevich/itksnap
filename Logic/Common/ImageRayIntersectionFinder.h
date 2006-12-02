/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: ImageRayIntersectionFinder.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:11 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __ImageRayIntersectionFinder_h_
#define __ImageRayIntersectionFinder_h_

#include "SNAPCommon.h"
#include <vnl/vnl_matrix_fixed.h>

/**
 * \class ImageRayIntersectionFinder
 * \brief An algorithm for testing ray hits against arbitrary images.
 * This algorithm traverses a ray until it finds a pixel that satisfies the
 * hit tester (a functor with operator () which returns 0 for no-hit and 
 * 1 for hit).
 */
template <class TPixel, class THitTester>
class ImageRayIntersectionFinder
{
public:
    virtual ~ImageRayIntersectionFinder() {}
  /** Image type */
  typedef itk::Image<TPixel,3> ImageType;

  /** Set the hit-test functor to evaluate for hits */
  irisSetMacro(HitTester,THitTester);

  /** 
   * Compute the intersection (index of the first pixel in the
   * image that the ray crosses and which satisfies the THitTester's 
   * condition.
   *
   * Returns: 1 on success, 0 on no hit and -1 if the ray misses the 
   * image completely.
   */
  int FindIntersection(ImageType *image,Vector3d xRayStart,
                       Vector3d xRayVector,Vector3i &xHitIndex) const;
private:
  /** The hit tester used internally */
  THitTester m_HitTester;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "ImageRayIntersectionFinder.txx"
#endif

#endif
