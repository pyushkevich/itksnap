/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SpeedColorMap.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:14 $
  Version:   $Revision: 1.3 $
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
#ifndef __SpeedColorMap_h_
#define __SpeedColorMap_h_

#include "itkRGBAPixel.h"
#include "GlobalState.h"

/**
 * \class SpeedColorMap
 * \brief A very simple functor used to map intensities from 
 * the range (-1,1) to RGB color space.
 */
class SpeedColorMap 
{
public:
  typedef itk::RGBAPixel<unsigned char> OutputType;

  /** Constructor, sets default color map */
  SpeedColorMap();

  /** 
   * Mapping operator, maps range [-1, 1] into colors. This operator must
   * be very fast, because it is called a lot of times in the rendering 
   * pipeline. So we define it inline, and sacrifice generality for speed 
   */
  OutputType operator()(float t)
    {
    // We do bounds checking, just in case
    if(t < -0.99999f) return m_ColorEntry.front();
    if(t >= 0.99999f) return m_ColorEntry.back();

    // Project u into the array of intensities
    float u = t * m_DeltaT - m_Shift;
    unsigned int iu = (int) u;
    float a = u - iu;

    // Compute the correct color
    // (1 - a) c[iu] + a c[iu + 1] = c[iu] - a * (c[iu+1] - c[iu])
    OutputType p = m_ColorEntry[iu];
    p[0] = (unsigned char) (p[0] + a * m_ColorEntryDelta[iu][0]);
    p[1] = (unsigned char) (p[1] + a * m_ColorEntryDelta[iu][1]);
    p[2] = (unsigned char) (p[2] + a * m_ColorEntryDelta[iu][2]);

    // Return the point
    return p;
    }

  bool operator == (const SpeedColorMap &z)
    {
    return 
      m_DeltaT == z.m_DeltaT && 
      m_Shift == z.m_Shift && 
      m_ColorMapSize == z.m_ColorMapSize &&
      m_ColorEntry == z.m_ColorEntry;
    }

  bool operator != (const SpeedColorMap &z)
    { return !(*this == z); }
  
  /** Set the color map by specifying three points (-1, 0 and 1) */
  void SetColorMap(OutputType inMinus, OutputType inZero, OutputType inPlus) 
    {
    OutputType colors[] = {inMinus, inZero, inPlus};
    SetColorMap(3, colors);
    }

  /** Set the color map for the positive range only */
  void SetColorMapPositive(OutputType inZero, OutputType inHalf, OutputType inOne)
    {
    OutputType colors[] = {inZero, inZero, inZero, inHalf, inOne};
    SetColorMap(5, colors);
    }

  /** Set the color map by specifying a set of n colors between -1 and 1 */
  void SetColorMap(unsigned int n, OutputType *colors);

  /** Generate a color map for one of the presets */
  static SpeedColorMap GetPresetColorMap(ColorMapPreset xPreset);

  bool operator!=( const SpeedColorMap & other ) const
  {
    bool value = ( ( this->m_DeltaT != other.m_DeltaT ) || 
                   ( this->m_Shift  != other.m_Shift  ) || 
                   ( this->m_ColorMapSize  != other.m_ColorMapSize ) );
      return value;
  }
    
private:
  // The colors in the color map
  std::vector<OutputType> m_ColorEntry;

  // Differences between pairs of colors
  std::vector<Vector3f> m_ColorEntryDelta;

  // Scaling factors used to reference color entries
  float m_DeltaT;
  float m_Shift;
  float m_ColorMapSize;
};  

#endif
