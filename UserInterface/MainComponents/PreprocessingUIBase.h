/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: PreprocessingUIBase.h,v $
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
#ifndef __PreprocessingUIBase_h_
#define __PreprocessingUIBase_h_

#include "FunctionPlot2DBox.h"

/**
 * \class PreprocessingUIBase
 * \brief Base class for preprocessing UI.
 */
class PreprocessingUIBase
{
public:
  virtual ~PreprocessingUIBase() {}
  // Callbacks for the Edge snake preprocessing window
  virtual void OnEdgeSettingsChange() = 0;
  virtual void OnEdgePreviewChange() = 0;
  virtual void OnEdgeOk() = 0;
  virtual void OnEdgeClose() = 0;
  virtual void OnEdgeApply() = 0;

  // Callbacks for the InOut snake preprocessing window
  virtual void OnThresholdDirectionChange() = 0;
  virtual void OnThresholdLowerChange(double value) = 0;
  virtual void OnThresholdUpperChange(double value) = 0;
  virtual void OnThresholdSettingsChange() = 0;
  virtual void OnThresholdOk() = 0;
  virtual void OnThresholdClose() = 0;
  virtual void OnThresholdApply() = 0;
  virtual void OnThresholdPreviewChange() = 0;
  virtual void OnThresholdOverlayChange() = 0;
};

#endif
