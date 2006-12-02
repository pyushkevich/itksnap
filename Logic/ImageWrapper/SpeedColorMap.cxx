/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: SpeedColorMap.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:13 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "SpeedColorMap.h"

SpeedColorMap
::SpeedColorMap()
{
  OutputType allBlack[2];
  SetColorMap(2, allBlack);
}

void
SpeedColorMap
::SetColorMap(unsigned int n, OutputType *colors)
{
  // Compute colors and color differences
  m_ColorEntry.resize(n);
  m_ColorEntryDelta.resize(n-1);
  for(unsigned int i = 0; i < n; i++)
    m_ColorEntry[i] = colors[i];
  for(unsigned int j = 0; j < n-1; j++)  
    for(unsigned int k = 0; k < 3; k++)
      m_ColorEntryDelta[j][k] = 
        ((float) colors[j+1][k]) - ((float) colors[j][k]);

  // Compute scaling factors
  m_DeltaT = 0.5 * (n - 1.0f);
  m_Shift = -m_DeltaT;
}

SpeedColorMap
SpeedColorMap
::GetPresetColorMap(ColorMapPreset xPreset)
{
  unsigned char blue[]   = {0   , 0   , 255 , 255 };
  unsigned char red[]    = {255 , 0   , 0   , 255 };
  unsigned char white[]  = {255 , 255 , 255 , 255 };
  unsigned char gray[]   = {128 , 128 , 128 , 255 };
  unsigned char black[]  = {0   , 0   , 0   , 255 };
  unsigned char yellow[] = {255 , 255 , 0   , 255 };
  
  SpeedColorMap xMap;
  switch(xPreset) 
    {
    case COLORMAP_BLUE_BLACK_WHITE : 
      xMap.SetColorMap(
        OutputType(blue), OutputType(black), OutputType(white));
      break;
      
    case COLORMAP_BLACK_GRAY_WHITE : 
      xMap.SetColorMap(
        OutputType(black), OutputType(gray), OutputType(white));
      break;
      
    case COLORMAP_BLUE_WHITE_RED : 
      xMap.SetColorMap(
        OutputType(blue), OutputType(white), OutputType(red));
      break;

    case COLORMAP_BLACK_BLACK_WHITE : 
      xMap.SetColorMap(
        OutputType(black), OutputType(black), OutputType(white));
      break;

    case COLORMAP_BLACK_YELLOW_WHITE : 
      xMap.SetColorMapPositive(
        OutputType(black), OutputType(yellow), OutputType(white));
      break;
    }

  return xMap;
}

