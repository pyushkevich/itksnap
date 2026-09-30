#ifndef IPCHANDLER_H
#define IPCHANDLER_H

#include <cstddef>
#include <set>
#include <string>
#include <vector>
#include <utility>

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
  virtual size_t GetSize() = 0;
};

/** One slot in the instance directory — one entry per running ITK-SNAP window. */
struct IPCDirectoryEntry
{
  long pid;              // 0 = empty slot
  char title[256];       // human-readable window title (main image filename)
  long pending_drop_id;  // incremented by sender on each write; 0 = never written
  char pending_drop[2048]; // filename or URL to open, UTF-8, null-terminated
};

static const int IPC_MAX_INSTANCES = 16;

struct IPCDirectory
{
  IPCDirectoryEntry entries[IPC_MAX_INSTANCES];
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

  /** Claim a slot in the instance directory, writing our PID and title. */
  void ClaimSlot(const char *title);

  /** Update the title in our already-claimed slot. */
  void UpdateSlotTitle(const char *title);

  /** Release our slot (called from Detach, including crash path). */
  void ReleaseSlot();

  /** Return {pid, title} for all live instances (excluding ourselves). */
  std::vector<std::pair<long, std::string>> ReadDirectory();

  /** Write a file/URL into target's pending_drop field. */
  void WriteDropRequest(long target_pid, const char *filename);

  /** Read and clear our own pending_drop field. Returns true if a new request was found. */
  bool ReadDropRequest(std::string &out);

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
  long m_ProcessID, m_MessageID, m_LastSender, m_LastReceivedMessageID, m_LastDropId;

  bool IsProcessRunning(int pid);

  // List of known process ids, with status (0 = alive, -1 = dead)
  std::set<long> m_KnownDeadPIDs;

  // Pointer into shared memory where the directory lives (after Header+message)
  IPCDirectory *m_Directory = nullptr;
  bool          m_DirectoryAvailable = false;

  IPCDirectory *GetDirectory();
};






#endif // IPCHANDLER_H
