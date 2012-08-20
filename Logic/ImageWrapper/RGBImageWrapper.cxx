/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: RGBImageWrapper.cxx,v $
  Language:  C++
  Date:      $Date: 2010/06/28 18:45:08 $
  Version:   $Revision: 1.7 $
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
#include "RGBImageWrapper.h"
#include "itkUnaryFunctorImageFilter.h"

template<class TComponent>
RGBImageWrapper<TComponent>
::RGBImageWrapper()
: VectorImageWrapper<PixelType> ()
{
  // Intialize display filters
  for(unsigned int i=0;i<3;i++) 
    {
    // Create the intensity mapping filter
    m_DisplayFilter[i] = IntensityFilterType::New();

    // Set the intensity functor
    m_DisplayFilter[i]->SetFunctor(m_IntensityFunctor);

    // Set the corresponding slice as the input image
    m_DisplayFilter[i]->SetInput(this->GetSlice(i));
    }
}

template<class TComponent>
RGBImageWrapper<TComponent>
::~RGBImageWrapper()
{
  for (size_t i = 0; i < 3; ++i)
    {
    m_DisplayFilter[i] = NULL;
    }
}

template<class TComponent>
typename RGBImageWrapper<TComponent>::DisplaySlicePointer
RGBImageWrapper<TComponent>
::GetDisplaySlice(unsigned int iSlice)
{
  // Depending on the current mode, return the display slice or the 
  // original slice from the parent
  //return m_IsModeInsideOutside ? 
  //  m_DisplayFilter[iSlice]->GetOutput() : 
  //  GetSlice(iSlice);
  return m_DisplayFilter[iSlice]->GetOutput();
}

template<class TComponent>
typename RGBImageWrapper<TComponent>::DisplayPixelType
RGBImageWrapper<TComponent>::IntensityFunctor
::operator()(const PixelType &x) const
{
  // Create a new pixel
  DisplayPixelType pixel;
  pixel[0] = x[0];
  pixel[1] = x[1];
  pixel[2] = x[2];
  if (x[0] != 0 || x[1] != 0 || x[2] != 0)
    pixel[3] = 255;
  else
    pixel[3] = 0;
  return pixel;
}

template class RGBImageWrapper<unsigned char>;

