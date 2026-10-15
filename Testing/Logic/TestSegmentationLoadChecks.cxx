// Test: checks performed when a segmentation image is loaded.
//
//   1. Overlap warning: a segmentation that partially overlaps the main image
//      does not produce a "do not overlap" warning (it used to, when neither
//      image's center was inside the other), but one that is far away does.
//   2. Loading a single time point into a 4D workspace: the image replaces the
//      current time point of the selected segmentation, so its size must match
//      that segmentation (error otherwise), and a different header produces a
//      warning that it is ignored.
//   3. Whether loading a segmentation into a 4D workspace can overwrite
//      unsaved changes. The answer depends on the incoming image only when
//      all the changes are in other time points; a remote image is never
//      downloaded for the check (it used to be, and then again to load it).
//
// Usage: TestSegmentationLoadChecks <testdata_dir> <temp_dir>

#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ImageIODelegates.h"
#include "UIReporterDelegates.h"
#include "IRISException.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkExtractImageFilter.h"
#include <itksys/SystemTools.hxx>
#include <iostream>
#include <string>

class SimpleSystemInfoDelegate : public SystemInfoDelegate
{
public:
  SimpleSystemInfoDelegate(const char *argv0, std::string tmp)
    : m_Exe(argv0), m_Temp(tmp) {}

  std::string GetApplicationDirectory() override
    { return itksys::SystemTools::GetFilenamePath(m_Exe); }
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

using LabelImage = itk::Image<unsigned short, 3>;
using LabelImage4D = itk::Image<unsigned short, 4>;

static int n_failures = 0;

static void Check(bool cond, const std::string &what)
{
  std::cout << (cond ? "  ok:   " : "  FAIL: ") << what << std::endl;
  if(!cond)
    n_failures++;
}

static bool HasWarning(const IRISWarningList &wl, const char *text)
{
  for(auto &w : wl)
    if(std::string(w.what()).find(text) != std::string::npos)
      return true;
  return false;
}

template <class TImage>
static typename TImage::Pointer ReadImage(const std::string &fn)
{
  auto reader = itk::ImageFileReader<TImage>::New();
  reader->SetFileName(fn);
  reader->Update();
  return reader->GetOutput();
}

template <class TImage>
static void WriteImage(TImage *img, const std::string &fn)
{
  auto writer = itk::ImageFileWriter<TImage>::New();
  writer->SetInput(img);
  writer->SetFileName(fn);
  writer->Update();
}

// Write a copy of a 3D label image with its origin shifted along x
static std::string WriteShifted(LabelImage *seg, double shift, const std::string &fn)
{
  auto copy = LabelImage::New();
  copy->Graft(seg);
  auto origin = seg->GetOrigin();
  itk::Vector<double, 3> offset;
  offset.Fill(0.0);
  offset[0] = shift;
  copy->SetOrigin(origin + seg->GetDirection() * offset);
  WriteImage<LabelImage>(copy, fn);
  return fn;
}

// Load a segmentation, returning the warnings, or the exception message in 'error'
static IRISWarningList LoadSeg(IRISApplication *app, const std::string &fn, std::string &error)
{
  IRISWarningList wl;
  error.clear();
  try
    {
    app->OpenImage(fn.c_str(), LABEL_ROLE, wl);
    }
  catch(std::exception &exc)
    {
    error = exc.what();
    }
  return wl;
}

int main(int argc, char *argv[])
{
  if(argc < 3)
    {
    std::cerr << "Usage: " << argv[0] << " <testdata_dir> <temp_dir>" << std::endl;
    return 1;
    }

  std::string data_dir = argv[1], temp_dir = argv[2];
  itksys::SystemTools::MakeDirectory(temp_dir);

  SimpleSystemInfoDelegate sidel(argv[0], temp_dir);
  SystemInterface::SetSystemInfoDelegate(&sidel);
  SimpleColorMapSource cmSource;
  ColorMap::SetColorMapPresetNameSource(&cmSource);

  const char *no_overlap = "do not overlap";
  std::string error;

  try
    {
    // ── 1. Overlap warning (MRIcrop: 78 x 110 x 64 voxels of 1mm) ────────
    std::cout << "Overlap check:" << std::endl;
    auto app = IRISApplication::New();
    IRISWarningList wl;
    app->OpenImage((data_dir + "/MRIcrop-orig.gipl.gz").c_str(), MAIN_ROLE, wl);

    auto seg = ReadImage<LabelImage>(data_dir + "/MRIcrop-seg.gipl.gz");

    wl = LoadSeg(app, data_dir + "/MRIcrop-seg.gipl.gz", error);
    Check(error.empty() && !HasWarning(wl, no_overlap), "same grid: no overlap warning");

    // Shifted by 60 voxels: 18 voxels overlap, but neither image's center is
    // inside the other one
    wl = LoadSeg(app, WriteShifted(seg, 60.0, temp_dir + "/TestSegLoad_partial.nii.gz"), error);
    Check(error.empty() && !HasWarning(wl, no_overlap), "partial overlap: no overlap warning");

    wl = LoadSeg(app, WriteShifted(seg, 200.0, temp_dir + "/TestSegLoad_far.nii.gz"), error);
    Check(error.empty() && HasWarning(wl, no_overlap), "no overlap: overlap warning");

    // ── 2. Single time point into a 4D workspace (38 x 35 x 38 x 11) ─────
    std::cout << "Single time point into a 4D workspace:" << std::endl;
    auto app4d = IRISApplication::New();
    app4d->OpenImage((data_dir + "/img4d_11f.nii.gz").c_str(), MAIN_ROLE, wl);

    // Extract one time point of the 4D segmentation as a 3D image
    auto seg4d = ReadImage<LabelImage4D>(data_dir + "/seg4d_11f.nii.gz");
    auto region = seg4d->GetLargestPossibleRegion();
    region.SetSize(3, 0);
    region.SetIndex(3, 5);
    auto extract = itk::ExtractImageFilter<LabelImage4D, LabelImage>::New();
    extract->SetInput(seg4d);
    extract->SetExtractionRegion(region);
    extract->SetDirectionCollapseToSubmatrix();
    extract->Update();
    LabelImage::Pointer tp = extract->GetOutput();
    std::string fn_tp = temp_dir + "/TestSegLoad_tp.nii.gz";
    WriteImage<LabelImage>(tp, fn_tp);

    wl = LoadSeg(app4d, fn_tp, error);
    Check(error.empty() && !HasWarning(wl, "Header Mismatch"), "matching time point: no error or header warning");

    // Same size, different header: loaded with a warning
    auto shifted = LabelImage::New();
    shifted->Graft(tp);
    auto spacing = tp->GetSpacing();
    spacing[0] *= 2.0;
    shifted->SetSpacing(spacing);
    std::string fn_tp_hdr = temp_dir + "/TestSegLoad_tp_header.nii.gz";
    WriteImage<LabelImage>(shifted, fn_tp_hdr);
    wl = LoadSeg(app4d, fn_tp_hdr, error);
    Check(error.empty() && HasWarning(wl, "Header Mismatch"), "different header: header warning");

    // Different size: error that names the mismatched dimensions
    wl = LoadSeg(app4d, data_dir + "/MRIcrop-seg.gipl.gz", error);
    Check(error.find("Mismatched dimensions") != std::string::npos,
          "different size: mismatched dimensions error (got: " + error + ")");

    // ── 3. Can a load overwrite unsaved changes in a 4D workspace? ───────
    std::cout << "Unsaved changes in a 4D workspace:" << std::endl;
    auto appUC = IRISApplication::New();
    appUC->OpenImage((data_dir + "/img4d_11f.nii.gz").c_str(), MAIN_ROLE, wl);
    appUC->OpenImage((data_dir + "/seg4d_11f.nii.gz").c_str(), LABEL_ROLE, wl);
    auto del = LoadSegmentationImageDelegate::New();
    del->Initialize(appUC);
    std::string fn_4d = data_dir + "/seg4d_11f.nii.gz";

    // Nothing listens on port 9, so a download attempt would throw
    std::string url = "http://127.0.0.1:9/seg.nii.gz";
    auto canOverwrite = [&](const std::string &fn) {
      // A new IO object for each file, as the callers use: PeekHeader caches
      auto io = GuidedNativeImageIO::New();
      try { return del->CanLoadOverwriteUnsavedChanges(io, fn) ? 1 : 0; }
      catch(std::exception &exc) { std::cout << "  (" << exc.what() << ")" << std::endl; return -1; }
    };

    appUC->SetCursorTimePoint(5);
    Check(canOverwrite(url) == 0, "no unsaved changes: no prompt, remote image not downloaded");
    Check(canOverwrite(fn_4d) == 0, "no unsaved changes: no prompt for a 4D image");

    // Unsaved changes in time point 2 only, cursor at time point 5
    appUC->SetCursorTimePoint(2);
    size_t n_relabeled = appUC->ReplaceLabel(7, 1);
    Check(n_relabeled > 0 && appUC->GetSelectedSegmentationLayer()->HasUnsavedChanges(2),
          "relabeling time point 2 leaves unsaved changes there");
    appUC->SetCursorTimePoint(5);
    Check(canOverwrite(fn_tp) == 0, "changes in another time point, 3D image: no prompt");
    Check(canOverwrite(fn_4d) == 1, "changes in another time point, 4D image: prompt");
    Check(canOverwrite(url) == 1, "changes in another time point, remote image: prompt without downloading");

    // Unsaved changes in the current time point
    appUC->SetCursorTimePoint(2);
    Check(canOverwrite(fn_tp) == 1, "changes in the current time point, 3D image: prompt");
    Check(canOverwrite(url) == 1, "changes in the current time point, remote image: prompt without downloading");
    }
  catch(std::exception &exc)
    {
    std::cerr << "Exception: " << exc.what() << std::endl;
    return 1;
    }

  if(n_failures)
    {
    std::cout << n_failures << " check(s) failed" << std::endl;
    return 1;
    }

  std::cout << "All checks passed" << std::endl;
  return 0;
}
