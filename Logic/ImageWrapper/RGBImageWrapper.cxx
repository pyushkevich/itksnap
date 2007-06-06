/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: RGBImageWrapper.cxx,v $
  Language:  C++
  Date:      $Date: 2007/06/06 22:27:21 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "RGBImageWrapper.h"
#include "ImageWrapper.txx"
#include "VectorImageWrapper.txx"

// Create an instance of ImageWrapper of appropriate type
template class ImageWrapper<RGBType>;
template class VectorImageWrapper<RGBType>;

unsigned char RGBImageWrapper::IntensityFunctor::m_Alpha = 0;

using namespace itk;

RGBImageWrapper
::RGBImageWrapper()
: VectorImageWrapper<RGBType> ()
{
  // Intialize display filters
  for(unsigned int i=0;i<3;i++) 
    {
    // Create the intensity mapping filter
    m_DisplayFilter[i] = IntensityFilterType::New();
    
    // Set the corresponding slice as the input image
    m_DisplayFilter[i]->SetInput(GetSlice(i));
    }

}

RGBImageWrapper
::~RGBImageWrapper()
{
}

RGBImageWrapper::DisplaySliceType * 
RGBImageWrapper
::GetDisplaySlice(unsigned int iSlice)
{
  // Depending on the current mode, return the display slice or the 
  // original slice from the parent
  //return m_IsModeInsideOutside ? 
  //  m_DisplayFilter[iSlice]->GetOutput() : 
  //  GetSlice(iSlice);
  return m_DisplayFilter[iSlice]->GetOutput();
}

void
RGBImageWrapper
::SetAlpha (unsigned char alpha)
{
RGBImageWrapper::IntensityFunctor::m_Alpha = alpha;
}

RGBImageWrapper::DisplayPixelType
RGBImageWrapper::IntensityFunctor
::operator()(const RGBType &x) const
{
  // Create a new pixel
  DisplayPixelType pixel;

  pixel[0] = x[0];
  pixel[1] = x[1];
  pixel[2] = x[2];
  pixel[3] = m_Alpha;
  return pixel;
}

