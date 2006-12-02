/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: SimpleFileDialogLogic.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:23 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __SimpleFileDialogLogic_h_
#define __SimpleFileDialogLogic_h_

#include "itkSmartPointer.h"
#include "SNAPCommonUI.h"
#include "SimpleFileDialog.h"
#include "itkCommand.h"

class Fl_File_Chooser;
namespace itk {
  class Command;
}

/**
 * \class SimpleFileDialogLogic
 * \brief A very basic file dialog with a history list
 * A simple file dialog used to load and save text files, such as
 * projects, voxel counts, etc.  
 */
class SimpleFileDialogLogic : public SimpleFileDialog {
public: 
  SimpleFileDialogLogic();
  virtual ~SimpleFileDialogLogic();

  // Vector type for passing in history
  typedef std::string StringType;
  typedef std::vector<StringType> HistoryListType;

  /** Called on creation, this method configures some controls */
  void MakeWindow();

  /** Display a load dialog.  The first parameter to this dialog
   * is a registry representing the history of recently loaded
   * files.  The second parameter is the optional text to display
   * in the filename box.  If NULL is passed in (default), the previously
   * used file name will be retained.  
   *
   * The history will not be updated until a 
   *
   * The return from this method is analogous
   * to fl_file_chooser, ie, a filename or NULL if user cancelled.*/
  void DisplayLoadDialog(
    const HistoryListType &history, const char *file = NULL);

  /** Display a save dialog.  \see DisplayLoadDialog */
  void DisplaySaveDialog(
    const HistoryListType &history, const char *file = NULL);

  /** Set the title of the dialog */
  void SetTitle(const char *title)
    {
    m_Title = title;
    m_Window->label(m_Title.c_str());
    }

  /** Set the text displayed above the filename box */
  void SetFileBoxTitle(const char *title) 
    {
    m_FileBoxTitle = title;
    m_InFile->label(m_FileBoxTitle.c_str());
    }

  /** Set the filename pattern (FLTK format) */
  irisSetMacro(Pattern,const char *);

  /** Get the file name currently in the box */
  const char *GetFileName() { return m_InFile->value(); }

  /** Set the command to call when the user clicks OK in the load dialog
   * This command should fire an exception (any exception) if the load 
   * fails, and then the dialog will remain open and the history list will
   * not be updated */
  template <class T> void SetLoadCallback(T *object, void (T::*member)())
  {
    typedef itk::SimpleMemberCommand<T> CommandType;
    typename CommandType::Pointer cmd = CommandType::New();
    cmd->SetCallbackFunction(object,member);
    m_LoadCallback = cmd;
  }

  /** Set save callback. \see SetLoadCallback */
  template <class T> void SetSaveCallback(T *object, void (T::*member)())
  {
    typedef itk::SimpleMemberCommand<T> CommandType;
    typename CommandType::Pointer cmd = CommandType::New();
    cmd->SetCallbackFunction(object,member);
    m_SaveCallback = cmd;
  }
  
  // User interface callbacks
  void OnFileChange();
  void OnHistoryChange();
  void OnBrowseAction();
  void OnOkAction();
  void OnCancelAction();
 
private:

  /** Common dialog config, regardless of save/load */
  void DisplayDialog(const HistoryListType &history, const char *file);

  // Whether we are in save or load mode
  bool m_SaveMode;
  
  // Labels
  StringType m_FileBoxTitle;
  StringType m_HistoryBoxTitle;
  StringType m_Pattern;
  StringType m_Title;

  // Callback commands
  typedef itk::SmartPointer<itk::Command> CommandPointer;
  CommandPointer m_SaveCallback;
  CommandPointer m_LoadCallback;

  // File choosers
  Fl_File_Chooser *m_FileChooserLoad;
  Fl_File_Chooser *m_FileChooserSave;

};

#endif // __SimpleFileDialogLogic_h_
