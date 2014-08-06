/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SystemInterface.h,v $
  Language:  C++
  Date:      $Date: 2010/04/14 10:06:23 $
  Version:   $Revision: 1.11 $
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
#include "GlobalState.h"

class IRISApplication;
class SNAPRegistryIO;
class HistoryManager;
class SystemInfoDelegate;
class vtkCamera;

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
   * Set a pointer to the delegate class that can be used to access
   * system-specific information. This method should be called before
   * the first time SystemInterface is constructed, otherwise an exception
   * will be fired.
   */
  static SystemInfoDelegate * GetSystemInfoDelegate()
    { return m_SystemInfoDelegate; }

  static void SetSystemInfoDelegate(SystemInfoDelegate *_arg)
    { m_SystemInfoDelegate = _arg; }

  /**
   * Get the full path to executable
   */
  std::string GetFullPathToExecutable() const;

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

  /** Get a filename history list by a particular name */
  irisGetMacro(HistoryManager, HistoryManager*);

  /** Find and load a registry file associated with a filename in the system.*/
  bool FindRegistryAssociatedWithFile(const char *file, 
                                      Registry &registry);

  /** Find and load a registry file associated with a filename in the system */
  bool AssociateRegistryWithFile(const char *file, Registry &registry);

  /** Get the thumbnail filename associated with an image file */
  std::string GetThumbnailAssociatedWithFile(const char *file);

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
   * This is a newer facility that makes saving registry objects more generic 
   * and atomic. Basically, any 'object' can be saved, read or deleted in the
   * user's directory. Objects are grouped by categories. The main difference
   * with the registry is that the objects are stored on disk, so different 
   * SNAP options are less likely to interfere with each other.
   */
  std::vector<std::string> GetSavedObjectNames(const char *category);
  Registry ReadSavedObject(const char *category, const char *name);
  void UpdateSavedObject(const char *category, const char *name, Registry &folder);
  void DeleteSavedObject(const char *category, const char *name);
  static std::string DecodeObjectName(std::string fname);
  static std::string EncodeObjectName(std::string fname);

  
  /** 
  * This is a more complex method for loading a file. It work for files that have 
  * been opened previously, so that the user specified the necessary parameters 
  * for loading. It works with conjunction with the registry class
  */
  void LoadPreviousGreyImageFile(const char *filename, Registry *registry);

  /** Enum for automatic update checking */
  enum UpdateStatus 
    {
    US_UP_TO_DATE, US_OUT_OF_DATE, US_CONNECTION_FAILED, US_TOO_SOON
    };

  /** 
    Check if an update is available. First parameter is output version string,
    other parameters specify timeout in seconds, millonth of a second 
   */
  UpdateStatus CheckUpdate(std::string &newversion, size_t sec, size_t usec, bool force = false);

  /**
   * Launch a child SNAP process with specified command line options. The first
   * option in argv should be the path to the SNAP executable
   */
  static void LaunchChildSNAP(int argc, char **argv, bool terminate_parent);

  /** Simplified, non-static version of the above */
  void LaunchChildSNAPSimple(std::list<std::string> args);

private:
  std::string m_UserPreferenceFile;
  std::string m_DocumentationDirectory;

  // An object used to write large chunks of SNAP data to the registry
  SNAPRegistryIO *m_RegistryIO;

  // History manager
  HistoryManager *m_HistoryManager;

  // Delegate
  static SystemInfoDelegate *m_SystemInfoDelegate;

  // Filename encoder
  std::string EncodeFilename(const std::string &src);

  // Associate a file with a unique code
  std::string FindUniqueCodeForFile(const char *file, bool generate_if_not_found);

  // Get the directory where application data (pref files, etc) should go
  std::string GetApplicationDataDirectory();
};



#endif //__SystemInterface_h_
