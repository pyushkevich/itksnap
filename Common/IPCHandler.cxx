#include "IPCHandler.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cerrno>
#include <functional>
#include <sstream>

#if defined(WIN32)
  #ifdef _WIN32_WINNT
  #undef _WIN32_WINNT
  #endif //_WIN32_WINNT
  #define _WIN32_WINNT	0x0600

  #include <Shlobj.h>
  #include <iostream>
  #include <process.h>
  #include <windows.h>
  #include <cstdlib>
#elif defined(__APPLE__)
  #include <sys/mman.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <unistd.h>
#else
  #include <sys/types.h>
  #include <sys/ipc.h>
  #include <sys/shm.h>
  #include <unistd.h>
  #include <signal.h>
  #include <sys/time.h>
#endif

using namespace std;

void IPCHandler::Attach(const char *path, short version, size_t message_size)
{
  // Initialize the data pointer
  m_SharedData = nullptr;
  m_UserData = nullptr;

  // Store the size of the actual message
  m_MessageSize = message_size;

  // Save the protocol version
  m_ProtocolVersion = version;

  // Determine size of shared memory
  size_t msize = message_size + sizeof(Header);

#if defined(WIN32)
  // Create a shared memory block (key based on the preferences file)
  m_Handle = CreateFileMappingA(
    INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD) msize, path);

  // If the return value is NULL, something failed
  if(m_Handle)
    {
    // Attach to the shared memory block
    m_SharedData = MapViewOfFile(m_Handle, FILE_MAP_ALL_ACCESS, 0, 0, msize);
    }

#elif defined(__APPLE__)

  // Generate a hash based on file and version
  std::ostringstream oss; oss << path << "_" << version;
  std::string to_hash = oss.str();
  std::size_t str_hash = std::hash<std::string>{}(to_hash);

  // Generate a complete key
  // std::ostringstream oss_key;
  // oss_key << "5A636Q488D.itksnap." << str_hash;
  m_SharedMemoryObjectName = "5A636Q488D.itksnap";

  // Try to connect to an existing memory space
  m_Handle = shm_open(m_SharedMemoryObjectName.c_str(), O_RDWR, 0644);
  if(m_Handle < 0)
    {
    m_Handle = shm_open(m_SharedMemoryObjectName.c_str(), O_RDWR | O_CREAT, 0644);
    if(m_Handle < 0)
      {
      cerr << "Shared memory (shmget) error: " << strerror(errno) << endl;
      cerr << "This error may occur if a user is running two versions of ITK-SNAP" << endl;
      cerr << "Multisession support is disabled" << endl;
      return;
      }

    // Set the size of the chunk
    ftruncate(m_Handle, msize);
    }

  m_SharedData = mmap(nullptr, msize, PROT_WRITE, MAP_SHARED, m_Handle, 0);

  // Check errors again
  if(m_SharedData == MAP_FAILED)
    {
    cerr << "Shared memory (shmat) error: " << strerror(errno) << endl;
    cerr << "Multisession support is disabled" << endl;
    m_SharedData = nullptr;
    return;
    }

#else

  // Create a unique key for this user
  key_t keyid = ftok(path, version);

  // Get a handle to shared memory
  m_Handle = shmget(keyid, msize, IPC_CREAT | 0644);

  // There may be an error!
  if(m_Handle < 0)
    {
    cerr << "Shared memory (shmget) error: " << strerror(errno) << endl;
    cerr << "This error may occur if a user is running two versions of ITK-SNAP" << endl;
    cerr << "Multisession support is disabled" << endl;
    m_SharedData = NULL;
    }
  else
    {
    // Get a pointer to shared data block
    m_SharedData = shmat(m_Handle, (void *) 0, 0);

    // Check errors again
    if(!m_SharedData)
      {
      cerr << "Shared memory (shmat) error: " << strerror(errno) << endl;
      cerr << "Multisession support is disabled" << endl;
      m_SharedData = NULL;
      }
    }

#endif

  // Set the user data pointer
  if(m_SharedData)
    {
    Header *hptr = static_cast<Header *>(m_SharedData);
    m_UserData = static_cast<void *>(hptr + 1);
    }
}

bool IPCHandler::Read(void *target_ptr)
{
  // Must have some shared memory
  if(!m_SharedData)
    return false;

  // Read the header, make sure it's the right version number
  Header *header = static_cast<Header *>(m_SharedData);
  if(header->version != m_ProtocolVersion)
    return false;

  // Store the last sender / id
  m_LastSender = header->sender_pid;
  m_LastReceivedMessageID = header->message_id;

  // Copy the message to the target pointer
  memcpy(target_ptr, m_UserData, m_MessageSize);

  // Success!
  return true;
}

bool IPCHandler::ReadIfNew(void *target_ptr)
{
  // Must have some shared memory
  if(!m_SharedData)
    return false;

  // Read the header, make sure it's the right version number
  Header *header = static_cast<Header *>(m_SharedData);
  if(header->version != m_ProtocolVersion)
    return false;

  // Ignore our own messages or messages from dead processes
  if(header->sender_pid == m_ProcessID || header->sender_pid == -1)
    return false;

  // If we have already seen this message from this sender, also ignore it
  if(m_LastSender == header->sender_pid && m_LastReceivedMessageID == header->message_id)
    return false;

  // Store the last sender / id
  m_LastSender = header->sender_pid;
  m_LastReceivedMessageID = header->message_id;

  // Copy the message to the target pointer
  memcpy(target_ptr, m_UserData, m_MessageSize);

  // Success!
  return true;
}


bool
IPCHandler
::Broadcast(const void *message_ptr)
{
  // Write to the shared memory
  if(m_SharedData)
    {
    // Access the message header
    Header *header = static_cast<Header *>(m_SharedData);

    // Write version number
    header->version = m_ProtocolVersion;

    // Write the process ID
    header->sender_pid = m_ProcessID;
    header->message_id = ++m_MessageID;

    // Copy the message contents into the shared memory
    memcpy(m_UserData, message_ptr, m_MessageSize);

    // Done
    return true;
    }

  return false;
}

void IPCHandler::Close()
{
  // Update the message with sender PID of -1 so that if shared memory is retained
  // for future runs of ITK-SNAP, it will be ignored
  if(m_SharedData)
    {
    // Access the message header
    Header *header = static_cast<Header *>(m_SharedData);

    // Is the current shared memory created by us? If so, we need to clear it
    if(header->version == m_ProtocolVersion && header->sender_pid == m_ProcessID)
      header->sender_pid = -1;
    }

#if defined(WIN32)
  CloseHandle(m_Handle);
#elif defined(__APPLE__)
  munmap(m_SharedData, m_MessageSize + sizeof(Header));

  // Unlinking causes new processes to use a new handle and I can't find a way
  // to keep track of the number of attached processes without messing with the
  // header, so I am going to let this be
  // shm_unlink(m_SharedMemoryObjectName.c_str());
#else
  // Detach from the shared memory segment
  shmdt(m_SharedData);

  // If there is noone attached to the memory segment, destroy it
  struct shmid_ds dsinfo;
  shmctl(m_Handle, IPC_STAT, &dsinfo);
  if(dsinfo.shm_nattch == 0)
    shmctl(m_Handle, IPC_RMID, NULL);
#endif

  m_SharedData = NULL;
}


IPCHandler::IPCHandler()
{
  // Set the message ID and last sender/message id values
  m_LastReceivedMessageID = -1;
  m_LastSender = -1;
  m_MessageID = 0;

  // Reset the shared memory
  m_SharedData = NULL;

  // Get the process ID
#ifdef WIN32
  m_ProcessID = _getpid();
#else
  m_ProcessID = getpid();
#endif
}

IPCHandler::~IPCHandler()
{

}
