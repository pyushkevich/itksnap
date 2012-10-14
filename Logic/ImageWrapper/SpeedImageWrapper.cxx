/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SpeedImageWrapper.cxx,v $
  Language:  C++
  Date:      $Date: 2010/06/28 18:45:08 $
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
#include "SpeedImageWrapper.h"
#include "itkUnaryFunctorImageFilter.h"
#include "IRISSlicer.h"
#include "ScalarImageHistogram.h"
#include "IntensityToColorLookupTableImageFilter.h"
#include "ColorMap.h"
#include "IntensityCurveInterface.h"

SpeedImageWrapper
::SpeedImageWrapper()
{
  // Initialize to Edge mode
  m_IsModeInsideOutside = false;

  // Set the native mapping (scale by 1 / 0x7fff). The speed image represents
  // floating point values in the range [-1 1]. Internally, it is represented
  // by an image of short type.
  InternalToNativeFunctor native(1.0 / 0x7fff, 0.0);
  this->SetNativeMapping(native);

  /*
    Tell the lookup table filter to use the whole range of short values as
    the domain of the lookup table. This is necessary because the speed
    image wrapper participates in the Preview pipeline, which means we may
    request the lookup table before knowing the intensity range of the
    full speed image. Since the speed image has range [-1 1] anyway, there
    is no memory overhead to using the full intensity range
    */
  this->m_LookupTableFilter->SetFixedLookupTableRange(-0x8000, 0x7fff);

  // By default, speed images use a blue to white colormap
  this->m_ColorMap->SetToSystemPreset(ColorMap::COLORMAP_SPEED);
}

SpeedImageWrapper
::~SpeedImageWrapper()
{
}

