#include "SSHConnectionPool.h"
#include <libssh/libssh.h>
#include <libssh/sftp.h>


sftp_session
SSHConnectionPool::MakeSFTPSession(ssh_session session)
{
  // Build the SFTP session step-by-step using sftp_new_channel() so each
  // failure produces a specific error instead of sftp_new()'s misleading
  // "out of memory" (libssh 0.11.x bug).
  //
  // Ownership rule: ssh_channel_new() registers the channel in the session's
  // internal channel list.  DO NOT call ssh_channel_free() in error paths
  // before sftp_new_channel() — that causes a double-free / use-after-free
  // because the session's disconnect path also walks the channel list.
  // Instead, let SessionGuard's ssh_disconnect() + ssh_free() clean up any
  // orphaned channel automatically.  Once sftp_new_channel() succeeds the
  // sftp session owns the channel; sftp_free() releases both.
  ssh_channel ch = ssh_channel_new(session);
  if (!ch)
    throw IRISException("Cannot create SSH channel: %s", ssh_get_error(session));

  if (ssh_channel_open_session(ch) != SSH_OK)
    throw IRISException("Cannot open SSH channel: %s", ssh_get_error(session));

  if (ssh_channel_request_subsystem(ch, "sftp") != SSH_OK)
    throw IRISException("Cannot start SFTP subsystem: %s", ssh_get_error(session));

  sftp_session sftp = sftp_new_channel(session, ch);
  if (!sftp)
    throw IRISException("Cannot create SFTP session: %s", ssh_get_error(session));

  if (sftp_init(sftp) != SSH_OK)
    {
    int code = sftp_get_error(sftp);
    sftp_free(sftp);   // closes and frees ch as well
    throw IRISException("SFTP initialization failed (code %d)", code);
    }

  return sftp;
}


SSHConnectionPool::SessionPair
SSHConnectionPool::GetOrCreate(const std::string   &host,
                               const std::string   &username,
                               SSHTunnel::Callback  callback,
                               void                *callback_data,
                               int                  port)
{
  // Build cache key: include port when non-default so that connections to the
  // same host on different ports are cached separately.
  std::string key = username.empty() ? host : username + "@" + host;
  if (port > 0)
    key += ":" + std::to_string(port);

  // Cache hit: return the existing pair if the SSH session is still alive
  auto it = m_Sessions.find(key);
  if (it != m_Sessions.end())
    {
    if (ssh_is_connected(it->second.ssh))
      return it->second;

    // Stale entry — clean up before recreating
    sftp_free(it->second.sftp);
    ssh_disconnect(it->second.ssh);
    ssh_free(it->second.ssh);
    m_Sessions.erase(it);
    }

  // Cache miss: establish a fresh authenticated SSH session
  ssh_session ssh = SSHTunnel::OpenSession(
      host.c_str(),
      username.empty() ? nullptr : username.c_str(),
      nullptr,        // keyfile: auto-detect from ~/.ssh
      callback,
      callback_data,
      false,          // verbose
      port);

  if (!ssh)
    throw IRISException("SSHConnectionPool: cannot connect to %s", host.c_str());

  // Initialise the SFTP subsystem on top of the authenticated session.
  // MakeSFTPSession uses sftp_new_channel() with explicit channel steps for
  // clearer error messages (sftp_new() can misleadingly report "out of memory"
  // when the real failure is a channel-open or subsystem-request rejection).
  sftp_session sftp;
  try
    {
    sftp = MakeSFTPSession(ssh);
    }
  catch (IRISException &exc)
    {
    ssh_disconnect(ssh);
    ssh_free(ssh);
    throw IRISException("SSHConnectionPool: %s for %s", exc.what(), host.c_str());
    }

  SessionPair pair{ssh, sftp};
  m_Sessions[key] = pair;
  return pair;
}


void SSHConnectionPool::CloseAll()
{
  for (auto &kv : m_Sessions)
    {
    sftp_free(kv.second.sftp);
    if (ssh_is_connected(kv.second.ssh))
      ssh_disconnect(kv.second.ssh);
    ssh_free(kv.second.ssh);
    }
  m_Sessions.clear();
}
