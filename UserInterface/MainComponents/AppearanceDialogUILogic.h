/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: AppearanceDialogUILogic.h,v $
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
#ifndef __AppearanceDialogUILogic_h_
#define __AppearanceDialogUILogic_h_

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#include "AppearanceDialogUI.h"

class UserInterfaceBase;
class SNAPAppearanceSettings;
class GlobalState;

class AppearanceDialogUILogic : public AppearanceDialogUI
{
public:
  AppearanceDialogUILogic();
  virtual ~AppearanceDialogUILogic();

  void Register( UserInterfaceBase *parent );

  /* Show the dialog */
  void ShowDialog();
  
  // Close callbacks
  void OnCloseAction() ;

  // Import/export
  void OnImportAction();
  void OnExportAction();

  // General slice options
  void OnSliceDisplayApplyAction();
  void OnSliceDisplayResetAction();

  // View arrangement callbacks
  void OnScreenLayoutApplyAction();
  void OnScreenLayoutResetAction();
  void OnSliceAnatomyOptionsChange(unsigned int order) ;

  // 3D Rendering callbacks
  void On3DRenderingApplyAction();
  void On3DRenderingResetAction();

  // Element appearance callbacks
  void OnUIElementUpdate() ;
  void OnUIElementSelection(int value) ;
  void OnElementAppearanceResetAllAction();
  void OnElementAppearanceResetAction();
  void OnElementAppearanceApplyAction();


  void OnOptionsExternalUpdate();
  void OnHideOverlaysAction();

private:
  // Referece to the parent
  UserInterfaceBase *m_Parent;

  // Pointer to the appearance settings, from parent.
  SNAPAppearanceSettings *m_Appearance;

  // Default appearance settings
  SNAPAppearanceSettings *m_DefaultAppearance;

  // global state pointer
  GlobalState *m_GlobalState;

  // Fill some global options
  void FillSliceLayoutOptions();
  void FillRenderingOptions();
  void FillAppearanceSettings();

  // Apply the global options
  void ApplyRenderingOptions();
  void ApplySliceLayoutOptions();
  void ApplyAppearanceSettings();

  // A mapping from SNAP appearance elements to menu indices
  const static int m_MapMenuToElementIndex[];
};

#endif
