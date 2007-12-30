/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPSegmentationROISettings.cxx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:13 $
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
#include "SNAPSegmentationROISettings.h"

bool 
SNAPSegmentationROISettings
::TransformImageVoxelToROIVoxel(const Vector3ui &vImage, Vector3ui &vROI)
{
  itk::Index<3> idx = to_itkIndex(vImage);
  if(m_ROI.IsInside(idx)) 
  {
    for(unsigned int i = 0; i < 3; i++)
    {
      unsigned int p = vImage[i] - m_ROI.GetIndex()[i];
      vROI[i] = (unsigned int) (p / m_VoxelScale[i]);
    }
    return true;
  }
  else return false;
}

void
SNAPSegmentationROISettings
::TransformROIVoxelToImageVoxel(const Vector3ui &vROI, Vector3ui &vImage)
{
  for(unsigned int i = 0; i < 3; i++)
  {
    unsigned int p = (unsigned int) (vROI[i] * m_VoxelScale[i]);
    vImage[i] = p + m_ROI.GetIndex()[i];
  }
}
