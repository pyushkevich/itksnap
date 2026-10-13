#include "RemoteFileCache.h"
#include "RemoteResourceSettings.h"
#include "Registry.h"
#include "IRISException.h"
#include <itksys/SystemTools.hxx>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <thread>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <cerrno>
#  include <fcntl.h>
#  include <sys/file.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif


// -----------------------------------------------------------------------
//  Internal helpers
// -----------------------------------------------------------------------
namespace {

/** Current time as seconds since epoch. */
int64_t NowSeconds()
{
  using namespace std::chrono;
  return static_cast<int64_t>(
      duration_cast<seconds>(system_clock::now().time_since_epoch()).count());
}

/** On-disk size of a file in bytes; 0 if the file does not exist. */
uint64_t FileBytes(const std::string &path)
{
#ifdef _WIN32
  WIN32_FILE_ATTRIBUTE_DATA info;
  if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &info))
    return 0;
  return (static_cast<uint64_t>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
#else
  struct stat st;
  if (::stat(path.c_str(), &st) != 0)
    return 0;
  return static_cast<uint64_t>(st.st_size);
#endif
}

/** Copy a file; returns false on failure. */
bool CopyFileContents(const std::string &src, const std::string &dst)
{
  std::ifstream in(src, std::ios::binary);
  if (!in) return false;
  std::ofstream out(dst, std::ios::binary);
  if (!out) return false;
  out << in.rdbuf();
  out.close();
  return !out.fail() && (in.good() || in.eof());
}

/**
 * Rename a file, replacing @p dst if it exists. This is atomic on the same
 * volume: other processes see either the old or the new file.
 */
bool RenameFile(const std::string &src, const std::string &dst)
{
#ifdef _WIN32
  return MoveFileExA(src.c_str(), dst.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
#else
  return std::rename(src.c_str(), dst.c_str()) == 0;
#endif
}

/**
 * Move a file. A rename is tried first; when that fails (e.g. the temp
 * directory is on another volume) the file is copied and the source removed.
 */
bool MoveFileTo(const std::string &src, const std::string &dst)
{
  if (RenameFile(src, dst))
    return true;
  if (!CopyFileContents(src, dst))
    {
    itksys::SystemTools::RemoveFile(dst);
    return false;
    }
  itksys::SystemTools::RemoveFile(src);
  return true;
}

/**
 * Read an unsigned number stored in the metadata. Older versions wrote these
 * through int, so a negative value means the number overflowed; it is read
 * as 0 (unknown).
 */
uint64_t ReadNumber(Registry &f, const char *key)
{
  std::string s = f.Entry(key)[""];
  if (s.empty() || s[0] == '-')
    return 0;
  return static_cast<uint64_t>(std::strtoull(s.c_str(), nullptr, 10));
}

/**
 * Exclusive lock on the cache's lock file, held while the metadata is read,
 * changed and written back, so that several ITK-SNAP processes sharing the
 * cache do not overwrite each other's changes.
 *
 * The lock is flock() on POSIX and LockFileEx() on Windows. The operating
 * system releases it when the process exits, so a crash cannot leave a stale
 * lock behind. The lock is held only for a short metadata update; if it
 * cannot be taken within LOCK_TIMEOUT_MS the cache proceeds without it,
 * since the cache is only an optimization.
 */
class MetadataLock
{
public:
  explicit MetadataLock(const std::string &path)
  {
    constexpr int attempts = LOCK_TIMEOUT_MS / LOCK_POLL_MS;
#ifdef _WIN32
    m_Handle = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (m_Handle == INVALID_HANDLE_VALUE)
      return;
    for (int i = 0; i < attempts && !m_Locked; i++)
      {
      OVERLAPPED ov = {};
      if (LockFileEx(m_Handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
                     0, 1, 0, &ov))
        m_Locked = true;
      else
        std::this_thread::sleep_for(std::chrono::milliseconds(LOCK_POLL_MS));
      }
#else
    m_FD = ::open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    if (m_FD < 0)
      return;
    for (int i = 0; i < attempts && !m_Locked; i++)
      {
      if (::flock(m_FD, LOCK_EX | LOCK_NB) == 0)
        m_Locked = true;
      else if (errno == EWOULDBLOCK || errno == EINTR)
        std::this_thread::sleep_for(std::chrono::milliseconds(LOCK_POLL_MS));
      else
        break; // locking not supported on this file system
      }
#endif
  }

  ~MetadataLock()
  {
#ifdef _WIN32
    if (m_Handle != INVALID_HANDLE_VALUE)
      {
      if (m_Locked)
        {
        OVERLAPPED ov = {};
        UnlockFileEx(m_Handle, 0, 1, 0, &ov);
        }
      CloseHandle(m_Handle);
      }
#else
    if (m_FD >= 0)
      {
      if (m_Locked)
        ::flock(m_FD, LOCK_UN);
      ::close(m_FD);
      }
#endif
  }

  MetadataLock(const MetadataLock &) = delete;
  MetadataLock &operator=(const MetadataLock &) = delete;

private:
  static constexpr int LOCK_TIMEOUT_MS = 10000;
  static constexpr int LOCK_POLL_MS = 20;

#ifdef _WIN32
  HANDLE m_Handle = INVALID_HANDLE_VALUE;
#else
  int m_FD = -1;
#endif
  bool m_Locked = false;
};

} // anonymous namespace


// -----------------------------------------------------------------------
//  RemoteFileCache
// -----------------------------------------------------------------------

std::string RemoteFileCache::MakeKey(const std::string &url)
{
  std::size_t h = std::hash<std::string>{}(url);
  char buf[32];
  snprintf(buf, sizeof(buf), "c%016zx", h);
  return buf;
}

void RemoteFileCache::Initialize(const std::string &app_data_dir,
                                 RemoteResourceSettings *settings)
{
  m_Settings     = settings;
  m_CacheDir     = app_data_dir + "/Cache";
  m_MetadataPath = app_data_dir + "/CacheMetadata.xml";
  m_LockPath     = app_data_dir + "/CacheMetadata.lock";

  // Ensure the cache directory exists.
  itksys::SystemTools::MakeDirectory(m_CacheDir);
}

void RemoteFileCache::LoadMetadata()
{
  m_Entries.clear();

  if (!itksys::SystemTools::FileExists(m_MetadataPath))
    return;

  Registry reg;
  try { reg.ReadFromXMLFile(m_MetadataPath.c_str()); }
  catch (...) { return; }

  Registry::StringListType keys;
  reg.GetFolderKeys(keys);
  for (auto &key : keys)
    {
    Registry &f = reg.Folder(key);
    Entry e;
    e.url          = f.Entry("URL")[""];
    e.local_path   = f.Entry("LocalPath")[""];
    e.remote_size  = ReadNumber(f, "RemoteSize");
    e.remote_mtime = static_cast<uint32_t>(ReadNumber(f, "RemoteMtime"));
    e.etag         = f.Entry("ETag")[""];
    e.last_modified= f.Entry("LastModified")[""];
    e.last_access  = static_cast<int64_t>(ReadNumber(f, "LastAccess"));
    if (!e.url.empty() && !e.local_path.empty())
      m_Entries[key] = e;
    }
}

void RemoteFileCache::SaveMetadata()
{
  Registry reg;
  for (auto &kv : m_Entries)
    {
    Registry &f = reg.Folder(kv.first);
    f.Entry("URL")          << kv.second.url;
    f.Entry("LocalPath")    << kv.second.local_path;
    f.Entry("RemoteSize")   << kv.second.remote_size;
    f.Entry("RemoteMtime")  << kv.second.remote_mtime;
    f.Entry("ETag")         << kv.second.etag;
    f.Entry("LastModified") << kv.second.last_modified;
    f.Entry("LastAccess")   << kv.second.last_access;
    }

  // Write a temp file and rename it, so that the metadata is never left
  // half-written if ITK-SNAP crashes.
  std::string tmp = m_MetadataPath + ".tmp";
  try { reg.WriteToXMLFile(tmp.c_str()); }
  catch (...)
    {
    itksys::SystemTools::RemoveFile(tmp);
    return;
    }
  if (!RenameFile(tmp, m_MetadataPath))
    itksys::SystemTools::RemoveFile(tmp);
}

void RemoteFileCache::RemoveEntryFiles(const std::string &key)
{
  // Each entry has its own directory, named by its key. Remove that directory
  // rather than the parent of the stored path, so that a corrupt metadata
  // file can never delete anything outside of the cache.
  itksys::SystemTools::RemoveADirectory(m_CacheDir + "/" + key);
}

void RemoteFileCache::Evict(const std::string &keep_key)
{
  if (!m_Settings) return;

  int64_t max_bytes = static_cast<int64_t>(m_Settings->GetMaxCacheSizeMB()) * 1024LL * 1024LL;

  // Compute total cached size and collect (access_time, key) pairs for sorting.
  int64_t total = 0;
  std::vector<std::pair<int64_t, std::string>> byAge;  // (last_access, key)
  for (auto &kv : m_Entries)
    {
    total += static_cast<int64_t>(FileBytes(kv.second.local_path));
    if (kv.first != keep_key)
      byAge.emplace_back(kv.second.last_access, kv.first);
    }

  if (total <= max_bytes) return;

  // Evict oldest first. The entry just stored is not in the list: it is about
  // to be loaded, and is kept even when it alone exceeds the limit.
  std::sort(byAge.begin(), byAge.end());
  for (auto &[ts, key] : byAge)
    {
    if (total <= max_bytes) break;
    auto it = m_Entries.find(key);
    if (it == m_Entries.end()) continue;

    total -= static_cast<int64_t>(FileBytes(it->second.local_path));
    RemoveEntryFiles(key);
    m_Entries.erase(it);
    }
}

std::string RemoteFileCache::Lookup(const std::string &url,
                                    uint64_t remote_size,
                                    uint32_t remote_mtime)
{
  MetadataLock lock(m_LockPath);
  LoadMetadata();

  std::string key = MakeKey(url);
  auto it = m_Entries.find(key);
  if (it == m_Entries.end())
    return "";

  Entry &e = it->second;

  // Verify URL matches (in case of hash collision).
  if (e.url != url)
    return "";

  // Staleness check: remote attributes must match stored values.
  if (remote_size != 0 && e.remote_size != remote_size)
    return "";
  if (remote_mtime != 0 && e.remote_mtime != remote_mtime)
    return "";

  // File must still exist on disk.
  if (!itksys::SystemTools::FileExists(e.local_path))
    {
    m_Entries.erase(it);
    SaveMetadata();
    return "";
    }

  // Update last-access time.
  e.last_access = NowSeconds();
  std::string path = e.local_path;
  SaveMetadata();
  return path;
}

std::string RemoteFileCache::StoreEntry(const std::string &temp_path,
                                        const Entry &entry)
{
  // When DeleteAfterDownload is set, skip caching entirely.
  if (m_CacheDir.empty() || (m_Settings && m_Settings->GetDeleteAfterDownload()))
    return temp_path;

  std::string key       = MakeKey(entry.url);
  std::string entry_dir = m_CacheDir + "/" + key;
  std::string basename  = itksys::SystemTools::GetFilenameName(temp_path);
  std::string dest      = entry_dir + "/" + basename;

  // Move the download onto the cache's volume before taking the lock: this
  // is a copy when the temp directory is on another volume, which can take a
  // while for a large image. The name is made unique with the name of the
  // download's temp directory, so that two processes storing the same URL do
  // not write the same file. It is kept outside of the entry directories,
  // which eviction may delete.
  std::string part;
  if (temp_path != dest)
    {
    std::string tmp_name =
      itksys::SystemTools::GetFilenameName(itksys::SystemTools::GetFilenamePath(temp_path));
    part = m_CacheDir + "/" + key + "-" + tmp_name + ".part";
    if (!MoveFileTo(temp_path, part))
      return temp_path;
    }

  MetadataLock lock(m_LockPath);
  LoadMetadata();

  // Under the lock, put the file in place with a quick rename.
  if (!part.empty())
    {
    itksys::SystemTools::MakeDirectory(entry_dir);
    if (!RenameFile(part, dest))
      {
      MoveFileTo(part, temp_path);
      return temp_path;
      }
    }

  Entry e = entry;
  e.local_path   = dest;
  e.last_access  = NowSeconds();
  m_Entries[key] = e;

  Evict(key);
  SaveMetadata();
  return dest;
}

std::string RemoteFileCache::Store(const std::string &url,
                                   const std::string &temp_path,
                                   uint64_t remote_size,
                                   uint32_t remote_mtime)
{
  Entry e;
  e.url          = url;
  e.remote_size  = remote_size;
  e.remote_mtime = remote_mtime;
  return StoreEntry(temp_path, e);
}

RemoteFileCache::HTTPCacheEntry
RemoteFileCache::LookupHTTP(const std::string &url)
{
  MetadataLock lock(m_LockPath);
  LoadMetadata();

  std::string key = MakeKey(url);
  auto it = m_Entries.find(key);
  if (it == m_Entries.end() || it->second.url != url)
    return {};

  Entry &e = it->second;

  // Without at least one token we cannot issue a conditional GET.
  if (e.etag.empty() && e.last_modified.empty())
    return {};

  if (!itksys::SystemTools::FileExists(e.local_path))
    {
    m_Entries.erase(it);
    SaveMetadata();
    return {};
    }

  e.last_access = NowSeconds();
  HTTPCacheEntry result = {e.local_path, e.etag, e.last_modified};
  SaveMetadata();
  return result;
}

std::string
RemoteFileCache::StoreHTTP(const std::string &url,
                           const std::string &temp_path,
                           const std::string &etag,
                           const std::string &last_modified)
{
  Entry e;
  e.url           = url;
  e.etag          = etag;
  e.last_modified = last_modified;
  return StoreEntry(temp_path, e);
}

uint64_t RemoteFileCache::GetTotalSize()
{
  MetadataLock lock(m_LockPath);
  LoadMetadata();

  uint64_t total = 0;
  for (auto &kv : m_Entries)
    total += FileBytes(kv.second.local_path);
  return total;
}

unsigned int RemoteFileCache::GetNumberOfEntries()
{
  MetadataLock lock(m_LockPath);
  LoadMetadata();
  return static_cast<unsigned int>(m_Entries.size());
}
