// GenericImageData::GetFullExtentImageRegion(): the voxels of the reference
// grid (the active segmentation) that lie in any layer.
//
// The region bounds the cursor, the zoom-to-fit and the cursor spin boxes. It
// is compared with a brute-force answer: the bounding box of all reference
// voxels whose centre is inside some layer's image box. Layouts:
//   1. main image only (reference = blank segmentation on the main grid): the
//      region must be the main image's own region;
//   2. a segmentation 2x finer than the main image and cropped, so that the
//      main image's edges fall exactly on voxel edges of the reference;
//   3. a segmentation at 1.5x spacing with an origin shifted off the grid;
//   4. a segmentation with a different direction matrix from the main image.
//
// Usage: FullExtentRegionTest <scratch dir>

#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ImageIODelegates.h"
#include "LayerIterator.h"
#include "UIReporterDelegates.h"
#include "ColorMap.h"
#include <itkImage.h>
#include <itkImageFileWriter.h>
#include <itksys/SystemTools.hxx>
#include <iostream>
#include <string>
#include <vector>

class SimpleSystemInfoDelegate : public SystemInfoDelegate
{
public:
  SimpleSystemInfoDelegate(const char *argv0) { m_Exe = argv0; }

  std::string GetApplicationDirectory() override
    { return itksys::SystemTools::GetFilenamePath(m_Exe); }
  std::string GetApplicationFile() override { return m_Exe; }
  std::string GetApplicationPermanentDataLocation() override { return ".itksnap_test"; }
  std::string GetUserDocumentsLocation() override { return ".itksnap_test"; }
  std::string GetTempDirectory() override { return m_Temp; }
  std::string EncodeServerURL(const std::string &url) override { return url; }

  typedef SystemInfoDelegate::GrayscaleImage GrayscaleImage;
  typedef SystemInfoDelegate::RGBAImageType  RGBAImageType;
  void LoadResourceAsImage2D(std::string, GrayscaleImage *) override {}
  void LoadResourceAsRegistry(std::string, Registry &) override {}
  void WriteRGBAImage2D(std::string, RGBAImageType *) override {}

  std::string m_Temp;

private:
  std::string m_Exe;
};

class SimpleColorMapSource : public AbstractColorMapPresetNameSource
{
public:
  std::string GetPresetName(ColorMap::SystemPreset p, bool) override
    { return "Preset " + std::to_string(static_cast<int>(p)); }
};

typedef itk::ImageRegion<3> RegionType;

// Write an image of the given geometry (all zeros) and return its filename
template <class TPixel>
static std::string WriteImage(const std::string &fn,
                              const itk::Size<3> &size,
                              const itk::Vector<double, 3> &spacing,
                              const itk::Point<double, 3> &origin,
                              const itk::Matrix<double, 3, 3> &direction)
{
  typedef itk::Image<TPixel, 3> ImageType;
  auto img = ImageType::New();
  img->SetRegions(RegionType(size));
  img->SetSpacing(spacing);
  img->SetOrigin(origin);
  img->SetDirection(direction);
  img->Allocate(true);

  auto writer = itk::ImageFileWriter<ImageType>::New();
  writer->SetInput(img);
  writer->SetFileName(fn);
  writer->Update();
  return fn;
}

// Brute force: bounding box of the reference voxels whose centre lies inside
// the image box [index - 0.5, index + size - 0.5] of some layer. None of the
// layers here has a registration transform.
static RegionType CoveredRegion(GenericImageData *id)
{
  const double eps = 1e-6;
  const long margin = 48;

  auto *ref = id->GetReferenceSpaceWrapper()->GetImageBase();
  auto ref_size = ref->GetLargestPossibleRegion().GetSize();

  std::vector<itk::ImageBase<3> *> layers;
  for (LayerIterator it(id); !it.IsAtEnd(); ++it)
    layers.push_back(it.GetLayer()->GetImageBase());

  itk::Index<3> lo, hi, k;
  bool any = false;
  for (k[2] = -margin; k[2] < (long)ref_size[2] + margin; ++k[2])
    for (k[1] = -margin; k[1] < (long)ref_size[1] + margin; ++k[1])
      for (k[0] = -margin; k[0] < (long)ref_size[0] + margin; ++k[0])
        {
        itk::Point<double, 3> p;
        ref->TransformIndexToPhysicalPoint(k, p);

        bool inside = false;
        for (auto *img : layers)
          {
          auto ci = img->TransformPhysicalPointToContinuousIndex<double>(p);
          auto rgn = img->GetLargestPossibleRegion();
          bool in = true;
          for (unsigned int d = 0; d < 3; d++)
            if (ci[d] < rgn.GetIndex()[d] - 0.5 - eps ||
                ci[d] > rgn.GetIndex()[d] + (double)rgn.GetSize()[d] - 0.5 + eps)
              in = false;
          if (in)
            {
            inside = true;
            break;
            }
          }

        if (inside)
          {
          for (unsigned int d = 0; d < 3; d++)
            {
            lo[d] = any ? std::min(lo[d], k[d]) : k[d];
            hi[d] = any ? std::max(hi[d], k[d]) : k[d];
            }
          any = true;
          }
        }

  RegionType r;
  r.SetIndex(lo);
  r.SetUpperIndex(hi);
  return r;
}

static int g_Failures = 0;

static void Check(const std::string &layout, IRISApplication *app)
{
  GenericImageData *id = app->GetCurrentImageData();
  RegionType actual = id->GetFullExtentImageRegion();
  RegionType expected = CoveredRegion(id);

  bool ok = (actual == expected);
  std::cout << (ok ? "ok   " : "FAIL ") << layout << ": index " << actual.GetIndex()
            << " upper " << actual.GetUpperIndex();
  if (!ok)
    {
    std::cout << ", expected index " << expected.GetIndex() << " upper "
              << expected.GetUpperIndex();
    ++g_Failures;
    }
  std::cout << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc < 2)
    {
    std::cerr << "Usage: " << argv[0] << " <scratch dir>" << std::endl;
    return 1;
    }

  std::string tempdir = argv[1];
  itksys::SystemTools::MakeDirectory(tempdir);

  SimpleSystemInfoDelegate sidel(argv[0]);
  sidel.m_Temp = tempdir;
  SystemInterface::SetSystemInfoDelegate(&sidel);

  SimpleColorMapSource cmSource;
  ColorMap::SetColorMapPresetNameSource(&cmSource);

  // Main image: 10x12x14 voxels, anisotropic, with an LPS-style flip
  itk::Size<3> main_size = {{10, 12, 14}};
  itk::Vector<double, 3> main_spacing;
  main_spacing[0] = 1.0; main_spacing[1] = 1.5; main_spacing[2] = 2.0;
  itk::Point<double, 3> main_origin;
  main_origin[0] = 3.0; main_origin[1] = -7.0; main_origin[2] = 11.0;
  itk::Matrix<double, 3, 3> flip;
  flip.SetIdentity();
  flip(0, 0) = -1.0; flip(1, 1) = -1.0;
  itk::Matrix<double, 3, 3> identity;
  identity.SetIdentity();

  // Physical position of a continuous index of the main image
  auto main_point = [&](double i, double j, double k) {
    itk::Point<double, 3> p;
    for (unsigned int d = 0; d < 3; d++)
      p[d] = main_origin[d] + flip(d, 0) * i * main_spacing[0]
                            + flip(d, 1) * j * main_spacing[1]
                            + flip(d, 2) * k * main_spacing[2];
    return p;
  };

  std::string fn_main = WriteImage<float>(tempdir + "/FullExtentRegionTest_main.nii.gz",
                                          main_size, main_spacing, main_origin, flip);

  // Segmentation A: 2x finer, first voxel centre at main index (1.75, 2.75, 3.75),
  // so every main voxel edge is a voxel edge of A
  itk::Size<3> a_size = {{8, 10, 12}};
  std::string fn_a = WriteImage<unsigned short>(
    tempdir + "/FullExtentRegionTest_segA.nii.gz", a_size,
    main_spacing * 0.5, main_point(1.75, 2.75, 3.75), flip);

  // Segmentation B: 1.5x spacing, origin off the main grid
  itk::Size<3> b_size = {{5, 6, 7}};
  std::string fn_b = WriteImage<unsigned short>(
    tempdir + "/FullExtentRegionTest_segB.nii.gz", b_size,
    main_spacing * 1.5, main_point(2.3, 1.6, 4.1), flip);

  // Segmentation C: identity direction, 0.8x spacing, starting inside the main
  // image. Its axes run against the main image's in x and y.
  itk::Size<3> c_size = {{9, 9, 9}};
  std::string fn_c = WriteImage<unsigned short>(
    tempdir + "/FullExtentRegionTest_segC.nii.gz", c_size,
    main_spacing * 0.8, main_point(7.2, 8.4, 2.6), identity);

  try
    {
    IRISWarningList wl;
    auto app = IRISApplication::New();

    app->OpenImage(fn_main.c_str(), MAIN_ROLE, wl);
    Check("1 main image only", app);

    app->OpenImage(fn_a.c_str(), LABEL_ROLE, wl);
    Check("2 segmentation 2x finer, edges aligned", app);

    app->OpenImage(fn_b.c_str(), LABEL_ROLE, wl, nullptr, nullptr, true);
    Check("3 segmentation 1.5x, shifted", app);

    app->OpenImage(fn_c.c_str(), LABEL_ROLE, wl, nullptr, nullptr, true);
    Check("4 segmentation with another direction", app);
    }
  catch (std::exception &exc)
    {
    std::cerr << "FAIL: exception: " << exc.what() << std::endl;
    return 1;
    }

  if (g_Failures)
    {
    std::cerr << g_Failures << " layout(s) failed" << std::endl;
    return 1;
    }

  std::cout << "All layouts passed" << std::endl;
  return 0;
}
