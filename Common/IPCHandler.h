#ifndef IPCHANDLER_H
#define IPCHANDLER_H

#include <cstddef>

/**
 * Base class for IPCHandler. This class contains the definitions of the
 * core methods and is independent of the data structure being shared.
 */
class IPCHandler
{
public:

  IPCHandler();
  ~IPCHandler();

  /**
   * Attach to the shared memory. The caller should supply the path to the
   * program executable (or a derived string), and a version number. The version
   * number is used to prevent problems when the format of the shared data structure
   * has changed between versions of the program. The version number should be
   * incremented whenever the data structure being shared changes. The last parameter
   * is the size of the message in bytes (obtained using size_of)
   */
  void Attach(const char *path, short version, size_t message_size);

  /** Release shared memory */
  void Close();

  /** Whether the shared memory is attached */
  bool IsAttached() { return m_SharedData != NULL; }

  /** Read a 'message', i.e., the contents of shared memory */
  bool Read(void *target_ptr);

  /** Read a 'message' but only if it has not been seen before */
  bool ReadIfNew(void *target_ptr);

  /** Broadcast a 'message' (i.e. replace shared memory contents */
  bool Broadcast(const void *message_ptr);

protected:

  struct Header
  {
    short version;
    long sender_pid;
    long message_id;
  };


  // Shared data pointer
  void *m_SharedData, *m_UserData;

  // Size of the shared data message
  size_t m_MessageSize;

  // Version of the protocol (to avoid problems with older code)
  short m_ProtocolVersion;

  // System-specific IPC related stuff
#ifdef WIN32
  void *m_Handle;
#else
  int m_Handle;
#endif

  // The version of the SNAP-IPC protocol. This way, when versions are different
  // IPC will not work. This is to account for an off chance of a someone running
  // two different versions of SNAP
  static const short IPC_VERSION;

  // Process ID and other values used by IPC
  long m_ProcessID, m_MessageID, m_LastSender, m_LastReceivedMessageID;

};






#endif // IPCHANDLER_H
