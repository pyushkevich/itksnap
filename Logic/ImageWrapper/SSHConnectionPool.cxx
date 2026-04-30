#include "SSHConnectionPool.h"
#include <libssh/libssh.h>
#include <libssh/sftp.h>


SSHConnectionPool::SessionPair
SSHConnectionPool::GetOrCreate(const std::string   &host,
                               const std::string   &username,
                               SSHTunnel::Callback  callback,
                               void                *callback_data)
{
  // Build cache key: "user@host" when a username is present, else just "host"
  std::string key = username.empty() ? host : username + "@" + host;

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
      callback_data);

  if (!ssh)
    throw IRISException("SSHConnectionPool: cannot connect to %s", host.c_str());

  // Initialise the SFTP subsystem on top of the authenticated session
  sftp_session sftp = sftp_new(ssh);
  if (!sftp)
    {
    // ssh_get_error() is valid until we call ssh_free(), so capture it first
    std::string err = ssh_get_error(ssh);
    ssh_disconnect(ssh);
    ssh_free(ssh);
    throw IRISException("SSHConnectionPool: cannot allocate SFTP session for %s: %s",
                        host.c_str(), err.c_str());
    }

  if (sftp_init(sftp) != SSH_OK)
    {
    int code = sftp_get_error(sftp);
    sftp_free(sftp);
    ssh_disconnect(ssh);
    ssh_free(ssh);
    throw IRISException("SSHConnectionPool: SFTP initialisation failed for %s (code %d)",
                        host.c_str(), code);
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
