/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SystemInterface.h,v $
  Language:  C++
  Date:      $Date: 2008/02/10 23:55:21 $
  Version:   $Revision: 1.3 $
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

#ifndef __SystemInterface_h_
#define __SystemInterface_h_

#include "Registry.h"

class IRISApplication;
class SNAPRegistryIO;

#ifdef WIN32
#include <windows.h>
#endif

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

  /** Interprocess communication: attach to shared memory */
  void IPCCursorAttach();

  /** Interprocess communication: read shared memory */
  bool IPCCursorRead(Vector3d &vout);

  /** Interprocess communication: write shared memory */
  bool IPCCursorWrite(const Vector3d &vin);

  /** Interprocess communication: release shared memory */
  void IPCCursorClose();

private:
  std::string m_UserPreferenceFile;
  std::string m_DataDirectory;
  std::string m_DocumentationDirectory;

  // An object used to write large chunks of SNAP data to the registry
  SNAPRegistryIO *m_RegistryIO;

  // Filename encoder
  std::string EncodeFilename(const std::string &src);

  // System-specific IPC related stuff
#ifdef WIN32
  HANDLE m_IPCHandle;
#else
  int m_IPCHandle;
#endif

  // Generic: shared data for IPC
  void *m_IPCSharedData;
};





#endif //__SystemInterface_h_
