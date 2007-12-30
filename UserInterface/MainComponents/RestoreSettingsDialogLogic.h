/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: RestoreSettingsDialogLogic.h,v $
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
#ifndef __RestoreSettingsDialogLogic_h_
#define __RestoreSettingsDialogLogic_h_

#include "Registry.h"
#include "SNAPCommonUI.h"
#include "RestoreSettingsDialog.h"

class SystemInterface;
class UserInterfaceBase;

/**
 * \class RestoreSettingsDialogLogic
 * \brief A dialog used to prompt the user whether to restore settings associated
 * with an image or not.
 */
class RestoreSettingsDialogLogic : public RestoreSettingsDialog {
public: 
  RestoreSettingsDialogLogic() {};
  virtual ~RestoreSettingsDialogLogic() {};

  /** Optionally display this dialog based on the options specified in
   * the system settings.  The caller can prompt whether the user wanted
   * to restore settings, and which settings the user wanted to restore
   * using the GetXXX methods in this class.
   *
   * The first parameter is the pointer to the \see SystemInterface object.
   * The second parameter is the 16 digit code used to refer to the image
   * in the system interface registry */
  void DisplayDialog(UserInterfaceBase *parent, Registry *associatedSettings);    

  /** Did the user want to restore settings */
  irisGetMacro(RestoreSettings,bool);
  
  /** Did the user want to restore segmentation labels */
  irisGetMacro(RestoreLabels,bool);
  
  /** Did the user want to restore preprocessing settings */
  irisGetMacro(RestorePreprocessing,bool);
  
  /** Did the user want to restore segmentation parameters */
  irisGetMacro(RestoreParameters,bool);
  
  /** Did the user want to restore display options */
  irisGetMacro(RestoreDisplayOptions,bool);

  /** Has the dialog changed the AssociatedSettings ? */
  irisGetMacro(AssociatedSettingsHaveChanged,bool);
  
  // User interface callbacks
  void OnRestoreSettingsAction();
  void OnDoNotRestoreSettingsAction();
  void OnCancelAction();
 
private:
  // The system interface pointer
  SystemInterface *m_SystemInterface;

  // Settings associated with the current image
  Registry *m_AssociatedSettings;

  // Settings from the restore state
  bool m_RestoreSettings;
  bool m_RestoreLabels;
  bool m_RestorePreprocessing;
  bool m_RestoreParameters;
  bool m_RestoreDisplayOptions;

  // Whether or not we updated the AssociatedSettings
  bool m_AssociatedSettingsHaveChanged;

  // Method that saves the settings for the future on user request
  void SaveDefaultSettingsForFutureIfRequested();
};

#endif // __RestoreSettingsDialogLogic_h_
