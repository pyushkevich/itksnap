#ifndef IMAGEIOREMOTE_H
#define IMAGEIOREMOTE_H

#include "SNAPCommon.h"
#include "IRISException.h"
#include "itkObject.h"
#include "itkObjectFactory.h"
#include <functional>
#include <cstddef>

// Forward declarations — full types only needed in .cxx files that use them.
class SSHConnectionPool;
class AbstractSSHAuthDelegate;

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
   * Cleanup of the temp directory is left to the OS for now.
   * Throws IRISException on failure.
   */
  virtual std::string Download(const std::string &url) = 0;

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
};


/**
 * SCP-based remote image source.  Handles URLs of the form
 *   scp://[user@]host/absolute/path/to/image.nii.gz
 * by invoking the system scp(1) client (requires key-based auth; no
 * interactive password prompts are supported).
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


/** Returns true when @p path contains "://" and is therefore a remote URL. */
bool IsRemoteImageURL(const std::string &path);

/**
 * Factory: create the RemoteImageSource appropriate for the scheme in @p url.
 * Throws IRISException for unrecognised schemes.
 */
SmartPtr<RemoteImageSource> CreateRemoteImageSource(const std::string &url);




#endif // IMAGEIOREMOTE_H
