/**
 * TestNrrdSequenceIO.cxx
 *
 * Round-trip test for the NRRD sequence (.seq.nrrd) writer in
 * GuidedNativeImageIO::SaveNrrdSequence(). A small 4D image with an oblique,
 * non-symmetric direction matrix is saved and read back, and its geometry and
 * voxel values are compared. This catches transposed space directions and
 * wrong NRRD type keywords (e.g. for plain char). Saving a multi-component
 * image must fail with an IRISException instead of writing truncated data.
 *
 * Usage: TestNrrdSequenceIO <temp_dir>
 */

#include "GuidedNativeImageIO.h"
#include "IRISException.h"
#include "Registry.h"

#include "itkImage.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkVectorImage.h"
#include "itksys/SystemTools.hxx"

#include <cmath>
#include <iostream>
#include <string>

static int g_Failures = 0;

#define CHECK(cond, msg)                                                       \
  if (!(cond))                                                                 \
  {                                                                            \
    std::cerr << "FAILED: " << msg << std::endl;                               \
    ++g_Failures;                                                              \
  }

// A rotation about an oblique axis, so that the direction matrix is not
// symmetric and a transposed matrix would be detected
static itk::Matrix<double, 4, 4>
MakeObliqueDirection()
{
  double c = std::cos(0.4), s = std::sin(0.4);
  double c2 = std::cos(0.3), s2 = std::sin(0.3);
  itk::Matrix<double, 3, 3> rz, rx;
  rz.SetIdentity();
  rz[0][0] = c;  rz[0][1] = -s;
  rz[1][0] = s;  rz[1][1] = c;
  rx.SetIdentity();
  rx[1][1] = c2; rx[1][2] = -s2;
  rx[2][1] = s2; rx[2][2] = c2;
  itk::Matrix<double, 3, 3> r = rz * rx;

  itk::Matrix<double, 4, 4> dir;
  dir.SetIdentity();
  for (unsigned int i = 0; i < 3; i++)
    for (unsigned int j = 0; j < 3; j++)
      dir[i][j] = r[i][j];
  return dir;
}

// TNative is the pixel type GuidedNativeImageIO reads the file back as, which
// for plain char data is signed char
template <typename TPixel, typename TNative = TPixel>
static void
TestRoundTrip(const std::string &tempdir, const char *type_name)
{
  using ImageType = itk::Image<TPixel, 4>;
  typename ImageType::Pointer img = ImageType::New();

  typename ImageType::SizeType size = { { 5, 4, 3, 2 } };
  img->SetRegions(typename ImageType::RegionType(size));
  typename ImageType::SpacingType spacing;
  spacing[0] = 0.5; spacing[1] = 0.75; spacing[2] = 1.25; spacing[3] = 1.0;
  img->SetSpacing(spacing);
  typename ImageType::PointType origin;
  origin[0] = -10.0; origin[1] = 20.5; origin[2] = 3.25; origin[3] = 0.0;
  img->SetOrigin(origin);
  img->SetDirection(MakeObliqueDirection());
  img->Allocate();

  // Distinct, sign-carrying values so that wrong type or ordering shows up
  int k = 0;
  for (itk::ImageRegionIterator<ImageType> it(img, img->GetBufferedRegion()); !it.IsAtEnd();
       ++it, ++k)
    it.Set(static_cast<TPixel>((k % 100) - 50));

  std::string fn = tempdir + "/TestNrrdSequenceIO_" + type_name + ".seq.nrrd";
  Registry    reg_save;
  GuidedNativeImageIO::Pointer io_save = GuidedNativeImageIO::New();
  io_save->SaveImage<ImageType>(fn.c_str(), reg_save, img);

  // Read the file back
  Registry    reg_load;
  GuidedNativeImageIO::Pointer io_load = GuidedNativeImageIO::New();
  io_load->ReadNativeImage(fn.c_str(), reg_load);
  auto *native = io_load->GetNativeImage();

  std::string tag = std::string("[") + type_name + "] ";
  CHECK(io_load->GetNumberOfComponentsInNativeImage() == 1,
        tag << "expected 1 component, got " << io_load->GetNumberOfComponentsInNativeImage());

  auto rsize = native->GetBufferedRegion().GetSize();
  for (unsigned int d = 0; d < 4; d++)
    CHECK(rsize[d] == size[d], tag << "size[" << d << "] = " << rsize[d] << ", expected " << size[d]);

  for (unsigned int d = 0; d < 3; d++)
  {
    CHECK(std::abs(native->GetSpacing()[d] - spacing[d]) < 1e-6,
          tag << "spacing[" << d << "] = " << native->GetSpacing()[d] << ", expected " << spacing[d]);
    CHECK(std::abs(native->GetOrigin()[d] - origin[d]) < 1e-6,
          tag << "origin[" << d << "] = " << native->GetOrigin()[d] << ", expected " << origin[d]);
    for (unsigned int e = 0; e < 3; e++)
      CHECK(std::abs(native->GetDirection()[d][e] - img->GetDirection()[d][e]) < 1e-6,
            tag << "direction[" << d << "][" << e << "] = " << native->GetDirection()[d][e]
                << ", expected " << img->GetDirection()[d][e]);
  }

  // Compare voxel values; the native image is a VectorImage of the file's type
  using NativeType = itk::VectorImage<TNative, 4>;
  auto *nimg = dynamic_cast<NativeType *>(native);
  CHECK(nimg, tag << "native image has unexpected pixel type " << io_load->GetComponentTypeAsStringInNativeImage());
  if (nimg)
  {
    itk::ImageRegionConstIterator<ImageType> it_src(img, img->GetBufferedRegion());
    itk::ImageRegionConstIterator<NativeType> it_dst(nimg, nimg->GetBufferedRegion());
    int n_bad = 0;
    for (; !it_src.IsAtEnd(); ++it_src, ++it_dst)
      if (static_cast<TNative>(it_src.Get()) != it_dst.Get()[0])
        n_bad++;
    CHECK(n_bad == 0, tag << n_bad << " voxels differ after round trip");
  }

  itksys::SystemTools::RemoveFile(fn);
}

static void
TestMultiComponentRejected(const std::string &tempdir)
{
  using ImageType = itk::VectorImage<short, 4>;
  ImageType::Pointer img = ImageType::New();
  ImageType::SizeType size = { { 2, 2, 2, 2 } };
  img->SetRegions(ImageType::RegionType(size));
  img->SetNumberOfComponentsPerPixel(3);
  img->Allocate();

  std::string fn = tempdir + "/TestNrrdSequenceIO_vector.seq.nrrd";
  Registry    reg;
  GuidedNativeImageIO::Pointer io = GuidedNativeImageIO::New();
  bool threw = false;
  try
  {
    io->SaveImage<ImageType>(fn.c_str(), reg, img);
  }
  catch (IRISException &)
  {
    threw = true;
  }
  CHECK(threw, "[vector] saving a multi-component image did not throw");
  itksys::SystemTools::RemoveFile(fn);
}

int
main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " <temp_dir>" << std::endl;
    return 1;
  }
  std::string tempdir = argv[1];
  itksys::SystemTools::MakeDirectory(tempdir);

  try
  {
    TestRoundTrip<char, signed char>(tempdir, "char");
    TestRoundTrip<short>(tempdir, "short");
    TestRoundTrip<float>(tempdir, "float");
    TestMultiComponentRejected(tempdir);
  }
  catch (std::exception &exc)
  {
    std::cerr << "FAILED with exception: " << exc.what() << std::endl;
    return 1;
  }

  if (g_Failures)
  {
    std::cerr << g_Failures << " check(s) failed" << std::endl;
    return 1;
  }
  std::cout << "All NRRD sequence IO checks passed" << std::endl;
  return 0;
}
