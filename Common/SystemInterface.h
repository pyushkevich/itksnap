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
#include "Trackball.h"

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
  std::string DecodeObjectName(std::string fname);
  std::string EncodeObjectName(std::string fname);

  
  /** 
  * This is a more complex method for loading a file. It work for files that have 
  * been opened previously, so that the user specified the necessary parameters 
  * for loading. It works with conjunction with the registry class
  */
  void LoadPreviousGreyImageFile(const char *filename, Registry *registry);

  /** Structure passed on to IPC */
  struct IPCMessage 
    {
    // Process ID of the sender
    long sender_pid, message_id;

    // The cursor position in world coordinates
    Vector3d cursor;

    // The common zoom factor (screen pixels / mm)
    double zoom_level;   

    // The position of the viewport center relative to cursor
    // in all three slice views
    Vector2f viewPositionRelative[3];

    // 3D view settings
    Trackball trackball;
    };

  /** Get process ID */
  long GetProcessID() 
    { return m_ProcessID; }

  /** Interprocess communication: attach to shared memory */
  void IPCAttach();

  /** Interprocess communication: read shared memory */
  bool IPCRead(IPCMessage &mout);

  /** Read IPC message, but only if it's new data */
  bool IPCReadIfNew(IPCMessage &mout);

  /** Interprocess communication: write shared memory */
  bool IPCBroadcast(IPCMessage mout);
  bool IPCBroadcastCursor(Vector3d cursor);
  bool IPCBroadcastTrackball(Trackball tball);
  bool IPCBroadcastZoomLevel(double zoom);
  bool IPCBroadcastViewPosition(Vector2f vec[3]);

  /** Interprocess communication: release shared memory */
  void IPCClose();

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
   * Launch a child SNAP process with specified command line options
   */
  void LaunchChildSNAP(std::list<std::string> args);

private:
  std::string m_UserPreferenceFile;
  std::string m_DataDirectory;
  std::string m_DocumentationDirectory;
  std::string m_FullPathToExecutable;

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

  // The version of the SNAP-IPC protocol. This way, when versions are different
  // IPC will not work. This is to account for an off chance of a someone running
  // two different versions of SNAP
  static const short IPC_VERSION;

  // Generic: shared data for IPC
  void *m_IPCSharedData;

  // Process ID and other values used by IPC
  long m_ProcessID, m_MessageID, m_LastSender, m_LastReceivedMessageID;
};





#endif //__SystemInterface_h_
