#include "IPCHandler.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cerrno>
#include <chrono>

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <signal.h>
#endif

using namespace std;

bool
IPCHandler::IsProcessRunning(int pid)
{
#if defined(_WIN32)
  HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)pid);
  if (h) { CloseHandle(h); return true; }
  return false;
#else
  return kill((pid_t)pid, 0) == 0;
#endif
}

IPCHandler::AttachStatus
IPCHandler::Attach(const char *path, short version, size_t message_size)
{
  // Initialize the data pointer
  m_SharedData = nullptr;
  m_UserData = nullptr;

  // Store the size of the actual message
  m_MessageSize = message_size;

  // Save the protocol version
  m_ProtocolVersion = version;

  // Determine size of shared memory: header + message + instance directory
  size_t msize = sizeof(Header) + message_size + sizeof(IPCDirectory);

  // Set the shared memory key
  m_Interface->SetKey("5A636Q488E.itksnap");

  // Attach or create shared memory
  AttachStatus status = IPC_ATTACHED;
  if (!m_Interface->Attach())
  {
    m_Interface->Create(msize);
    status = IPC_CREATED;
  }

  // Check if attached
  if (m_Interface->IsAttached())
  {
    m_SharedData = m_Interface->Data();
  }

  if (!m_SharedData)
  {
    cerr << "Error attaching to or creating shared memory: " << strerror(errno) << endl;
    cerr << "This error may occur if a user is running two versions of ITK-SNAP" << endl;
    cerr << "Multisession support is disabled" << endl;
    return IPC_ERROR;
  }

  // Set the user data pointer and the directory pointer
  if (m_SharedData)
  {
    Header *hptr = static_cast<Header *>(m_SharedData);
    m_UserData = static_cast<void *>(hptr + 1);

    // The directory sits after the message. Guard against attaching to an
    // older, smaller segment (created before the directory was added).
    m_DirectoryAvailable = (m_Interface->GetSize() >= msize);
    if (m_DirectoryAvailable)
      m_Directory = reinterpret_cast<IPCDirectory *>(
        static_cast<char *>(m_UserData) + message_size);
  }

  return status;
}

bool IPCHandler::Read(void *target_ptr)
{
  // Must have some shared memory
  if(!m_SharedData)
    return false;

  // Lock the segment before reading it
  m_Interface->Lock();

  // Read the header, make sure it's the right version number
  Header *header = static_cast<Header *>(m_SharedData);
  bool success = false;
  if(header->version == m_ProtocolVersion)
  {
    // Store the last sender / id
    m_LastSender = header->sender_pid;
    m_LastReceivedMessageID = header->message_id;

    // Copy the message to the target pointer
    memcpy(target_ptr, m_UserData, m_MessageSize);
    success = true;
  }

  // Unlock the segment
  m_Interface->Unlock();

  // Success!
  return success;
}

bool
IPCHandler::ReadIfNew(void *target_ptr)
{
  // Must have some shared memory
  if (!m_SharedData)
    return false;

  // Lock the segment before reading it
  m_Interface->Lock();

  // Read the header, make sure it's the right version number
  Header *header = static_cast<Header *>(m_SharedData);
  bool    success = false;
  if (header->version == m_ProtocolVersion)
  {
    // Ignore our own messages or messages from dead processes
    if (header->sender_pid != m_ProcessID && header->sender_pid != -1)
    {
      // If we have already seen this message from this sender, also ignore it
      if (m_LastSender != header->sender_pid || m_LastReceivedMessageID != header->message_id)
      {
        std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);

        // Store the last sender / id
        m_LastSender = header->sender_pid;
        m_LastReceivedMessageID = header->message_id;

        // Copy the message to the target pointer
        memcpy(target_ptr, m_UserData, m_MessageSize);
        success = true;
      }
    }
  }

  // Unlock the segment
  m_Interface->Unlock();

  // Success!
  return success;
}


bool
IPCHandler::Broadcast(const void *message_ptr)
{
  // Write to the shared memory
  if (m_SharedData)
  {
    // Lock the segment before reading it
    m_Interface->Lock();

    // Access the message header
    Header *header = static_cast<Header *>(m_SharedData);

    // Write version number
    header->version = m_ProtocolVersion;

    // Write the process ID
    header->sender_pid = m_ProcessID;
    header->message_id = ++m_MessageID;

    // Copy the message contents into the shared memory
    memcpy(m_UserData, message_ptr, m_MessageSize);

    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);

    // Unlock the segment
    m_Interface->Unlock();

    // Done
    return true;
  }

  return false;
}

void
IPCHandler::Detach()
{
  // Release our instance directory slot before detaching
  ReleaseSlot();

  if (m_SharedData)
  {
    m_Interface->Lock();

    Header *header = static_cast<Header *>(m_SharedData);
    if (header->version == m_ProtocolVersion && header->sender_pid == m_ProcessID)
      header->sender_pid = -1;

    m_Interface->Unlock();
  }

  m_Interface->Detach();
  m_SharedData  = nullptr;
  m_Directory   = nullptr;
  m_DirectoryAvailable = false;
}

IPCDirectory *
IPCHandler::GetDirectory()
{
  if (!m_DirectoryAvailable || !m_Directory)
    return nullptr;
  return m_Directory;
}

void
IPCHandler::ClaimSlot(const char *title)
{
  IPCDirectory *dir = GetDirectory();
  if (!dir) return;

  m_Interface->Lock();
  // Find an empty or dead slot and claim it
  int target = -1;
  for (int i = 0; i < IPC_MAX_INSTANCES; i++)
  {
    long p = dir->entries[i].pid;
    if (p == 0 || p == m_ProcessID || !IsProcessRunning((int)p))
    {
      target = i;
      break;
    }
  }
  if (target >= 0)
  {
    dir->entries[target].pid = m_ProcessID;
    std::strncpy(dir->entries[target].title, title ? title : "", 255);
    dir->entries[target].title[255] = '\0';
  }
  m_Interface->Unlock();
}

void
IPCHandler::UpdateSlotTitle(const char *title)
{
  IPCDirectory *dir = GetDirectory();
  if (!dir) return;

  m_Interface->Lock();
  for (int i = 0; i < IPC_MAX_INSTANCES; i++)
  {
    if (dir->entries[i].pid == m_ProcessID)
    {
      std::strncpy(dir->entries[i].title, title ? title : "", 255);
      dir->entries[i].title[255] = '\0';
      break;
    }
  }
  m_Interface->Unlock();
}

void
IPCHandler::ReleaseSlot()
{
  IPCDirectory *dir = GetDirectory();
  if (!dir) return;

  m_Interface->Lock();
  for (int i = 0; i < IPC_MAX_INSTANCES; i++)
  {
    if (dir->entries[i].pid == m_ProcessID)
    {
      dir->entries[i].pid = 0;
      dir->entries[i].title[0] = '\0';
      break;
    }
  }
  m_Interface->Unlock();
}

std::vector<std::pair<long, std::string>>
IPCHandler::ReadDirectory()
{
  std::vector<std::pair<long, std::string>> result;
  IPCDirectory *dir = GetDirectory();
  if (!dir) return result;

  m_Interface->Lock();
  for (int i = 0; i < IPC_MAX_INSTANCES; i++)
  {
    long p = dir->entries[i].pid;
    if (p != 0 && IsProcessRunning((int)p))
      result.push_back({p, std::string(dir->entries[i].title)});
  }
  m_Interface->Unlock();

  return result;
}

bool
IPCHandler::IsAttached()
{
  return m_Interface->IsAttached();
}

IPCHandler::IPCHandler(AbstractSharedMemorySystemInterface *interface)
{
  // Save the pointer to the interface
  m_Interface = interface;

  // Set the message ID and last sender/message id values
  m_LastReceivedMessageID = -1;
  m_LastSender = -1;
  m_MessageID = 0;
  m_LastDropId = -1;

  // Reset the shared memory
  m_SharedData = NULL;

  // Get the process ID
  m_ProcessID = m_Interface->GetProcessID();
}

void
IPCHandler::WriteDropRequest(long target_pid, const char *filename)
{
  IPCDirectory *dir = GetDirectory();
  if (!dir) return;

  m_Interface->Lock();
  for (int i = 0; i < IPC_MAX_INSTANCES; i++)
  {
    if (dir->entries[i].pid == target_pid)
    {
      std::strncpy(dir->entries[i].pending_drop, filename ? filename : "", 2047);
      dir->entries[i].pending_drop[2047] = '\0';
      dir->entries[i].pending_drop_id++;
      break;
    }
  }
  m_Interface->Unlock();
}

bool
IPCHandler::ReadDropRequest(std::string &out)
{
  IPCDirectory *dir = GetDirectory();
  if (!dir) return false;

  m_Interface->Lock();
  bool found = false;
  for (int i = 0; i < IPC_MAX_INSTANCES; i++)
  {
    if (dir->entries[i].pid == m_ProcessID)
    {
      if (dir->entries[i].pending_drop_id != m_LastDropId &&
          dir->entries[i].pending_drop[0] != '\0')
      {
        std::cout << "PID=" << m_ProcessID
                  << " pending_drop_id=" << dir->entries[i].pending_drop_id
                  << " pending_drop=\"" << dir->entries[i].pending_drop << "\""
                  << std::endl;
        out = std::string(dir->entries[i].pending_drop);
        m_LastDropId = dir->entries[i].pending_drop_id;
        dir->entries[i].pending_drop[0] = '\0';
        found = true;
      }
      break;
    }
  }
  m_Interface->Unlock();
  return found;
}

IPCHandler::~IPCHandler()
{
}
