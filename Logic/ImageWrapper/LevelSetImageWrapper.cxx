/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LevelSetImageWrapper.cxx,v $
  Language:  C++
  Date:      $Date: 2010/06/28 18:45:08 $
  Version:   $Revision: 1.6 $
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
#include "LevelSetImageWrapper.h"
#include "ImageWrapper.txx"
#include "ScalarImageWrapper.txx"
#include "ColorLabel.h"

// Create an instance of ImageWrapper of appropriate type
template class ImageWrapper<float>;
template class ScalarImageWrapper<float>;


LevelSetImageWrapper::DisplayPixelType
LevelSetImageWrapper::MappingFunctor
::operator()(float in)
{
  // Negative values are 'inside'
  return in <= 0.0f ? m_InsidePixel : m_OutsidePixel;

  // Use this code to display the level set directly
  // TODO: Add an option to use this form of display
  /* 
  LevelSetImageWrapper::DisplayPixelType pixel(m_OutsidePixel);

  if(in > -4.0 && in < 4.0) 
  {
    if(in <= 0)
    {
      pixel[0] = (unsigned char) (255 + in * 64);
      pixel[1] = (unsigned char) (255 + in * 64);
      pixel[2] = 255;
    }
    else
    {
      pixel[0] = 255;
      pixel[1] = (unsigned char) (255 - in * 64);
      pixel[2] = (unsigned char) (255 - in * 64);
    }    

    pixel[3] = m_InsidePixel[3];
  }
  else if(in <= -4.0)
    {
    pixel[0] = 0; pixel[1] = 0; pixel[2] = 255;
    pixel[3] = 255;
    }
  else if(in >= 4.0)
    {
    pixel[0] = 255; pixel[1] = 0; pixel[2] = 0;
    pixel[3] = 255;
    }

  return pixel;
  */
}

LevelSetImageWrapper
::LevelSetImageWrapper()
: ScalarImageWrapper<float> ()
{
  // Intialize display filters
  for(unsigned int i=0;i<3;i++) 
    {
    // Create the intensity mapping filter
    m_DisplayFilter[i] = IntensityFilterType::New();

    // Set the corresponding slice as the input image
    m_DisplayFilter[i]->SetInput(GetSlice(i));
    }

  // Initialize to Edge mode
  m_MappingFunctor.m_InsidePixel.Fill(0);
  m_MappingFunctor.m_OutsidePixel.Fill(0);  
}

LevelSetImageWrapper
::~LevelSetImageWrapper()
{
}

LevelSetImageWrapper::DisplaySlicePointer 
LevelSetImageWrapper
::GetDisplaySlice(unsigned int iSlice) const
{
  // Depending on the current mode, return the display slice or the 
  // original slice from the parent
  return m_DisplayFilter[iSlice]->GetOutput();
}

void 
LevelSetImageWrapper
::SetColorLabel(const ColorLabel &label)
{
  m_MappingFunctor.m_InsidePixel[0] = label.GetRGB(0);
  m_MappingFunctor.m_InsidePixel[1] = label.GetRGB(1);
  m_MappingFunctor.m_InsidePixel[2] = label.GetRGB(2);

  // Figure out the alpha for display
  unsigned char alpha = 
    label.IsVisible() ? (label.IsOpaque() ? 255 : label.GetAlpha()) : 0;

  m_MappingFunctor.m_InsidePixel[3] = alpha;

  // Intialize display filters
  for(unsigned int i=0;i<3;i++) 
    {
    m_DisplayFilter[i]->SetFunctor(m_MappingFunctor);
    }
}
