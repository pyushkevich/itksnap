#ifndef FLYWHEELREMOTEIMAGESOURCE_H
#define FLYWHEELREMOTEIMAGESOURCE_H

#include "ImageIORemote.h"

/**
 * Flywheel.io remote image source.  Handles two fw:// URL forms:
 *
 *   Direct (ID based):
 *     fw://host/<type>/<id>/files/<filename>
 *     where <type> is acquisitions, analyses, sessions, subjects or projects.
 *
 *   Label based (find):
 *     fw://host/find/<group>/<project>/<subject>/<session>/<type>/<label>/files/<filename>
 *     where <type> is acquisitions or analyses. Each label is resolved to an
 *     ID by matching its "label" field via REST API calls.
 *
 * Files can be uploaded to both forms of URL as well.
 *
 * Authentication uses an API key supplied via AbstractSSHAuthDelegate::PromptForAPIKey().
 * The key is cached in memory for the lifetime of the process (per server).
 *
 * See Documentation/Developer/RemoteURLs.md for the full description.
 */
class FlywheelRemoteImageSource : public RemoteImageSource
{
public:
  irisITKObjectMacro(FlywheelRemoteImageSource, RemoteImageSource)

  std::string Download(const std::string &url) override;

  bool CanUpload() const override { return true; }

  /**
   * Upload a file to the container that the URL names (either form), under
   * the URL's file name. Flywheel keeps the file it replaces as an older
   * version.
   */
  void Upload(const std::string &local_path, const std::string &url) override;

protected:
  FlywheelRemoteImageSource() {}
  virtual ~FlywheelRemoteImageSource() {}
};

#endif // FLYWHEELREMOTEIMAGESOURCE_H
