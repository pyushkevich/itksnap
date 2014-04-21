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

#ifdef WIN32
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif //_WIN32_WINNT
#define _WIN32_WINNT	0x0600

#include <Shlobj.h>
#endif //WIN32


//#define WINVER 0x0600
//#define WINVER _WIN32_WINNT
//#define _WIN32_IE 0x0500 

#include "SystemInterface.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISException.h"
#include "GlobalState.h"
#include "SNAPRegistryIO.h"
#include "HistoryManager.h"
#include "UIReporterDelegates.h"
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
  #include <windows.h>
  #include <cstdlib>
#else
  #include <sys/types.h> 
  #include <sys/ipc.h> 
  #include <sys/shm.h> 
  #include <unistd.h>
  #include <sys/time.h>
#endif

using namespace std;
using namespace itksys;

// Initialize the info delegate to NULL
SystemInfoDelegate *SystemInterface::m_SystemInfoDelegate = NULL;

#if defined(WIN32)

std::string
SystemInterface::GetApplicationDataDirectory()
{
  // TODO: when the username is not ASCII, this crashes!
  wchar_t path_w[4096];
  DWORD bufferSize = GetEnvironmentVariableW(L"APPDATA", path_w, 4096);
  if (!bufferSize)
    throw IRISException("Can not access APPDATA path on WIN32.");

  // Convert to UTF8
  std::string utf8_path;
  int nch = WideCharToMultiByte(CP_ACP, 0, path_w, -1, NULL, 0, NULL, NULL);
  if(nch > 0)
    {
    LPSTR pszMBCS = (LPSTR) _alloca ( nch );
    if ( NULL != pszMBCS )
      {
      nch = WideCharToMultiByte ( CP_ACP, 0, path_w, -1, pszMBCS, nch, NULL, NULL );
      if(nch > 0)
        {
        utf8_path=pszMBCS;
        }
      }
    }

  if(utf8_path.length() == 0)
    throw IRISException("Can not convert APPDATA path to multi-byte string");

  // Append the full information
  std::string strPath = utf8_path + "/itksnap.org/ITK-SNAP";

  itksys::SystemTools::ConvertToUnixSlashes(strPath);

  return strPath;
}

/*
std::string
SystemInterface::GetApplicationDataDirectory()
{
  // This old code seems unnecessary - Qt delegate returns the right place for Mac
  // std::string path("~/Library/Application Support/itksnap.org/ITK-SNAP");
  std::string path = m_SystemInfoDelegate->GetApplicationPermanentDataLocation();
  itksys::SystemTools::ConvertToUnixSlashes(path);
  return path;
}
*/


#elif defined(__APPLE__)

std::string
SystemInterface::GetApplicationDataDirectory()
{
  // This old code seems unnecessary - Qt delegate returns the right place for Mac
  // std::string path("~/Library/Application Support/itksnap.org/ITK-SNAP");
  std::string path = m_SystemInfoDelegate->GetApplicationPermanentDataLocation();
  itksys::SystemTools::ConvertToUnixSlashes(path);
  return path;
}

#else

std::string
SystemInterface::GetApplicationDataDirectory()
{
  std::string path("~/.itksnap.org/ITK-SNAP");
  itksys::SystemTools::ConvertToUnixSlashes(path);
  return path;
}

#endif

SystemInterface
::SystemInterface()
{
  // Crash if no delegate
  if(!SystemInterface::m_SystemInfoDelegate)
    throw IRISException("Creating SystemInterface without a global SystemInfoDelegate set!");

  // Initialize the registry
  m_RegistryIO = new SNAPRegistryIO;

  // Initialize the history manager
  m_HistoryManager = new HistoryManager();

  // Register the Image IO factories that are not part of ITK
  itk::ObjectFactoryBase::RegisterFactory( 
    itk::VoxBoCUBImageIOFactory::New() );

  // Make sure we have a preferences directory
  std::string appdir = GetApplicationDataDirectory();
  if(!itksys::SystemTools::MakeDirectory(appdir.c_str()))
     throw IRISException("Unable to create application data directory %s", appdir.c_str());

  // Set the preferences file
  m_UserPreferenceFile = appdir + "/UserPreferences.xml";

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
  delete m_HistoryManager;

  // Detach shared memory
  IPCClose();
}

string SystemInterface::GetFullPathToExecutable() const
{
  assert(m_SystemInfoDelegate);
  return m_SystemInfoDelegate->GetApplicationFile();
}


void
SystemInterface
::SaveUserPreferences()
{
  // Write all the global histories to the registry
  m_HistoryManager->SaveGlobalHistory(this->Folder("IOHistory"));

  // Write the registry to disk
  WriteToXMLFile(m_UserPreferenceFile.c_str());
}


void
SystemInterface
::LoadUserPreferences()
{
  // Check if the file exists, may throw an exception here
  if(SystemTools::FileExists(m_UserPreferenceFile.c_str()))
    {
    // Read the contents of the preferences from file
    ReadFromXMLFile(m_UserPreferenceFile.c_str());

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

  // Read all the global histories from the file.
  m_HistoryManager->LoadGlobalHistory(this->Folder("IOHistory"));
}

void SystemInterface
::LaunchChildSNAP(int argc, char **argv, bool terminate_parent)
{
  // Create new argument list
  char **newargv = new char * [argc + 2];

  // Zeroth argument remains
  newargv[0] = argv[0];

  // First argument is --no-fork
  newargv[1] = new char[40];
  strcpy(newargv[1], "--no-fork");

  // Now copy all the other parameters
  for (int i = 1; i < argc; i++)
    newargv[i + 1] = argv[i];

  newargv[argc+1] = NULL;

  // Now we have a nice argument list to send to the child SNAP process
#ifdef WIN32
  _spawnvp(_P_NOWAIT, exefile.c_str(), argv);
  if(terminate_parent)
    exit(0);
#else
  pid_t pid = fork();
  if(pid == 0)
    {
    /* Make sure we survive our shell */
    setsid();

    /* Restarts the vim process, will not return. */
    execvp(argv[0], newargv);

    /* Should never get here! */
    exit(-1);
    }
  else
    {
    if(terminate_parent)
      exit(0);
    }
#endif
}

void 
SystemInterface
::LaunchChildSNAPSimple(std::list<std::string> args)
{
  // Must have a valid path to the EXE
  std::string exefile = this->GetFullPathToExecutable();
  if(exefile.length() == 0)
    throw IRISException("Path to executable unknown in LaunchChildSNAP");

  // Create the argument array
  char **argv = new char* [args.size()+2];
  int iarg = 0;
  argv[iarg++] = (char *) exefile.c_str();
  for(std::list<std::string>::iterator it=args.begin(); it!=args.end(); ++it)
    argv[iarg++] = (char *) it->c_str();
  argv[iarg++] = NULL;

  // Create child process
  LaunchChildSNAP(args.size()+1, argv, false);
}

std::string
SystemInterface
::FindUniqueCodeForFile(const char *file, bool generate_if_not_found)
{
  // Convert the filename to absolute path
  string path = SystemTools::CollapseFullPath(file);

  // Convert to unix slashes for consistency
  SystemTools::ConvertToUnixSlashes(path);

  // Encode the filename as ASCII
  path = EncodeFilename(path);

  // Get the key associated with this filename
  string key = this->Key("ImageAssociation.Mapping.Element[%s]",path.c_str());

  // Return the existing code for this key
  string code = this->Entry(key)[""];

  // If the code was not found, create a new code if requested
  if(generate_if_not_found && code.size() == 0)
    {
    // Compute a timestamp from the start of computer time
    time_t timestr = time(NULL);

    // Compute a hash
    long hash = 0;
    for(int i = 0; i < path.size(); i+=sizeof(long))
      {
      long word = 0;
      for(int j = i, k = 0; j < path.size() && k < sizeof(long); j++,k++)
        word += ((long) path[j]) << (8 * k);
      hash ^= word;
      }

    // Create a key for the file
    IRISOStringStream scode;
    scode << setfill('0') << setw(16) << hex << timestr;
    scode << setfill('0') << setw(2 * sizeof(long)) << hex << hash;

    // Return the existing key or the new key
    code = scode.str();

    // Store the code
    this->Entry(key) << code;
    }

  return code;
}

bool 
SystemInterface
::FindRegistryAssociatedWithFile(const char *file, Registry &registry)
{
  // Find the code previously associated with that file
  string code = this->FindUniqueCodeForFile(file, false);

  // If the code does not exist, return w/o success
  if(code.length() == 0) return false;

  // Generate the association filename
  string appdir = GetApplicationDataDirectory();
  string assfil = appdir + "/ImageAssociations/" + code + ".xml";

  // Try loading the registry
  try 
    {
    registry.ReadFromXMLFile(assfil.c_str());
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
  // Make sure we have a directory for this category
  std::string appdir = GetApplicationDataDirectory();
  std::string catdir = appdir + std::string("/") + std::string(category);
  if(!itksys::SystemTools::MakeDirectory(catdir.c_str()))
     throw IRISException("Unable to create data directory %s", catdir.c_str());

  // Get the names
  vector<string> names;

  // Get the listing of all files in there
  Directory dlist;
  dlist.Load(catdir.c_str());
  for(size_t i = 0; i < dlist.GetNumberOfFiles(); i++)
    {
    string fname = dlist.GetFile(i);

    // Check regular file
    ostringstream ffull; 
    ffull << catdir << "/" << fname;
    if(SystemTools::FileExists(ffull.str().c_str(), true))
      {
      // Check extension
      if(SystemTools::GetFilenameExtension(fname) == ".xml")
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
  // Make sure we have a directory for this category
  std::string appdir = GetApplicationDataDirectory();
  std::string catdir = appdir + std::string("/") + std::string(category);
  if(!itksys::SystemTools::MakeDirectory(catdir.c_str()))
     throw IRISException("Unable to create data directory %s", catdir.c_str());

  // Create a save filename
  IRISOStringStream sfile;
  sfile << catdir << "/" << EncodeObjectName(name) << ".xml";
  string fname = sfile.str();

  // Check the filename
  if(!SystemTools::FileExists(fname.c_str(), true))
    throw IRISException("Saved object file does not exist");

  Registry reg;
  reg.ReadFromXMLFile(fname.c_str());
  return reg;
}

void 
SystemInterface
::UpdateSavedObject(const char *category, const char *name, Registry &folder)
{
  // Make sure we have a directory for this category
  std::string appdir = GetApplicationDataDirectory();
  std::string catdir = appdir + std::string("/") + std::string(category);
  if(!itksys::SystemTools::MakeDirectory(catdir.c_str()))
     throw IRISException("Unable to create data directory %s", catdir.c_str());

  // Create a save filename
  IRISOStringStream sfile;
  sfile << catdir << "/" << EncodeObjectName(name) << ".xml";
  string fname = sfile.str();

  // Save the data
  folder.WriteToXMLFile(fname.c_str());
}

void 
SystemInterface
::DeleteSavedObject(const char *category, const char *name)
{
  // Make sure we have a directory for this category
  std::string appdir = GetApplicationDataDirectory();
  std::string catdir = appdir + std::string("/") + std::string(category);
  if(!itksys::SystemTools::MakeDirectory(catdir.c_str()))
     throw IRISException("Unable to create data directory %s", catdir.c_str());

  // Create a save filename
  IRISOStringStream sfile;
  sfile << catdir << "/" << EncodeObjectName(name) << ".txt";

  // Save the data
  SystemTools::RemoveFile(sfile.str().c_str());
}




string
SystemInterface
::EncodeFilename(const string &src)
{
  IRISOStringStream sout;

  const char *chararray = src.c_str();
  for(unsigned int i=0;i<strlen(chararray);i++)
    {
    unsigned char c = (unsigned char) chararray[i];
    sout << setw(2) << setfill('0') << (int) c;
    }

  return sout.str();
}

bool 
SystemInterface
::AssociateRegistryWithFile(const char *file, Registry &registry)
{
  // Get the unique code
  string code = FindUniqueCodeForFile(file, true);
  
  // Create an association file in the settings directory
  string appdir = this->GetApplicationDataDirectory();
  string assdir = appdir + "/ImageAssociations";
  if(!SystemTools::MakeDirectory(assdir.c_str()))
    throw IRISException("Unable to create image associations directory %s",
                        assdir.c_str());

  // Create the association filename
  string assfil = assdir + "/" + code + ".xml";

  // Store the registry to that path
  try 
    {
    registry.WriteToXMLFile(assfil.c_str());
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

std::string
SystemInterface
::GetThumbnailAssociatedWithFile(const char *file)
{
  // Get a string giving the thumbnail name
  string code = this->FindUniqueCodeForFile(file, true);

  // Create an association file in the settings directory
  string appdir = this->GetApplicationDataDirectory();
  string thumbdir = appdir + "/Thumbnails";
  if(!SystemTools::MakeDirectory(thumbdir.c_str()))
    throw IRISException("Unable to create thumbnail directory %s",
                        thumbdir.c_str());

  // Create the association filename
  return thumbdir + "/" + code + ".png";
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

// We start versioning at 1000. Every time we change
// the protocol, we should increment the version id
// 0x1004 - Dec 2013, ITK-SNAP 3.0.0 alpha
const short SystemInterface::IPC_VERSION = 0x1004;

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
    cerr << "This error may occur if a user is running two versions of ITK-SNAP" << endl;
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
::IPCBroadcastZoomLevel(AnatomicalDirection dir, double zoom)
{
  // Try reading the current memory
  IPCMessage mcurr;
  
  // This may fail, but that's ok
  IPCRead(mcurr);

  // Update the message
  mcurr.zoom_level[dir] = zoom;
  return IPCBroadcast(mcurr);
}

bool
SystemInterface
::IPCBroadcastViewPosition(AnatomicalDirection dir, Vector2f vec)
{
  // Try reading the current memory
  IPCMessage mcurr;
  
  // This may fail, but that's ok
  IPCRead(mcurr);

  // Update the message
  mcurr.viewPositionRelative[dir] = vec;

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
      Entry("System.LastUpdateTimeStamp") << time(NULL);
      }
    else if (atoi(lastUpdateTimeStamp.c_str()) + 604800 >= time(NULL))
      {
      return US_TOO_SOON;
      }
    else
      {
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
    oss << "GET /version3/" << SNAPArch
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
    string vqual, vdate;
    iss >> vdate;
    iss >> vmajor;
    iss >> vminor;
    iss >> vpatch;
    iss >> vqual;

    // Format the version in printable format
    ostringstream over;
    over << vmajor << "." << vminor << "." << vpatch;
    if(vqual.length())
      over << "-" << vqual;
    newversion = over.str();

    // Compare version
    if(atoi(vdate.c_str()) > atoi(SNAPCurrentVersionReleaseDate))
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

long get_system_time_ms()
{
#ifdef WIN32
  SYSTEMTIME time;
  GetSystemTime(&time);
  WORD millis = (time.wSecond * 1000) + time.wMilliseconds;
  return (long) millis;
#else
  timeval time;
  gettimeofday(&time, NULL);
  long millis = (time.tv_sec * 1000) + (time.tv_usec / 1000);
  return millis;
#endif
}
