/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SimpleFileDialogLogic.cxx,v $
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
#include "SimpleFileDialogLogic.h"
#include "itkCommand.h"
#include "FL/Fl_File_Chooser.H"

#include <algorithm>

using namespace std;

SimpleFileDialogLogic
::SimpleFileDialogLogic()
{
  m_FileChooserLoad = NULL;
  m_FileChooserSave = NULL;
}

SimpleFileDialogLogic
::~SimpleFileDialogLogic()
{
  if(m_FileChooserLoad)
    delete m_FileChooserLoad;
  if(m_FileChooserSave)
    delete m_FileChooserSave;
}

void
SimpleFileDialogLogic
::MakeWindow()
{
  SimpleFileDialog::MakeWindow();
  m_FileChooserLoad = 
    new Fl_File_Chooser(NULL,NULL,Fl_File_Chooser::SINGLE,"Select a File");
  m_FileChooserSave = 
    new Fl_File_Chooser(NULL,NULL,Fl_File_Chooser::CREATE,"Select a File");
}

void
SimpleFileDialogLogic
::DisplayLoadDialog(const HistoryListType &history,const char *file)
{
  m_SaveMode = false;
  this->DisplayDialog(history,file);
}
  
void
SimpleFileDialogLogic
::DisplaySaveDialog(const HistoryListType &history,const char *file)
{
  m_SaveMode = true;
  this->DisplayDialog(history,file);
}

void 
SimpleFileDialogLogic
::DisplayDialog(const HistoryListType &history, const char *file)
{
  // If the filename was supplied, update it in the UI
  if(file)
    {
    m_InFile->value(file);
    }

  // Clear the history drop box
  m_InHistory->clear();

  // Add elements in the history list
  if(history.size() > 0)
    {  
    // Add each item to the history menu (history is traversed backwards)
    HistoryListType::const_reverse_iterator it;
    for(it=history.rbegin();it!=history.rend();it++)
      {
      // FLTK's add() treats slashes as submenu separators, hence this code
      m_InHistory->replace(m_InHistory->add("dummy"),it->c_str());
      }

    // Activate the history menu    
    m_InHistory->activate();
    }
  else
    {
    // Deactivate history
    m_InHistory->deactivate();
    }

  // Deactivate / activate the OK button
  this->OnFileChange();

  // Show the dialog and wait until it closes
  m_Window->show();
  while(m_Window->shown())
    Fl::wait();  
}

void 
SimpleFileDialogLogic
::OnFileChange()
{
  // Disable the OK button if the file box is empty
  if(!m_InFile->value() || strlen(m_InFile->value()) == 0)
    m_BtnOk->deactivate();
  else
    m_BtnOk->activate();
}

void 
SimpleFileDialogLogic
::OnHistoryChange()
{
  // Put the seleted history into the file box
  m_InFile->value(m_InHistory->mvalue()->text);

  // Act as if the user changed the file
  OnFileChange();
}

void 
SimpleFileDialogLogic
::OnBrowseAction()
{
  // Choose which file chooser to use
  Fl_File_Chooser *fc = (m_SaveMode) ? m_FileChooserSave : m_FileChooserLoad;

  // If there is something in the file box, pass it to the chooser
  if(m_InFile->value() && strlen(m_InFile->value()))
    fc->value(m_InFile->value());

  // Set the pattern
  fc->filter(m_Pattern.c_str());

  // Show the dialog
  fc->show();  
  while(fc->shown()) Fl::wait();

  // Once the dialog is done, get the value
  if(fc->value() && strlen(fc->value()))
    {
    m_InFile->value(fc->value());
    m_BtnOk->activate();
    }  
}

void 
SimpleFileDialogLogic
::OnOkAction()
{
  try 
    {
    // Fire the appropriate event
    if(m_SaveMode)
      m_SaveCallback->Execute((itk::Object *) 0,itk::NoEvent());
    else 
      m_LoadCallback->Execute((itk::Object *) 0,itk::NoEvent());
  
    // Hide the window
    m_Window->hide();
  }
  catch(...) { }
}

void 
SimpleFileDialogLogic
::OnCancelAction()
{
  // Hide the window
  m_Window->hide();
}
