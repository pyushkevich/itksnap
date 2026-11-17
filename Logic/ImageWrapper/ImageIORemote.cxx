#include "ImageIORemote.h"
#include "AbstractProgressDelegate.h"
#include "AbstractSSHAuthDelegate.h"
#include "FlywheelRemoteImageSource.h"
#include "RemoteFileCache.h"
#include "RESTClient.h"
#include "SSHConnectionPool.h"
#include "SystemInterface.h"
#include "UIReporterDelegates.h"
#include <itksys/Directory.hxx>
#include <itksys/SystemTools.hxx>
#include <libssh/sftp.h>
#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <memory>
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

/** RAII wrapper: closes an sftp_file on scope exit. */
struct SFTPFileGuard {
  sftp_file f;
  ~SFTPFileGuard() { sftp_close(f); }
};

} // anonymous namespace


// -----------------------------------------------------------------------
//  DownloadTempDir, RemoteDownload
// -----------------------------------------------------------------------

DownloadTempDir::DownloadTempDir(const char *prefix)
{
  std::string base = SystemInterface::GetSystemInfoDelegate()->GetTempDirectory();
  if (base.empty())
    throw IRISException("SystemInfoDelegate::GetTempDirectory() returned an empty path");

#ifdef _WIN32
  char name[MAX_PATH + 1] = "";
  if (!GetTempFileNameA(base.c_str(), prefix, 0, name))
    throw IRISException("Cannot generate temp filename for remote image in: %s", base.c_str());

  std::string dir = std::string(name) + "_d";
  DeleteFileA(name);
  if (!CreateDirectoryA(dir.c_str(), nullptr))
    throw IRISException("Cannot create temp directory: %s", dir.c_str());
  m_Path = dir;
#else
  std::string tmpl = base + "/" + prefix + "_XXXXXX";
  std::vector<char> buf(tmpl.begin(), tmpl.end());
  buf.push_back('\0');
  char *dir = mkdtemp(buf.data());
  if (!dir)
    throw IRISException("Cannot create temp directory under: %s", base.c_str());
  m_Path = dir;
#endif
}

DownloadTempDir::~DownloadTempDir()
{
  if (!m_Keep)
    itksys::SystemTools::RemoveADirectory(m_Path);
}

RemoteDownload::RemoteDownload(std::string path, std::string temp_dir)
  : m_Path(std::move(path)), m_TempDir(std::move(temp_dir))
{}

RemoteDownload::~RemoteDownload()
{
  DeleteTemporaryFile();
}

RemoteDownload::RemoteDownload(RemoteDownload &&other) noexcept
  : m_Path(std::move(other.m_Path)), m_TempDir(std::move(other.m_TempDir))
{
  other.m_TempDir.clear();
}

RemoteDownload &RemoteDownload::operator=(RemoteDownload &&other) noexcept
{
  if (this != &other)
    {
    DeleteTemporaryFile();
    m_Path    = std::move(other.m_Path);
    m_TempDir = std::move(other.m_TempDir);
    other.m_TempDir.clear();
    }
  return *this;
}

void RemoteDownload::DeleteTemporaryFile()
{
  if (!m_TempDir.empty())
    itksys::SystemTools::RemoveADirectory(m_TempDir);
  m_TempDir.clear();
}


// -----------------------------------------------------------------------
//  SCPRemoteImageSource
// -----------------------------------------------------------------------

std::string
SCPRemoteImageSource::Download(const std::string &url)
{
  m_TemporaryDirectory.clear();

  SCPURL parsed = ParseSCPURL(url);
  const std::string &host = parsed.host, &username = parsed.username, &remotepath = parsed.path;
  int port = parsed.port;

  // Preserve the original basename so ITK extension detection works.
  std::string basename = itksys::SystemTools::GetFilenameName(remotepath);
  if (basename.empty() || basename == "." || basename == "..")
    throw IRISException("SCPRemoteImageSource: URL path ends in a directory, not a file: %s", url.c_str());

  // Acquire an authenticated SSH + SFTP session via the connection pool.
  // When no external pool was provided (single-file download) we create a
  // temporary one that lives for the duration of this call; its destructor
  // calls CloseAll(), disconnecting and freeing the session automatically.
  // Any passwords/passphrases are requested through m_AuthDelegate between
  // connection attempts; failures throw with the specific SSH error.
  SmartPtr<SSHConnectionPool> tmp_pool;
  if (!m_ConnectionPool)
    tmp_pool = SSHConnectionPool::New();

  SSHConnectionPool *pool = m_ConnectionPool ? m_ConnectionPool : tmp_pool.GetPointer();

  SSHConnectionPool::SessionPair pair = pool->GetOrCreate(host, username, m_AuthDelegate, port);

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

  // Create local temp directory and destination path. The directory, with any
  // partial file, is removed if the download fails or is cancelled.
  DownloadTempDir tmpdir("itksnap_remote");
  std::string dest = tmpdir.GetPath() + "/" + basename;

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
  if (!outfile)
    throw IRISException("SCPRemoteImageSource: Write error to '%s'", dest.c_str());

  // Store in cache (moves the file to the cache dir; returns final path to use).
  std::string path = m_FileCache ? m_FileCache->Store(url, dest, attr_size, attr_mtime) : dest;
  if (path == dest)
    m_TemporaryDirectory = tmpdir.Keep();
  return path;
}

void
SCPRemoteImageSource::Upload(const std::string &local_path, const std::string &url)
{
  SCPURL parsed = ParseSCPURL(url);
  const std::string &remotepath = parsed.path;

  std::string basename = itksys::SystemTools::GetFilenameName(remotepath);
  if (basename.empty() || basename == "." || basename == "..")
    throw IRISException("SCPRemoteImageSource: URL path ends in a directory, not a file: %s", url.c_str());

  std::ifstream infile(local_path, std::ios::binary | std::ios::ate);
  if (!infile)
    throw IRISException("SCPRemoteImageSource: Cannot read local file '%s'", local_path.c_str());
  std::size_t file_size = static_cast<std::size_t>(infile.tellg());
  infile.seekg(0);

  // The session comes from the pool, as in Download()
  SmartPtr<SSHConnectionPool> tmp_pool;
  if (!m_ConnectionPool)
    tmp_pool = SSHConnectionPool::New();
  SSHConnectionPool *pool = m_ConnectionPool ? m_ConnectionPool : tmp_pool.GetPointer();
  SSHConnectionPool::SessionPair pair =
    pool->GetOrCreate(parsed.host, parsed.username, m_AuthDelegate, parsed.port);
  ssh_session  session = pair.ssh;
  sftp_session sftp    = pair.sftp;

  // Write to a temporary name next to the target. It is removed if anything
  // fails before the file has been renamed over the target.
  std::string remote_dir = itksys::SystemTools::GetFilenamePath(remotepath);
  std::string temp_path = remote_dir + "/." + basename + ".itksnap-upload";
  struct TempFileGuard
  {
    sftp_session sftp;
    std::string  path;
    bool         keep = false;
    ~TempFileGuard() { if (!keep) sftp_unlink(sftp, path.c_str()); }
  };

  sftp_file remote_file =
    sftp_open(sftp, temp_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (!remote_file)
    throw IRISException("SCPRemoteImageSource: Cannot create remote file '%s': %s",
                        temp_path.c_str(), ssh_get_error(session));
  TempFileGuard temp_guard{ sftp, temp_path };

  {
  SFTPFileGuard file_guard{remote_file};

  constexpr std::size_t chunk = 65536;
  std::vector<char>     buf(chunk);
  std::size_t           bytes_sent = 0;
  ReportProgress(0, file_size);
  while (bytes_sent < file_size)
    {
    std::size_t n = std::min(chunk, file_size - bytes_sent);
    if (!infile.read(buf.data(), n))
      throw IRISException("SCPRemoteImageSource: Read error on '%s'", local_path.c_str());

    // sftp_write may write less than asked, up to the server's limit
    for (std::size_t done = 0; done < n;)
      {
      ssize_t w = sftp_write(remote_file, buf.data() + done, n - done);
      if (w <= 0)
        throw IRISException("SCPRemoteImageSource: SFTP write error on '%s': %s",
                            temp_path.c_str(), ssh_get_error(session));
      done += static_cast<std::size_t>(w);
      }
    bytes_sent += n;
    ReportProgress(bytes_sent, file_size);
    }
  if (file_size == 0)
    ReportProgress(0, 0);
  }

  // Rename over the target. SFTP version 3, which OpenSSH uses, cannot rename
  // onto an existing file, so remove the target first if the rename fails.
  if (sftp_rename(sftp, temp_path.c_str(), remotepath.c_str()) == 0)
    {
    temp_guard.keep = true;
    return;
    }

  sftp_attributes target = sftp_stat(sftp, remotepath.c_str());
  if (!target)
    throw IRISException("SCPRemoteImageSource: Cannot rename '%s' to '%s': %s",
                        temp_path.c_str(), remotepath.c_str(), ssh_get_error(session));
  sftp_attributes_free(target);

  if (sftp_unlink(sftp, remotepath.c_str()) != 0)
    throw IRISException("SCPRemoteImageSource: Cannot replace remote file '%s': %s",
                        remotepath.c_str(), ssh_get_error(session));

  // The old file is gone now, so keep the uploaded copy whatever happens
  temp_guard.keep = true;
  if (sftp_rename(sftp, temp_path.c_str(), remotepath.c_str()) != 0)
    throw IRISException("SCPRemoteImageSource: Cannot rename '%s' to '%s': %s. "
                        "The uploaded file has been kept as '%s'.",
                        temp_path.c_str(), remotepath.c_str(), ssh_get_error(session),
                        temp_path.c_str());
}

void
RemoteImageSource::Upload(const std::string &, const std::string &url)
{
  throw IRISException("Files cannot be saved to this kind of URL: %s", url.c_str());
}


// -----------------------------------------------------------------------
//  HTTPRemoteImageSource
// -----------------------------------------------------------------------

std::string
HTTPRemoteImageSource::Download(const std::string &url)
{
  m_TemporaryDirectory.clear();

  // The local filename keeps the extension, so that ITK can detect the format
  std::string basename = RemoteFilenameFromURL(url);

  // Cache lookup: if we have a cached copy with validation tokens, we can do
  // a conditional GET to avoid a full re-download.
  RemoteFileCache::HTTPCacheEntry cached;
  if (m_FileCache)
    cached = m_FileCache->LookupHTTP(url);

  // The temp directory, with any partial file, is removed if the download
  // fails or is cancelled. The file is closed first.
  DownloadTempDir tmpdir("itksnap_remote");
  std::string dest = tmpdir.GetPath() + "/" + basename;

  std::unique_ptr<FILE, int (*)(FILE *)> outfile(fopen(dest.c_str(), "wb"), fclose);
  if (!outfile)
    throw IRISException("HTTPRemoteImageSource: cannot create local file '%s'", dest.c_str());

  RESTClient<> client;
  client.SetOutputFile(outfile.get());
  client.SetStallTimeout(DOWNLOAD_STALL_TIMEOUT_SEC);

  // Follow redirects (moved files, share links). The cache entry is still
  // keyed by the original URL.
  client.SetFollowRedirects(true);

  if (m_ProgressCallback)
    {
    client.SetProgressCallback(
      &m_ProgressCallback,
      [](void *data, size_t done, size_t total) -> bool
      {
        auto *cb = static_cast<DownloadProgressCallback *>(data);
        return (*cb)(done, total);
      });
    }

  // If we have cached tokens, attempt a conditional GET.
  if (!cached.local_path.empty())
    {
    if (!cached.etag.empty())
      client.SetRequestHeader("If-None-Match", cached.etag.c_str());
    if (!cached.last_modified.empty())
      client.SetRequestHeader("If-Modified-Since", cached.last_modified.c_str());
    }

  client.Get("%s", url.c_str());
  if (fclose(outfile.release()) != 0)
    throw IRISException("HTTPRemoteImageSource: write error to '%s'", dest.c_str());

  long code = client.GetHTTPCode();

  // Cached copy is still valid — the empty temp file is discarded.
  if (code == 304)
    return cached.local_path;

  if (code != 200)
    throw IRISException("HTTPRemoteImageSource: HTTP %ld downloading %s", code, url.c_str());

  // Store the downloaded file in cache along with any HTTP validation tokens.
  std::string path = m_FileCache
    ? m_FileCache->StoreHTTP(url, dest,
                             client.GetResponseHeader("etag"),
                             client.GetResponseHeader("last-modified"))
    : dest;
  if (path == dest)
    m_TemporaryDirectory = tmpdir.Keep();
  return path;
}


bool IsRemoteImageURL(const std::string &path)
{
  return path.find("://") != std::string::npos;
}

bool CanSaveToURL(const std::string &url)
{
  return url.substr(0, 6) == "scp://" || url.substr(0, 7) == "sftp://";
}

std::string SiblingURL(const std::string &url, const std::string &filename)
{
  std::string url_path = url.substr(0, url.find_first_of("?#"));
  std::string suffix = url.substr(url_path.size());
  auto slash = url_path.rfind('/');
  if (slash == std::string::npos)
    throw IRISException("Cannot determine the directory of URL: %s", url.c_str());
  return url_path.substr(0, slash + 1) + UrlEncodePathSegment(filename) + suffix;
}

std::string ResolveITKSnapURL(const std::string &url)
{
  if (url.substr(0, 8) == "itksnap-")
    return url.substr(8);
  return url;
}

bool HasWorkspaceExtension(const std::string &path_or_url)
{
  std::string path = path_or_url;
  if (IsRemoteImageURL(path))
    path = path.substr(0, path.find_first_of("?#"));

  const std::string ext = ".itksnap";
  if (path.size() < ext.size())
    return false;
  for (std::size_t i = 0; i < ext.size(); i++)
    if (std::tolower((unsigned char) path[path.size() - ext.size() + i]) != ext[i])
      return false;
  return true;
}

std::string CollapseFullPathOrURL(const std::string &path_or_url)
{
  return IsRemoteImageURL(path_or_url)
           ? path_or_url
           : itksys::SystemTools::CollapseFullPath(path_or_url);
}

SCPURL ParseSCPURL(const std::string &url)
{
  // Expected form:  scp://[user@]host[:port]/path  or  sftp://...
  std::size_t scheme_len = 0;
  if (url.substr(0, 6) == "scp://")       scheme_len = 6;
  else if (url.substr(0, 7) == "sftp://") scheme_len = 7;
  else
    throw IRISException("SCPRemoteImageSource: URL must start with scp:// or sftp://: %s", url.c_str());

  std::string rest = url.substr(scheme_len);   // "[user@]host[:port]/path"

  auto slash = rest.find('/');
  if (slash == std::string::npos)
    throw IRISException("SCPRemoteImageSource: malformed URL (no path after host): %s", url.c_str());

  SCPURL parsed;
  std::string hostpart = rest.substr(0, slash);   // "user@host" or "host"
  parsed.path = UrlDecode(rest.substr(slash));    // "/absolute/path/to/file.ext"

  // Split optional user@ from host
  auto at = hostpart.rfind('@');
  if (at != std::string::npos)
    {
    parsed.username = UrlDecode(hostpart.substr(0, at));
    parsed.host     = hostpart.substr(at + 1);
    }
  else
    {
    parsed.host = hostpart;
    }

  // Split optional :port from host (e.g. "localhost:2222")
  auto colon = parsed.host.find(':');
  if (colon != std::string::npos)
    {
    std::string port_str = parsed.host.substr(colon + 1);
    char *end = nullptr;
    long port = std::strtol(port_str.c_str(), &end, 10);
    if (port_str.empty() || *end != '\0' || port < 1 || port > 65535)
      throw IRISException("SCPRemoteImageSource: invalid port in URL: %s", url.c_str());
    parsed.port = static_cast<int>(port);
    parsed.host = parsed.host.substr(0, colon);
    }

  if (parsed.host.empty())
    throw IRISException("SCPRemoteImageSource: malformed URL (no host): %s", url.c_str());

  return parsed;
}

std::string RemoteFilenameFromURL(const std::string &url)
{
  // Decode the last path component without the query string, e.g.
  // "scan%20T1.nii.gz?token=x" gives "scan T1.nii.gz". Taking the filename
  // again after decoding drops any directory that an encoded "/" (%2F) adds.
  std::string url_path = url.substr(0, url.find_first_of("?#"));
  std::string basename = itksys::SystemTools::GetFilenameName(
    UrlDecode(itksys::SystemTools::GetFilenameName(url_path)));
  if (basename.empty() || basename == "." || basename == "..")
    throw IRISException("Cannot determine the filename from URL: %s", url.c_str());
  return basename;
}

SmartPtr<RemoteImageSource> CreateRemoteImageSource(const std::string &url)
{
  if (url.substr(0, 6) == "scp://" || url.substr(0, 7) == "sftp://")
    return SCPRemoteImageSource::New().GetPointer();

  if (url.substr(0, 7) == "http://" || url.substr(0, 8) == "https://")
    return HTTPRemoteImageSource::New().GetPointer();

  if (url.substr(0, 5) == "fw://")
    return FlywheelRemoteImageSource::New().GetPointer();

  throw IRISException("No remote image handler for URL: %s", url.c_str());
}

RemoteDownload
DownloadRemoteFile(const std::string &url,
                   const RemoteIOContext &ctx,
                   const char *title)
{
  SmartPtr<RemoteImageSource> src = CreateRemoteImageSource(url);

  if (ctx.connectionPool)
    src->SetConnectionPool(ctx.connectionPool);
  if (ctx.authDelegate)
    src->SetAuthDelegate(ctx.authDelegate);

  RemoteFileCache file_cache;
  if (!ctx.appDataDir.empty() && ctx.remoteSettings)
    {
    file_cache.Initialize(ctx.appDataDir, ctx.remoteSettings);
    src->SetFileCache(&file_cache);
    }

  if (!title)
    title = SNAP_TRANSLATE_NOOP("ProgressTask", "Downloading %1");
  std::string filename = UrlDecode(
    itksys::SystemTools::GetFilenameName(url.substr(0, url.find_first_of("?#"))));

  ProgressTaskGuard guard(ctx.progressDelegate, title, true, filename);
  src->SetProgressCallback([&guard](std::size_t done, std::size_t total) {
    return guard.UpdateProgress(done, total);
  });

  std::string path = src->Download(url);
  return RemoteDownload(path, src->GetTemporaryDirectory());
}

void
SaveRemoteFile(const std::string &url,
               const RemoteIOContext &ctx,
               const std::function<void(const std::string &)> &write,
               const char *title)
{
  SmartPtr<RemoteImageSource> src = CreateRemoteImageSource(url);
  if (!src->CanUpload())
    throw IRISException("Files cannot be saved to this kind of URL: %s. "
                        "Please save to a local file, or to an scp:// or sftp:// URL.",
                        url.c_str());

  if (ctx.connectionPool)
    src->SetConnectionPool(ctx.connectionPool);
  if (ctx.authDelegate)
    src->SetAuthDelegate(ctx.authDelegate);

  // Write into a temp directory, under the URL's file name so that the
  // format is chosen from its extension. The directory is deleted on return.
  std::string filename = RemoteFilenameFromURL(url);
  DownloadTempDir tmpdir("itksnap_upload");
  std::string local_file = tmpdir.GetPath() + "/" + filename;
  write(local_file);

  // Every file that was written: the main one last, so that a reader of the
  // URL does not see it before the files it refers to
  std::vector<std::string> files;
  itksys::Directory dir;
  if (dir.Load(tmpdir.GetPath()))
    for (unsigned long i = 0; i < dir.GetNumberOfFiles(); i++)
      {
      std::string f = dir.GetFile(i);
      if (f != "." && f != ".." && f != filename &&
          !itksys::SystemTools::FileIsDirectory(tmpdir.GetPath() + "/" + f))
        files.push_back(f);
      }
  if (!itksys::SystemTools::FileExists(local_file, true))
    throw IRISException("No file was written for %s", url.c_str());
  files.push_back(filename);

  if (!title)
    title = SNAP_TRANSLATE_NOOP("ProgressTask", "Uploading %1");

  // The URL's cache entry is stale from the moment the upload starts
  if (!ctx.appDataDir.empty() && ctx.remoteSettings)
    {
    RemoteFileCache file_cache;
    file_cache.Initialize(ctx.appDataDir, ctx.remoteSettings);
    file_cache.Invalidate(url);
    }

  for (const std::string &f : files)
    {
    ProgressTaskGuard guard(ctx.progressDelegate, title, true, f);
    src->SetProgressCallback([&guard](std::size_t done, std::size_t total) {
      return guard.UpdateProgress(done, total);
    });
    src->Upload(tmpdir.GetPath() + "/" + f, f == filename ? url : SiblingURL(url, f));
    }
}
