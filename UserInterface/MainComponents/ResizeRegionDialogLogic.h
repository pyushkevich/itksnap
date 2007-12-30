/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ResizeRegionDialogLogic.h,v $
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
#ifndef __ResizeRegionDialogLogic_h_
#define __ResizeRegionDialogLogic_h_

#include "ResizeRegionDialog.h"
#include "SNAPSegmentationROISettings.h"

class ResizeRegionDialogLogic : public ResizeRegionDialog {
public:
    virtual ~ResizeRegionDialogLogic() {}
  // A list of scaling choices
  static const int NumberOfScaleChoices;
  static const int ScaleChoices[10][2];

  // Resampling method
  enum ResamplingMethod {
    NearestNeighbor, Linear
  };
  
  // Get the new spacing
  double GetSpacing(unsigned int dim) {
    return m_InSize[dim]->value();
  }

  // Get the selected resampling method
  ResamplingMethod GetResamplingMethod() {
    return (ResamplingMethod)(NearestNeighbor + m_InInterpolation->value());
  }

  // Callback functions
  void MakeWindow();
  bool DisplayDialog(const double *voxelSpacing,
                     SNAPSegmentationROISettings &targetROI);
  void OnVoxelSizeChange();
  void OnVoxelScaleChange();
  void OnOkAction();
  void OnCancelAction();

private:
  bool m_Accept;
};

#endif // __ResizeRegionDialogLogic_h_
