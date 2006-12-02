/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: PreprocessingUIBase.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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
