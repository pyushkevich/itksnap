#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ImageIODelegates.h"
#include "ImageIORemote.h"
#include "GlobalState.h"
#include "ImageMeshLayers.h"
#include "RemoteFileCache.h"
#include "RemoteResourceSettings.h"
#include "UIReporterDelegates.h"
#include "LayerIterator.h"
#include "TDigestImageFilter.h"
#include "WorkspaceAPI.h"
#include "HistoryManager.h"
#include "SystemInterface.h"
#include <itksys/SystemTools.hxx>
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <cmath>
#ifdef _WIN32
#  include <windows.h>
#else
#  include <sys/stat.h>
#endif

// Minimal delegate — returns system temp dir, stubs everything else
class SimpleSystemInfoDelegate : public SystemInfoDelegate
{
public:
  SimpleSystemInfoDelegate(const char *argv0) { m_Exe = argv0; }

  std::string GetApplicationDirectory() override
    { return itksys::SystemTools::GetFilenamePath(m_Exe); }
  std::string GetApplicationFile() override { return m_Exe; }
  std::string GetApplicationPermanentDataLocation() override { return ".itksnap_test"; }
  std::string GetUserDocumentsLocation() override { return ".itksnap_test"; }
  std::string GetTempDirectory() override
    {
    const char *t = getenv("TMPDIR");
    if (t) return std::string(t);
#ifdef _WIN32
    const char *tmp = getenv("TEMP");
    if (!tmp) tmp = getenv("TMP");
    return tmp ? std::string(tmp) : std::string("C:\\Temp");
#else
    return std::string("/tmp");
#endif
    }
  std::string EncodeServerURL(const std::string &url) override { return url; }

  typedef SystemInfoDelegate::GrayscaleImage GrayscaleImage;
  typedef SystemInfoDelegate::RGBAPixelType  RGBAPixelType;
  typedef SystemInfoDelegate::RGBAImageType  RGBAImageType;
  void LoadResourceAsImage2D(std::string, GrayscaleImage *) override {}
  void LoadResourceAsRegistry(std::string, Registry &) override {}
  void WriteRGBAImage2D(std::string, RGBAImageType *) override {}

private:
  std::string m_Exe;
};

class SimpleColorMapSource : public AbstractColorMapPresetNameSource
{
public:
  std::string GetPresetName(ColorMap::SystemPreset p, bool) override
    { return "Preset " + std::to_string(static_cast<int>(p)); }
};


// Print geometry + intensity quantiles for one image layer
void PrintLayerInfo(ImageWrapperBase *layer)
{
  auto sz = layer->GetSize();
  std::cout << "  Dimensions : " << sz[0] << " x " << sz[1] << " x " << sz[2] << std::endl;

  auto *base = layer->GetImageBase();
  auto sp = base->GetSpacing();
  std::cout << "  Spacing    : " << sp[0] << " x " << sp[1] << " x " << sp[2] << " mm" << std::endl;

  auto org = base->GetOrigin();
  std::cout << "  Origin     : " << org[0] << ", " << org[1] << ", " << org[2] << std::endl;

  std::cout << "  Components : " << layer->GetNumberOfComponents() << std::endl;

  TDigestDataObject *td = layer->GetTDigest();
  if (td)
    {
    td->Update();
    const double ranks[] = {0.0, 0.05, 0.25, 0.50, 0.75, 0.95, 1.0};
    std::cout << "  Quantiles  :";
    for (double r : ranks)
      std::cout << "  p" << static_cast<int>(r * 100) << "=" << td->GetImageQuantile(r);
    std::cout << std::endl;
    }
  else
    {
    std::cout << "  Quantiles  : (not available)" << std::endl;
    }
}


// Human-readable role names
static const char *RoleName(LayerRole role)
{
  switch (role)
    {
    case MAIN_ROLE:    return "MainRole";
    case OVERLAY_ROLE: return "OverlayRole";
    case LABEL_ROLE:   return "SegmentationRole";
    default:           return "UnknownRole";
    }
}


static void PrintUsage(const char *prog)
{
  std::cerr << "Usage:" << std::endl;
  std::cerr << "  " << prog << " -g <url-or-path>   load a single image" << std::endl;
  std::cerr << "  " << prog << " -w <url-or-path>   load a workspace" << std::endl;
  std::cerr << "  " << prog << " --regression-test -g <remote-url> <local-ref>  compare remote/local image" << std::endl;
  std::cerr << "  " << prog << " --regression-test -w <remote-url> <local-ref>  compare remote/local workspace" << std::endl;
  std::cerr << "  " << prog << " --test-cache <remote-url>  verify cache fill and conditional-GET hit" << std::endl;
  std::cerr << "  " << prog << " --test-save-workspace <remote-url> <temp-dir>  save/reopen a workspace with a remote layer" << std::endl;
  std::cerr << "  " << prog << " --test-cancel <remote-url>  cancel an HTTP download from the progress callback" << std::endl;
  std::cerr << "  " << prog << " --test-url-keys <remote-url>  history and per-image settings use the URL" << std::endl;
}


// -----------------------------------------------------------------------
//  Helpers for regression comparison
// -----------------------------------------------------------------------

struct LayerSnapshot
{
  Vector3ui           size;
  std::vector<double> spacing;   // [3] — copied from GetSpacing()
  unsigned int        components;
  std::vector<double> quantiles; // at p0,5,25,50,75,95,100
};

static const double kRanks[] = {0.0, 0.05, 0.25, 0.50, 0.75, 0.95, 1.0};
static const int    kNRanks  = 7;

static LayerSnapshot SnapshotLayer(ImageWrapperBase *layer)
{
  LayerSnapshot s;
  s.size       = layer->GetSize();
  auto sp = layer->GetImageBase()->GetSpacing();
  s.spacing    = {sp[0], sp[1], sp[2]};
  s.components = layer->GetNumberOfComponents();

  TDigestDataObject *td = layer->GetTDigest();
  s.quantiles.resize(kNRanks, 0.0);
  if (td)
    {
    td->Update();
    for (int i = 0; i < kNRanks; ++i)
      s.quantiles[i] = td->GetImageQuantile(kRanks[i]);
    }
  return s;
}

// Returns true if two snapshots match within tolerance.
static bool CompareSnapshots(const LayerSnapshot &a, const LayerSnapshot &b,
                             std::string &msg)
{
  for (int d = 0; d < 3; ++d)
    if (a.size[d] != b.size[d])
      {
      msg = "dimension mismatch at axis " + std::to_string(d);
      return false;
      }

  for (int d = 0; d < 3; ++d)
    {
    double diff = std::abs(a.spacing[d] - b.spacing[d]);
    if (diff > 1e-6 * std::max(std::abs(a.spacing[d]), 1.0))
      {
      msg = "spacing mismatch at axis " + std::to_string(d);
      return false;
      }
    }

  if (a.components != b.components)
    {
    msg = "component count mismatch";
    return false;
    }

  // Quantiles come from a T-Digest, which is approximate and not fully
  // deterministic (results vary slightly between runs on the same image), so
  // compare them to within a fraction of the reference intensity range rather
  // than exactly. This still catches a wrong or corrupted image.
  double range = std::abs(b.quantiles[kNRanks - 1] - b.quantiles[0]);
  double tol = std::max(0.01 * range, 1e-6);
  for (int i = 0; i < kNRanks; ++i)
    {
    if (std::abs(a.quantiles[i] - b.quantiles[i]) > tol)
      {
      msg = "quantile p" + std::to_string(static_cast<int>(kRanks[i] * 100))
            + " mismatch: remote=" + std::to_string(a.quantiles[i])
            + " local=" + std::to_string(b.quantiles[i]);
      return false;
      }
    }

  return true;
}

// Collect snapshots for all image layers in load order.
static std::vector<LayerSnapshot> SnapshotAllLayers(IRISApplication *app)
{
  std::vector<LayerSnapshot> snaps;
  GenericImageData *imgdata = app->GetCurrentImageData();
  for (LayerIterator it(imgdata); !it.IsAtEnd(); ++it)
    snaps.push_back(SnapshotLayer(it.GetLayer()));
  return snaps;
}

// Build and configure a fresh IRISApplication using the shared delegate.
static IRISApplication::Pointer MakeApp()
{
  auto app = IRISApplication::New();
  // Enable caching with a generous limit; use the test data dir for storage.
  app->GetGlobalState()->GetRemoteResourceSettings()->SetMaxCacheSizeMB(512);
  app->GetGlobalState()->GetRemoteResourceSettings()->SetDeleteAfterDownload(false);
  return app;
}


// -----------------------------------------------------------------------
//  --regression-test
// -----------------------------------------------------------------------

static int RegressionTest(const std::string &flag,
                          const std::string &remote_url,
                          const std::string &local_ref)
{
  IRISWarningList warnings;

  // ── Remote load ─────────────────────────────────────────────────────
  std::cout << "Loading remote: " << remote_url << std::endl;
  auto remote_app = MakeApp();

  try
    {
    if (flag == "-g")
      remote_app->OpenImage(remote_url.c_str(), MAIN_ROLE, warnings);
    else
      remote_app->OpenWorkspace(remote_url, warnings);
    }
  catch (std::exception &e)
    {
    std::cerr << "FAIL: remote load error: " << e.what() << std::endl;
    return 1;
    }
  for (auto &w : warnings)
    std::cerr << "Warning (remote): " << w.what() << std::endl;

  // ── Local reference load ─────────────────────────────────────────────
  std::cout << "Loading local reference: " << local_ref << std::endl;
  auto local_app = MakeApp();

  try
    {
    if (flag == "-g")
      local_app->OpenImage(local_ref.c_str(), MAIN_ROLE, warnings);
    else
      local_app->OpenWorkspace(local_ref, warnings);
    }
  catch (std::exception &e)
    {
    std::cerr << "FAIL: local reference load error: " << e.what() << std::endl;
    return 1;
    }
  for (auto &w : warnings)
    std::cerr << "Warning (local): " << w.what() << std::endl;

  // ── Compare ──────────────────────────────────────────────────────────
  auto remote_snaps = SnapshotAllLayers(remote_app);
  auto local_snaps  = SnapshotAllLayers(local_app);

  if (remote_snaps.empty())
    {
    std::cerr << "FAIL: no image layers found in remote load" << std::endl;
    return 1;
    }

  if (remote_snaps.size() != local_snaps.size())
    {
    std::cerr << "FAIL: layer count mismatch: remote=" << remote_snaps.size()
              << " local=" << local_snaps.size() << std::endl;
    return 1;
    }

  bool all_ok = true;
  for (std::size_t i = 0; i < remote_snaps.size(); ++i)
    {
    std::string msg;
    if (!CompareSnapshots(remote_snaps[i], local_snaps[i], msg))
      {
      std::cerr << "FAIL: layer " << i << ": " << msg << std::endl;
      all_ok = false;
      }
    else
      {
      std::cout << "PASS: layer " << i << " matches reference" << std::endl;
      }
    }

  // For workspace loads, also compare mesh layer counts.
  if (flag == "-w")
    {
    std::size_t remote_meshes =
      remote_app->GetCurrentImageData()->GetMeshLayers()->size();
    std::size_t local_meshes  =
      local_app->GetCurrentImageData()->GetMeshLayers()->size();

    if (remote_meshes != local_meshes)
      {
      std::cerr << "FAIL: mesh layer count mismatch: remote=" << remote_meshes
                << " local=" << local_meshes << std::endl;
      all_ok = false;
      }
    else
      {
      std::cout << "PASS: mesh layer count matches (" << remote_meshes << ")" << std::endl;
      }
    }

  return all_ok ? 0 : 1;
}


// -----------------------------------------------------------------------
//  --test-cache
// -----------------------------------------------------------------------

static int TestCache(const std::string &url)
{
  // Derive the cache paths from the same data directory the app uses. CMake
  // gives this test a directory of its own: on macOS the delegate's location
  // in the working directory, on Windows APPDATA and on Linux HOME
  auto app1 = MakeApp();
  std::string data_dir  = app1->GetSystemInterface()->GetApplicationDataDirectory();
  std::string cache_dir = data_dir + "/Cache";
  std::string meta_path = data_dir + "/CacheMetadata.xml";

  // ── 1. Clear cache ───────────────────────────────────────────────────
  std::cout << "Clearing cache at " << data_dir << std::endl;
  itksys::SystemTools::RemoveADirectory(cache_dir);
  itksys::SystemTools::RemoveFile(meta_path);

  if (itksys::SystemTools::FileExists(meta_path))
    {
    std::cerr << "FAIL: could not remove CacheMetadata.xml" << std::endl;
    return 1;
    }

  // ── 2. First download ────────────────────────────────────────────────
  std::cout << "First download: " << url << std::endl;

  IRISWarningList warnings;
  try
    {
    app1->OpenImage(url.c_str(), MAIN_ROLE, warnings);
    }
  catch (std::exception &e)
    {
    std::cerr << "FAIL: first download error: " << e.what() << std::endl;
    return 1;
    }

  // Verify that the cache metadata file was created.
  if (!itksys::SystemTools::FileExists(meta_path))
    {
    std::cerr << "FAIL: CacheMetadata.xml not created after first download" << std::endl;
    return 1;
    }
  std::cout << "PASS: cache populated after first download" << std::endl;

  // Check whether the server sent validation tokens (ETag or Last-Modified).
  // Parse the metadata XML minimally — just look for a non-empty ETag or
  // LastModified entry.  If neither was returned the server doesn't support
  // conditional GET and we skip the 304 check rather than failing.
  Registry meta_reg;
  bool has_tokens = false;
  try
    {
    meta_reg.ReadFromXMLFile(meta_path.c_str());
    Registry::StringListType keys;
    meta_reg.GetFolderKeys(keys);
    for (auto &k : keys)
      {
      std::string etag = meta_reg.Folder(k)["ETag"][""];
      std::string lm   = meta_reg.Folder(k)["LastModified"][""];
      if (!etag.empty() || !lm.empty())
        {
        has_tokens = true;
        std::cout << "PASS: cache entry has validation tokens (ETag=\"" << etag
                  << "\", Last-Modified=\"" << lm << "\")" << std::endl;
        break;
        }
      }
    }
  catch (...) {}

  if (!has_tokens)
    {
    std::cout << "NOTE: server did not return ETag/Last-Modified; "
              << "skipping 304 conditional-GET check" << std::endl;
    return 0;
    }

  // ── 3. Record the cached file's local path and stat ──────────────────
  // Find the local_path entry in the metadata.
  std::string cached_path;
  try
    {
    Registry::StringListType keys;
    meta_reg.GetFolderKeys(keys);
    if (!keys.empty())
      cached_path = meta_reg.Folder(keys.front())["LocalPath"][""];
    }
  catch (...) {}

  if (cached_path.empty() || !itksys::SystemTools::FileExists(cached_path))
    {
    std::cerr << "FAIL: could not locate cached file from metadata" << std::endl;
    return 1;
    }

#ifdef _WIN32
  FILETIME ft_before = {};
  {
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (GetFileAttributesExA(cached_path.c_str(), GetFileExInfoStandard, &info))
      ft_before = info.ftLastWriteTime;
  }
#else
  struct stat st_before;
  ::stat(cached_path.c_str(), &st_before);
#endif

  // ── 4. Second download (should be a 304 cache hit) ───────────────────
  std::cout << "Second download (expect cache hit): " << url << std::endl;

  auto app2 = MakeApp();
  try
    {
    app2->OpenImage(url.c_str(), MAIN_ROLE, warnings);
    }
  catch (std::exception &e)
    {
    std::cerr << "FAIL: second download error: " << e.what() << std::endl;
    return 1;
    }

  // Verify that the cached file was not re-written (mtime unchanged).
#ifdef _WIN32
  FILETIME ft_after = {};
  {
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (GetFileAttributesExA(cached_path.c_str(), GetFileExInfoStandard, &info))
      ft_after = info.ftLastWriteTime;
  }
  bool mtime_unchanged = (CompareFileTime(&ft_before, &ft_after) == 0);
#else
  struct stat st_after;
  ::stat(cached_path.c_str(), &st_after);
  bool mtime_unchanged = (st_before.st_mtime == st_after.st_mtime);
#endif

  if (!mtime_unchanged)
    {
    std::cerr << "FAIL: cached file was re-written on second download "
              << "(expected 304 conditional-GET hit)" << std::endl;
    return 1;
    }

  std::cout << "PASS: cached file not re-written — 304 conditional-GET confirmed" << std::endl;
  return 0;
}


// -----------------------------------------------------------------------
//  --test-cancel
//
//  Download a file over HTTP(S) without the cache, cancelling from the
//  progress callback. The download must stop with IRISUserCancelException,
//  both when cancelled right away and once data has started to arrive. A
//  download that is not cancelled must still succeed.
// -----------------------------------------------------------------------

static int TestCancel(const std::string &url)
{
  // Download with a callback that cancels once at least min_bytes arrived.
  // Returns 0 if cancelled, 1 if it completed, 2 on another error.
  auto download = [&url](std::size_t min_bytes, bool cancel, int &n_calls) -> int
  {
    n_calls = 0;
    SmartPtr<RemoteImageSource> src = CreateRemoteImageSource(url);
    src->SetProgressCallback([&](std::size_t done, std::size_t) {
      n_calls++;
      return !(cancel && done >= min_bytes);
    });
    try
      {
      // Delete the downloaded file again
      std::string path = src->Download(url);
      RemoteDownload dl(path, src->GetTemporaryDirectory());
      return 1;
      }
    catch (IRISUserCancelException &)
      {
      return 0;
      }
    catch (std::exception &e)
      {
      std::cerr << "  error: " << e.what() << std::endl;
      return 2;
      }
  };

  int n_calls = 0;
  int rc = download(0, true, n_calls);
  if (rc != 0)
    {
    std::cerr << "FAIL: download cancelled at the first callback was not cancelled (rc="
              << rc << ")" << std::endl;
    return 1;
    }
  if (n_calls != 1)
    {
    std::cerr << "FAIL: callback was called " << n_calls << " times after cancelling" << std::endl;
    return 1;
    }
  std::cout << "PASS: download cancelled at the first progress callback" << std::endl;

  rc = download(1, true, n_calls);
  if (rc == 0)
    std::cout << "PASS: download cancelled after data started to arrive" << std::endl;
  else if (rc == 1)
    std::cout << "SKIP: download finished before any partial progress was reported" << std::endl;
  else
    {
    std::cerr << "FAIL: unexpected error when cancelling mid-download" << std::endl;
    return 1;
    }

  rc = download(0, false, n_calls);
  if (rc != 1)
    {
    std::cerr << "FAIL: download without cancelling did not complete" << std::endl;
    return 1;
    }
  std::cout << "PASS: download without cancelling completed (" << n_calls
            << " progress callbacks)" << std::endl;
  return 0;
}


// -----------------------------------------------------------------------
//  --test-save-workspace
//
//  Load an image from a remote URL, save a local workspace, and check that
//  the workspace refers to the image by its URL (not by a mangled local
//  path or the cache/temp file), that it can be reopened with the same data,
//  that it can be exported (as for an upload to the distributed segmentation
//  service), and that saving a workspace to a remote URL is refused.
// -----------------------------------------------------------------------

static int TestSaveWorkspace(const std::string &url, const std::string &tempdir)
{
  std::string ws_file = tempdir + "/RemoteImageLoadTest_SaveWorkspace.itksnap";
  itksys::SystemTools::MakeDirectory(tempdir);
  IRISWarningList warnings;

  // Load the remote image and save a workspace
  std::vector<LayerSnapshot> snaps1;
  try
    {
    IRISApplication::Pointer app1 = MakeApp();
    app1->OpenImage(url.c_str(), MAIN_ROLE, warnings);
    snaps1 = SnapshotAllLayers(app1);
    app1->SaveProject(ws_file);

    // Saving to a remote location must fail cleanly
    bool threw = false;
    try
      {
      app1->SaveProject(url + ".itksnap");
      }
    catch (IRISException &)
      {
      threw = true;
      }
    if (!threw)
      {
      std::cerr << "FAIL: saving a workspace to a remote URL did not throw" << std::endl;
      return 1;
      }
    }
  catch (std::exception &exc)
    {
    std::cerr << "FAIL: could not load image / save workspace: " << exc.what() << std::endl;
    return 1;
    }

  // The main layer must be stored by its URL
  Registry reg;
  reg.ReadFromXMLFile(ws_file.c_str());
  std::string stored = reg["Layers.Layer[000].AbsolutePath"][""];
  if (stored != url)
    {
    std::cerr << "FAIL: workspace stores '" << stored << "', expected '" << url << "'" << std::endl;
    return 1;
    }

  // Reopen the workspace and compare the data
  std::vector<LayerSnapshot> snaps2;
  try
    {
    IRISApplication::Pointer app2 = MakeApp();
    app2->OpenWorkspace(ws_file, warnings);
    snaps2 = SnapshotAllLayers(app2);
    }
  catch (std::exception &exc)
    {
    std::cerr << "FAIL: could not reopen saved workspace: " << exc.what() << std::endl;
    return 1;
    }

  if (snaps1.size() != snaps2.size())
    {
    std::cerr << "FAIL: reopened workspace has " << snaps2.size() << " layers, expected "
              << snaps1.size() << std::endl;
    return 1;
    }
  for (size_t i = 0; i < snaps1.size(); ++i)
    {
    std::string msg;
    if (!CompareSnapshots(snaps1[i], snaps2[i], msg))
      {
      std::cerr << "FAIL: layer " << i << " differs after reopening: " << msg << std::endl;
      return 1;
      }
    }

  // Export the workspace, as the distributed segmentation service does
  // before uploading it: the remote layer must be downloaded and saved as a
  // local file, and the export must open with the same data
  std::string export_dir = tempdir + "/RemoteImageLoadTest_Export";
  std::string export_file = export_dir + "/exported.itksnap";
  itksys::SystemTools::RemoveADirectory(export_dir);
  itksys::SystemTools::MakeDirectory(export_dir);
  std::vector<LayerSnapshot> snaps3;
  try
    {
    WorkspaceAPI ws;
    ws.ReadFromXMLFile(ws_file.c_str());
    ws.ExportWorkspace(export_file.c_str());

    WorkspaceAPI exported;
    exported.ReadFromXMLFile(export_file.c_str());
    for (int i = 0; i < exported.GetNumberOfLayers(); i++)
      {
      std::string path = exported.GetLayerActualPath(exported.GetLayerFolder(i));
      if (IsRemoteImageURL(path) || !itksys::SystemTools::FileExists(path))
        {
        std::cerr << "FAIL: exported layer " << i << " is not a local file: " << path << std::endl;
        return 1;
        }
      }

    IRISApplication::Pointer app3 = MakeApp();
    app3->OpenWorkspace(export_file, warnings);
    snaps3 = SnapshotAllLayers(app3);
    }
  catch (std::exception &exc)
    {
    std::cerr << "FAIL: could not export the workspace: " << exc.what() << std::endl;
    return 1;
    }

  for (size_t i = 0; i < snaps1.size() && i < snaps3.size(); ++i)
    {
    std::string msg;
    if (!CompareSnapshots(snaps1[i], snaps3[i], msg))
      {
      std::cerr << "FAIL: layer " << i << " differs after exporting: " << msg << std::endl;
      return 1;
      }
    }
  if (snaps1.size() != snaps3.size())
    {
    std::cerr << "FAIL: exported workspace has " << snaps3.size() << " layers, expected "
              << snaps1.size() << std::endl;
    return 1;
    }

  itksys::SystemTools::RemoveFile(ws_file);
  itksys::SystemTools::RemoveADirectory(export_dir);
  std::cout << "PASS: workspace with a remote layer saved, reopened and exported" << std::endl;
  return 0;
}


// -----------------------------------------------------------------------
//  --test-url-keys
// -----------------------------------------------------------------------

// A remote image is known by its URL, not by the path of the downloaded copy:
// the layer name, the recent-image history and the per-image settings
// (display settings, tags, IO hints) all use the URL. The settings saved when
// the image is closed must come back when it is opened again.
static int TestURLKeys(const std::string &url)
{
  IRISWarningList warnings;
  try
    {
    IRISApplication::Pointer app = MakeApp();
    HistoryManager *hm = app->GetHistoryManager();

    // Open the image and tag it
    app->OpenImage(url.c_str(), MAIN_ROLE, warnings);
    ImageWrapperBase *layer = app->GetCurrentImageData()->GetMain();
    std::cout << "Layer file name: " << layer->GetFileName() << std::endl;
    if (url != layer->GetFileName())
      {
      std::cerr << "FAIL: the layer file name is not the URL" << std::endl;
      return 1;
      }

    // The history holds the URL, not the downloaded copy
    for (const char *cat : { "MainImage", "AnatomicImage" })
      {
      HistoryManager::HistoryListType hist = hm->GetGlobalHistory(cat);
      std::string last = hist.size() ? hist.back() : std::string();
      std::cout << "Last " << cat << " history entry: " << last << std::endl;
      if (last != url)
        {
        std::cerr << "FAIL: the " << cat << " history does not end with the URL" << std::endl;
        return 1;
        }
      }

    TagList tags;
    tags.push_back("url-keys-test");
    layer->SetTags(tags);

    // Closing the image saves its settings, under the URL
    app->UnloadMainImage();
    Registry assoc;
    if (!app->GetSystemInterface()->FindRegistryAssociatedWithFile(url.c_str(), assoc))
      {
      std::cerr << "FAIL: no settings associated with the URL" << std::endl;
      return 1;
      }

    // Opening it again restores them
    app->OpenImage(url.c_str(), MAIN_ROLE, warnings);
    TagList restored = app->GetCurrentImageData()->GetMain()->GetTags();
    std::cout << "Tags after reopening: " << restored.size() << std::endl;
    if (restored.size() != 1 || restored.front() != "url-keys-test")
      {
      std::cerr << "FAIL: the image's settings were not restored when it was reopened" << std::endl;
      return 1;
      }
    }
  catch (std::exception &exc)
    {
    std::cerr << "FAIL: " << exc.what() << std::endl;
    return 1;
    }

  std::cout << "PASS: history and per-image settings use the URL" << std::endl;
  return 0;
}


// -----------------------------------------------------------------------
//  main
// -----------------------------------------------------------------------

int main(int argc, char *argv[])
{
  if (argc < 2)
    {
    PrintUsage(argv[0]);
    return 1;
    }

  // --- Setup ---
  SimpleSystemInfoDelegate sidel(argv[0]);
  SystemInterface::SetSystemInfoDelegate(&sidel);

  SimpleColorMapSource cmSource;
  ColorMap::SetColorMapPresetNameSource(&cmSource);

  std::string cmd = argv[1];

  // ── --regression-test ───────────────────────────────────────────────
  if (cmd == "--regression-test")
    {
    if (argc < 5)
      {
      PrintUsage(argv[0]);
      return 1;
      }
    std::string flag       = argv[2];
    std::string remote_url = ResolveITKSnapURL(argv[3]);
    std::string local_ref  = argv[4];

    if (flag != "-g" && flag != "-w")
      {
      PrintUsage(argv[0]);
      return 1;
      }
    return RegressionTest(flag, remote_url, local_ref);
    }

  // ── --test-cache ─────────────────────────────────────────────────────
  if (cmd == "--test-cache")
    {
    if (argc < 3)
      {
      PrintUsage(argv[0]);
      return 1;
      }
    std::string url = ResolveITKSnapURL(argv[2]);
    return TestCache(url);
    }

  // ── --test-cancel ────────────────────────────────────────────────────
  if (cmd == "--test-cancel")
    {
    if (argc < 3)
      {
      PrintUsage(argv[0]);
      return 1;
      }
    std::string url = ResolveITKSnapURL(argv[2]);
    return TestCancel(url);
    }

  // ── --test-save-workspace ────────────────────────────────────────────
  if (cmd == "--test-save-workspace")
    {
    if (argc < 4)
      {
      PrintUsage(argv[0]);
      return 1;
      }
    std::string url = ResolveITKSnapURL(argv[2]);
    return TestSaveWorkspace(url, argv[3]);
    }

  // ── --test-url-keys ──────────────────────────────────────────────────
  if (cmd == "--test-url-keys")
    {
    if (argc < 3)
      {
      PrintUsage(argv[0]);
      return 1;
      }
    std::string url = ResolveITKSnapURL(argv[2]);
    return TestURLKeys(url);
    }

  // ── Legacy: -g / -w ─────────────────────────────────────────────────
  if (argc < 3)
    {
    PrintUsage(argv[0]);
    return 1;
    }

  std::string flag = argv[1];
  std::string path = ResolveITKSnapURL(argv[2]);

  if (flag != "-g" && flag != "-w")
    {
    PrintUsage(argv[0]);
    return 1;
    }

  IRISWarningList warnings;
  auto app = MakeApp();

  // ── Single image (-g) ────────────────────────────────────────────────
  if (flag == "-g")
    {
    std::cout << "Loading image: " << path << std::endl;
    try
      {
      app->OpenImage(path.c_str(), MAIN_ROLE, warnings);
      }
    catch (std::exception &e)
      {
      std::cerr << "ERROR: " << e.what() << std::endl;
      return 1;
      }

    for (auto &w : warnings)
      std::cerr << "Warning: " << w.what() << std::endl;

    ImageWrapperBase *layer = app->GetMainImage();
    if (!layer)
      {
      std::cerr << "ERROR: main image is null after load" << std::endl;
      return 1;
      }

    std::cout << "Nickname   : " << layer->GetNickname() << std::endl;
    const std::string &remoteURL = layer->GetRemoteURL();
    if (!remoteURL.empty())
      std::cout << "Remote URL : " << remoteURL << std::endl;

    PrintLayerInfo(layer);
    return 0;
    }

  // ── Workspace (-w) ───────────────────────────────────────────────────
  std::cout << "Loading workspace: " << path << std::endl;
  try
    {
    app->OpenWorkspace(path, warnings);
    }
  catch (std::exception &e)
    {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
    }

  for (auto &w : warnings)
    std::cerr << "Warning: " << w.what() << std::endl;

  // Iterate over all image layers in load order
  GenericImageData *imgdata = app->GetCurrentImageData();
  int layerIndex = 0;
  for (LayerIterator it(imgdata); !it.IsAtEnd(); ++it)
    {
    ImageWrapperBase *layer = it.GetLayer();
    LayerRole         role  = it.GetRole();

    std::cout << std::endl;
    std::cout << "Layer " << layerIndex++ << std::endl;
    std::cout << "  Role       : " << RoleName(role) << std::endl;
    std::cout << "  Nickname   : " << layer->GetNickname() << std::endl;

    const std::string &custom = layer->GetCustomNickname();
    if (!custom.empty())
      std::cout << "  Custom nick: " << custom << std::endl;

    std::cout << "  File       : " << layer->GetFileName() << std::endl;

    const std::string &remoteURL = layer->GetRemoteURL();
    if (!remoteURL.empty())
      std::cout << "  Remote URL : " << remoteURL << std::endl;

    PrintLayerInfo(layer);
    }

  if (layerIndex == 0)
    {
    std::cerr << "ERROR: no layers found after workspace load" << std::endl;
    return 1;
    }

  return 0;
}
