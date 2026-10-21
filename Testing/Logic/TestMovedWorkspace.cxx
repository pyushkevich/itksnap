// Test: opening a workspace after it has been copied or moved together with
// its images. A workspace stores absolute image paths and the directory it was
// saved in; when it is opened elsewhere, each path is rebased relative to the
// new location.
//
//   1. GetMovedFilePath: files under the workspace directory, next to it,
//      in a sibling directory whose name starts with the workspace
//      directory's name, missing files, and remote URLs
//   2. Save a workspace, copy the whole tree, open the copy: every layer
//      comes from the copy, also after the original tree is deleted
//   3. itksnap-wt (WorkspaceAPI) finds the same files
//   4. A layer missing from the copy is an error that names the file
//
// Usage: TestMovedWorkspace <testdata_dir> <temp_dir>
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ImageIODelegates.h"
#include "LabelImageWrapper.h"
#include "UIReporterDelegates.h"
#include "WorkspaceAPI.h"
#include "IRISException.h"
#include <itksys/SystemTools.hxx>
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>

using itksys::SystemTools;

class SimpleSystemInfoDelegate : public SystemInfoDelegate
{
public:
  SimpleSystemInfoDelegate(const char *argv0, std::string tmp)
    : m_Exe(argv0), m_Temp(tmp) {}
  std::string GetApplicationDirectory() override
    { return SystemTools::GetFilenamePath(m_Exe); }
  std::string GetApplicationFile() override { return m_Exe; }
  std::string GetApplicationPermanentDataLocation() override { return ".itksnap_test"; }
  std::string GetUserDocumentsLocation() override { return ".itksnap_test"; }
  std::string GetTempDirectory() override { return m_Temp; }
  std::string EncodeServerURL(const std::string &url) override { return url; }
  void LoadResourceAsImage2D(std::string, GrayscaleImage *) override {}
  void LoadResourceAsRegistry(std::string, Registry &) override {}
  void WriteRGBAImage2D(std::string, RGBAImageType *) override {}
private:
  std::string m_Exe, m_Temp;
};

class SimpleColorMapSource : public AbstractColorMapPresetNameSource
{
public:
  std::string GetPresetName(ColorMap::SystemPreset p, bool) override
    { return "Preset " + std::to_string(static_cast<int>(p)); }
};

static int n_failures = 0;

static void Check(bool cond, const std::string &what)
{
  std::cout << (cond ? "  ok:   " : "  FAIL: ") << what << std::endl;
  if(!cond)
    n_failures++;
}

static void CheckEqual(const std::string &got, const std::string &expected, const std::string &what)
{
  Check(got == expected, what + " -> \"" + got + "\"" +
        (got == expected ? "" : " (expected \"" + expected + "\")"));
}

static std::string Moved(std::string orig_dir, std::string crnt_dir, std::string file)
{
  return IRISApplication::GetMovedFilePath(orig_dir, crnt_dir, file);
}

static void CopyFile(const std::string &src, const std::string &dst)
{
  SystemTools::MakeDirectory(SystemTools::GetFilenamePath(dst));
  if(!SystemTools::CopyFileAlways(src, dst))
    throw IRISException("Cannot copy %s to %s", src.c_str(), dst.c_str());
}

static std::vector<std::string> Sorted(std::vector<std::string> v)
{
  std::sort(v.begin(), v.end());
  return v;
}

// Files of the image layers of the open workspace, sorted (the workspace
// stores the layers by role, not in the order they were loaded)
static std::vector<std::string> LayerFiles(IRISApplication *app)
{
  std::vector<std::string> files;
  for(LayerIterator it = app->GetCurrentImageData()->GetLayers(MAIN_ROLE | LABEL_ROLE | OVERLAY_ROLE);
      !it.IsAtEnd(); ++it)
    files.push_back(it.GetLayer()->GetFileName());
  return Sorted(files);
}

static unsigned long CountLabelVoxels(IRISApplication *app)
{
  unsigned long n = 0;
  LabelImageWrapper::ImageType *img = app->GetSelectedSegmentationLayer()->GetModifiableImage();
  itk::ImageRegionConstIterator<LabelImageWrapper::ImageType> it(img, img->GetBufferedRegion());
  for(; !it.IsAtEnd(); ++it)
    if(it.Get())
      n++;
  return n;
}

// The layer files the workspace should load, sorted, given the layers' file
// names relative to the tree root
static std::vector<std::string> Expected(const std::string &root, const std::vector<std::string> &rel)
{
  std::vector<std::string> files;
  for(const auto &r : rel)
    files.push_back(SystemTools::CollapseFullPath(root + "/" + r));
  return Sorted(files);
}

static void CheckLayers(IRISApplication *app, const std::vector<std::string> &expected,
                        const std::string &what)
{
  std::vector<std::string> got = LayerFiles(app);
  Check(got.size() == expected.size(), what + ": " + std::to_string(got.size()) + " layers");
  for(size_t i = 0; i < got.size() && i < expected.size(); i++)
    CheckEqual(got[i], expected[i], what + ": layer " + std::to_string(i));
}

int main(int argc, char *argv[])
{
  if(argc < 3)
    {
    std::cerr << "Usage: " << argv[0] << " <testdata_dir> <temp_dir>" << std::endl;
    return 1;
    }

  std::string data_dir = argv[1];
  std::string temp_dir = SystemTools::CollapseFullPath(std::string(argv[2]) + "/TestMovedWorkspace");
  SystemTools::RemoveADirectory(temp_dir);
  SystemTools::MakeDirectory(temp_dir);
  SimpleSystemInfoDelegate sidel(argv[0], argv[2]);
  SystemInterface::SetSystemInfoDelegate(&sidel);
  SimpleColorMapSource cmSource;
  ColorMap::SetColorMapPresetNameSource(&cmSource);

  // The tree that is saved and then copied:
  //   proj/ws.itksnap             the workspace
  //   proj/images/main.gipl.gz    main image, under the workspace directory
  //   proj/seg/seg.gipl.gz        segmentation, under the workspace directory
  //   proj_data/ovl1.gipl.gz      overlay in a directory whose name starts
  //                               with the workspace directory's name
  //   shared/ovl2.gipl.gz         overlay in a sibling directory
  std::string orig = temp_dir + "/orig", copy = temp_dir + "/copy";
  std::vector<std::string> rel_files = {
    "proj/images/main.gipl.gz", "proj/seg/seg.gipl.gz",
    "proj_data/ovl1.gipl.gz", "shared/ovl2.gipl.gz" };

  try
    {
    CopyFile(data_dir + "/MRIcrop-orig.gipl.gz", orig + "/" + rel_files[0]);
    CopyFile(data_dir + "/MRIcrop-seg.gipl.gz", orig + "/" + rel_files[1]);
    CopyFile(data_dir + "/MRIcrop-orig.gipl.gz", orig + "/" + rel_files[2]);
    CopyFile(data_dir + "/MRIcrop-orig.gipl.gz", orig + "/" + rel_files[3]);

    // ── 1. Rebasing a single path ─────────────────────────────────────────
    std::cout << "GetMovedFilePath:" << std::endl;
    std::string o_proj = orig + "/proj", c_proj = copy + "/proj";
    for(const auto &r : rel_files)
      CopyFile(orig + "/" + r, copy + "/" + r);
    CheckEqual(Moved(o_proj, c_proj, orig + "/proj/images/main.gipl.gz"),
               copy + "/proj/images/main.gipl.gz", "file under the workspace directory");
    CheckEqual(Moved(o_proj, c_proj, orig + "/shared/ovl2.gipl.gz"),
               copy + "/shared/ovl2.gipl.gz", "file in a sibling directory");
    CheckEqual(Moved(o_proj, c_proj, orig + "/proj_data/ovl1.gipl.gz"),
               copy + "/proj_data/ovl1.gipl.gz", "file in a sibling whose name starts with the directory's");
    CheckEqual(Moved(o_proj, c_proj, orig + "/proj/missing.nii.gz"),
               orig + "/proj/missing.nii.gz", "file missing from the copy keeps its path");
    CheckEqual(Moved(o_proj, c_proj, "scp://host/data/img.nii.gz"),
               "scp://host/data/img.nii.gz", "remote layer keeps its URL");
    CheckEqual(Moved("/data/proj", "scp://host/data/proj", "/data/proj/img/a.nii.gz"),
               "scp://host/data/proj/img/a.nii.gz", "layer of a remote workspace becomes a URL");
    SystemTools::RemoveADirectory(copy);

    // ── 2. Save, copy, open ───────────────────────────────────────────────
    std::cout << "Workspace opened from a copy:" << std::endl;
    unsigned long n_label_voxels = 0;
    {
    auto app = IRISApplication::New();
    IRISWarningList wl;
    app->OpenImage((orig + "/" + rel_files[0]).c_str(), MAIN_ROLE, wl);
    app->OpenImage((orig + "/" + rel_files[1]).c_str(), LABEL_ROLE, wl);
    app->OpenImage((orig + "/" + rel_files[2]).c_str(), OVERLAY_ROLE, wl);
    app->OpenImage((orig + "/" + rel_files[3]).c_str(), OVERLAY_ROLE, wl);
    n_label_voxels = CountLabelVoxels(app);
    Check(n_label_voxels > 0, "segmentation has labels");
    app->SaveProject(o_proj + "/ws.itksnap");
    }

    for(const auto &r : rel_files)
      CopyFile(orig + "/" + r, copy + "/" + r);
    CopyFile(o_proj + "/ws.itksnap", c_proj + "/ws.itksnap");

    {
    auto app = IRISApplication::New();
    IRISWarningList wl;
    app->OpenWorkspace(o_proj + "/ws.itksnap", wl);
    CheckLayers(app, Expected(orig, rel_files), "original");
    }

    {
    auto app = IRISApplication::New();
    IRISWarningList wl;
    app->OpenWorkspace(c_proj + "/ws.itksnap", wl);
    CheckLayers(app, Expected(copy, rel_files), "copy, original present");
    }

    SystemTools::RemoveADirectory(orig);
    {
    auto app = IRISApplication::New();
    IRISWarningList wl;
    app->OpenWorkspace(c_proj + "/ws.itksnap", wl);
    CheckLayers(app, Expected(copy, rel_files), "copy, original deleted");
    Check(CountLabelVoxels(app) == n_label_voxels, "segmentation has the same labels");
    }

    // ── 3. itksnap-wt ─────────────────────────────────────────────────────
    std::cout << "WorkspaceAPI:" << std::endl;
    {
    WorkspaceAPI ws;
    ws.ReadFromXMLFile((c_proj + "/ws.itksnap").c_str());
    std::vector<std::string> expected = Expected(copy, rel_files), got;
    for(int i = 0; i < ws.GetNumberOfLayers(); i++)
      got.push_back(ws.GetLayerActualPath(ws.GetLayerFolder(i)));
    got = Sorted(got);
    Check(got.size() == expected.size(), std::to_string(got.size()) + " layers");
    for(size_t i = 0; i < got.size() && i < expected.size(); i++)
      CheckEqual(got[i], expected[i], "layer " + std::to_string(i));
    }

    // ── 4. A layer missing from the copy ──────────────────────────────────
    std::cout << "Missing layer:" << std::endl;
    SystemTools::RemoveFile(copy + "/" + rel_files[2]);
    {
    auto app = IRISApplication::New();
    IRISWarningList wl;
    std::string error;
    try { app->OpenWorkspace(c_proj + "/ws.itksnap", wl); }
    catch(IRISException &exc) { error = exc.what(); }
    Check(!error.empty(), "opening fails: " + error);
    Check(error.find("ovl1.gipl.gz") != std::string::npos, "error names the missing file");
    }
    }
  catch(std::exception &exc)
    {
    std::cerr << "Exception: " << exc.what() << std::endl;
    return 1;
    }

  std::cout << (n_failures ? "FAILED: " : "PASSED: ") << n_failures << " failure(s)" << std::endl;
  return n_failures ? 1 : 0;
}
