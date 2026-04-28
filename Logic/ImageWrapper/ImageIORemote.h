#ifndef IMAGEIOREMOTE_H
#define IMAGEIOREMOTE_H

#include "SNAPCommon.h"
#include "IRISException.h"
#include "itkObject.h"
#include "itkObjectFactory.h"

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

protected:
  RemoteImageSource() {}
  virtual ~RemoteImageSource() {}
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
