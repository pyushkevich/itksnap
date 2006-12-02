/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: ResizeRegionDialogBase.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __ResizeRegionDialogBase_h_
#define __ResizeRegionDialogBase_h_

class ResizeRegionDialogBase {
public:
    virtual ~ResizeRegionDialogBase() {}
  virtual void OnVoxelSizeChange() = 0;
  virtual void OnVoxelScaleChange() = 0;
  virtual void OnOkAction() = 0;
  virtual void OnCancelAction() = 0;
};

#endif // __ResizeRegionDialogBase_h_
