#ifndef IMAGEIOREMOTE_H
#define IMAGEIOREMOTE_H

#include "SNAPCommon.h"
#include "IRISException.h"
#include "itkObject.h"
#include "itkObjectFactory.h"
#include <functional>
#include <cstddef>
#include <string>
#include <cctype>

/** Decode percent-encoded characters in a URL component.
 *  Only %XX sequences are decoded; '+' is left as-is (it is literal in paths). */
inline std::string UrlDecode(const std::string &s)
{
  std::string out;
  out.reserve(s.size());
  for (std::size_t i = 0; i < s.size(); ++i)
    {
    if (s[i] == '%' && i + 2 < s.size()
        && std::isxdigit((unsigned char)s[i+1])
        && std::isxdigit((unsigned char)s[i+2]))
      {
      char hex[3] = { s[i+1], s[i+2], '\0' };
      out += static_cast<char>(std::strtol(hex, nullptr, 16));
      i += 2;
      }
    else
      out += s[i];
    }
  return out;
}

// Forward declarations — full types only needed in .cxx files that use them.
class SSHConnectionPool;
class AbstractSSHAuthDelegate;
class AbstractProgressDelegate;
class RemoteFileCache;
class RemoteResourceSettings;

/**
 * A bundle of all the objects needed to perform authenticated, cached, and
 * progress-reporting remote downloads.  Build one from IRISApplication via
 * GetRemoteIOContext() and pass it down to GuidedMeshIO, MeshWrapperBase, or
 * any other subsystem that calls CreateRemoteImageSource() internally.
 *
 * All pointers are non-owning; the objects must outlive this struct.
 */
struct RemoteIOContext
{
  AbstractProgressDelegate  *progressDelegate   = nullptr;
  SSHConnectionPool         *connectionPool     = nullptr;
  AbstractSSHAuthDelegate   *authDelegate       = nullptr;
  std::string                appDataDir;
  RemoteResourceSettings    *remoteSettings     = nullptr;
};

/**
 * Progress callback type used by RemoteImageSource::Download.
 *
 *   bytes_done  — cumulative bytes received so far
 *   bytes_total — total file size in bytes, or 0 if not known in advance
 *
 * Returns false to request cancellation of the download.
 */
using DownloadProgressCallback =
    std::function<bool(std::size_t bytes_done,
                       std::size_t bytes_total)>;

/**
 * HTTP downloads that receive no data for this many seconds are aborted, so
 * that a stalled connection does not hang the load forever.
 */
constexpr long DOWNLOAD_STALL_TIMEOUT_SEC = 60;


/**
 * A new, unique temp directory for one download. Unless Keep() is called, it
 * is deleted with its contents when this object goes out of scope, so that a
 * download that fails or is cancelled leaves no partial file behind.
 */
class DownloadTempDir
{
public:
  /** Create the directory under the system temp directory. Throws on failure. */
  explicit DownloadTempDir(const char *prefix);
  ~DownloadTempDir();

  DownloadTempDir(const DownloadTempDir &) = delete;
  DownloadTempDir &operator=(const DownloadTempDir &) = delete;

  const std::string &GetPath() const { return m_Path; }

  /** Do not delete the directory; returns its path. */
  const std::string &Keep() { m_Keep = true; return m_Path; }

private:
  std::string m_Path;
  bool        m_Keep = false;
};


/**
 * Abstract base class for remote image source handlers. Each subclass
 * implements downloading for one URL scheme (scp://, https://, fw://, …).
 *
 * Extend this hierarchy to support new schemes; register them in
 * CreateRemoteImageSource() below.
 */
class RemoteImageSource : public itk::Object
{
public:
  irisITKAbstractObjectMacro(RemoteImageSource, itk::Object)

  /**
   * Download the image at @p url into a newly created OS temp directory
   * and return the full local path of the downloaded file.  The filename
   * inside the temp directory preserves the original remote basename so
   * that ITK format-detection by extension continues to work.
   *
   * When a file cache is attached, the returned file may instead be in the
   * cache. Otherwise GetTemporaryDirectory() names the temp directory, which
   * the caller must delete once it is done with the file. On failure or
   * cancellation nothing is left behind. Throws IRISException on failure.
   */
  virtual std::string Download(const std::string &url) = 0;

  /**
   * After Download(): the temp directory holding the returned file, or ""
   * when the file is in the cache. The caller owns this directory.
   */
  const std::string &GetTemporaryDirectory() const
    { return m_TemporaryDirectory; }

  /** Attach an optional progress callback invoked during Download().
   *  Pass an empty std::function to clear an existing callback. */
  void SetProgressCallback(DownloadProgressCallback cb)
    { m_ProgressCallback = std::move(cb); }

  /**
   * Attach an SSH connection pool so that repeated calls to Download() for
   * the same host reuse an already-authenticated session instead of performing
   * a new SSH handshake each time.  Pass nullptr to disable pooling.
   *
   * The pool's lifetime must exceed that of this object; it is not owned here.
   */
  void SetConnectionPool(SSHConnectionPool *pool)
    { m_ConnectionPool = pool; }

  /**
   * Attach an SSH auth delegate so that password/passphrase prompts are
   * displayed to the user when public-key auth fails.  Pass nullptr to
   * disable interactive prompting (connection fails on key-auth failure).
   */
  void SetAuthDelegate(AbstractSSHAuthDelegate *delegate)
    { m_AuthDelegate = delegate; }

  /**
   * Attach a persistent file cache.  When set, Download() checks the cache
   * before fetching and stores the result after a successful fetch.
   * Pass nullptr to disable caching.  The cache's lifetime must exceed this object.
   */
  void SetFileCache(RemoteFileCache *cache)
    { m_FileCache = cache; }

protected:
  RemoteImageSource() {}
  virtual ~RemoteImageSource() {}

  void ReportProgress(std::size_t bytes_done, std::size_t bytes_total)
  {
    if (m_ProgressCallback && !m_ProgressCallback(bytes_done, bytes_total))
      throw IRISUserCancelException("Download cancelled by user");
  }

  DownloadProgressCallback  m_ProgressCallback;
  SSHConnectionPool        *m_ConnectionPool = nullptr;
  AbstractSSHAuthDelegate  *m_AuthDelegate   = nullptr;
  RemoteFileCache          *m_FileCache      = nullptr;
  std::string               m_TemporaryDirectory;
};


/**
 * SCP/SFTP-based remote image source.  Handles URLs of the form
 *   scp://[user@]host[:port]/absolute/path/to/image.nii.gz
 *   sftp://[user@]host[:port]/absolute/path/to/image.nii.gz
 */
class SCPRemoteImageSource : public RemoteImageSource
{
public:
  irisITKObjectMacro(SCPRemoteImageSource, RemoteImageSource)

  std::string Download(const std::string &url) override;

protected:
  SCPRemoteImageSource() {}
  virtual ~SCPRemoteImageSource() {}
};


/**
 * HTTP/HTTPS remote image source.  Handles public URLs of the form
 *   http://host/path/to/image.nii.gz
 *   https://host/path/to/image.nii.gz
 * No authentication is performed; intended for publicly accessible datasets.
 * Uses RESTClient<GenericServerTraits> (libcurl) for the transfer.
 */
class HTTPRemoteImageSource : public RemoteImageSource
{
public:
  irisITKObjectMacro(HTTPRemoteImageSource, RemoteImageSource)

  std::string Download(const std::string &url) override;

protected:
  HTTPRemoteImageSource() {}
  virtual ~HTTPRemoteImageSource() {}
};


/** Returns true when @p path contains "://" and is therefore a remote URL. */
bool IsRemoteImageURL(const std::string &path);

/**
 * Strip the "itksnap-" wrapper prefix from a URL if present, returning the
 * underlying protocol URL.  Local paths and already-bare remote URLs are
 * returned unchanged.
 *   itksnap-sftp://host/path  →  sftp://host/path
 *   itksnap-scp://host/path   →  scp://host/path
 *   sftp://host/path          →  sftp://host/path   (unchanged)
 *   /local/path               →  /local/path        (unchanged)
 */
std::string ResolveITKSnapURL(const std::string &url);

/**
 * Whether a file name or URL has the workspace extension (.itksnap, any case).
 * For remote URLs the query and fragment are ignored, so that
 * https://host/study.itksnap?token=abc counts as a workspace.
 */
bool HasWorkspaceExtension(const std::string &path_or_url);

/** The parts of an scp:// or sftp:// URL */
struct SCPURL
{
  std::string username;   // decoded; empty if the URL has no user@
  std::string host;
  int         port = 0;   // 0 if the URL has no :port
  std::string path;       // decoded absolute path on the server
};

/**
 * Parse scp://[user@]host[:port]/path or sftp://..., throwing IRISException
 * if the URL is malformed (wrong scheme, no path, invalid port).
 */
SCPURL ParseSCPURL(const std::string &url);

/**
 * The local filename for a downloaded URL: the decoded last path component,
 * without any ?query or #fragment. Throws IRISException if there is none
 * (e.g., the URL ends in "/" or "..").
 */
std::string RemoteFilenameFromURL(const std::string &url);

/**
 * Factory: create the RemoteImageSource appropriate for the scheme in @p url.
 * Throws IRISException for unrecognised schemes.
 */
SmartPtr<RemoteImageSource> CreateRemoteImageSource(const std::string &url);

/**
 * A local copy of a remote file, returned by DownloadRemoteFile(). A file in
 * the persistent cache is kept. A file in a temp directory (no cache,
 * DeleteAfterDownload set, or the cache could not store it) is deleted with
 * its directory when this object is destroyed, so keep the object alive
 * until the file has been read.
 */
class RemoteDownload
{
public:
  RemoteDownload() = default;
  RemoteDownload(std::string path, std::string temp_dir);
  ~RemoteDownload();

  RemoteDownload(RemoteDownload &&other) noexcept;
  RemoteDownload &operator=(RemoteDownload &&other) noexcept;
  RemoteDownload(const RemoteDownload &) = delete;
  RemoteDownload &operator=(const RemoteDownload &) = delete;

  /** Local path of the file. */
  const std::string &GetPath() const { return m_Path; }

private:
  void DeleteTemporaryFile();

  std::string m_Path;
  std::string m_TempDir;
};

/**
 * Download a remote file and return its local copy.
 * This is the one-stop helper: it creates the appropriate RemoteImageSource,
 * applies all fields from @p ctx (connection pool, auth delegate, file cache,
 * and a ProgressTaskGuard wrapping ctx.progressDelegate), and calls Download().
 *
 * @p title  Label shown in the progress bar, marked with SNAP_TRANSLATE_NOOP;
 *           "%1" in it is replaced by the filename. Defaults to "Downloading %1".
 */
RemoteDownload DownloadRemoteFile(const std::string &url,
                                  const RemoteIOContext &ctx,
                                  const char *title = nullptr);




#endif // IMAGEIOREMOTE_H
