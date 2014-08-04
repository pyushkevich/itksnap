/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ColorMap.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
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
#include "IRISException.h"
#include <algorithm>
#include <cstdio>

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

bool
ColorMap::CMPoint
::operator!=(const ColorMap::CMPoint& rhs) const
{
  return !(*this == rhs);
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

  // Set up the enum map
  if(!m_ColorMapPresetEnumMap.Size())
    {
    for(int i = 0; i <= COLORMAP_CUSTOM; i++)
      {
      SystemPreset preset = static_cast<SystemPreset>(i);
      m_ColorMapPresetEnumMap.AddPair(preset, this->GetPresetName(preset));
      }
    }
}

ColorMap
::~ColorMap()
{
}

RegistryEnumMap<ColorMap::SystemPreset> ColorMap::m_ColorMapPresetEnumMap;

bool
ColorMap
::operator==(const ColorMap& rhs) const
{
  // Compare two colormaps for equality
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

size_t
ColorMap
::InsertInterpolatedCMPoint(double j)
{
  // Create the new point
  RGBAType rgba = MapIndexToRGBA(j);
  CMPoint newbie(j, rgba[0], rgba[1], rgba[2], rgba[3]);

  // Find where to insert the new point
  CMPointIterator it = 
    std::lower_bound(m_CMPoints.begin(), m_CMPoints.end(), newbie);

  // Insert the point after the lower bound
  CMPointIterator itnew = m_CMPoints.insert(it, newbie);

  this->UpdateInterpolants(); 

  m_CMPreset = COLORMAP_CUSTOM;

  return itnew - m_CMPoints.begin();
}


void
ColorMap
::UpdateInterpolants()
{
  // Set the size of the interpolant array to n+1
  size_t n = m_CMPoints.size();
  m_Interpolants.resize(n+1);

  // Loop over the component
  for(unsigned int d = 0; d < 4; d++)
    {
    // Set first and last interpolants
    m_Interpolants[0].slope[d] = 0.0f; 
    m_Interpolants[0].intercept[d] = m_CMPoints[0].m_RGBA[0][d];
    m_Interpolants[n].slope[d] = 0.0f; 
    m_Interpolants[n].intercept[d] = m_CMPoints[n-1].m_RGBA[1][d];

    // Set the intermediate interpolants
    for(size_t i = 1; i < n; i++)
      {
      float t0 = m_CMPoints[i-1].m_Index;
      float t1 = m_CMPoints[i].m_Index;
      float c0 = m_CMPoints[i-1].m_RGBA[1][d];
      float c1 = m_CMPoints[i].m_RGBA[0][d];
      if(t1 > t0)
        {
        m_Interpolants[i].slope[d] = (c1 - c0) / (t1 - t0);
        m_Interpolants[i].intercept[d] = c0 - m_Interpolants[i].slope[d] * t0;
        }
      else
        {
        m_Interpolants[i].slope[d] = 0;
        m_Interpolants[i].intercept[d] = c0;
        }
      }
    }

  // Update state
  this->Modified();
}

ColorMap::RGBAType
ColorMap
::MapIndexToRGBA(double j) const
{
  // We use simple linear search because most colormaps
  // are tiny and the overhead of binary search is not worth it
  size_t n = m_CMPoints.size(), lb;
  for(lb = 0; lb < n; lb++)
    {
    double t = m_CMPoints[lb].m_Index;

    // The right side of this conditional expression is the special case when
    // j equals to the index in the last control point. In that case we want
    // to use the left-side value of the last control point, not the right
    // hand side. Otherwise, the maximum value in the image is treated as
    // being "outside" of the colormap, and for most colormaps is assigned
    // a transparent value. That's undesirable
    if(j < t || (lb == (n-1) && j == t))
      break;
    }

  // Get the interpolants
  const InterpolantData &ic = m_Interpolants[lb];

  // Compute the output value
  RGBAType c;
  c[0] = (unsigned char)(ic.intercept[0] + ic.slope[0] * j);
  c[1] = (unsigned char)(ic.intercept[1] + ic.slope[1] * j);
  c[2] = (unsigned char)(ic.intercept[2] + ic.slope[2] * j);
  c[3] = (unsigned char)(ic.intercept[3] + ic.slope[3] * j);
  return c;
}

void
ColorMap
::PrintSelf(std::ostream & os, itk::Indent indent) const
{
  char buffer[256];
  for(size_t i = 0; i < m_CMPoints.size(); i++)
    {
    CMPoint p = m_CMPoints[i];
    if(p.m_Type == CONTINUOUS)
      {
      sprintf(buffer,
              "%02d-C %7.2f   (%03d %03d %03d %03d)\n", (int) i, p.m_Index, p.m_RGBA[0][0], p.m_RGBA[0][1], p.m_RGBA[0][2], p.m_RGBA[0][3]);
      os << indent << buffer;
      }
    else
      {
      sprintf(buffer,
             "%02d-L %7.2f   (%03d %03d %03d %03d)\n", (int) i, p.m_Index, p.m_RGBA[0][0], p.m_RGBA[0][1], p.m_RGBA[0][2], p.m_RGBA[0][3]);
      os << indent << buffer;
      sprintf(buffer,
              "%02d-R %7.2f   (%03d %03d %03d %03d)\n", (int) i, p.m_Index, p.m_RGBA[1][0], p.m_RGBA[1][1], p.m_RGBA[1][2], p.m_RGBA[1][3]);
      os << indent << buffer;
      }
    }
}
void
ColorMap
::SetToSystemPreset(SystemPreset preset)
{
  m_CMPreset = preset;

  // Handle the special case of COLORMAP_CUSTOM, in which case we don't mess
  // mess with the colormap
  if(preset == COLORMAP_CUSTOM)
    return;

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

    case COLORMAP_BWR:
      m_CMPoints.push_back( CMPoint(0.0,    0x00, 0x00, 0xff, 0x00, 0xff) ); 
      m_CMPoints.push_back( CMPoint(0.5,    0xff, 0xff, 0xff, 0xff) ); 
      m_CMPoints.push_back( CMPoint(1.0,    0xff, 0x00, 0x00, 0xff, 0x00) );
      break;

    case COLORMAP_RWB:
      m_CMPoints.push_back( CMPoint(0.0,    0xff, 0x00, 0x00, 0x00, 0xff) ); 
      m_CMPoints.push_back( CMPoint(0.5,    0xff, 0xff, 0xff, 0xff) ); 
      m_CMPoints.push_back( CMPoint(1.0,    0x00, 0x00, 0xff, 0xff, 0x00) );
      break;

    case COLORMAP_SPEED:
      m_CMPoints.push_back( CMPoint(0.0,    0x00, 0x00, 0xff, 0xff) );
      m_CMPoints.push_back( CMPoint(0.5,    0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0,    0xff, 0xff, 0xff, 0xff) );
      break;

    case COLORMAP_SPEED_OVERLAY:
      m_CMPoints.push_back( CMPoint(0.0,    0xff, 0xff, 0xff, 0x00) );
      m_CMPoints.push_back( CMPoint(0.4,    0xff, 0xff, 0xff, 0x00) );
      m_CMPoints.push_back( CMPoint(0.6,    0xff, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0,    0xff, 0x00, 0x00, 0xff) );
      break;

    case COLORMAP_LEVELSET:
      m_CMPoints.push_back( CMPoint(0.0,    0xff, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(0.4,    0xff, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(0.6,    0xff, 0xff, 0xff, 0x00) );
      m_CMPoints.push_back( CMPoint(1.0,    0xff, 0xff, 0xff, 0x00) );

    case COLORMAP_CUSTOM:
      break;
   }

  this->UpdateInterpolants();
}

void ColorMap::UpdateCMPoint(size_t j, const ColorMap::CMPoint &p)
{
  if(m_CMPoints[j] != p)
    {
    m_CMPoints[j] = p;
    this->UpdateInterpolants();
    m_CMPreset = COLORMAP_CUSTOM;
    }
}

void ColorMap::DeleteCMPoint(size_t j)
{
  m_CMPoints.erase(m_CMPoints.begin() + j);
  this->UpdateInterpolants();
  m_CMPreset = COLORMAP_CUSTOM;
}


void ColorMap
::SaveToRegistry(Registry &reg)
{
  // Save what system preset we are using (if any)
  reg.Entry("Preset").PutEnum(m_ColorMapPresetEnumMap, m_CMPreset);

  // For custom colormaps, save them
  if(m_CMPreset == COLORMAP_CUSTOM)
    {
    // Store the number of control points
    reg["NumberOfControlPoints"] << m_CMPoints.size();

    RegistryEnumMap<CMPointType> emap;
    emap.AddPair(CONTINUOUS,"Continuous");
    emap.AddPair(DISCONTINUOUS,"Discontinuous");

    // Save each control point
    for(size_t iPoint = 0; iPoint < m_CMPoints.size(); iPoint++)
      {
      // Get the current values, just in case
      CMPoint p = m_CMPoints[iPoint];

      // Create a folder in the registry
      std::string key = reg.Key("ControlPoint[%04d]",iPoint);
      Registry &folder = reg.Folder(key);
      folder["Index"] << p.m_Index;
      folder["Type"].PutEnum(emap, p.m_Type);
      folder["Left.R"] << (int) p.m_RGBA[0][0];
      folder["Left.G"] << (int) p.m_RGBA[0][1];
      folder["Left.B"] << (int) p.m_RGBA[0][2];
      folder["Left.A"] << (int) p.m_RGBA[0][3];
      folder["Right.R"] << (int) p.m_RGBA[1][0];
      folder["Right.G"] << (int) p.m_RGBA[1][1];
      folder["Right.B"] << (int) p.m_RGBA[1][2];
      folder["Right.A"] << (int) p.m_RGBA[1][3];
      }
    }
}

// TODO: the colormap should know if it is in a given system preset or not, and if
// so, only save the preset name.
void ColorMap
::LoadFromRegistry(Registry &reg)
{
  // See what system preset we are using (if any)
  SystemPreset preset =
      reg.Entry("Preset").GetEnum(m_ColorMapPresetEnumMap, COLORMAP_GREY);

  // If system preset, apply it and we're done
  if(preset != COLORMAP_CUSTOM)
    {
    SetToSystemPreset(preset);
    }
  else
    {
    // Store the number of control points
    size_t n = reg["NumberOfControlPoints"][0];
    if(n == 0)
      return;

    // Read each control point
    CMPointList newpts;

    RegistryEnumMap<CMPointType> emap;
    emap.AddPair(CONTINUOUS,"Continuous");
    emap.AddPair(DISCONTINUOUS,"Discontinuous");

    // Save each control point
    for(size_t iPoint = 0; iPoint < n; iPoint++)
      {
      // Get the current values, just in case
      CMPoint p;

      // Create a folder in the registry
      std::string key = reg.Key("ControlPoint[%04d]",iPoint);
      Registry &folder = reg.Folder(key);

      p.m_Index = folder["Index"][-1.0];
      p.m_Type = folder["Type"].GetEnum(emap, CONTINUOUS);
      p.m_RGBA[0][0] = (unsigned char) folder["Left.R"][0];
      p.m_RGBA[0][1] = (unsigned char) folder["Left.G"][0];
      p.m_RGBA[0][2] = (unsigned char) folder["Left.B"][0];
      p.m_RGBA[0][3] = (unsigned char) folder["Left.A"][0];
      if(p.m_Type == CONTINUOUS)
        {
        p.m_RGBA[1][0] = p.m_RGBA[0][0];
        p.m_RGBA[1][1] = p.m_RGBA[0][1];
        p.m_RGBA[1][2] = p.m_RGBA[0][2];
        p.m_RGBA[1][3] = p.m_RGBA[0][3];
        }
      else
        {
        p.m_RGBA[1][0] = (unsigned char) folder["Right.R"][0];
        p.m_RGBA[1][1] = (unsigned char) folder["Right.G"][0];
        p.m_RGBA[1][2] = (unsigned char) folder["Right.B"][0];
        p.m_RGBA[1][3] = (unsigned char) folder["Right.A"][0];
        }

      // Check validity
      if(iPoint == 0 && p.m_Index != 0.0)
        throw IRISException("Can not read color map. First point has non-zero index.");

      if(iPoint == n-1 && p.m_Index != 1.0)
        throw IRISException("Can not read color map. Last point has index not equal to 1.");

      if(iPoint > 0 && p.m_Index < newpts[iPoint-1].m_Index)
        throw IRISException("Can not read color map. Indices are not stored in order.");

      newpts.push_back(p);
      }

    // Got this far? store the new map
    m_CMPoints = newpts;
    this->UpdateInterpolants();

    m_CMPreset = COLORMAP_CUSTOM;
    }
 }

void ColorMap::CopyInformation(const itk::DataObject *source)
{
  const ColorMap *cm_source = dynamic_cast<const ColorMap *>(source);
  m_CMPreset = cm_source->m_CMPreset;
  m_CMPoints = cm_source->m_CMPoints;
  m_Interpolants = cm_source->m_Interpolants;
  this->Modified();
}

 const char * ColorMap::GetPresetName(ColorMap::SystemPreset preset)
 {
   static const char *preset_names[] = {
     "Grayscale",
     "Jet",
     "Hot",
     "Cool",
     "Black to red",
     "Black to green",
     "Black to blue",
     "Spring",
     "Summer",
     "Autumn",
     "Winter",
     "Copper",
     "HSV",
     "Blue to white to red",
     "Red to white to blue",
     "Speed image (blue to black to white)",
     "Speed image (semi-transparent overlay)",
     "Level set image",
     "Custom"
   };

   return preset_names[preset];
 }
