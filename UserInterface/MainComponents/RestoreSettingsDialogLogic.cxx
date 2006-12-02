/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: RestoreSettingsDialogLogic.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "RestoreSettingsDialogLogic.h"
#include "UserInterfaceBase.h"
#include "SystemInterface.h"

#include <vector>

// Disable some windows debug length messages
#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4503 )
#endif

using namespace std;

void 
RestoreSettingsDialogLogic
::DisplayDialog(UserInterfaceBase *parent, Registry *associatedSettings)
{
  // Remember the code and the system interface
  m_SystemInterface = parent->GetSystemInterface();
  m_AssociatedSettings = associatedSettings;

  // Clear this flag
  m_AssociatedSettingsHaveChanged = false;
  
  // Determine whether the options should be displayed at all
  Registry *folder =  
    &m_SystemInterface->Folder("ImageAssociation.RestoreOptions.Generic");
  bool noprompt = folder->Entry("DoNotPrompt")[false];

  // If the generic setting is to prompt, check for a specific setting
  if(!noprompt) 
    {
    folder = &m_AssociatedSettings->Folder("RestoreOptions");
    noprompt = folder->Entry("DoNotPrompt")[false]; 
    }
  
  // Now the noprompt and key combination should reflect either the generic or 
  // the specific state
  if(noprompt)
    {
    // The user doesn't want to be prompted.  Load the settings
    m_RestoreSettings = folder->Entry("RestoreAny")[true];
    m_RestoreLabels = folder->Entry("RestoreLabels")[true];
    m_RestoreParameters = folder->Entry("RestoreParameters")[true];
    m_RestorePreprocessing = folder->Entry("RestorePreprocessing")[true];
    m_RestoreDisplayOptions = folder->Entry("RestoreDisplayOptions")[true];
    }
  else
    {
    // Show the dialog and let the user specify what features to restore
    parent->CenterChildWindowInMainWindow(m_WinMain);
    m_WinMain->show(); 
    while(m_WinMain->shown()) Fl::wait();
    }
}

void 
RestoreSettingsDialogLogic
::OnRestoreSettingsAction()
{
  // Set the current state
  m_RestoreSettings = true;
  m_RestoreLabels = m_ChkLabels->value() != 0;
  m_RestorePreprocessing = m_ChkPreprocessing->value() != 0;
  m_RestoreParameters = m_ChkParameters->value() != 0;
  m_RestoreDisplayOptions = m_ChkDisplayOptions->value() != 0;
  
  // Save for the future
  SaveDefaultSettingsForFutureIfRequested();
  
  // Hide the window
  m_WinMain->hide();
}

void 
RestoreSettingsDialogLogic
::OnDoNotRestoreSettingsAction()
{
  // Set the current state
  m_RestoreSettings = false;
  
  // Save for the future
  SaveDefaultSettingsForFutureIfRequested();
  
  // Hide the window
  m_WinMain->hide();
}

void 
RestoreSettingsDialogLogic
::OnCancelAction()
{
  // Hide the window
  m_WinMain->hide();
}

void 
RestoreSettingsDialogLogic
::SaveDefaultSettingsForFutureIfRequested()
{
  // If the user specified a future action, record that he/she does not
  // want to be bothered with this question again
  unsigned int future = m_InFutureApproach->value();
  if(!future) return;

  // Get a folder into which the settings should be dumped
  Registry *folder = (future == 2) ?
    &m_SystemInterface->Folder("ImageAssociation.RestoreOptions.Generic") : 
    &m_AssociatedSettings->Folder("RestoreOptions");

  // Store the current settings as associated with the key
  folder->Entry("DoNotPrompt") << true;
  folder->Entry("RestoreAny") << m_RestoreSettings;

  // Store the individual settings 
  folder->Entry("RestoreLabels") << m_RestoreLabels;
  folder->Entry("RestoreParameters") << m_RestoreParameters;
  folder->Entry("RestorePreprocessing") << m_RestorePreprocessing;
  folder->Entry("RestoreDisplayOptions") << m_RestoreDisplayOptions;

  // Set this flag
  if(future == 1)
    m_AssociatedSettingsHaveChanged = true;
}
