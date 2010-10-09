/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: AppearanceDialogUIBase.h,v $
  Language:  C++
  Date:      $Date: 2010/10/09 04:20:08 $
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
#ifndef __AppearanceDialogUIBase_h_
#define __AppearanceDialogUIBase_h_

class AppearanceDialogUIBase
{
public:
  virtual ~AppearanceDialogUIBase() {}
  virtual void OnUIElementUpdate() = 0;
  virtual void OnUIElementSelection(int value) = 0;
  virtual void OnSliceAnatomyOptionsChange(unsigned int order) = 0;
  virtual void OnCloseAction() = 0;
  virtual void OnExportAction() = 0;
  virtual void OnImportAction() = 0;

  virtual void OnSliceDisplayApplyAction() = 0;
  virtual void OnSliceDisplayResetAction() = 0;
  virtual void OnScreenLayoutApplyAction() = 0;
  virtual void OnScreenLayoutResetAction() = 0;
  virtual void On3DRenderingApplyAction() = 0;
  virtual void On3DRenderingResetAction() = 0;
  virtual void OnElementAppearanceResetAllAction() = 0;
  virtual void OnElementAppearanceResetAction() = 0;
  virtual void OnElementAppearanceApplyAction() = 0;

  virtual void OnOptionsExternalUpdate() = 0;
  virtual void OnHideOverlaysAction() = 0;
};

#endif

