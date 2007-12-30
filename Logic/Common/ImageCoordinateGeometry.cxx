/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ImageCoordinateGeometry.cxx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:12 $
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
#include "ImageCoordinateGeometry.h"

ImageCoordinateGeometry
::ImageCoordinateGeometry()
{
  // By default all transforms are identity.  Nothing to do here
}

ImageCoordinateGeometry
::ImageCoordinateGeometry(std::string imageAnatomyRAICode,
                          std::string displayAnatomyRAICode[3],
                          const Vector3ui &imageSize)
{
  SetGeometry(imageAnatomyRAICode,displayAnatomyRAICode,imageSize);
}


void
ImageCoordinateGeometry
::SetGeometry(std::string imageAnatomyRAICode,
              std::string displayAnatomyRAICode[3],
              const Vector3ui &imageSize)
{
  // Make sure the RAI codes are valid
  assert(IsRAICodeValid(imageAnatomyRAICode.c_str()));
  
    
  // We can easily compute the image to anatomy transform
  m_ImageToAnatomyTransform.SetTransform(
    ConvertRAIToCoordinateMapping(imageAnatomyRAICode.c_str()),imageSize);

  // Compute the size of the anatomy image in its coordinates
  Vector3ui szAnatomy = m_ImageToAnatomyTransform.TransformSize(imageSize);

  // Compute the anatomy to display transform
  for(unsigned int slice=0;slice < 3;slice++)
    {
    // Make sure the code is valid
    assert(IsRAICodeValid(displayAnatomyRAICode[slice].c_str()));
    
    m_AnatomyToDisplayTransform[slice].SetTransform(
      InvertMappingVector(
        ConvertRAIToCoordinateMapping(displayAnatomyRAICode[slice].c_str())),
      szAnatomy);

    // Compute the combined transform
    m_ImageToDisplayTransform[slice] = 
      m_AnatomyToDisplayTransform[slice].Product(m_ImageToAnatomyTransform);
  
    // Compute the opposite direction transform
    m_DisplayToImageTransform[slice] = 
      m_ImageToDisplayTransform[slice].Inverse();
    }
}


bool 
ImageCoordinateGeometry
::IsRAICodeValid(const char *rai)
{
  if(rai == NULL)
    return false;

  // Check the validity of the RAI code - no repetition
  // is allowed
  bool rl = false,ap = false,is = false;
  for (int i=0;i<3;i++) 
    {
    switch (rai[i]) 
      {
      case 'r' :
      case 'R' :
      case 'l' :
      case 'L' :
        rl = true;break;
      case 'a' :
      case 'A' :
      case 'p' :
      case 'P' :
        ap = true;break;
      case 'i' :
      case 'I' :
      case 's' :
      case 'S' :
        is = true;break;
      default:
        return false;
      }
    }

  // All three should have been encountered!
  return rl && ap && is;
}

Vector3i
ImageCoordinateGeometry
::ConvertRAIToCoordinateMapping(const char *rai)
{
  assert(IsRAICodeValid(rai));
  
  Vector3i result;

  for(unsigned int i=0;i<3;i++)
    {
    switch(rai[i]) 
      {
      case 'r' :
      case 'R' : result[i] =  1;break;
      case 'l' :
      case 'L' : result[i] = -1;break;
      case 'a' :
      case 'A' : result[i] =  2;break;
      case 'p' :
      case 'P' : result[i] = -2;break;
      case 'i' :
      case 'I' : result[i] =  3;break;
      case 's' :
      case 'S' : result[i] = -3;break;
      default: assert(0);
      }
    }

  return result;
}

Vector3i
ImageCoordinateGeometry
::InvertMappingVector(const Vector3i &map)
{
  Vector3i inv;

  for(int i=0;i<3;i++)
    {
    if(map[i] > 0)
      inv[map[i]-1] = i+1;
    else
      inv[-1-map[i]] = -i-1;
    }

  return inv;
}
