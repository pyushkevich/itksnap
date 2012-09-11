/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: MeshIOWizardUILogic.cxx,v $
  Language:  C++
  Date:      $Date: 2011/04/18 17:35:30 $
  Version:   $Revision: 1.10 $
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
#include "MeshIOWizardUILogic.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "GuidedMeshIO.h"
#include "FL/Fl_Native_File_Chooser.H"
#include "FL/filename.H"
#include "FL/fl_ask.H"

using namespace std;

MeshIOWizardUILogic
::MeshIOWizardUILogic()
{
  m_MeshSelected = false;
  m_Driver = NULL;

  // Initialize the file format database
  m_FileFormatPattern[GuidedMeshIO::FORMAT_VTK] = "vtk";
  m_FileFormatPattern[GuidedMeshIO::FORMAT_STL] = "stl";
  m_FileFormatPattern[GuidedMeshIO::FORMAT_BYU] = "byu,y";

  m_FileFormatDescription[GuidedMeshIO::FORMAT_VTK] = "VTK PolyData File";
  m_FileFormatDescription[GuidedMeshIO::FORMAT_STL] = "STL Mesh File";
  m_FileFormatDescription[GuidedMeshIO::FORMAT_BYU] = "BYU Mesh File";

}

string
MeshIOWizardUILogic
::GetFilePattern() 
{
  // String containing the whole patterns
  StringType pattern = "";
  bool patternNeedsNewline = false;

  // String containing the "All Image Files" pattern
  StringType allImageFiles = "All Mesh Files\t*.{"; 
  bool allImageFilesNeedsComma = false;

  // Go through all supported formats
  for(size_t i = 0; i < GuidedMeshIO::FORMAT_COUNT; i++)
    {
    // Add comma to allImageFiles
    if(allImageFilesNeedsComma)
      allImageFiles += ",";
    else
      allImageFilesNeedsComma = true;

    // Add extension to all image files
    allImageFiles += m_FileFormatPattern[i];

    // Add a tab to the pattern
    if(patternNeedsNewline)
      pattern += "\n";
    else
      patternNeedsNewline = true;

    // Construct the pattern
    pattern += m_FileFormatDescription[i];
    pattern += " Files\t*.{";
    pattern += m_FileFormatPattern[i];
    pattern += "}";
    }

  // Finish the all image pattern
  allImageFiles += "}\n";

  // Compete the pattern
  pattern = allImageFiles + pattern;
  return pattern;
}

void 
MeshIOWizardUILogic::
OnCancel()
{
  m_MeshSelected = false;
  m_WinWizard->hide();  
}

void 
MeshIOWizardUILogic
::OnFilePageBrowse()
{
  // Get the pattern for selecting the file
  string pattern = GetFilePattern();

  // Get the current pathname
  const char *path = m_InFilePageBrowser->value();
  path = strlen(path) ? path : NULL;

  // Configure a file dialog
  const char *fName = NULL;
  Fl_Native_File_Chooser chooser;
  chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
  chooser.title("Select a mesh file");
  chooser.options(Fl_Native_File_Chooser::NEW_FOLDER);
  chooser.preset_file(path);
  chooser.filter(pattern.c_str());
  if(chooser.show() == 0)
    fName = chooser.filename(); 

  // Bring up th choice dialog
  if (fName && strlen(fName) > 0)
    {
    // Set the new filename
    m_InFilePageBrowser->value(fName);

    // Reset the format drop-down box to a null value
    m_InFilePageFormat->value(0);

    // Call the filename-changed callback
    OnFilePageFileInputChange();
    }
}

void 
MeshIOWizardUILogic
::OnFilePageFileInputChange()
{
  // Clear the registry
  m_Registry.Clear();

  // Deactivate the next page
  m_BtnFilePageNext->deactivate();

  // Check the length of the input
  const char *text = m_InFilePageBrowser->value();
  if (text != NULL && strlen(text) > 0)
    {
    // Try to load the registry associated with this filename
    m_Driver->GetSystemInterface()->FindRegistryAssociatedWithFile(
      m_InFilePageBrowser->value(), m_Registry);

    // If the registry contains a file format, override with that
    GuidedMeshIO::FileFormat fmt = 
      m_GuidedIO.GetFileFormat(m_Registry, GuidedMeshIO::FORMAT_COUNT);

    // Try to select a file format accoring to the file name
    if(fmt == GuidedMeshIO::FORMAT_COUNT)
      fmt = DetermineFileFormatFromFileName(text);

    // If the filename does not match any format, we do not change the 
    // format choice box in case that the user has already set it manually
    if(fmt < GuidedMeshIO::FORMAT_COUNT)
      {
      m_InFilePageFormat->value((int)fmt+1);
      m_BtnFilePageNext->activate();
      }
    else
      {
      m_InFilePageFormat->value(0);
      }
    } 
}

GuidedMeshIO::FileFormat
MeshIOWizardUILogic
::DetermineFileFormatFromFileName(const char *testFile) 
{
  // Iterate over the known file types
  for(size_t i = 0;i < GuidedMeshIO::FORMAT_COUNT;i++)
    {
    // Create a matching pattern
    StringType pattern = "*.{" + m_FileFormatPattern[i] + "}";

    // Check if the filename matches the pattern
    if(fl_filename_match(testFile, pattern.c_str()))
      return (GuidedMeshIO::FileFormat) i;
    }

  // Failed: return illegal pattern
  return GuidedMeshIO::FORMAT_COUNT;
}

void 
MeshIOWizardUILogic
::SetHistory(const HistoryType &history)
{
  // Store the history
  m_History = history;

  // Clear the history drop box
  m_InFilePageHistory->clear();

  // Add the history
  if(history.size() > 0)
    {  
    // Add each item to the history menu (history is traversed
    // backwards)
    for(HistoryType::reverse_iterator it=m_History.rbegin();
        it!=m_History.rend();it++)
      {
      // FLTK's add() treats slashes as submenu separators, hence this code
      m_InFilePageHistory->replace(
        m_InFilePageHistory->add("dummy"),it->c_str());      
      }

    // Activate the history menu    
    m_InFilePageHistory->activate();
    }
  else
    {
    // Deactivate history
    m_InFilePageHistory->deactivate();
    }
}

void 
MeshIOWizardUILogic
::OnFilePageNext()
{
  // There must be a file format
  assert(m_InFilePageFormat->value() > 0);

  // There must be a selected label
  assert((size_t) m_InMeshPageSelectedLabel->value() < m_ColorLabelMenuIndex.size());

  // The mesh has been selected
  m_MeshSelected = true;

  // Set the mesh filename
  m_ExportSettings.SetMeshFileName(m_InFilePageBrowser->value());

  // Set the mesh file type
  m_GuidedIO.SetFileFormat(m_Registry, 
    static_cast<GuidedMeshIO::FileFormat>(m_InFilePageFormat->value() - 1));
  m_ExportSettings.SetMeshFormat(m_Registry);

  // Set the export properties
  if(m_BtnMeshPageSingleExport->value())
    {
    m_ExportSettings.SetFlagSingleLabel(true);
    m_ExportSettings.SetFlagSingleScene(false);
    m_ExportSettings.SetExportLabel(
      m_ColorLabelMenuIndex[m_InMeshPageSelectedLabel->value()]);
    }
  else if(m_BtnMeshPageMultiExport->value())
    {
    m_ExportSettings.SetFlagSingleLabel(false);
    m_ExportSettings.SetExportLabel(0);
    if(m_BtnMeshPageSceneExport->value())
      {
      m_ExportSettings.SetFlagSingleScene(true);
      }
    else
      {
      m_ExportSettings.SetFlagSingleScene(false);
      }
    }

  // Hide the wizard
  m_WinWizard->hide();
}


void 
MeshIOWizardUILogic
::OnFilePageFileHistoryChange()
{
  // Copy the history value to the filename
  m_InFilePageBrowser->value(
    m_InFilePageHistory->mvalue()->label());

  // Update everything
  OnFilePageFileInputChange();  
}

void 
MeshIOWizardUILogic
::OnFilePageFileFormatChange()
{
  // Activate the next button if there is a format selected
  const std::string strFileName = m_InFilePageBrowser->value();
  if(strFileName != "") {
    if(m_InFilePageFormat->value() > 0)
      m_BtnFilePageNext->activate();
    else 
      m_BtnFilePageNext->deactivate();
  }
}

void
MeshIOWizardUILogic
::OnFilePageBack()
{
  m_GrpWizard->value(m_PageMesh);
}

void 
MeshIOWizardUILogic
::OnMeshPageNext()
{
  m_GrpWizard->value(m_PageFile);
}

void 
MeshIOWizardUILogic
::OnMeshPageRadioChange()
{
  // Set the other radio buttons
  if(m_BtnMeshPageSingleExport->value())
    {
    m_InMeshPageSelectedLabel->activate();
    m_BtnMeshPageIndexedExport->deactivate();
    m_BtnMeshPageSceneExport->deactivate();
    }
  else
    {
    m_InMeshPageSelectedLabel->deactivate();
    m_BtnMeshPageIndexedExport->activate();
    m_BtnMeshPageSceneExport->activate();
    }
}

// Custom initialization code
void 
MeshIOWizardUILogic
::MakeWindow()
{
  // Parent's method
  MeshIOWizardUI::MakeWindow();

  // Initialize the file save dialog box based on the allowed file types
  for(size_t i = 0; i < GuidedMeshIO::FORMAT_COUNT; i++)
    {
    // Create an appropriate description
    string text = m_FileFormatDescription[i];
    
    // Add a menu option to the save menu, disabling it if it's unsupported
    m_InFilePageFormat->add(text.c_str(), 0, NULL, NULL, 0);    
    }

  // Set the format description to 0
  m_InFilePageFormat->value(0);
}

// Display the wizard and get the save settings
bool 
MeshIOWizardUILogic
::DisplayWizard(IRISApplication *driver, bool snapmode)
{
  // Set the driver
  m_Driver = driver;

  // The loaded flag is false
  m_MeshSelected = false;

  // Switch IRIS mode / SNAP mode
  if(snapmode)
    {
    // Just go to the file page
    m_GrpWizard->value(m_PageFile);
    }
  else
    {
    // Point the wizard to the first page
    m_GrpWizard->value(m_PageMesh);
    }
    
  // Get the list of all currenly available labels
  ColorLabelTable *clt = m_Driver->GetColorLabelTable();

  // Initialize the mapping from menu index to color label
  m_ColorLabelMenuIndex.clear();

  // Populate the color label list
  m_InMeshPageSelectedLabel->clear();
  size_t k = 0;
  for(size_t i = 1; i < MAX_COLOR_LABELS; i++)
    {
    if(clt->IsColorLabelValid(i))
      {
      ColorLabel label = clt->GetColorLabel(i);
      m_InMeshPageSelectedLabel->add(label.GetLabel());
      if(i == m_Driver->GetGlobalState()->GetDrawingColorLabel())
        m_InMeshPageSelectedLabel->value(k);
      m_ColorLabelMenuIndex.push_back(i);
      k++;
      }
    }

  // Show the input window
  m_WinWizard->show();

  // Loop until the window has been closed
  while (m_WinWizard->visible())
    Fl::wait();

  // Remove the driver
  m_Driver = NULL;

  // Whether or not the load has been succesfull
  return m_MeshSelected;  
}

