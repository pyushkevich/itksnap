#ifndef SSHCONNECTIONPOOL_H
#define SSHCONNECTIONPOOL_H

#include "SNAPCommon.h"
#include "IRISException.h"
#include "SSHTunnel.h"
#include "itkObject.h"
#include "itkObjectFactory.h"
#include <libssh/sftp.h>
#include <map>
#include <string>

/**
 * Thread-unsafe SSH + SFTP session cache, keyed by "[user@]host".
 *
 * Callers (typically SCPRemoteImageSource) call GetOrCreate() to obtain an
 * authenticated (ssh_session, sftp_session) pair.  On a cache hit the pair is
 * returned immediately, skipping the SSH handshake.  On a miss a fresh
 * session is established via SSHTunnel::OpenSession() and the SFTP subsystem
 * is initialised, then both are cached before being returned.
 *
 * The pool owns all cached sessions and disconnects / frees them in its
 * destructor (CloseAll).  Callers must *not* close a session they obtained
 * from the pool.
 *
 * Intended lifetime: one pool per workspace-load batch, destroyed as soon as
 * all layers have been downloaded.  Create with SSHConnectionPool::New().
 */
class SSHConnectionPool : public itk::Object
{
public:
  irisITKObjectMacro(SSHConnectionPool, itk::Object)

  /** Opaque pair of an authenticated SSH session and its SFTP sub-session. */
  struct SessionPair
  {
    ssh_session  ssh  = nullptr;
    sftp_session sftp = nullptr;
  };

  /**
   * Return a live (ssh, sftp) pair for @p host / @p username.
   *
   * On a cache hit the cached pair is returned without any network activity.
   * On a miss (or a stale disconnected entry) SSHTunnel::OpenSession() is
   * called with @p callback / @p callback_data to establish and authenticate
   * a new SSH session; then sftp_new() + sftp_init() initialise the SFTP
   * subsystem.  The resulting pair is cached and returned.
   *
   * @throws IRISException if the SSH connection or SFTP initialisation fails.
   */
  SessionPair GetOrCreate(const std::string   &host,
                          const std::string   &username,
                          SSHTunnel::Callback  callback,
                          void                *callback_data);

  /** Disconnect and free every cached session.  Called by the destructor. */
  void CloseAll();

protected:
  SSHConnectionPool()  {}
  ~SSHConnectionPool() { CloseAll(); }

private:
  std::map<std::string, SessionPair> m_Sessions;
};

#endif // SSHCONNECTIONPOOL_H
