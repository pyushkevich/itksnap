/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ResizeRegionDialogLogic.cxx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:17 $
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
#include "ResizeRegionDialogLogic.h"

const int 
ResizeRegionDialogLogic::NumberOfScaleChoices = 10;

const int 
ResizeRegionDialogLogic::ScaleChoices[10][2] = 
  {{0,0},{5,1},{4,1},{3,1},{2,1},{1,1},{1,2},{1,3},{1,4},{1,5}};

void 
ResizeRegionDialogLogic
::MakeWindow()
{
  // Call parent method
  ResizeRegionDialog::MakeWindow();

  // Update the UI choices to defaults
  m_InScale[0]->value(5);
  m_InScale[1]->value(5);
  m_InScale[2]->value(5);
  m_InInterpolation->value(2);
}

bool 
ResizeRegionDialogLogic
::DisplayDialog(const double *voxelSpacing, 
                SNAPSegmentationROISettings &targetROI)
{
  // Set the input spacing values
  for(unsigned int i=0;i<3;i++)
    m_OutSize[i]->value(voxelSpacing[i]);

  // Compute the output voxel sizes based on the current scaling settings
  OnVoxelScaleChange();

  // Show the dialog
  m_WinResample->show();

  // Run until closed
  m_Accept = false;
  while(m_WinResample->shown())
    Fl::wait();

  // Compute the voxel scaling
  Vector3d voxelScaling(1.0);
  if(m_Accept)
    {
    voxelScaling[0] = GetSpacing(0) / voxelSpacing[0];
    voxelScaling[1] = GetSpacing(1) / voxelSpacing[1];
    voxelScaling[2] = GetSpacing(2) / voxelSpacing[2];
    }
  
  // Update the settings object
  targetROI.SetVoxelScale(voxelScaling);
  targetROI.SetResampleFlag(m_Accept);

  // Update the interpolation mode
  switch(m_InInterpolation->value()) 
    {
    case 0 : 
      targetROI.SetInterpolationMethod(
        SNAPSegmentationROISettings::NEAREST_NEIGHBOR);
      break;
    case 1 : 
      targetROI.SetInterpolationMethod(
        SNAPSegmentationROISettings::TRILINEAR);
      break;
    case 2 : 
      targetROI.SetInterpolationMethod(
        SNAPSegmentationROISettings::TRICUBIC);
      break;
    case 3 : 
      targetROI.SetInterpolationMethod(
        SNAPSegmentationROISettings::SINC_WINDOW_05);
      break;
    };

  // Return the accept flag
  return m_Accept;
}

void 
ResizeRegionDialogLogic
::OnVoxelScaleChange()
{
  // Set the output spacing values
  for(unsigned int i=0;i<3;i++)
    {
    // Get the numerator and denominator
    int choice = m_InScale[i]->value();
    int scaleFrom = ScaleChoices[choice][0];
    int scaleTo = ScaleChoices[choice][1];

    // If both scale choices are 0 and 0, i.e, custom mode, leave the values
    // unchanged
    if(scaleFrom == 0 && scaleTo == 0)
      continue;

    if(scaleFrom == 1)
      m_InSize[i]->value(scaleTo * m_OutSize[i]->value());
    else
      m_InSize[i]->value(m_OutSize[i]->value() / scaleFrom);
    }
}

void 
ResizeRegionDialogLogic
::OnVoxelSizeChange()
{
  // Set the output spacing values
  for(unsigned int i=0;i<3;i++)
    {
    int choice = 0; // Custom choice
    for(int j = 1;j < NumberOfScaleChoices;j++)
      {
      // Get the numerator and denominator
      int scaleFrom = ScaleChoices[j][0];
      int scaleTo = ScaleChoices[j][1];

      // See if the choice fits
      if(m_InSize[i]->value() * scaleFrom == m_OutSize[i]->value() * scaleTo)
        {
        choice = j;
        break;
        }
      }

    m_InScale[i]->value(choice);
    }
}

void
ResizeRegionDialogLogic
::OnOkAction()
{
  m_WinResample->hide();
  m_Accept = true;
}

void
ResizeRegionDialogLogic
::OnCancelAction()
{
  m_WinResample->hide();
  m_Accept = false;
}


