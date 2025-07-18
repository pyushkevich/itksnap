#ifndef IPCHANDLER_H
#define IPCHANDLER_H

#include <cstddef>
#include <set>
#include <string>

class AbstractSharedMemorySystemInterface
{
public:
  virtual ~AbstractSharedMemorySystemInterface() {};
  virtual void SetKey(const std::string &key) = 0;
  virtual bool Attach() = 0;
  virtual bool Detach() = 0;
  virtual bool Create(unsigned int size) = 0;
  virtual bool IsAttached() = 0;
  virtual std::string GetErrorMessage() = 0;
  virtual void* Data() = 0;
  virtual bool Lock() = 0;
  virtual bool Unlock() = 0;
  virtual int GetProcessID() = 0;
};

/**
 * Base class for IPCHandler. This class contains the definitions of the
 * core methods and is independent of the data structure being shared.
 */
class IPCHandler
{
public:

  IPCHandler(AbstractSharedMemorySystemInterface *interface);
  ~IPCHandler();

  enum AttachStatus {
    IPC_CREATED, IPC_ATTACHED, IPC_ERROR
  };

  /**
   * Attach to the shared memory. The caller should supply the path to the
   * program executable (or a derived string), and a version number. The version
   * number is used to prevent problems when the format of the shared data structure
   * has changed between versions of the program. The version number should be
   * incremented whenever the data structure being shared changes. The last parameter
   * is the size of the message in bytes (obtained using size_of)
   */
  AttachStatus Attach(const char *path, short version, size_t message_size);

  /** Release shared memory */
  void Detach();

  /** Whether the shared memory is attached */
  bool IsAttached();

  /** Read a 'message', i.e., the contents of shared memory */
  bool Read(void *target_ptr);

  /** Read a 'message' but only if it has not been seen before */
  bool ReadIfNew(void *target_ptr);

  /** Broadcast a 'message' (i.e. replace shared memory contents */
  bool Broadcast(const void *message_ptr);

  /** Get the process Id */
  long GetProcessID() { return m_ProcessID; };

  /** Get the PID of last sender */
  long GetLastMessageSenderProcessID() { return m_LastSender; }

protected:

  struct Header
  {
    short version;
    long sender_pid;
    long message_id;
  };


  // Shared data pointer
  void *m_SharedData = nullptr, *m_UserData = nullptr;

  // Size of the shared data message
  size_t m_MessageSize;

  // Version of the protocol (to avoid problems with older code)
  short m_ProtocolVersion;

  // System-specific IPC related stuff
  AbstractSharedMemorySystemInterface *m_Interface;

  // The version of the SNAP-IPC protocol. This way, when versions are different
  // IPC will not work. This is to account for an off chance of a someone running
  // two different versions of SNAP
  static const short IPC_VERSION;

  // Process ID and other values used by IPC
  long m_ProcessID, m_MessageID, m_LastSender, m_LastReceivedMessageID;

  bool IsProcessRunning(int pid);

  // List of known process ids, with status (0 = alive, -1 = dead)
  std::set<long> m_KnownDeadPIDs;
};






#endif // IPCHANDLER_H
