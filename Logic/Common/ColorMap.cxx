/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ColorMap.cxx,v $
  Language:  C++
  Date:      $Date: 2009/08/27 20:02:17 $
  Version:   $Revision: 1.2 $
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
#include "ColorMap.h"
#include <algorithm>

ColorMap::CMPoint
::CMPoint()
{
  this->m_Index = 0.0;
  this->m_Type = CONTINUOUS;

  this->m_RGBA[0][0] = 0.0;  this->m_RGBA[1][0] = 0.0;
  this->m_RGBA[0][1] = 0.0;  this->m_RGBA[1][1] = 0.0;
  this->m_RGBA[0][2] = 0.0;  this->m_RGBA[1][2] = 0.0;
  this->m_RGBA[0][3] = 0.0;  this->m_RGBA[1][3] = 0.0;
  }

// Continuous point
ColorMap::CMPoint
::CMPoint(double j, EltType r, EltType g, EltType b, EltType a)
{
  this->m_Index = j;
  this->m_Type = CONTINUOUS;

  this->m_RGBA[0][0] = r;  this->m_RGBA[1][0] = r;
  this->m_RGBA[0][1] = g;  this->m_RGBA[1][1] = g;
  this->m_RGBA[0][2] = b;  this->m_RGBA[1][2] = b;
  this->m_RGBA[0][3] = a;  this->m_RGBA[1][3] = a;
}

// CMPoint with alpha discontinuity
ColorMap::CMPoint
::CMPoint(double j, EltType r, EltType g, EltType b, EltType a1, EltType a2)
{
  this->m_Index = j;
  this->m_Type = DISCONTINUOUS;

  this->m_RGBA[0][0] = r;  this->m_RGBA[1][0] = r;
  this->m_RGBA[0][1] = g;  this->m_RGBA[1][1] = g;
  this->m_RGBA[0][2] = b;  this->m_RGBA[1][2] = b;
  this->m_RGBA[0][3] = a1; this->m_RGBA[1][3] = a2;
}

// CMPoint with full discontinuity
ColorMap::CMPoint
::CMPoint(double j, 
    EltType r1, EltType g1, EltType b1, EltType a1,
    EltType r2, EltType g2, EltType b2, EltType a2)
{
  this->m_Index = j;
  this->m_Type = DISCONTINUOUS;

  this->m_RGBA[0][0] = r1; this->m_RGBA[1][0] = r2;
  this->m_RGBA[0][1] = g1; this->m_RGBA[1][1] = g2;
  this->m_RGBA[0][2] = b1; this->m_RGBA[1][2] = b2;
  this->m_RGBA[0][3] = a1; this->m_RGBA[1][3] = a2;
}

bool operator < (const ColorMap::CMPoint &p1, const ColorMap::CMPoint &p2)
{
  return p1.m_Index < p2.m_Index;
}

ColorMap::RGBAType
ColorMap::MapIndexToRGBA(double j) const
{
  // Perform binary search for the value j
  CMPoint p(j, 0x00, 0x00, 0x00, 0x00);
  CMPointConstIterator it = std::lower_bound(m_CMPoints.begin(), m_CMPoints.end(), p);

  // If this is the first point, return it's value 
  if(it == m_CMPoints.begin())
    return m_CMPoints.front().m_RGBA[0];

  // Same for the last point
  if(it == m_CMPoints.end())
    return m_CMPoints.back().m_RGBA[1];

  // Otherwise, there are two points to interpolate between
  CMPointConstIterator it0 = it; --it0;

  double dj = j - it0->m_Index;
  RGBAType rout;
  for(size_t i = 0; i < 4; i++)
    rout[i] = it0->m_RGBA[1][i] * (1.0 - dj) + it->m_RGBA[0][i] * dj;

  return rout;
}

void ColorMap::SetToSystemPreset(SystemPreset preset)
{
  m_CMPoints.clear();
  switch(preset)
  {
    case COLORMAP_GREY:
      m_CMPoints.push_back( CMPoint(0.0, 0x00, 0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0xff, 0xff, 0xff, 0xff, 0x00) );
      break;

    case COLORMAP_RED:
      m_CMPoints.push_back( CMPoint(0.0, 0x00, 0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0xff, 0x00, 0x00, 0xff, 0x00) );
      break;

    case COLORMAP_GREEN:
      m_CMPoints.push_back( CMPoint(0.0, 0x00, 0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0x00, 0xff, 0x00, 0xff, 0x00) );
      break;

    case COLORMAP_BLUE:
      m_CMPoints.push_back( CMPoint(0.0, 0x00, 0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0x00, 0x00, 0xff, 0xff, 0x00) );
      break;

    case COLORMAP_HOT:
      m_CMPoints.push_back( CMPoint(0.0        , 0x00, 0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint( 2.0 / 63.0, 0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(22.0 / 63.0, 0xd8, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(28.0 / 63.0, 0xff, 0x3a, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(48.0 / 63.0, 0xff, 0xff, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0        , 0xff, 0xff, 0xff, 0xff, 0x00) );
      break;

    case COLORMAP_COOL:
      m_CMPoints.push_back( CMPoint(0.0, 0xff, 0x00, 0xff, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0x00, 0xff, 0xff, 0xff, 0x00) );
      break;

    case COLORMAP_SPRING:
      m_CMPoints.push_back( CMPoint(0.0, 0xff, 0x00, 0xff, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0xff, 0xff, 0x00, 0xff, 0x00) );
      break;

    case COLORMAP_SUMMER:
      m_CMPoints.push_back( CMPoint(0.0, 0x00, 0x80, 0x66, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0xff, 0xff, 0x66, 0xff, 0x00) );
      break;

    case COLORMAP_AUTUMN:
      m_CMPoints.push_back( CMPoint(0.0, 0xff, 0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0xff, 0xff, 0x00, 0xff, 0x00) );
      break;

    case COLORMAP_WINTER:
      m_CMPoints.push_back( CMPoint(0.0, 0x00, 0x00, 0xff, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0x00, 0xff, 0x80, 0xff, 0x00) );
      break;

    case COLORMAP_COPPER:
      m_CMPoints.push_back( CMPoint(0.0,    0x00, 0x00, 0xff, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(0.8334, 0xff, 0xaa, 0x6a, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0,    0xff, 0xcc, 0x80, 0xff, 0x00) );
      break;

    case COLORMAP_HSV:
      m_CMPoints.push_back( CMPoint(0.0,    0xff, 0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(0.1667, 0xff, 0xff, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(0.3334, 0x00, 0xff, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(0.5000, 0x00, 0xff, 0xff, 0xff) );
      m_CMPoints.push_back( CMPoint(0.6667, 0x00, 0x00, 0xff, 0xff) );
      m_CMPoints.push_back( CMPoint(0.8334, 0xff, 0x00, 0xff, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0,    0xff, 0x00, 0x00, 0xff, 0x00) );
      break;

    case COLORMAP_JET:
      m_CMPoints.push_back( CMPoint(0.0,    0x00, 0x00, 0x80, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(0.1   , 0x00, 0x00, 0xff, 0xff) );
      m_CMPoints.push_back( CMPoint(0.36  , 0x00, 0xff, 0xff, 0xff) );
      m_CMPoints.push_back( CMPoint(0.6   , 0xff, 0xff, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(0.9   , 0xff, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0,    0x80, 0x00, 0x00, 0xff, 0x00) );
      break;
   }
}

