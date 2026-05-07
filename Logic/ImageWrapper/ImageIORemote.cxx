#include "ImageIORemote.h"
#include "AbstractSSHAuthDelegate.h"
#include "RemoteFileCache.h"
#include "RESTClient.h"
#include "SSHConnectionPool.h"
#include "SSHTunnel.h"
#include "SystemInterface.h"
#include "UIReporterDelegates.h"
#include <itksys/SystemTools.hxx>
#include <libssh/sftp.h>
#include <fcntl.h>
#include <fstream>
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

  // Split optional :port from host (e.g. "localhost:2222")
  int port = 0;
  auto colon = host.find(':');
  if (colon != std::string::npos)
  {
    try { port = std::stoi(host.substr(colon + 1)); } catch (...) {}
    host = host.substr(0, colon);
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
        std::string pw;
        if (pi.username.empty())
          {
          // No username from URL or SSH config — ask for both
          std::string user;
          if (!d.authDelegate->PromptForUsernameAndPassword(pi.server, pi.error_message, user, pw))
            return {1, ""}; // cancelled
          d.username = user;
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
    pair = pool->GetOrCreate(host, username, session_cb, &cbdata, port);
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

  // Query remote file attributes: size for progress display, mtime for cache validation.
  std::size_t file_size   = 0;
  uint64_t    attr_size   = 0;
  uint32_t    attr_mtime  = 0;
  if (sftp_attributes attrs = sftp_stat(sftp, remotepath.c_str()))
    {
    attr_size  = attrs->size;
    attr_mtime = attrs->mtime;
    file_size  = static_cast<std::size_t>(attr_size);
    sftp_attributes_free(attrs);
    }

  // Cache lookup: if we have a valid cached copy, return it immediately.
  if (m_FileCache)
    {
    std::string cached = m_FileCache->Lookup(url, attr_size, attr_mtime);
    if (!cached.empty())
      {
      ReportProgress(file_size, file_size);
      return cached;
      }
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

  ReportProgress(0, file_size);

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
    ReportProgress(bytes_read, file_size);
  }

  // If file size was unknown the loop never posted a 100% signal; do it now
  // so the callback can finalise its display unconditionally.
  if (file_size == 0)
    ReportProgress(bytes_read, bytes_read);

  outfile.close();

  // Store in cache (may copy file to cache dir; returns final path to use).
  if (m_FileCache)
    return m_FileCache->Store(url, dest, attr_size, attr_mtime);

  return dest;
}


// -----------------------------------------------------------------------
//  HTTPRemoteImageSource
// -----------------------------------------------------------------------

std::string
HTTPRemoteImageSource::Download(const std::string &url)
{
  std::string basename = itksys::SystemTools::GetFilenameName(url);
  // Strip any query string from the basename so extension detection works.
  auto qmark = basename.find('?');
  if (qmark != std::string::npos)
    basename = basename.substr(0, qmark);
  if (basename.empty())
    throw IRISException("HTTPRemoteImageSource: cannot determine filename from URL: %s", url.c_str());

  std::string tmpdir = MakeTempDir();
  std::string dest   = tmpdir + "/" + basename;

  FILE *outfile = fopen(dest.c_str(), "wb");
  if (!outfile)
    throw IRISException("HTTPRemoteImageSource: cannot create local file '%s'", dest.c_str());

  RESTClient<> client;
  client.SetOutputFile(outfile);

  // Adapt DownloadProgressCallback (std::function<bool(size_t,size_t)>) to
  // RESTClient's void(*)(void*, size_t, size_t).  Cancellation is not
  // propagated through RESTClient; the callback is used for progress display only.
  if (m_ProgressCallback)
    {
    client.SetProgressCallback(
      &m_ProgressCallback,
      [](void *data, size_t done, size_t total)
      {
        auto *cb = static_cast<DownloadProgressCallback *>(data);
        (*cb)(done, total);
      });
    }

  bool ok = client.Get(url.c_str());
  fclose(outfile);

  if (!ok)
    throw IRISException("HTTPRemoteImageSource: HTTP %ld downloading %s",
                        client.GetHTTPCode(), url.c_str());

  return dest;
}


bool IsRemoteImageURL(const std::string &path)
{
  return path.find("://") != std::string::npos;
}

std::string ResolveITKSnapURL(const std::string &url)
{
  if (url.substr(0, 8) == "itksnap-")
    return url.substr(8);
  return url;
}

SmartPtr<RemoteImageSource> CreateRemoteImageSource(const std::string &url)
{
  if (url.substr(0, 6) == "scp://" || url.substr(0, 7) == "sftp://")
    return SCPRemoteImageSource::New().GetPointer();

  if (url.substr(0, 7) == "http://" || url.substr(0, 8) == "https://")
    return HTTPRemoteImageSource::New().GetPointer();

  throw IRISException("No remote image handler for URL: %s", url.c_str());
}
