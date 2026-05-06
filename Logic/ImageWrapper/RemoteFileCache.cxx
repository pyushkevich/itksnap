#include "RemoteFileCache.h"
#include "RemoteResourceSettings.h"
#include "Registry.h"
#include "IRISException.h"
#include <itksys/SystemTools.hxx>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <list>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <sys/stat.h>
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
bool CopyFile(const std::string &src, const std::string &dst)
{
  std::ifstream in(src, std::ios::binary);
  if (!in) return false;
  std::ofstream out(dst, std::ios::binary);
  if (!out) return false;
  out << in.rdbuf();
  return in.good() || in.eof();
}

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

  // Ensure the cache directory exists.
  itksys::SystemTools::MakeDirectory(m_CacheDir);
}

void RemoteFileCache::EnsureLoaded()
{
  if (!m_Loaded)
    {
    LoadMetadata();
    m_Loaded = true;
    }
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
    e.remote_size  = static_cast<uint64_t>(f.Entry("RemoteSize")[0]);
    e.remote_mtime = static_cast<uint32_t>(f.Entry("RemoteMtime")[0]);
    e.last_access  = static_cast<int64_t>(f.Entry("LastAccess")[0]);
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
    f.Entry("RemoteSize")   << static_cast<int>(kv.second.remote_size);
    f.Entry("RemoteMtime")  << static_cast<int>(kv.second.remote_mtime);
    f.Entry("LastAccess")   << static_cast<int>(kv.second.last_access);
    }
  try { reg.WriteToXMLFile(m_MetadataPath.c_str()); }
  catch (...) {}
}

void RemoteFileCache::Evict()
{
  if (!m_Settings) return;

  int64_t max_bytes = static_cast<int64_t>(m_Settings->GetMaxCacheSizeMB()) * 1024LL * 1024LL;

  // Compute total cached size and collect (access_time, key) pairs for sorting.
  int64_t total = 0;
  std::vector<std::pair<int64_t, std::string>> byAge;  // (last_access, key)
  for (auto &kv : m_Entries)
    {
    total += static_cast<int64_t>(FileBytes(kv.second.local_path));
    byAge.emplace_back(kv.second.last_access, kv.first);
    }

  if (total <= max_bytes) return;

  // Evict oldest first.
  std::sort(byAge.begin(), byAge.end());
  for (auto &[ts, key] : byAge)
    {
    if (total <= max_bytes) break;
    auto it = m_Entries.find(key);
    if (it == m_Entries.end()) continue;

    const std::string &path = it->second.local_path;
    uint64_t sz = FileBytes(path);
    itksys::SystemTools::RemoveFile(path);
    // Remove the per-entry subdirectory if now empty.
    std::string dir = itksys::SystemTools::GetFilenamePath(path);
    itksys::SystemTools::RemoveADirectory(dir);
    total -= static_cast<int64_t>(sz);
    m_Entries.erase(it);
    }
}

std::string RemoteFileCache::Lookup(const std::string &url,
                                    uint64_t remote_size,
                                    uint32_t remote_mtime)
{
  EnsureLoaded();

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
  SaveMetadata();
  return e.local_path;
}

std::string RemoteFileCache::Store(const std::string &url,
                                   const std::string &temp_path,
                                   uint64_t remote_size,
                                   uint32_t remote_mtime)
{
  // When DeleteAfterDownload is set, skip caching entirely.
  if (m_Settings && m_Settings->GetDeleteAfterDownload())
    return temp_path;

  EnsureLoaded();

  std::string key      = MakeKey(url);
  std::string entry_dir = m_CacheDir + "/" + key;
  itksys::SystemTools::MakeDirectory(entry_dir);

  std::string basename = itksys::SystemTools::GetFilenameName(temp_path);
  std::string dest     = entry_dir + "/" + basename;

  if (!CopyFile(temp_path, dest))
    return temp_path;  // copy failed — fall back to temp file

  Entry e;
  e.url          = url;
  e.local_path   = dest;
  e.remote_size  = remote_size;
  e.remote_mtime = remote_mtime;
  e.last_access  = NowSeconds();
  m_Entries[key] = e;

  Evict();
  SaveMetadata();
  return dest;
}
