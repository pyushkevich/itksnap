/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: MeshIOWizardUILogic.h,v $
  Language:  C++
  Date:      $Date: 2007/05/10 20:19:51 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __MeshIOWizardUILogic_h_
#define __MeshIOWizardUILogic_h_

#include "FLTKWidgetActivationManager.h"
#include "MeshIOWizardUI.h"
#include "MeshExportSettings.h"
#include "GuidedMeshIO.h"
#include <string>
#include <vector>

class IRISApplication;

/**
 * \class MeshIOWizardUILogic
 * The UI logic class for the Mesh Export Wizard UI.
 */
class MeshIOWizardUILogic : public MeshIOWizardUI {
public:

  // History typedef
  typedef std::string StringType;
  typedef std::vector<StringType> HistoryType;

  // Dummy constructor and destructor
  MeshIOWizardUILogic();
  virtual ~MeshIOWizardUILogic() {};

  // Callback methods extended from the UIBase class
  void OnCancel();
  void OnFilePageBrowse();
  void OnFilePageNext();
  void OnFilePageFileInputChange();
  void OnFilePageFileHistoryChange();
  void OnFilePageFileFormatChange();
  void OnFilePageBack();
  void OnMeshPageNext();
  void OnMeshPageRadioChange();

  // Custom initialization code
  void MakeWindow();

  // Display the wizard and get the save settings
  bool DisplayWizard(IRISApplication *driver);

  /** Set the history list of recently opened files */
  void SetHistory(const HistoryType &history);

  // Set the history values for the wizard
  irisGetMacro(History, HistoryType);

  // Get the export settings
  irisGetMacro(ExportSettings, MeshExportSettings);

private:
  
  // State flag system
  enum UIStateFlags {
    UIF_NULL
  };

  // Activation manager
  FLTKWidgetActivationManager<UIStateFlags> m_Activation;

  // Parent-level IRIS application
  IRISApplication *m_Driver;

  // History of loaded filenames
  HistoryType m_History;

  // The settings collected by the wizard
  MeshExportSettings m_ExportSettings;

  // Whether a mesh has been loaded in the wizard
  bool m_MeshSelected;

  /** Extensions for different file formats */
  StringType m_FileFormatPattern[GuidedMeshIO::FORMAT_COUNT];

  /** Brief descriptions of different file formats */
  StringType m_FileFormatDescription[GuidedMeshIO::FORMAT_COUNT];  

  // Generate an FLTK search pattern based on selection
  StringType GetFilePattern();

  // Guess file format from a filename
  GuidedMeshIO::FileFormat DetermineFileFormatFromFileName(const char *testFile);

  // The registry associated with the mesh being saves
  Registry m_Registry;

  // A guided mesh IO object used to save meshes
  GuidedMeshIO m_GuidedIO;

  // An array used to link labels in the drop down to label indices
  std::vector<LabelType> m_ColorLabelMenuIndex;
};

#endif // __MeshIOWizardUILogic_h_

