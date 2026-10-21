// Test: sampling of large 2D images, which are always sliced with the
// non-orthogonal slicer (an image wider than 1024 voxels does not use
// orthogonal slicing, see ImageWrapper::CanOrthogonalSlicingBeUsed).
//
//   1. The main image (large2d_main, 1280 x 640 x 1) is sliced non-orthogonally
//      and samples to its own voxel values.
//   2. Overlays hold 255 minus the main image, so main + overlay = 255 at every
//      voxel. This must hold for overlays whose geometry differs slightly from
//      the main image along z (plane shifted by less than half a voxel, z axis
//      flipped, different slice thickness), as happens with 2D images exported
//      by different programs, and for an overlay at half the resolution.
//   3. Segmentations: one on the main grid, and one covering only part of it
//      (large2d_seg_crop), which is sliced non-orthogonally when it is not the
//      active segmentation. Voxels just outside the cropped one must be 0.
//
// The images are made by Testing/TestData/make_large2d.py; the overlays with
// different z geometry are made here from the main image.
//
// Usage: TestLarge2DSlicing <testdata_dir> <temp_dir>

#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ImageWrapperBase.h"
#include "ImageIODelegates.h"
#include "UIReporterDelegates.h"
#include "IRISException.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImageRegionIterator.h"
#include <itksys/SystemTools.hxx>
#include <iostream>
#include <string>
#include <vector>

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

using ByteImage = itk::Image<unsigned char, 3>;

static int n_failures = 0;

static void Check(bool cond, const std::string &what)
{
  std::cout << (cond ? "  ok:   " : "  FAIL: ") << what << std::endl;
  if(!cond)
    n_failures++;
}

// Value of a layer at a voxel of the reference space (the main image grid)
static double Sample(ImageWrapperBase *layer, int x, int y)
{
  vnl_vector<double> v;
  layer->SampleIntensityAtReferenceIndex({{x, y, 0}}, 0, false, v);
  return v[0];
}

// The voxels where the layers are compared: the corners of the image, and the
// centers of a set of the 128 x 128 blocks of the pattern, and unless only
// centers are requested, the corners of these blocks. (With linear interpolation,
// a lower resolution overlay blends neighboring blocks at the block corners.)
static std::vector<std::array<int, 2>> SamplePoints(int nx, int ny, bool centers_only = false)
{
  std::vector<std::array<int, 2>> pts = {{0, 0}, {nx - 1, 0}, {0, ny - 1}, {nx - 1, ny - 1}};
  for(int by = 0; by < ny / 128; by += 2)
    for(int bx = 0; bx < nx / 128; bx += 3)
      {
      pts.push_back({128 * bx + 64, 128 * by + 64});
      if(!centers_only)
        {
        pts.push_back({128 * bx, 128 * by});
        pts.push_back({128 * bx + 127, 128 * by + 127});
        }
      }
  return pts;
}

// Write 255 - main with the given changes to the geometry
static std::string WriteOverlay(ByteImage *main, const std::string &fn,
                                double z_shift_vox, bool flip_z, double z_spacing)
{
  auto ov = ByteImage::New();
  ov->SetRegions(main->GetBufferedRegion());
  ov->Allocate();
  itk::ImageRegionConstIterator<ByteImage> it_m(main, main->GetBufferedRegion());
  itk::ImageRegionIterator<ByteImage> it_o(ov, ov->GetBufferedRegion());
  for(; !it_m.IsAtEnd(); ++it_m, ++it_o)
    it_o.Set(255 - it_m.Get());

  auto origin = main->GetOrigin();
  auto spacing = main->GetSpacing();
  auto dir = main->GetDirection();
  origin[2] += z_shift_vox * spacing[2];
  if(flip_z)
    dir(2, 2) = -dir(2, 2);
  if(z_spacing > 0)
    spacing[2] = z_spacing;
  ov->SetOrigin(origin);
  ov->SetSpacing(spacing);
  ov->SetDirection(dir);

  auto writer = itk::ImageFileWriter<ByteImage>::New();
  writer->SetInput(ov);
  writer->SetFileName(fn);
  writer->Update();
  return fn;
}

// Check that main + overlay = 255 at the sample points
static void CheckOverlay(IRISApplication *app, const std::string &fn, const std::string &what)
{
  IRISWarningList wl;
  app->OpenImage(fn.c_str(), OVERLAY_ROLE, wl);
  GenericImageData *gid = app->GetCurrentImageData();
  ImageWrapperBase *main = gid->GetMain(), *ov = gid->GetLastOverlay();
  auto sz = main->GetSize();

  Check(!ov->ImageSpaceMatchesReferenceSpace(), what + ": sliced non-orthogonally");

  // Check with nearest neighbor and with linear interpolation (the GUI default)
  for(auto mode : {ImageWrapperBase::NEAREST, ImageWrapperBase::LINEAR})
    {
    main->SetSlicingInterpolationMode(mode);
    ov->SetSlicingInterpolationMode(mode);
    int n_bad = 0;
    std::string first_bad;
    auto pts = SamplePoints(sz[0], sz[1], mode == ImageWrapperBase::LINEAR);
    for(auto &p : pts)
      {
      double m = Sample(main, p[0], p[1]), o = Sample(ov, p[0], p[1]);
      if(m + o != 255.0 && !n_bad++)
        first_bad = "; first at " + std::to_string(p[0]) + "," + std::to_string(p[1]) +
                    ": main " + std::to_string((int) m) + ", overlay " + std::to_string(o);
      }
    if(n_bad)
      first_bad = " (" + std::to_string(n_bad) + " of " + std::to_string(pts.size()) + first_bad + ")";
    std::string mode_name = (mode == ImageWrapperBase::NEAREST) ? "nearest" : "linear";
    Check(n_bad == 0, what + ", " + mode_name + ": main + overlay = 255" + first_bad);
    }
  app->UnloadOverlay(ov);
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

  try
    {
    std::string fn_main = data_dir + "/large2d_main.nii.gz";
    auto reader = itk::ImageFileReader<ByteImage>::New();
    reader->SetFileName(fn_main);
    reader->Update();
    ByteImage *main_img = reader->GetOutput();

    auto app = IRISApplication::New();
    IRISWarningList wl;
    app->OpenImage(fn_main.c_str(), MAIN_ROLE, wl);
    GenericImageData *gid = app->GetCurrentImageData();
    ImageWrapperBase *main = gid->GetMain();
    auto sz = main->GetSize();

    // ── 1. The main image ─────────────────────────────────────────────────
    std::cout << "Main image:" << std::endl;
    Check(sz[0] == 1280 && sz[1] == 640 && sz[2] == 1, "main image is 1280 x 640 x 1");
    Check(!main->ImageSpaceMatchesReferenceSpace(), "main image is sliced non-orthogonally");
    for(auto mode : {ImageWrapperBase::NEAREST, ImageWrapperBase::LINEAR})
      {
      main->SetSlicingInterpolationMode(mode);
      int n_bad = 0;
      for(auto &p : SamplePoints(sz[0], sz[1]))
        if(Sample(main, p[0], p[1]) != main_img->GetPixel({{p[0], p[1], 0}}))
          n_bad++;
      Check(n_bad == 0, std::string("main image samples to its voxel values, ") +
                        (mode == ImageWrapperBase::NEAREST ? "nearest" : "linear"));
      }

    // ── 2. Overlays ───────────────────────────────────────────────────────
    std::cout << "Overlays:" << std::endl;
    CheckOverlay(app, WriteOverlay(main_img, temp_dir + "/l2d_same.nii.gz", 0, false, 0),
                 "same geometry");
    CheckOverlay(app, WriteOverlay(main_img, temp_dir + "/l2d_zp03.nii.gz", 0.3, false, 0),
                 "plane shifted by +0.3 voxel in z");
    CheckOverlay(app, WriteOverlay(main_img, temp_dir + "/l2d_zm045.nii.gz", -0.45, false, 0),
                 "plane shifted by -0.45 voxel in z");
    CheckOverlay(app, WriteOverlay(main_img, temp_dir + "/l2d_zflip.nii.gz", 0, true, 0),
                 "z axis flipped");
    CheckOverlay(app, WriteOverlay(main_img, temp_dir + "/l2d_zsp.nii.gz", 0, false, 0.7),
                 "slice thickness 0.7 instead of 2.0");
    CheckOverlay(app, data_dir + "/large2d_ov_halfres.nii.gz", "half resolution in plane");

    // ── 3. Segmentations ──────────────────────────────────────────────────
    std::cout << "Segmentations:" << std::endl;
    app->OpenImage((data_dir + "/large2d_seg.nii.gz").c_str(), LABEL_ROLE, wl);
    LabelImageWrapper *seg = gid->GetActiveSegmentationLayer();
    Check(Sample(seg, 300, 200) == 1 && Sample(seg, 900, 450) == 2 && Sample(seg, 100, 100) == 0,
          "segmentation on the main grid has its labels");
    Check(Sample(seg, 255, 200) == 0 && Sample(seg, 256, 200) == 1 &&
          Sample(seg, 511, 200) == 1 && Sample(seg, 512, 200) == 0,
          "label 1 rectangle has the right extent in x");

    // The cropped segmentation covers main voxels x = 384..895, y = 160..479. It is
    // sliced non-orthogonally once the full segmentation is active again.
    app->OpenImage((data_dir + "/large2d_seg_crop.nii.gz").c_str(), LABEL_ROLE, wl,
                   nullptr, nullptr, true);
    LabelImageWrapper *crop = gid->GetActiveSegmentationLayer();
    Check(crop != seg, "cropped segmentation is loaded as an additional segmentation");
    gid->SetActiveSegmentationLayer(seg->GetUniqueId());
    Check(gid->GetActiveSegmentationLayer() == seg, "full segmentation is active again");
    Check(!crop->ImageSpaceMatchesReferenceSpace(), "cropped segmentation is sliced non-orthogonally");

    struct Edge { int x, y; double expected; const char *what; };
    std::vector<Edge> edges = {
      {384, 300, 1, "first column"},  {383, 300, 0, "column before the first"},
      {895, 300, 1, "last column"},   {896, 300, 0, "column after the last"},
      {600, 160, 1, "first row"},     {600, 159, 0, "row before the first"},
      {600, 479, 1, "last row"},      {600, 480, 0, "row after the last"}};
    for(auto &e : edges)
      {
      double v = Sample(crop, e.x, e.y);
      Check(v == e.expected, std::string("cropped segmentation, ") + e.what + " (" +
                             std::to_string(e.x) + "," + std::to_string(e.y) + ") = " +
                             std::to_string((int) v));
      }
    }
  catch(std::exception &exc)
    {
    std::cerr << "Exception: " << exc.what() << std::endl;
    return 1;
    }

  if(n_failures)
    {
    std::cerr << n_failures << " check(s) failed" << std::endl;
    return 1;
    }

  std::cout << "All checks passed" << std::endl;
  return 0;
}
