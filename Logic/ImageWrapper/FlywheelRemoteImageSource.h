#ifndef FLYWHEELREMOTEIMAGESOURCE_H
#define FLYWHEELREMOTEIMAGESOURCE_H

#include "ImageIORemote.h"

/**
 * Flywheel.io remote image source.  Handles URLs of the form
 *   fw://host/group/project/subject/session/acquisition/filename.nii.gz
 *
 * The group label is used directly as the Flywheel group ID (Flywheel groups
 * use human-readable string IDs).  Project, subject, session, and acquisition
 * are resolved by matching their "label" field via REST API calls.
 *
 * Authentication uses an API key supplied via AbstractSSHAuthDelegate::PromptForAPIKey().
 * The key is cached in memory for the lifetime of the process (per server).
 */
class FlywheelRemoteImageSource : public RemoteImageSource
{
public:
  irisITKObjectMacro(FlywheelRemoteImageSource, RemoteImageSource)

  std::string Download(const std::string &url) override;

protected:
  FlywheelRemoteImageSource() {}
  virtual ~FlywheelRemoteImageSource() {}
};

#endif // FLYWHEELREMOTEIMAGESOURCE_H
