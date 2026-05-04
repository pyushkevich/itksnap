#include "ImageIORemote.h"
#include "AbstractSSHAuthDelegate.h"
#include "SSHConnectionPool.h"
#include "SSHTunnel.h"
#include "SystemInterface.h"
#include "UIReporterDelegates.h"
#include <itksys/SystemTools.hxx>
#include <libssh/sftp.h>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <cstring>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif


// -----------------------------------------------------------------------
//  Internal helpers
// -----------------------------------------------------------------------
namespace {

/** Create a fresh unique temp directory under the platform temp location. */
std::string MakeTempDir()
{
  std::string base = SystemInterface::GetSystemInfoDelegate()->GetTempDirectory();
  if (base.empty())
    throw IRISException("SystemInfoDelegate::GetTempDirectory() returned an empty path");

#ifdef _WIN32
  char name[MAX_PATH + 1] = "";
  if (!GetTempFileNameA(base.c_str(), "itksnap_remote", 0, name))
    throw IRISException("Cannot generate temp filename for remote image in: %s", base.c_str());

  std::string dir = std::string(name) + "_d";
  DeleteFileA(name);
  if (!CreateDirectoryA(dir.c_str(), nullptr))
    throw IRISException("Cannot create temp directory: %s", dir.c_str());
  return dir;
#else
  std::string tmpl = base + "/itksnap_remote_XXXXXX";
  std::vector<char> buf(tmpl.begin(), tmpl.end());
  buf.push_back('\0');
  char *dir = mkdtemp(buf.data());
  if (!dir)
    throw IRISException("Cannot create temp directory under: %s", base.c_str());
  return dir;
#endif
}

/** RAII wrapper: closes an sftp_file on scope exit. */
struct SFTPFileGuard {
  sftp_file f;
  ~SFTPFileGuard() { sftp_close(f); }
};

} // anonymous namespace


// -----------------------------------------------------------------------
//  SCPRemoteImageSource
// -----------------------------------------------------------------------

std::string
SCPRemoteImageSource::Download(const std::string &url)
{
  // Expected form:  scp://[user@]host/path  or  sftp://[user@]host/path
  std::size_t scheme_len = 0;
  if (url.substr(0, 6) == "scp://")       scheme_len = 6;
  else if (url.substr(0, 7) == "sftp://") scheme_len = 7;
  else
    throw IRISException("SCPRemoteImageSource: URL must start with scp:// or sftp://: %s", url.c_str());

  std::string rest = url.substr(scheme_len);   // "[user@]host/path"

  auto slash = rest.find('/');
  if (slash == std::string::npos)
    throw IRISException("SCPRemoteImageSource: malformed URL (no path after host): %s", url.c_str());

  std::string hostpart   = rest.substr(0, slash);   // "user@host" or "host"
  std::string remotepath = rest.substr(slash);       // "/absolute/path/to/file.ext"

  // Split optional user@ from host
  std::string host, username;
  auto at = hostpart.rfind('@');
  if (at != std::string::npos)
  {
    username = hostpart.substr(0, at);
    host     = hostpart.substr(at + 1);
  }
  else
  {
    host = hostpart;
  }

  // Preserve the original basename so ITK extension detection works.
  std::string basename = itksys::SystemTools::GetFilenameName(remotepath);
  if (basename.empty())
    throw IRISException("SCPRemoteImageSource: URL path ends in a directory, not a file: %s", url.c_str());

  // Callback data bundles the last error string and the optional auth delegate.
  struct SessionCBData
  {
    std::string              error;
    AbstractSSHAuthDelegate *authDelegate = nullptr;
    // Mutable username: may be filled in by PromptForUsernameAndPassword when
    // no username is present in the URL.
    std::string             &username;
  };
  SessionCBData cbdata{std::string{}, m_AuthDelegate, username};

  auto session_cb = [](SSHTunnel::CallbackType type,
                       SSHTunnel::CallbackInfo  info,
                       void                    *data) -> SSHTunnel::CallbackResponse
  {
    auto &d = *static_cast<SessionCBData *>(data);
    switch (type)
    {
      case SSHTunnel::CB_ERROR:
        d.error = std::get<SSHTunnel::ErrorInfo>(info).error_message;
        break;

      case SSHTunnel::CB_PROMPT_PASSWORD:
      {
        auto &pi = std::get<SSHTunnel::PromptPasswordInfo>(info);
        if (!d.authDelegate)
          {
          d.error = "SSH password authentication required but no UI is available. "
                    "Configure key-based authentication or supply credentials in the URL.";
          return {1, ""};
          }
        std::string pw;
        if (pi.username.empty())
          {
          // No username from URL or SSH config — ask for both
          std::string user;
          if (!d.authDelegate->PromptForUsernameAndPassword(pi.server, pi.error_message, user, pw))
            return {1, ""}; // cancelled
          // Write the username back so libssh can use it on the next attempt
          d.username = user;
          // Re-attempt with the provided username by returning it alongside the password.
          // SSHTunnel will call ssh_userauth_password(session, nullptr, pw) — the username
          // was already set on the session via ssh_options_set before the first prompt.
          // We update it here for display purposes; libssh uses whatever is on the session.
          }
        else
          {
          if (!d.authDelegate->PromptForPassword(pi.server, pi.username, pi.error_message, pw))
            return {1, ""}; // cancelled
          }
        return {0, pw};
      }

      case SSHTunnel::CB_PROMPT_PASSKEY:
      {
        auto &pi = std::get<SSHTunnel::PromptPasskeyInfo>(info);
        if (!d.authDelegate)
          {
          d.error = "SSH passphrase required but no UI is available.";
          return {1, ""};
          }
        std::string pp;
        if (!d.authDelegate->PromptForPassphrase(pi.keyfile, pi.error_message, pp))
          return {1, ""}; // cancelled
        return {0, pp};
      }

      default:
        break;
    }
    return {0, ""};
  };

  // Acquire an authenticated SSH + SFTP session.  Two paths:
  // Acquire an authenticated SSH + SFTP session via the connection pool.
  // When no external pool was provided (single-file download) we create a
  // temporary one that lives for the duration of this call; its destructor
  // calls CloseAll(), disconnecting and freeing the session automatically.
  SmartPtr<SSHConnectionPool> tmp_pool;
  if (!m_ConnectionPool)
    tmp_pool = SSHConnectionPool::New();

  SSHConnectionPool *pool = m_ConnectionPool ? m_ConnectionPool : tmp_pool.GetPointer();

  SSHConnectionPool::SessionPair pair;
  try
    {
    pair = pool->GetOrCreate(host, username, session_cb, &cbdata);
    }
  catch (IRISException &)
    {
    // Re-throw with the specific SSH error captured by the callback, which is
    // more informative than the generic pool-level message.
    if (!cbdata.error.empty())
      throw IRISException("SSH connection to %s failed: %s",
                          host.c_str(), cbdata.error.c_str());
    throw;
    }

  ssh_session  session = pair.ssh;
  sftp_session sftp    = pair.sftp;

  // Query remote file size for progress display (best-effort; 0 = unknown)
  std::size_t file_size = 0;
  if (sftp_attributes attrs = sftp_stat(sftp, remotepath.c_str()))
    {
    file_size = static_cast<std::size_t>(attrs->size);
    sftp_attributes_free(attrs);
    }

  // Open the remote file for reading
  sftp_file remote_file = sftp_open(sftp, remotepath.c_str(), O_RDONLY, 0);
  if (!remote_file)
    throw IRISException("SCPRemoteImageSource: Cannot open remote file '%s': %s",
                        remotepath.c_str(), ssh_get_error(session));

  SFTPFileGuard file_guard{remote_file};

  // Create local temp directory and destination path
  std::string tmpdir = MakeTempDir();
  std::string dest   = tmpdir + "/" + basename;

  std::ofstream outfile(dest, std::ios::binary);
  if (!outfile)
    throw IRISException("SCPRemoteImageSource: Cannot create local file '%s'", dest.c_str());

  // Stream remote file to disk in chunks, reporting progress each iteration
  constexpr std::size_t chunk = 65536;
  std::vector<char>     buf(chunk);
  std::size_t           bytes_read = 0;

  ReportProgress(url, 0, file_size);

  while (true)
  {
    ssize_t n = sftp_read(remote_file, buf.data(), chunk);
    if (n == 0)
      break;
    if (n < 0)
      throw IRISException("SCPRemoteImageSource: SFTP read error on '%s': %s",
                          remotepath.c_str(), ssh_get_error(session));
    outfile.write(buf.data(), n);
    if (!outfile)
      throw IRISException("SCPRemoteImageSource: Write error to '%s'", dest.c_str());
    bytes_read += static_cast<std::size_t>(n);
    ReportProgress(url, bytes_read, file_size);
  }

  // If file size was unknown the loop never posted a 100% signal; do it now
  // so the callback can finalise its display unconditionally.
  if (file_size == 0)
    ReportProgress(url, bytes_read, bytes_read);

  outfile.close();
  return dest;
}


// -----------------------------------------------------------------------
//  Free functions
// -----------------------------------------------------------------------

DownloadProgressCallback MakeStdoutProgressCallback()
{
  return [](const std::string &url,
            std::size_t        bytes_done,
            std::size_t        bytes_total)
  {
    // Use the basename of the URL as the display name
    std::string name = itksys::SystemTools::GetFilenameName(url);
    if (name.empty())
      name = url;

    constexpr int   bar_width = 30;
    const double    mb_done   = bytes_done  / (1024.0 * 1024.0);

    if (bytes_total > 0)
      {
      const double mb_total = bytes_total / (1024.0 * 1024.0);
      const int    pct      = static_cast<int>(bytes_done * 100 / bytes_total);
      const int    filled   = pct * bar_width / 100;

      std::cout << "\r  " << name << ": "
                << std::setw(3) << pct << "% ["
                << std::string(filled,             '#')
                << std::string(bar_width - filled, ' ')
                << "] "
                << std::fixed << std::setprecision(1)
                << mb_done << "/" << mb_total << " MB"
                << std::flush;

      if (bytes_done >= bytes_total)
        std::cout << std::endl;
      }
    else
      {
      // File size unknown — show bytes downloaded without a percentage
      std::cout << "\r  " << name << ": "
                << std::fixed << std::setprecision(1)
                << mb_done << " MB"
                << std::flush;

      // Completion is signalled by bytes_done == bytes_total > 0
      if (bytes_done > 0 && bytes_done == bytes_total)
        std::cout << std::endl;
      }
  };
}


bool IsRemoteImageURL(const std::string &path)
{
  return path.find("://") != std::string::npos;
}

SmartPtr<RemoteImageSource> CreateRemoteImageSource(const std::string &url)
{
  if (url.substr(0, 6) == "scp://" || url.substr(0, 7) == "sftp://")
    return SCPRemoteImageSource::New().GetPointer();

  throw IRISException("No remote image handler for URL: %s", url.c_str());
}
