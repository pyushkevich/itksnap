/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SystemInterface.cxx,v $
  Language:  C++
  Date:      $Date: 2010/04/16 05:14:38 $
  Version:   $Revision: 1.23 $
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
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISException.h"
#include "GlobalState.h"
#include "SNAPRegistryIO.h"
#include "FL/Fl_Preferences.H"
#include "FL/filename.H"
#include <itksys/Directory.hxx>
#include <itksys/SystemTools.hxx>
#include "itkVoxBoCUBImageIOFactory.h"
#include <algorithm>
#include <ctime>
#include <cerrno>
#include <cstring>
#include <iomanip>

#ifdef WIN32
  #include <iostream>
  #include <process.h>
#else
  #include <sys/types.h> 
  #include <sys/ipc.h> 
  #include <sys/shm.h> 
  #include <unistd.h>
#endif

using namespace std;
using namespace itksys;

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

  // Get the process ID
#ifdef WIN32
  m_ProcessID = _getpid();
#else
  m_ProcessID = getpid();
#endif

  // Set the message ID and last sender/message id values
  m_LastReceivedMessageID = -1;
  m_LastSender = -1;
  m_MessageID = 0;

  // Attach to the shared memory
  IPCAttach();
}

SystemInterface
::~SystemInterface()
{
  delete m_RegistryIO;

  // Detach shared memory
  IPCClose();
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
  if(SystemTools::FileExists(m_UserPreferenceFile.c_str()))
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
    }
  // Enter the current SNAP version into the registry
  Entry("System.CreatedBySNAPVersion") << SNAPCurrentVersionReleaseDate;
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
  StringType sExeFullPath = SystemTools::FindProgram(pathToExe);
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
    if(SystemTools::FileIsDirectory(sAssociatedPath.c_str()) && 
       SystemTools::FileExists(sSearchName.c_str()))
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
      SystemTools::GetFilenamePath(sExeFullPath) + "/ProgramData");

    // Look one directory up from that
    vPathList.push_back(
      SystemTools::GetFilenamePath(
        SystemTools::GetFilenamePath(sExeFullPath)) + 
      "/ProgramData");

    // Also, for UNIX installations, look for ${INSTALL_PATH}/share/snap/ProgramData
    vPathList.push_back(
       SystemTools::GetFilenamePath(
         SystemTools::GetFilenamePath(sExeFullPath)) + 
       "/share/snap/ProgramData");

    // Search for the token file in the path list
    StringType sFoundFile = 
      SystemTools::FindFile(
        GetProgramDataDirectoryTokenFileName(),vPathList);
    if(sFoundFile.length())
      sRootDir = SystemTools::GetFilenamePath(sFoundFile);
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

  // Save the path to executable
  m_FullPathToExecutable = sExeFullPath;

  // Done, success
  return true;
}


void 
SystemInterface
::LaunchChildSNAP(std::list<std::string> args)
{
  // Must have a valid path to the EXE
  if(m_FullPathToExecutable.length() == 0)
    throw IRISException("Path to executable unknown in LaunchChildSNAP");

  // Create the argument array
  char **argv = new char* [args.size()+2];
  int iarg = 0;
  argv[iarg++] = (char *) m_FullPathToExecutable.c_str();
  for(std::list<std::string>::iterator it=args.begin(); it!=args.end(); ++it)
    argv[iarg++] = (char *) it->c_str();
  argv[iarg++] = NULL;

  // Create child process
#ifdef WIN32
  _spawnvp(_P_NOWAIT, m_FullPathToExecutable.c_str(), argv);
#else
  int pid;
  if((pid = fork()) == 0)
    {
    if(execvp(m_FullPathToExecutable.c_str(), argv) < 0)
      exit(-1);
    } 
#endif
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
  SystemTools::ConvertToUnixSlashes(path);

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
::EncodeObjectName(string text)
{
  ostringstream oss;
  size_t n = text.length();
  for(size_t i = 0; i < n; i++)
    {
    char c = text[i];
    if(c >= 'a' && c <= 'z')
      oss << c;
    else if(c >= 'A' && c <= 'Z')
      oss << c;
    else if(c >= '0' && c <= '9')
      oss << c;
    else if(c == ' ')
      oss << "__";
    else
      {
      char buffer[5];
      sprintf(buffer, "_%03d", (int) c);
      oss << buffer;
      }
    }

  return oss.str();
}  

string
SystemInterface
::DecodeObjectName(string fname)
{
  ostringstream oss;
  size_t n = fname.length();
  for(size_t i = 0; i < n; i++)
    {
    char c = fname[i];
    if(c == '_')
      {
      if(i+1 < n && fname[i+1] == '_')
        { oss << ' '; i++; }
      else if(i+3 < n)
        { oss << (char)(atoi(fname.substr(i+1,3).c_str())); i+=3; }
      else return "";
      }
    else
      oss << c;
    }
  return oss.str();
}

vector<string> 
SystemInterface
::GetSavedObjectNames(const char *category)
{
  // Create a preferences object for the associations subdirectory
  string subpath("SNAP/");
  subpath += category;
  Fl_Preferences pref(Fl_Preferences::USER,"itk.org",subpath.c_str());

  // Use it to get a path for user data
  char userDataPath[1024]; 
  pref.getUserdataPath(userDataPath,1024);

  // Get the names
  vector<string> names;

  // Get the listing of all files in there
  Directory dlist;
  dlist.Load(userDataPath);
  for(size_t i = 0; i < dlist.GetNumberOfFiles(); i++)
    {
    string fname = dlist.GetFile(i);

    // Check regular file
    ostringstream ffull; 
    ffull << userDataPath << "/" << fname;
    if(SystemTools::FileExists(ffull.str().c_str(), true))
      {
      // Check extension
      if(SystemTools::GetFilenameExtension(fname) == ".txt")
        {
        string base = SystemTools::GetFilenameWithoutExtension(fname);
        string name = DecodeObjectName(base);
        if(name.length())
          names.push_back(name);
        }
      }
    }

  return names;
}

Registry 
SystemInterface
::ReadSavedObject(const char *category, const char *name)
{
  // Create a preferences object for the associations subdirectory
  string subpath("SNAP/");
  subpath += category;
  Fl_Preferences pref(Fl_Preferences::USER,"itk.org",subpath.c_str());

  // Use it to get a path for user data
  char userDataPath[1024]; 
  pref.getUserdataPath(userDataPath,1024);

  // Create a save filename
  IRISOStringStream sfile;
  sfile << userDataPath << "/" << EncodeObjectName(name) << ".txt";
  string fname = sfile.str();

  // Check the filename
  if(!SystemTools::FileExists(fname.c_str(), true))
    throw IRISException("Saved object file does not exist");

  Registry reg(fname.c_str());
  return reg;
}

void 
SystemInterface
::UpdateSavedObject(const char *category, const char *name, Registry &folder)
{
  // Create a preferences object for the associations subdirectory
  string subpath("SNAP/");
  subpath += category;
  Fl_Preferences pref(Fl_Preferences::USER,"itk.org",subpath.c_str());

  // Use it to get a path for user data
  char userDataPath[1024]; 
  pref.getUserdataPath(userDataPath,1024);

  // Create a save filename
  IRISOStringStream sfile;
  sfile << userDataPath << "/" << EncodeObjectName(name) << ".txt";
  string fname = sfile.str();

  // Save the data
  folder.WriteToFile(fname.c_str());
}

void 
SystemInterface
::DeleteSavedObject(const char *category, const char *name)
{
  // Create a preferences object for the associations subdirectory
  string subpath("SNAP/");
  subpath += category;
  Fl_Preferences pref(Fl_Preferences::USER,"itk.org",subpath.c_str());

  // Use it to get a path for user data
  char userDataPath[1024]; 
  pref.getUserdataPath(userDataPath,1024);

  // Create a save filename
  IRISOStringStream sfile;
  sfile << userDataPath << "/" << EncodeObjectName(name) << ".txt";

  // Save the data
  SystemTools::RemoveFile(sfile.str().c_str());
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
  SystemTools::ConvertToUnixSlashes(path);
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


// We start versioning at 1000. Every time we change
// the protocol, we should increment the version id
const short SystemInterface::IPC_VERSION = 0x1003;

void
SystemInterface
::IPCAttach()
{
  // Initialize the data pointer
  m_IPCSharedData = NULL;

  // Determine size of shared memory
  size_t msize = sizeof(IPCMessage) + sizeof(short);

#ifdef WIN32
  // Create a shared memory block (key based on the preferences file)
  m_IPCHandle = CreateFileMapping(
    INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, msize, m_UserPreferenceFile.c_str());

  // If the return value is NULL, something failed
  if(m_IPCHandle)
    {
    // Attach to the shared memory block
    m_IPCSharedData = MapViewOfFile(m_IPCHandle, FILE_MAP_ALL_ACCESS, 0, 0, msize);
    }

#else

  // Create a unique key for this user 
  key_t keyid = ftok(m_UserPreferenceFile.c_str(), IPC_VERSION);

  // Get a handle to shared memory
  m_IPCHandle = shmget(keyid, msize, IPC_CREAT | 0644);

  // There may be an error!
  if(m_IPCHandle < 0)
    {
    cerr << "Shared memory (shmget) error: " << strerror(errno) << endl;
    cerr << "Multisession support is disabled" << endl;
    m_IPCSharedData = NULL;
    }
  else
    {
    // Get a pointer to shared data block
    m_IPCSharedData = shmat(m_IPCHandle, (void *) 0, 0);

    // Check errors again
    if(!m_IPCSharedData)
      {
      cerr << "Shared memory (shmat) error: " << strerror(errno) << endl;
      cerr << "Multisession support is disabled" << endl;
      m_IPCSharedData = NULL;
      }
    }

#endif

}

bool 
SystemInterface
::IPCRead(IPCMessage &msg)
{
  // Must have some shared memory
  if(!m_IPCSharedData) 
    return false;

  // Read the first short, make sure it's the version number
  short *vid = static_cast<short *>(m_IPCSharedData);
  if(*vid != IPC_VERSION)
    return false;

  // Cast shared memory to the message type
  IPCMessage *p = reinterpret_cast<IPCMessage *>(vid + 1);

  // Return a copy
  msg = *p;

  // Store the last sender / id
  m_LastSender = p->sender_pid;
  m_LastReceivedMessageID = p->message_id;

  // Success!
  return true;
}


bool 
SystemInterface
::IPCReadIfNew(IPCMessage &msg)
{
  // Must have some shared memory
  if(!m_IPCSharedData) 
    return false;

  // Read the first short, make sure it's the version number
  short *vid = static_cast<short *>(m_IPCSharedData);
  if(*vid != IPC_VERSION)
    return false;

  // Cast shared memory to the message type
  IPCMessage *p = reinterpret_cast<IPCMessage *>(vid + 1);

  // If we are the sender, the message should be ignored
  if(p->sender_pid == this->GetProcessID())
    return false;

  // If we have already seen this message from this sender, also ignore it
  if(m_LastSender == p->sender_pid && m_LastReceivedMessageID == p->message_id)
    return false;

  // Store the last sender / id
  m_LastSender = p->sender_pid;
  m_LastReceivedMessageID = p->message_id;

  // Return a copy
  msg = *p;

  // Success!
  return true;
}

bool
SystemInterface
::IPCBroadcastCursor(Vector3d cursor)
{
  // Try reading the current memory
  IPCMessage mcurr;
  
  // This may fail, but that's ok
  IPCRead(mcurr);

  // Update the message
  mcurr.cursor = cursor;
  return IPCBroadcast(mcurr);
}

bool
SystemInterface
::IPCBroadcastTrackball(Trackball tball)
{
  // Try reading the current memory
  IPCMessage mcurr;
  
  // This may fail, but that's ok
  IPCRead(mcurr);

  // Update the message
  mcurr.trackball = tball;
  return IPCBroadcast(mcurr);
}

bool
SystemInterface
::IPCBroadcastZoomLevel(double zoom_level)
{
  // Try reading the current memory
  IPCMessage mcurr;
  
  // This may fail, but that's ok
  IPCRead(mcurr);

  // Update the message
  mcurr.zoom_level = zoom_level;
  return IPCBroadcast(mcurr);
}

bool
SystemInterface
::IPCBroadcastViewPosition(Vector2f vec[3])
{
  // Try reading the current memory
  IPCMessage mcurr;
  
  // This may fail, but that's ok
  IPCRead(mcurr);

  // Update the message
  mcurr.viewPositionRelative[0] = vec[0];
  mcurr.viewPositionRelative[1] = vec[1];
  mcurr.viewPositionRelative[2] = vec[2];

  return IPCBroadcast(mcurr);
}

bool
SystemInterface
::IPCBroadcast(IPCMessage msg)
{
  // Write to the shared memory
  if(m_IPCSharedData)
    {
    // Write version number
    short *vid = static_cast<short *>(m_IPCSharedData);
    *vid = IPC_VERSION;

    // Write the process ID
    msg.sender_pid = this->GetProcessID();
    msg.message_id = ++m_MessageID;

    // Write message
    IPCMessage *p = reinterpret_cast<IPCMessage *>(vid + 1);
    *p = msg;

    // Done
    return true;
    }
  return false;
}

void 
SystemInterface
::IPCClose()
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


// Socket headers
#ifdef WIN32
#include <winsock.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif


SystemInterface::UpdateStatus
SystemInterface
::CheckUpdate(std::string &newversion, size_t sec, size_t usec, bool force)
{
  if (force)
    {
    // std::cout << "user initiated update" << std::endl;
    Entry("System.LastUpdateTimeStamp") << time(NULL);
    }
  else
    {
    // Check the last update time stamp
    string lastUpdateTimeStamp = Entry("System.LastUpdateTimeStamp")["00000000"];
    // check for update every week
    if (atoi(lastUpdateTimeStamp.c_str()) == 0)
      {
   	 // std::cout << "initialize auto update" << std::endl;
      Entry("System.LastUpdateTimeStamp") << time(NULL);
	 }
    else if (atoi(lastUpdateTimeStamp.c_str()) + 604800 >= time(NULL))
      {
      // std::cout << "too soon for auto update check" << std::endl;
      return US_TOO_SOON;
      }
    else
      {
   	 // std::cout << "auto update" << std::endl;
      Entry("System.LastUpdateTimeStamp") << time(NULL);
  	 }
    }

  int rv = -1, sockfd = -1, n = -1;
  UpdateStatus us = US_CONNECTION_FAILED;
  
#ifdef WIN32
  // Initialize windows sockets
  WSADATA wsaData;
  if(WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
    return US_CONNECTION_FAILED;
#endif

  // Remaining operations require closing socket/WSA on failure
  try
    {
#ifdef WIN32

    // Resolve the host
    struct hostent *he;
    if((he = gethostbyname("www.itksnap.org")) == NULL)
      throw IRISException("Can't resolve address");

    // Get the IP address
    char *ipaddr = inet_ntoa (*(struct in_addr *)*he->h_addr_list);

    // Set up the socket
    if((sockfd=socket(AF_INET, SOCK_STREAM, 0)) < 0)
      throw IRISException("socket creation failed"); 
    
    // Connect
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(ipaddr);
    sa.sin_port = htons(80);

    if((rv = connect(sockfd, (sockaddr *)&sa, sizeof(sa))) < 0)
      throw IRISException("connect failed");

#else

    // Get address info
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if((rv = getaddrinfo("www.itksnap.org","80",&hints,&res)) != 0)
      throw IRISException("getaddrinfo failed");
    
    // Create socket
    if((sockfd=socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
      throw IRISException("socket creation failed"); 

    // Connect to server
    if((rv = connect(sockfd, res->ai_addr, res->ai_addrlen)) < 0)
      throw IRISException("connect failed");

#endif

    // Create the HTTP request
    ostringstream oss;
    oss << "GET /version/" << SNAPArch 
      << ".txt HTTP/1.1\r\nHost: www.itksnap.org\r\n\r\n";

    // Create HTTP request
    if((n = send(sockfd, oss.str().c_str(), oss.str().length(), 0)) < 0)
      throw IRISException("Can't write to server");

    // Set up select to watch for data available
    fd_set readfds;
    struct timeval tv;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    tv.tv_sec = sec;
    tv.tv_usec = usec;
    if((rv = select(sockfd+1, &readfds, NULL, NULL, &tv)) < 0)
      throw IRISException("Select failed");
    else if(rv == 0)
      throw IRISException("Timed out");

    // Read buffer 
    char buffer[0x8000];
    if((n = recv(sockfd, buffer, 0x7fff, 0)) <= 0)
      throw IRISException("Can't read from server");

    // Parse the output
    istringstream iss(string(buffer, n));
    char line[1024];

    // Check first line
    iss.getline(line, 1024);
    if(strncmp(line,"HTTP/1.1 200 OK",strlen("HTTP/1.1 200 OK")))
      throw IRISException("HTTP request failed");

    // Read lines from output until two blank lines
    while(strlen(line) > 0 && line[0] != '\r' && !iss.eof())
      { iss.getline(line, 1024); }

    // Read the next four values
    unsigned int vmajor = 0, vminor = 0, vpatch = 0;
    string vqual;
    iss >> vmajor;
    iss >> vminor;
    iss >> vpatch;
    iss >> vqual;

    // Format the version in printable format
    ostringstream over;
    over << vmajor << "." << vminor << "." << vpatch << vqual;
    newversion = over.str();

    // Compare version
    if(vmajor > SNAPVersionMajor
      || (vmajor == SNAPVersionMajor && vminor > SNAPVersionMinor)
      || (vmajor == SNAPVersionMajor && vminor == SNAPVersionMinor && vpatch > SNAPVersionPatch))
      us = US_OUT_OF_DATE;
    else 
      us = US_UP_TO_DATE;
    }
  catch(...)
    {
    us = US_CONNECTION_FAILED;
    }

  // Close socket if necessary
  if(sockfd >= 0)
#ifdef WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif

  // Windows cleanup
#ifdef WIN32
  WSACleanup();
#endif

  return us;
}

