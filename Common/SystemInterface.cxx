/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SystemInterface.cxx,v $
  Language:  C++
  Date:      $Date: 2008/02/11 13:06:52 $
  Version:   $Revision: 1.4 $
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
#include "SystemInterface.h"
#include "IRISApplication.h"
#include "GlobalState.h"
#include "SNAPRegistryIO.h"
#include "FL/Fl_Preferences.H"
#include "FL/filename.H"
#include <itksys/SystemTools.hxx>
#include "itkVoxBoCUBImageIOFactory.h"
#include <algorithm>
#include <ctime>
#include <iomanip>

#ifdef WIN32
  #include <iostream>
#else
  #include <sys/types.h> 
  #include <sys/ipc.h> 
  #include <sys/shm.h> 
#endif

using namespace std;

SystemInterface
::SystemInterface()
{
  // Initialize the registry
  m_RegistryIO = new SNAPRegistryIO;

  // Register the Image IO factories that are not part of ITK
  itk::ObjectFactoryBase::RegisterFactory( 
    itk::VoxBoCUBImageIOFactory::New() );

  // Create a preferences object
  Fl_Preferences test(Fl_Preferences::USER,"itk.org","SNAP");
  
  // Use it to get a path for user data
  char userDataPath[1024]; 
  test.getUserdataPath(userDataPath,1024);
  
  // Construct a valid path
  m_UserPreferenceFile = string(userDataPath) + "/" + "UserPreferences.txt";

  // Attach to the shared memory
  IPCCursorAttach();
}

SystemInterface
::~SystemInterface()
{
  delete m_RegistryIO;

  // Detach shared memory
  IPCCursorClose();
}


void
SystemInterface
::SaveUserPreferences()
{
  WriteToFile(m_UserPreferenceFile.c_str());
}


void
SystemInterface
::LoadUserPreferences()
{
  // Check if the file exists, may throw an exception here
  if(itksys::SystemTools::FileExists(m_UserPreferenceFile.c_str()))
  {
    // Read the contents of the preferences from file
    ReadFromFile(m_UserPreferenceFile.c_str());

    // Check if the preferences contain a version string
    string version = Entry("System.CreatedBySNAPVersion")["00000000"];

    // If the version is less than the latest incompatible version, delete the
    // contents of the version
    if(atoi(version.c_str()) < atoi(SNAPLastIncompatibleReleaseDate))
      {
      // Clear the contents of the registry since it's incompatible
      this->Clear();
      }

    // Enter the current SNAP version into the registry
    Entry("System.CreatedBySNAPVersion") << SNAPCurrentVersionReleaseDate;
  }
}

/** 
 * A method that checks whether the SNAP system directory can be found and 
 * if it can't, prompts the user for the directory.  The path parameter is the
 * location of the executable, i.e., argv[0] 
 */
bool 
SystemInterface
::FindDataDirectory(const char *pathToExe)
{
  // Get the directory where the SNAP executable was launched
  using namespace itksys;
  typedef std::string StringType;

  // This is the directory we're trying to set
  StringType sRootDir = "";

  // This is a key that we will use to represent the executable location
  StringType sCodedExePath;

  // First of all, find the executable file.  Since the program may have been
  // in the $PATH variable, we don't know for sure where the data is
  // Create a vector of paths that will be searched for 
  // the file SNAPProgramDataDirectory.txt
  StringType sExeFullPath = itksys::SystemTools::FindProgram(pathToExe);
  if(sExeFullPath.length())
    {
    // Encode the path to the executable so that we can search for an associated
    // program data path
    sCodedExePath = EncodeFilename(sExeFullPath);    
    }
  else
    {
    // Use a dummy token, so that next time the user runs without an executable,
    // we can find a data directory
    sCodedExePath = "00000000";
    }

  // Check if there is a path associated with the code
  StringType sAssociationKey = 
    Key("System.ProgramDataDirectory.Element[%s]",sCodedExePath.c_str());
  StringType sAssociatedPath = Entry(sAssociationKey)[""];

  // If the associated path exists, prepemd the path to the search list
  if(sAssociatedPath.length())
    {
    // Check that the associated path is a real path containing the required 
    // file
    StringType sSearchName = sAssociatedPath + "/" + 
      GetProgramDataDirectoryTokenFileName();

    // Perform a sanity check on the directory
    if(itksys::SystemTools::FileIsDirectory(sAssociatedPath.c_str()) && 
       itksys::SystemTools::FileExists(sSearchName.c_str()))
      {
      // We've found the path
      sRootDir = sAssociatedPath;
      }
    }
  
  // If the associated path check failed, but the executable has been found,
  // which should be the case the first time the program is run, look around
  // the executable to find the path
  if(!sRootDir.length() && sExeFullPath.length())
    {
    // Create a search list for the filename
    vector<StringType> vPathList;

    // Look at the directory where the exe sits
    vPathList.push_back(
      itksys::SystemTools::GetFilenamePath(sExeFullPath) + "/ProgramData");

    // Look one directory up from that
    vPathList.push_back(
      itksys::SystemTools::GetFilenamePath(
        itksys::SystemTools::GetFilenamePath(sExeFullPath)) + 
      "/ProgramData");

    // Also, for UNIX installations, look for ${INSTALL_PATH}/share/snap/ProgramData
    vPathList.push_back(
       itksys::SystemTools::GetFilenamePath(
         itksys::SystemTools::GetFilenamePath(sExeFullPath)) + 
       "/share/snap/ProgramData");

    // Search for the token file in the path list
    StringType sFoundFile = 
      itksys::SystemTools::FindFile(
        GetProgramDataDirectoryTokenFileName(),vPathList);
    if(sFoundFile.length())
      sRootDir = itksys::SystemTools::GetFilenamePath(sFoundFile);
    }

  // If we still don't have a root path, there's no home
  if(!sRootDir.length())
    return false;

  // Store the property, so the next time we don't have to search at all
  Entry(sAssociationKey) << sRootDir;
  
  // Set the root directory and relative paths
  m_DataDirectory = sRootDir;

  // Append the paths to get the other directories
  m_DocumentationDirectory = m_DataDirectory + "/HTMLHelp";

  // Done, success
  return true;
}

std::string
SystemInterface
::GetFileInRootDirectory(const char *fnRelative)
{
  // Construct the file name
  string path = m_DataDirectory + "/" + fnRelative;

  // Make sure the file exists ?

  // Return the file
  return path;
}

bool 
SystemInterface
::FindRegistryAssociatedWithFile(const char *file, Registry &registry)
{
  // Convert the file to an absolute path
  char buffer[1024];
  fl_filename_absolute(buffer,1024,file);

  // Convert to unix slashes for consistency
  string path(buffer);
  itksys::SystemTools::ConvertToUnixSlashes(path);

  // Convert the filename to a numeric string (to prevent clashes with the Registry class)
  path = EncodeFilename(path);

  // Get the key associated with this filename
  string key = Key("ImageAssociation.Mapping.Element[%s]",path.c_str());
  string code = Entry(key)[""];

  // If the code does not exist, return w/o success
  if(code.length() == 0) return false;

  // Create a preferences object for the associations subdirectory
  Fl_Preferences test(Fl_Preferences::USER,"itk.org","SNAP/ImageAssociations");

  // Use it to get a path for user data
  char userDataPath[1024]; 
  test.getUserdataPath(userDataPath,1024);

  // Create a save filename
  IRISOStringStream sfile;
  sfile << userDataPath << "/" << "ImageAssociation." << code << ".txt";

  // Try loading the registry
  try 
    {
    registry.ReadFromFile(sfile.str().c_str());
    return true;
    }
  catch(...)
    {
    return false;
    }
}

string
SystemInterface
::EncodeFilename(const string &src)
{
  IRISOStringStream sout;
  
  for(unsigned int i=0;i<src.length();i++)
    sout << setw(2) << setfill('0') << hex << (int)src[i];

  return sout.str();
}

bool 
SystemInterface
::AssociateRegistryWithFile(const char *file, Registry &registry)
{
  // Convert the file to an absolute path
  char buffer[1024];
  fl_filename_absolute(buffer,1024,file);

  // Convert to unix slashes for consistency
  string path(buffer);
  itksys::SystemTools::ConvertToUnixSlashes(path);
  path = EncodeFilename(path);

  // Compute a timestamp from the start of computer time
  time_t timestr = time(NULL);

  // Create a key for the file
  IRISOStringStream scode;
  scode << setfill('0') << setw(16) << hex << timestr;
  
  // Look for the file in the registry (may already exist, if not use the
  // code just generated
  string key = Key("ImageAssociation.Mapping.Element[%s]",path.c_str());
  string code = Entry(key)[scode.str().c_str()];
  
  // Put the key in the registry
  Entry(key) << code;

  // Create a preferences object for the associations subdirectory
  Fl_Preferences test(Fl_Preferences::USER,"itk.org","SNAP/ImageAssociations");

  // Use it to get a path for user data
  char userDataPath[1024]; 
  test.getUserdataPath(userDataPath,1024);

  // Create a save filename
  IRISOStringStream sfile;
  sfile << userDataPath << "/" << "ImageAssociation." << code << ".txt";

  // Store the registry to that path
  try 
    {
    registry.WriteToFile(sfile.str().c_str());
    return true;
    }
  catch(...)
    {
    return false;
    }  
}

bool 
SystemInterface
::AssociateCurrentSettingsWithCurrentImageFile(const char *file, IRISApplication *app)
{
  // Get a registry already associated with this filename
  Registry registry;
  FindRegistryAssociatedWithFile(file,registry);

  // Write the current state into that registry
  m_RegistryIO->WriteImageAssociatedSettings(app,registry);

  // Write the registry back
  return AssociateRegistryWithFile(file,registry);
}

bool 
SystemInterface
::RestoreSettingsAssociatedWithImageFile(
  const char *file, IRISApplication *app,
  bool restoreLabels, bool restorePreprocessing,
  bool restoreParameters, bool restoreDisplayOptions)
{
  // Get a registry already associated with this filename
  Registry registry;
  if(FindRegistryAssociatedWithFile(file,registry))
    {
    m_RegistryIO->ReadImageAssociatedSettings(
      registry, app,
      restoreLabels, restorePreprocessing,
      restoreParameters, restoreDisplayOptions);
    return true;
    }
  else
    return false;
}

/** Get a filename history list by a particular name */
SystemInterface::HistoryListType 
SystemInterface
::GetHistory(const char *key)
{
  // Get the history array
  return Folder("IOHistory").Folder(string(key)).GetArray(string(""));
}

/** Update a filename history list with another filename */
void
SystemInterface
::UpdateHistory(const char *key, const char *filename)
{
  // Create a string for the new file
  string file(filename);

  // Get the current history registry
  HistoryListType array = GetHistory(key);

  // First, search the history for the instance of the file and delete
  // existing occurences
  HistoryListType::iterator it;
  while((it = find(array.begin(),array.end(),file)) != array.end())
    array.erase(it);

  // Append the file to the end of the array
  array.push_back(file);

  // Trim the array to appropriate size
  if(array.size() > 20)
    array.erase(array.begin(),array.begin() + array.size() - 20);

  // Store the new array to the registry
  Folder("IOHistory").Folder(string(key)).PutArray(array);

  // Save the preferences at this point
  SaveUserPreferences();
}



void
SystemInterface
::IPCCursorAttach()
{
  // Initialize the data pointer
  m_IPCSharedData = NULL;

#ifdef WIN32
  // Create a shared memory block (key based on the preferences file)
  m_IPCHandle = CreateFileMapping(
    INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(Vector3d), m_UserPreferenceFile.c_str());

  // If the return value is NULL, something failed
  if(m_IPCHandle)
    {
    // Attach to the shared memory block
    m_IPCSharedData = MapViewOfFile(m_IPCHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Vector3d));
    }

#else

  // Create a unique key for this user 
  key_t keyid = ftok(m_UserPreferenceFile.c_str(), 0);

  // Get a handle to shared memory
  m_IPCHandle = shmget(keyid, sizeof(Vector3d), IPC_CREAT | 0644);

  // Get a pointer to shared data block
  m_IPCSharedData = shmat(m_IPCHandle, (void *) 0, 0);

#endif
}

bool 
SystemInterface
::IPCCursorRead(Vector3d &vout)
{
  // Read from the shared memory
  if(m_IPCSharedData)
    {
    vout.copy_in(static_cast<double *>(m_IPCSharedData));
    return true;
    }
  return false;
}

bool
SystemInterface
::IPCCursorWrite(const Vector3d &vin)
{
  // Write to the shared memory
  if(m_IPCSharedData)
    {
    vin.copy_out(static_cast<double *>(m_IPCSharedData));
    return true;
    }
  return false;
}

void 
SystemInterface
::IPCCursorClose()
{
#ifdef WIN32
  CloseHandle(m_IPCHandle);
#else
  // Detach from the shared memory segment
  shmdt(m_IPCSharedData);

  // If there is noone attached to the memory segment, destroy it
  struct shmid_ds dsinfo;
  shmctl(m_IPCHandle, IPC_STAT, &dsinfo);
  if(dsinfo.shm_nattch == 0)
    shmctl(m_IPCHandle, IPC_RMID, NULL);
#endif
}
