#ifndef REMOTERESOURCESETTINGS_H
#define REMOTERESOURCESETTINGS_H

#include "AbstractPropertyContainerModel.h"

/**
 * User preferences for remote file downloads (SSH/SFTP).
 *
 * Serialized under the "RemoteResources" key in the user preferences
 * Registry (alongside DefaultBehavior, MeshOptions, etc.).  The cache
 * infrastructure reads these settings to decide whether to keep, evict,
 * or immediately delete locally cached copies of remote files.
 */
class RemoteResourceSettings : public AbstractPropertyContainerModel
{
public:
  irisITKObjectMacro(RemoteResourceSettings, AbstractPropertyContainerModel)

  /** Maximum on-disk cache size in megabytes. */
  irisRangedPropertyAccessMacro(MaxCacheSizeMB, int)

  /** When true, delete the local copy immediately after use. */
  irisSimplePropertyAccessMacro(DeleteAfterDownload, bool)

protected:
  SmartPtr<ConcreteRangedIntProperty>      m_MaxCacheSizeMBModel;
  SmartPtr<ConcreteSimpleBooleanProperty>  m_DeleteAfterDownloadModel;

  RemoteResourceSettings();
};

#endif // REMOTERESOURCESETTINGS_H
