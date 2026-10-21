/**
 * TestVTIImageIO.cxx
 *
 * Tests for the VTK XML image (.vti) reader/writer in VTIImageIO:
 *  - an oblique 3D image round-trips through GuidedNativeImageIO with its
 *    origin, spacing, direction and voxel values intact;
 *  - a file written by VTK whose extent does not start at zero gets the
 *    correct ITK origin (the position of its first voxel);
 *  - a file whose point data array is not marked as the active scalars
 *    (as written by some tools) can still be read;
 *  - saving a 4D image fails with an exception instead of silently dropping
 *    all but the first time point.
 *
 * Usage: TestVTIImageIO <temp_dir>
 */

#include "GuidedNativeImageIO.h"
#include "IRISException.h"
#include "Registry.h"

#include "itkImage.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkVectorImage.h"
#include "itksys/SystemTools.hxx"

#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkShortArray.h>
#include <vtkXMLImageDataWriter.h>

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

using NativeImage = itk::VectorImage<short, 4>;

// Read a file with GuidedNativeImageIO and return the native image
static itk::SmartPointer<itk::ImageBase<4>>
ReadNative(const std::string &fn, GuidedNativeImageIO::Pointer &io)
{
  Registry reg;
  io = GuidedNativeImageIO::New();
  io->ReadNativeImage(fn.c_str(), reg);
  return io->GetNativeImage();
}

static void
TestObliqueRoundTrip(const std::string &tempdir)
{
  using ImageType = itk::Image<short, 3>;
  ImageType::Pointer img = ImageType::New();
  ImageType::SizeType size = { { 5, 4, 3 } };
  img->SetRegions(ImageType::RegionType(size));

  ImageType::SpacingType spacing;
  spacing[0] = 0.5; spacing[1] = 0.75; spacing[2] = 1.25;
  img->SetSpacing(spacing);

  ImageType::PointType origin;
  origin[0] = -10.0; origin[1] = 20.5; origin[2] = 3.25;
  img->SetOrigin(origin);

  // Non-symmetric rotation, so a transposed matrix would be detected
  double c = std::cos(0.4), s = std::sin(0.4), c2 = std::cos(0.3), s2 = std::sin(0.3);
  ImageType::DirectionType dir;
  dir[0][0] = c;  dir[0][1] = -s * c2; dir[0][2] = s * s2;
  dir[1][0] = s;  dir[1][1] = c * c2;  dir[1][2] = -c * s2;
  dir[2][0] = 0;  dir[2][1] = s2;      dir[2][2] = c2;
  img->SetDirection(dir);
  img->Allocate();

  int k = 0;
  for (itk::ImageRegionIterator<ImageType> it(img, img->GetBufferedRegion()); !it.IsAtEnd(); ++it, ++k)
    it.Set(static_cast<short>(k * 7 - 100));

  std::string fn = tempdir + "/TestVTIImageIO_oblique.vti";
  Registry reg;
  GuidedNativeImageIO::Pointer io_save = GuidedNativeImageIO::New();
  io_save->SaveImage<ImageType>(fn.c_str(), reg, img);

  GuidedNativeImageIO::Pointer io;
  auto native = ReadNative(fn, io);
  for (unsigned int d = 0; d < 3; d++)
  {
    CHECK(native->GetBufferedRegion().GetSize()[d] == size[d], "[oblique] size[" << d << "]");
    CHECK(std::abs(native->GetSpacing()[d] - spacing[d]) < 1e-6, "[oblique] spacing[" << d << "]");
    CHECK(std::abs(native->GetOrigin()[d] - origin[d]) < 1e-6,
          "[oblique] origin[" << d << "] = " << native->GetOrigin()[d] << ", expected " << origin[d]);
    for (unsigned int e = 0; e < 3; e++)
      CHECK(std::abs(native->GetDirection()[d][e] - dir[d][e]) < 1e-6,
            "[oblique] direction[" << d << "][" << e << "] = " << native->GetDirection()[d][e]
                                   << ", expected " << dir[d][e]);
  }

  auto *nimg = dynamic_cast<NativeImage *>(native.GetPointer());
  CHECK(nimg, "[oblique] unexpected native pixel type " << io->GetComponentTypeAsStringInNativeImage());
  if (nimg)
  {
    itk::ImageRegionConstIterator<ImageType> it_src(img, img->GetBufferedRegion());
    itk::ImageRegionConstIterator<NativeImage> it_dst(nimg, nimg->GetBufferedRegion());
    int n_bad = 0;
    for (; !it_src.IsAtEnd(); ++it_src, ++it_dst)
      if (it_src.Get() != it_dst.Get()[0])
        n_bad++;
    CHECK(n_bad == 0, "[oblique] " << n_bad << " voxels differ after round trip");
  }

  itksys::SystemTools::RemoveFile(fn);
}

// Write a small VTI file with VTK directly. The extent starts at (10, 20, 30)
// and, if as_scalars is false, the data array is not set as active scalars.
static std::string
WriteVTKFile(const std::string &fn, bool as_scalars)
{
  vtkNew<vtkImageData> img;
  img->SetExtent(10, 13, 20, 22, 30, 31);
  img->SetSpacing(0.5, 2.0, 3.0);
  img->SetOrigin(1.0, 2.0, 3.0);

  vtkNew<vtkShortArray> arr;
  arr->SetName("density");
  arr->SetNumberOfTuples(img->GetNumberOfPoints());
  for (vtkIdType i = 0; i < img->GetNumberOfPoints(); i++)
    arr->SetValue(i, static_cast<short>(i));

  if (as_scalars)
    img->GetPointData()->SetScalars(arr);
  else
    img->GetPointData()->AddArray(arr);

  vtkNew<vtkXMLImageDataWriter> writer;
  writer->SetInputData(img);
  writer->SetFileName(fn.c_str());
  writer->Write();
  return fn;
}

static void
TestExtentAndArrays(const std::string &tempdir)
{
  for (bool as_scalars : { true, false })
  {
    std::string tag = as_scalars ? "[extent] " : "[inactive array] ";
    std::string fn = WriteVTKFile(
      tempdir + (as_scalars ? "/TestVTIImageIO_extent.vti" : "/TestVTIImageIO_array.vti"), as_scalars);

    GuidedNativeImageIO::Pointer io;
    itk::SmartPointer<itk::ImageBase<4>> native;
    try
    {
      native = ReadNative(fn, io);
    }
    catch (std::exception &exc)
    {
      CHECK(false, tag << "could not read file: " << exc.what());
      continue;
    }

    // The first voxel is at origin + extent_start .* spacing
    double expected_origin[3] = { 1.0 + 10 * 0.5, 2.0 + 20 * 2.0, 3.0 + 30 * 3.0 };
    unsigned int expected_size[3] = { 4, 3, 2 };
    for (unsigned int d = 0; d < 3; d++)
    {
      CHECK(native->GetBufferedRegion().GetSize()[d] == expected_size[d], tag << "size[" << d << "]");
      CHECK(std::abs(native->GetOrigin()[d] - expected_origin[d]) < 1e-6,
            tag << "origin[" << d << "] = " << native->GetOrigin()[d] << ", expected "
                << expected_origin[d]);
    }

    auto *nimg = dynamic_cast<NativeImage *>(native.GetPointer());
    CHECK(nimg, tag << "unexpected native pixel type");
    if (nimg)
    {
      int i = 0, n_bad = 0;
      for (itk::ImageRegionConstIterator<NativeImage> it(nimg, nimg->GetBufferedRegion()); !it.IsAtEnd();
           ++it, ++i)
        if (it.Get()[0] != i)
          n_bad++;
      CHECK(n_bad == 0, tag << n_bad << " voxels have unexpected values");
    }

    itksys::SystemTools::RemoveFile(fn);
  }
}

static void
Test4DRejected(const std::string &tempdir)
{
  using ImageType = itk::Image<short, 4>;
  ImageType::Pointer img = ImageType::New();
  ImageType::SizeType size = { { 2, 2, 2, 3 } };
  img->SetRegions(ImageType::RegionType(size));
  img->Allocate();
  img->FillBuffer(1);

  std::string fn = tempdir + "/TestVTIImageIO_4d.vti";
  Registry reg;
  GuidedNativeImageIO::Pointer io = GuidedNativeImageIO::New();
  bool threw = false;
  try
  {
    io->SaveImage<ImageType>(fn.c_str(), reg, img);
  }
  catch (std::exception &)
  {
    threw = true;
  }
  CHECK(threw, "[4D] saving a 4D image as .vti did not throw");
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
    TestObliqueRoundTrip(tempdir);
    TestExtentAndArrays(tempdir);
    Test4DRejected(tempdir);
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
  std::cout << "All VTI image IO checks passed" << std::endl;
  return 0;
}
