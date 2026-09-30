#include "RemoteResourceSettings.h"

RemoteResourceSettings::RemoteResourceSettings()
{
  // Max cache size: default 2 GB, range 128 MB – 100 GB, step 128 MB
  m_MaxCacheSizeMBModel = NewRangedProperty("MaxCacheSizeMB", 2048, 128, 102400, 128);

  // Delete local copy immediately after loading (no persistent cache)
  m_DeleteAfterDownloadModel = NewSimpleProperty("DeleteAfterDownload", false);
}
