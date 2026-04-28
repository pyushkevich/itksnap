#include "ImageIORemote.h"
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

/** RAII wrapper: disconnects and frees an ssh_session on scope exit. */
struct SessionGuard {
  ssh_session s;
  ~SessionGuard() { if (ssh_is_connected(s)) ssh_disconnect(s); ssh_free(s); }
};

/** RAII wrapper: frees an sftp_session on scope exit. */
struct SFTPGuard {
  sftp_session s;
  ~SFTPGuard() { sftp_free(s); }
};

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

  // Callback for OpenSession: capture error messages; abort on interactive prompts
  // since there is no UI available in the logic layer.
  std::string auth_error;
  auto session_cb = [](SSHTunnel::CallbackType type,
                       SSHTunnel::CallbackInfo  info,
                       void                    *data) -> SSHTunnel::CallbackResponse
  {
    std::string &err = *static_cast<std::string *>(data);
    switch (type)
    {
      case SSHTunnel::CB_ERROR:
        err = std::get<SSHTunnel::ErrorInfo>(info).error_message;
        break;
      case SSHTunnel::CB_PROMPT_PASSWORD:
      case SSHTunnel::CB_PROMPT_PASSKEY:
        err = "Password/passphrase authentication is not supported for scp:// downloads; "
              "configure key-based SSH authentication";
        return {1, ""}; // abort
      default:
        break;
    }
    return {0, ""};
  };

  // Open a short-lived authenticated SSH session via libssh
  ssh_session session = SSHTunnel::OpenSession(host.c_str(),
                                               username.empty() ? nullptr : username.c_str(),
                                               nullptr,
                                               session_cb, &auth_error);
  if (!session)
    throw IRISException("SCPRemoteImageSource: SSH connection to %s failed: %s",
                        host.c_str(), auth_error.c_str());

  SessionGuard session_guard{session};

  // Initialize SFTP subsystem on top of the authenticated session
  sftp_session sftp = sftp_new(session);
  if (!sftp)
    throw IRISException("SCPRemoteImageSource: Cannot allocate SFTP session: %s",
                        ssh_get_error(session));

  SFTPGuard sftp_guard{sftp};

  if (sftp_init(sftp) != SSH_OK)
    throw IRISException("SCPRemoteImageSource: SFTP initialization failed (code %d)",
                        sftp_get_error(sftp));

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

  // Stream remote file to disk in chunks
  constexpr std::size_t chunk = 65536;
  std::vector<char>     buf(chunk);
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
  }

  outfile.close();
  return dest;
}


// -----------------------------------------------------------------------
//  Free functions
// -----------------------------------------------------------------------

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
