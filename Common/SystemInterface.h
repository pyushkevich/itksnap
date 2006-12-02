/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: SystemInterface.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:10 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/

#ifndef __SystemInterface_h_
#define __SystemInterface_h_

#include "Registry.h"

class IRISApplication;
class SNAPRegistryIO;

/**
 * \class SystemInterface 
 * \brief An interface between SNAP and the operating system.
 * This class is responsible for finding the system directory, reading and 
 * writing user preferences to disk, etc.
 */
class SystemInterface : public Registry
{
public:
  SystemInterface();
  virtual ~SystemInterface();

  /** 
   * A method that checks whether the SNAP system directory can be found and 
   * if it can't, prompts the user for the directory.  If the user refuses to 
   * supply the directory, it throws an exception 
   */
  bool FindDataDirectory(const char *pathToExe);

  /** 
   * Get a file relative to the root directory (returns absolute filename), or
   * throws exception if the file does not exist
   */
  std::string GetFileInRootDirectory(const char *fnRelative);

  /** Loads the registry containing user preferences */
  void LoadUserPreferences();

  /** Save the preferences */
  void SaveUserPreferences();

  /** Get the filename for the user preferences */
  const char *GetUserPreferencesFileName() 
  { 
    return m_UserPreferenceFile.c_str(); 
  }

  /** The name of the token file that sits at the root of the program data
   * directory */
  static const char *GetProgramDataDirectoryTokenFileName()
  {
    return "SNAPProgramDataDirectory.txt";
  }

  // Typedef for history lists
  typedef std::vector<std::string> HistoryListType;

  /** Get a filename history list by a particular name */
  HistoryListType GetHistory(const char *key);

  /** Update a filename history list with another filename */
  void UpdateHistory(const char *key, const char *file);

  /** Find and load a registry file associated with a filename in the system.*/
  bool FindRegistryAssociatedWithFile(const char *file, 
                                      Registry &registry);

  /** Find and load a registry file associated with a filename in the system */
  bool AssociateRegistryWithFile(const char *file, Registry &registry);

  /** A higher level method: associates current settings with the current image
   * so that the next time the image is loaded, it can be saved */
  bool AssociateCurrentSettingsWithCurrentImageFile(
    const char *file,IRISApplication *app);

  /** A higher level method: associates current settings with the current image
   * so that the next time the image is loaded, it can be saved */
  bool RestoreSettingsAssociatedWithImageFile(
    const char *file, IRISApplication *app,
    bool restoreLabels, bool restorePreprocessing,
    bool restoreParameters, bool restoreDisplayOptions);
  
  /** 
  * This is a more complex method for loading a file. It work for files that have 
  * been opened previously, so that the user specified the necessary parameters 
  * for loading. It works with conjunction with the registry class
  */
  void LoadPreviousGreyImageFile(const char *filename, Registry *registry);

private:
  std::string m_UserPreferenceFile;
  std::string m_DataDirectory;
  std::string m_DocumentationDirectory;

  // An object used to write large chunks of SNAP data to the registry
  SNAPRegistryIO *m_RegistryIO;

  // Filename encoder
  std::string EncodeFilename(const std::string &src);
};





#endif //__SystemInterface_h_
