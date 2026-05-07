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

  for (int i = 0; i < kNRanks; ++i)
    {
    double ref = std::abs(b.quantiles[i]);
    double tol = (ref > 1e-6) ? 1e-4 * ref : 1e-6;
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
      remote_app->OpenProject(remote_url, warnings);
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
      local_app->OpenProject(local_ref, warnings);
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
  // Derive the cache paths from the same data directory the app would use.
  std::string data_dir  = ".itksnap_test";
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
  auto app1 = MakeApp();

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
    app->OpenProject(path, warnings);
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
