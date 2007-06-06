/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: LevelSetImageWrapper.cxx,v $
  Language:  C++
  Date:      $Date: 2007/06/06 22:27:21 $
  Version:   $Revision: 1.2 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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

using namespace itk;

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
::GetDisplaySlice(unsigned int iSlice)
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
