/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LevelSetExtensionFilter.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:14 $
  Version:   $Revision: 1.2 $
  Copyright (c) 2007 Paul A. Yushkevich
  
  This file is part of ITK-SNAP 

   Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#ifndef __LevelSetExtensionFilter_h_
#define __LevelSetExtensionFilter_h_

#include "itkFiniteDifferenceFunction.h" 
#include "itkFiniteDifferenceImageFilter.h" 
#include "itkMutexLock.h"
#include "itkConditionVariable.h"
#include "itkBarrier.h"
#include "itkCommand.h"

/** A generic extension of a filter (intended to be a 
 * FiniteDifferenceImageFilter) that let's use control it
 * in a VCR (play-stop-step-rewind) fashion */
template <class TFilter>
class LevelSetExtensionFilter : public TFilter  
{
public:
  
  /** Standard class typedefs. */
  typedef LevelSetExtensionFilter<TFilter> Self;
  typedef TFilter Superclass;
  typedef itk::SmartPointer<Self> Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  /** Run-time type information. */
  itkTypeMacro(LevelSetExtensionFilter,TFilter);

  /** Capture information from the superclass. */
  typedef typename Superclass::InputImageType   InputImageType;
  typedef typename Superclass::OutputImageType  OutputImageType;

  /** Dimensionality of input and output data is assumed to be the same.
   * It is inherited from the superclass. */
  itkStaticConstMacro(ImageDimension, unsigned int,Superclass::ImageDimension);

  /** ITK new macro */
  itkNewMacro(LevelSetExtensionFilter);

  /** The pixel type of the output image will be used in computations.
   * Inherited from the superclass. */
  typedef typename Superclass::PixelType PixelType;
  typedef typename Superclass::TimeStepType TimeStepType;

protected:
  LevelSetExtensionFilter() {}
  virtual ~LevelSetExtensionFilter() {}

  /** Just a print method */
  void PrintSelf(std::ostream& os, itk::Indent indent) const ITK_OVERRIDE
  {
    Superclass::PrintSelf(os,indent);
  }

private:
  LevelSetExtensionFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented
 
};


#endif // __LevelSetExtensionFilter_h_

