/**
 * TestRemoteFileCache.cxx
 *
 * Tests RemoteFileCache and RemoteDownload without a network:
 *   - Store() moves the download into the cache;
 *   - remote sizes above 4 GB survive the metadata round trip;
 *   - eviction removes the oldest entries but never the one just stored;
 *   - DeleteAfterDownload leaves the file in its temp directory;
 *   - several cache instances storing at the same time (as several ITK-SNAP
 *     processes would) do not lose each other's entries;
 *   - RemoteDownload deletes a temporary download when destroyed.
 *
 * Usage: TestRemoteFileCache <temp_dir>
 */

#include "ImageIORemote.h"
#include "RemoteFileCache.h"
#include "RemoteResourceSettings.h"

#include "itksys/SystemTools.hxx"

#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using itksys::SystemTools;

static int g_Failures = 0;

#define CHECK(cond, msg)                                              \
  do {                                                                \
    if (cond)                                                         \
      std::cout << "PASS: " << msg << std::endl;                      \
    else                                                              \
      {                                                               \
      std::cerr << "FAIL: " << msg << " (" #cond ")" << std::endl;    \
      g_Failures++;                                                   \
      }                                                               \
  } while (0)

static std::string g_Root;

/** Create <root>/downloads/<name>/<file> holding @p bytes bytes; return its path. */
static std::string MakeDownload(const std::string &name, const std::string &file, size_t bytes)
{
  std::string dir = g_Root + "/downloads/" + name;
  SystemTools::MakeDirectory(dir);
  std::string path = dir + "/" + file;
  std::ofstream out(path, std::ios::binary);
  std::vector<char> buf(bytes, 'x');
  out.write(buf.data(), buf.size());
  return path;
}

static bool IsInDir(const std::string &path, const std::string &dir)
{
  return path.compare(0, dir.size() + 1, dir + "/") == 0;
}

static void TestStoreAndLookup(RemoteResourceSettings *settings)
{
  std::string app_dir = g_Root + "/app1";
  RemoteFileCache cache;
  cache.Initialize(app_dir, settings);

  // A size above 4 GB, which an int would truncate
  const uint64_t big_size = 5000000000ULL;
  const std::string url = "scp://host/data/a.nii.gz";
  std::string tmp = MakeDownload("a", "a.nii.gz", 100);
  std::string cached = cache.Store(url, tmp, big_size, 1234);

  CHECK(IsInDir(cached, app_dir + "/Cache"), "Store returns a path in the cache");
  CHECK(SystemTools::FileExists(cached), "cached file exists");
  CHECK(!SystemTools::FileExists(tmp), "download was moved, not copied");

  // A new instance must read the entry back from disk
  RemoteFileCache cache2;
  cache2.Initialize(app_dir, settings);
  CHECK(cache2.Lookup(url, big_size, 1234) == cached, "lookup with a size above 4 GB hits");
  CHECK(cache2.Lookup(url, big_size + 1, 1234).empty(), "lookup with a different size misses");
  CHECK(cache2.Lookup(url, big_size, 1235).empty(), "lookup with a different mtime misses");
}

static void TestEviction(RemoteResourceSettings *settings)
{
  std::string app_dir = g_Root + "/app2";
  RemoteFileCache cache;
  cache.Initialize(app_dir, settings);
  settings->SetMaxCacheSizeMB(1);

  const size_t kb = 1024;
  std::string a = cache.Store("http://h/a.nii", MakeDownload("ea", "a.nii", 100), 0, 0);
  std::string b = cache.Store("http://h/b.nii", MakeDownload("eb", "b.nii", 600 * kb), 0, 0);
  CHECK(cache.GetNumberOfEntries() == 2, "two entries within the limit are kept");

  std::string c = cache.Store("http://h/c.nii", MakeDownload("ec", "c.nii", 600 * kb), 0, 0);
  CHECK(SystemTools::FileExists(c), "entry just stored is kept");
  CHECK(!SystemTools::FileExists(a) && !SystemTools::FileExists(b),
        "older entries are evicted when over the limit");
  CHECK(cache.GetTotalSize() <= 1024 * kb, "cache is within the limit after eviction");

  // An entry larger than the whole limit is still kept, since it is about to
  // be loaded; everything else goes.
  std::string d = cache.Store("http://h/d.nii", MakeDownload("ed", "d.nii", 1500 * kb), 0, 0);
  CHECK(SystemTools::FileExists(d), "entry larger than the limit is kept");
  CHECK(!SystemTools::FileExists(c), "all other entries are evicted");
  CHECK(cache.GetNumberOfEntries() == 1, "only the new entry remains");
  CHECK(!SystemTools::FileExists(SystemTools::GetFilenamePath(c)),
        "directory of an evicted entry is removed");

  settings->SetMaxCacheSizeMB(1024);
}

static void TestDeleteAfterDownload(RemoteResourceSettings *settings)
{
  std::string app_dir = g_Root + "/app3";
  RemoteFileCache cache;
  cache.Initialize(app_dir, settings);
  settings->SetDeleteAfterDownload(true);

  std::string tmp = MakeDownload("del", "x.nii", 100);
  std::string path = cache.Store("scp://host/x.nii", tmp, 100, 1);
  CHECK(path == tmp, "with DeleteAfterDownload, Store returns the temp path");
  CHECK(SystemTools::FileExists(tmp), "with DeleteAfterDownload, the temp file is left alone");
  CHECK(cache.GetNumberOfEntries() == 0, "with DeleteAfterDownload, nothing is cached");

  settings->SetDeleteAfterDownload(false);
}

static void TestConcurrentStores(RemoteResourceSettings *settings)
{
  // Each thread uses its own cache instance, like separate processes would.
  // The lock is a file lock on a separately opened file, which excludes
  // other threads of the same process too.
  std::string app_dir = g_Root + "/app4";
  const int n_threads = 8, n_per_thread = 10;

  std::vector<std::thread> threads;
  for (int t = 0; t < n_threads; t++)
    {
    threads.emplace_back([&, t]() {
      RemoteFileCache cache;
      cache.Initialize(app_dir, settings);
      for (int i = 0; i < n_per_thread; i++)
        {
        std::string name = "t" + std::to_string(t) + "_" + std::to_string(i);
        std::string url = "scp://host/" + name + ".nii";
        std::string tmp = MakeDownload(name, name + ".nii", 1000);
        cache.Store(url, tmp, 1000, 7);
        cache.Lookup(url, 1000, 7);
        }
    });
    }
  for (auto &th : threads)
    th.join();

  RemoteFileCache cache;
  cache.Initialize(app_dir, settings);
  unsigned int n = cache.GetNumberOfEntries();
  std::cout << "  entries after concurrent stores: " << n << std::endl;
  CHECK(n == n_threads * n_per_thread, "no entry is lost when stores run concurrently");

  int n_hits = 0;
  for (int t = 0; t < n_threads; t++)
    for (int i = 0; i < n_per_thread; i++)
      {
      std::string name = "t" + std::to_string(t) + "_" + std::to_string(i);
      if (!cache.Lookup("scp://host/" + name + ".nii", 1000, 7).empty())
        n_hits++;
      }
  CHECK(n_hits == n_threads * n_per_thread, "every concurrently stored entry can be looked up");
}

static void TestRemoteDownload()
{
  std::string a = MakeDownload("rd_a", "a.nii", 10);
  std::string a_dir = SystemTools::GetFilenamePath(a);
  {
  RemoteDownload dl(a, a_dir);
  CHECK(dl.GetPath() == a, "RemoteDownload returns the path");
  }
  CHECK(!SystemTools::FileExists(a_dir), "temporary download is deleted when released");

  std::string b = MakeDownload("rd_b", "b.nii", 10);
  std::string b_dir = SystemTools::GetFilenamePath(b);
  {
  RemoteDownload outer;
  {
  RemoteDownload inner(b, b_dir);
  outer = std::move(inner);
  }
  CHECK(SystemTools::FileExists(b), "moved-from RemoteDownload does not delete the file");
  }
  CHECK(!SystemTools::FileExists(b_dir), "moved-to RemoteDownload deletes the file");

  // A file in the cache (no temp dir) is never deleted
  std::string c = MakeDownload("rd_c", "c.nii", 10);
  {
  RemoteDownload dl(c, "");
  }
  CHECK(SystemTools::FileExists(c), "cached file is kept");
}

int main(int argc, char *argv[])
{
  if (argc < 2)
    {
    std::cerr << "Usage: " << argv[0] << " <temp_dir>" << std::endl;
    return 1;
    }

  g_Root = std::string(argv[1]) + "/TestRemoteFileCache";
  SystemTools::RemoveADirectory(g_Root);
  SystemTools::MakeDirectory(g_Root);

  auto settings = RemoteResourceSettings::New();
  settings->SetMaxCacheSizeMB(1024);
  settings->SetDeleteAfterDownload(false);

  TestStoreAndLookup(settings);
  TestEviction(settings);
  TestDeleteAfterDownload(settings);
  TestConcurrentStores(settings);
  TestRemoteDownload();

  SystemTools::RemoveADirectory(g_Root);

  if (g_Failures)
    {
    std::cerr << g_Failures << " check(s) failed" << std::endl;
    return 1;
    }
  std::cout << "All checks passed" << std::endl;
  return 0;
}
