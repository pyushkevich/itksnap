/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ColorMap.cxx,v $
  Language:  C++
  Date:      $Date: 2009/09/12 23:27:57 $
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
#include "ColorMap.h"
#include <algorithm>

bool
ColorMap::CMPoint
::operator==(const ColorMap::CMPoint& rhs) const
{
  if (m_Index != rhs.m_Index)
    return false;
  if (m_Type != rhs.m_Type)
    return false;
  for (int i = 0; i < 2; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      if (m_RGBA[i][j] != rhs.m_RGBA[i][j])
        return false;
      }
    }
  return true;
}

ColorMap::CMPoint
::CMPoint()
{
  m_Index = 0.0;
  m_Type = CONTINUOUS;

  m_RGBA[0][0] = 0;  m_RGBA[1][0] = 0;
  m_RGBA[0][1] = 0;  m_RGBA[1][1] = 0;
  m_RGBA[0][2] = 0;  m_RGBA[1][2] = 0;
  m_RGBA[0][3] = 0;  m_RGBA[1][3] = 0;
  }

// Continuous point
ColorMap::CMPoint
::CMPoint(double j, EltType r, EltType g, EltType b, EltType a)
{
  m_Index = j;
  m_Type = CONTINUOUS;

  m_RGBA[0][0] = r;  m_RGBA[1][0] = r;
  m_RGBA[0][1] = g;  m_RGBA[1][1] = g;
  m_RGBA[0][2] = b;  m_RGBA[1][2] = b;
  m_RGBA[0][3] = a;  m_RGBA[1][3] = a;
}

// CMPoint with alpha discontinuity
ColorMap::CMPoint
::CMPoint(double j, EltType r, EltType g, EltType b, EltType a1, EltType a2)
{
  m_Index = j;
  m_Type = DISCONTINUOUS;

  m_RGBA[0][0] = r;  m_RGBA[1][0] = r;
  m_RGBA[0][1] = g;  m_RGBA[1][1] = g;
  m_RGBA[0][2] = b;  m_RGBA[1][2] = b;
  m_RGBA[0][3] = a1; m_RGBA[1][3] = a2;
}

// CMPoint with full discontinuity
ColorMap::CMPoint
::CMPoint(double j, 
    EltType r1, EltType g1, EltType b1, EltType a1,
    EltType r2, EltType g2, EltType b2, EltType a2)
{
  m_Index = j;
  m_Type = DISCONTINUOUS;

  m_RGBA[0][0] = r1; m_RGBA[1][0] = r2;
  m_RGBA[0][1] = g1; m_RGBA[1][1] = g2;
  m_RGBA[0][2] = b1; m_RGBA[1][2] = b2;
  m_RGBA[0][3] = a1; m_RGBA[1][3] = a2;
}

// CMPoint copy constructor
ColorMap::CMPoint
::CMPoint(const CMPoint& rhs)
{
  m_Index = rhs.m_Index;
  m_Type = rhs.m_Type;
  for (int i = 0; i < 2; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      m_RGBA[i][j] = rhs.m_RGBA[i][j];
      }
    }
}

bool operator < (const ColorMap::CMPoint &p1, const ColorMap::CMPoint &p2)
{
  return p1.m_Index < p2.m_Index;
}

ColorMap
::ColorMap()
{
  SetToSystemPreset(COLORMAP_GREY);
}

ColorMap
::ColorMap(SystemPreset preset)
{
  SetToSystemPreset(preset);
}

ColorMap
::ColorMap(const ColorMap& rhs)
{
  CMPointConstIterator it = rhs.m_CMPoints.begin();
  m_CMPreset = rhs.m_CMPreset;
  while (it != rhs.m_CMPoints.end())
    {
    m_CMPoints.push_back( *it );
    ++it;
    }
}

bool
ColorMap
::operator==(const ColorMap& rhs) const
{
  if (m_CMPoints.size() != rhs.m_CMPoints.size())
    return false;
  CMPointConstIterator it = m_CMPoints.begin();
  CMPointConstIterator rit = rhs.m_CMPoints.begin();
  while (it != m_CMPoints.end())
    {
    if (!(*it == *rit))
      return false;
    ++it;
    ++rit;
    }
  return true;
}

ColorMap::RGBAType
ColorMap
::MapIndexToRGBA(double j) const
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
    rout[i] = (unsigned char)(it0->m_RGBA[1][i] * (1.0 - dj) + it->m_RGBA[0][i] * dj);

  return rout;
}

void
ColorMap
::SetToSystemPreset(SystemPreset preset)
{
  m_CMPoints.clear();
  m_CMPreset = preset;
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
    case COLORMAP_SIZE:
      // to suppress compiler warning
      std::cerr << "COLORMAP_SIZE: should never get there ..." << std::endl;
      break;
   }
}

