#include "IPCHandler.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cerrno>
#include <functional>
#include <sstream>
#include <chrono>

#if defined(WIN32)
  #include <process.h>
#else
  #include <unistd.h>
#endif

using namespace std;

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

  // Determine size of shared memory
  size_t msize = message_size + sizeof(Header);

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

  // Set the user data pointer
  if (m_SharedData)
  {
    Header *hptr = static_cast<Header *>(m_SharedData);
    m_UserData = static_cast<void *>(hptr + 1);
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
  // Update the message with sender PID of -1 so that if shared memory is retained
  // for future runs of ITK-SNAP, it will be ignored
  if (m_SharedData)
  {
    // Lock the segment before reading it
    m_Interface->Lock();

    // Access the message header
    Header *header = static_cast<Header *>(m_SharedData);

    // Is the current shared memory created by us? If so, we need to clear it
    if (header->version == m_ProtocolVersion && header->sender_pid == m_ProcessID)
      header->sender_pid = -1;

    // Lock the segment before reading it
    m_Interface->Unlock();
  }

  // Detach from shared memory
  m_Interface->Detach();
  m_SharedData = NULL;
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
