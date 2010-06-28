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
#include "ImageWrapper.txx"
#include "ScalarImageWrapper.txx"


SpeedImageWrapper
::SpeedImageWrapper()
: ScalarImageWrapper<float> ()
{
  // Intialize display filters
  for(unsigned int i=0;i<3;i++) 
    {
    // Create the intensity mapping filter
    m_DisplayFilter[i] = IntensityFilterType::New();

    // Set the corresponding slice as the input image
    m_DisplayFilter[i]->SetInput(GetSlice(i));

    // Create the overlay mapping filter
    m_OverlayFilter[i] = OverlayFilterType::New();

    // Set the corresponding slice as the input image
    m_OverlayFilter[i]->SetInput(GetSlice(i));
    }

  // Initialize the overlay functor
  m_OverlayFunctor.m_Cutoff = 0.0f;

  // Initialize to Edge mode
  m_IsModeInsideOutside = false;
}

SpeedImageWrapper
::~SpeedImageWrapper()
{
}

SpeedImageWrapper::DisplaySlicePointer
SpeedImageWrapper
::GetDisplaySlice(unsigned int iSlice) const
{
  // Depending on the current mode, return the display slice or the 
  // original slice from the parent
  //return m_IsModeInsideOutside ? 
  //  m_DisplayFilter[iSlice]->GetOutput() : 
  //  GetSlice(iSlice);
  return m_DisplayFilter[iSlice]->GetOutput();
}


SpeedImageWrapper::OverlaySliceType * 
SpeedImageWrapper
::GetOverlaySlice(unsigned int dim)
{
  return m_OverlayFilter[dim]->GetOutput();
}

void
SpeedImageWrapper
::SetOverlayCutoff(float cutoff)
{
  if(cutoff != m_OverlayFunctor.m_Cutoff) 
    {
    // Update the variable
    m_OverlayFunctor.m_Cutoff = cutoff;

    // Update the filters
    for(unsigned int i=0;i<3;i++)
      m_OverlayFilter[i]->SetFunctor(m_OverlayFunctor);
    }
}

float
SpeedImageWrapper
::GetOverlayCutoff() const
{
  return m_OverlayFunctor.m_Cutoff;
}

void
SpeedImageWrapper
::SetOverlayColor(const OverlayPixelType &color)
{
  if(color != m_OverlayFunctor.m_Color) 
    {
    // Update the variable
    m_OverlayFunctor.m_Color = color;

    // Update the filters
    for(unsigned int i=0;i<3;i++)
      m_OverlayFilter[i]->SetFunctor(m_OverlayFunctor);
    }
}

void 
SpeedImageWrapper
::UpdateImagePointer(ImageType *newImage)
{
  // Call the parent's method
  ImageWrapper<float>::UpdateImagePointer(newImage);

  // Update the sources
  for(unsigned int i=0;i<3;i++)
    if(m_PreviewSource[i])
      m_Slicer[i]->SetInput(m_PreviewSource[i]);      
}


SpeedImageWrapper::OverlayPixelType 
SpeedImageWrapper
::GetOverlayColor() const
{
  return m_OverlayFunctor.m_Color;
}
  
SpeedImageWrapper::OverlayPixelType 
SpeedImageWrapper::OverlayFunctor
::operator()(float in)
{
  // Initialize with a clear pixel
  const unsigned char clear[] = {0,0,0,0};
  SpeedImageWrapper::OverlayPixelType rtn(clear);
  
  // Check the threshold and return appropriate value
  if(in<m_Cutoff)
    {
    return clear;
    } 
  return m_Color;
}

void 
SpeedImageWrapper
::SetSliceSourceForPreview(unsigned int slice,ImageType *source)
{
  m_PreviewSource[slice] = source;
  m_Slicer[slice]->SetInput(source);    
}

void 
SpeedImageWrapper
::RemoveSliceSourcesForPreview()
{
  for(unsigned int i=0;i<3;i++)
    {
    m_PreviewSource[i] = NULL;
    m_Slicer[i]->SetInput(m_Image);
    }
}

float 
SpeedImageWrapper
::GetPreviewVoxel(const Vector3ui &point) const
{
  // Better be in preview mode
  assert(m_PreviewSource[0] && m_PreviewSource[1] && m_PreviewSource[2]);

  // Create an index
  itk::Index<3> index;
  index[0] = point[0];
  index[1] = point[1];
  index[2] = point[2];

  // Make sure the slice is current
  m_Slicer[0]->Update();

  // Make sure the voxel is in the computed region
  assert(m_Slicer[0]->GetInput()->GetBufferedRegion().IsInside(index));

  // Return the pixel
  return m_Slicer[0]->GetInput()->GetPixel(index);    
}

void
SpeedImageWrapper
::SetColorMap(const SpeedColorMap &xColorMap)
{
  // Store the color map
  m_ColorMap = xColorMap;

  // Assign it to the three filters
  m_DisplayFilter[0]->SetFunctor(m_ColorMap);
  m_DisplayFilter[1]->SetFunctor(m_ColorMap);
  m_DisplayFilter[2]->SetFunctor(m_ColorMap);
}
