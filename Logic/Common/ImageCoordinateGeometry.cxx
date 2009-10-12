/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ImageCoordinateGeometry.cxx,v $
  Language:  C++
  Date:      $Date: 2009/10/12 19:05:56 $
  Version:   $Revision: 1.4 $
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
::ImageCoordinateGeometry(DirectionMatrix imageDirection,
                          std::string displayAnatomyRAICode[3],
                          const Vector3ui &imageSize)
{
  SetGeometry(imageDirection,displayAnatomyRAICode,imageSize);
}


void
ImageCoordinateGeometry
::SetGeometry(DirectionMatrix imageDirection,
              std::string displayAnatomyRAICode[3],
              const Vector3ui &imageSize)
{
  // Store the image direction matrix
  m_ImageDirectionCosineMatrix = imageDirection; 

  // Remap the direction matrix to an RAI code
  std::string imageAnatomyRAICode = 
    ConvertDirectionMatrixToClosestRAICode(m_ImageDirectionCosineMatrix);  

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


std::string 
ImageCoordinateGeometry
::ConvertDirectionMatrixToClosestRAICode(DirectionMatrix mat)
{
  // RAI codes for cardinal directions
  const static std::string rai_start("RAI"), rai_end("LPS");
  std::string rai_out("...");

  for(size_t i = 0; i < 3; i++)
    {
    // Get the direction of the i-th voxel coordinate
    vnl_vector<double> dir_i = mat.get_column(i);

    // Get the maximum angle with any axis
    double maxabs_i = dir_i.inf_norm();
    for(size_t off = 0; off < 3; off++)
      {
      // This trick allows us to visit (i,i) first, so that if one of the
      // direction cosines makes the same angle with two of the axes, we 
      // can still assign a valid RAI code
      size_t j = (i + off) % 3;

      // Is j the best-matching direction?
      if(fabs(dir_i[j]) == maxabs_i)
        {
        rai_out[i] = dir_i[j] > 0 ? rai_start[j] : rai_end[j];
        break;
        }
      }
    }
      
  return rai_out;  
}

bool
ImageCoordinateGeometry
::IsDirectionMatrixOblique(DirectionMatrix mat)
{
  for(size_t i = 0; i < 3; i++)
    for(size_t j = 0; j < 3; j++)
      if(fabs(mat[i][j]) > 0 && fabs(mat[i][j]) < 1)
        return true;
  return false;
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
