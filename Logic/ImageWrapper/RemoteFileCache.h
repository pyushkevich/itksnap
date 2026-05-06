#ifndef REMOTEFILECACHE_H
#define REMOTEFILECACHE_H

#include <cstdint>
#include <map>
#include <string>

class RemoteResourceSettings;

/**
 * Persistent on-disk cache for remotely downloaded files.
 *
 * Files are stored under <app_data_dir>/Cache/<key>/<basename> where <key>
 * is a hex hash of the URL.  Metadata (URL, remote mtime, remote size, last
 * access time) is persisted in <app_data_dir>/CacheMetadata.xml using the
 * Registry XML format.
 *
 * Staleness check: an entry is a cache hit only when the remote file's mtime
 * and size match the stored metadata values.
 *
 * Eviction: after each Store(), the total size of all cached files is compared
 * against RemoteResourceSettings::MaxCacheSizeMB.  If exceeded, the least-
 * recently-accessed entries are deleted until the total is within the limit.
 *
 * This class is not thread-safe; create one instance per download operation.
 * It loads metadata from disk lazily on first use and saves after each mutation.
 */
class RemoteFileCache
{
public:
  /**
   * Configure the cache.  Must be called before Lookup() or Store().
   * @p app_data_dir  path returned by SystemInterface::GetApplicationDataDirectory()
   * @p settings      RemoteResourceSettings instance; must outlive this object.
   */
  void Initialize(const std::string &app_data_dir, RemoteResourceSettings *settings);

  /**
   * Look up a URL in the cache.
   * Returns the local path of the cached file if it exists on disk and the
   * remote attributes match the stored metadata; otherwise returns "".
   */
  std::string Lookup(const std::string &url,
                     uint64_t remote_size,
                     uint32_t remote_mtime);

  /**
   * Store a newly downloaded file in the cache.
   *
   * If DeleteAfterDownload is set, the file is left at @p temp_path and that
   * path is returned unchanged (no caching occurs).  Otherwise the file is
   * copied into the cache directory, metadata is updated and saved, LRU
   * eviction is applied if needed, and the new cached path is returned.
   *
   * @p url           the remote URL that was downloaded
   * @p temp_path     path to the locally downloaded file (in a temp directory)
   * @p remote_size   file size from sftp_stat (0 if unknown)
   * @p remote_mtime  modification time from sftp_stat (0 if unknown)
   */
  std::string Store(const std::string &url,
                    const std::string &temp_path,
                    uint64_t remote_size,
                    uint32_t remote_mtime);

private:
  struct Entry
  {
    std::string url;
    std::string local_path;
    uint64_t    remote_size  = 0;
    uint32_t    remote_mtime = 0;
    int64_t     last_access  = 0;  // seconds since epoch
  };

  static std::string MakeKey(const std::string &url);

  void EnsureLoaded();
  void LoadMetadata();
  void SaveMetadata();
  void Evict();

  std::string             m_CacheDir;
  std::string             m_MetadataPath;
  RemoteResourceSettings *m_Settings = nullptr;
  std::map<std::string, Entry> m_Entries;  // hex-key → entry
  bool                    m_Loaded = false;
};

#endif // REMOTEFILECACHE_H
