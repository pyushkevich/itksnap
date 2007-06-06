/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: VectorImageWrapper.txx,v $
  Language:  C++
  Date:      $Date: 2007/06/06 22:27:21 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __VectorImageWrapper_txx_
#define __VectorImageWrapper_txx_

#include "VectorImageWrapper.h"
#include "itkImageRegionIterator.h"
#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkNumericTraits.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkIdentityTransform.h"
#include "IRISSlicer.h"
#include "SNAPSegmentationROISettings.h"
#include "itkCommand.h"

#include <iostream>

template <class TPixel>
typename VectorImageWrapper<TPixel>::ImagePointer
VectorImageWrapper<TPixel>
::DeepCopyRegion(const SNAPSegmentationROISettings &roi,
                 itk::Command *progressCommand) const
{
   std::cout << "VectorImageWrapper::DeepCopyRegion" << std::endl;
   std::cout << std::flush;
  return NULL;
}

#endif // __VectorImageWrapper_txx_
