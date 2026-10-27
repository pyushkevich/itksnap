/**
 * TestDicomSeriesDetection.cxx
 *
 * Loading one series from a directory that holds several DICOM series (#218).
 * A CT exam from GE or Siemens used to be treated as a 4D CTA series as a
 * whole, based on the first file, so the series could not be chosen and the
 * first one was always loaded. Now every file in the directory is guessed as
 * a DICOM series directory, and only a series with several images at each
 * position is read as 4D CTA.
 *
 * The test exam (Testing/TestData/dicom_ct_exam, see make_dicom_ct_exam.py)
 * has two 3D series and one 4D CTA series. Voxel intensity encodes the series,
 * slice and frame, so reading the wrong series, slice or frame order shows up.
 *
 * Usage: TestDicomSeriesDetection <testdata_dir>
 */

#include "GuidedNativeImageIO.h"
#include "IRISException.h"
#include "Registry.h"

#include "itkVectorImage.h"
#include "itksys/Directory.hxx"

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

// The series in the test exam
struct SeriesSpec
{
  const char *desc;
  unsigned int n_slices, n_frames;
  bool         is_4dcta;
  const char  *dimensions;
  int          base; // intensity is base + 100 * frame + slice
};

static const SeriesSpec g_Series[] = { { "Axial 3D A", 8, 1, false, "16 x 16 x 8", 1000 },
                                       { "Axial 3D B", 6, 1, false, "16 x 16 x 6", 2000 },
                                       { "Dynamic CTA", 5, 3, true, "16 x 16 x 5 x 3", 0 } };

static void
TestGuessFormat(const std::string &dir)
{
  itksys::Directory d;
  d.Load(dir);
  int n_files = 0;
  for (unsigned long i = 0; i < d.GetNumberOfFiles(); i++)
  {
    std::string fn = d.GetFile(i);
    if (fn.size() < 4 || fn.substr(fn.size() - 4) != ".dcm")
      continue;
    n_files++;
    auto fmt = GuidedNativeImageIO::GuessFormatForFileName(dir + "/" + fn, true);
    CHECK(fmt == GuidedNativeImageIO::FORMAT_DICOM_DIR,
          fn << " guessed as format " << fmt << ", expected DICOM series directory");
  }
  CHECK(n_files == 29, "found " << n_files << " DICOM files, expected 29");
}

// Find the series ID for a description in the parse result
static std::string
FindSeries(const GuidedNativeImageIO::DicomDirectoryParseResult &pr, const std::string &desc)
{
  for (auto &it : pr.SeriesMap)
  {
    Registry r = it.second.MetaData;
    std::string d = r["SeriesDescription"][""];
    if (d.substr(0, desc.size()) == desc)
      return it.first;
  }
  return std::string();
}

static void
TestParse(const std::string &dir)
{
  GuidedNativeImageIO::Pointer io = GuidedNativeImageIO::New();
  io->ParseDicomDirectory(dir);
  auto pr = io->GetLastDicomParseResult();
  CHECK(pr.SeriesMap.size() == 3, "found " << pr.SeriesMap.size() << " series, expected 3");

  for (auto &s : g_Series)
  {
    std::string id = FindSeries(pr, s.desc);
    CHECK(id.size(), "series '" << s.desc << "' not found");
    if (id.empty())
      continue;

    Registry r = pr.SeriesMap[id].MetaData;
    bool     is4d = r["Is4DCTA"][false];
    int      n_images = r["NumberOfImages"][0];
    CHECK(is4d == s.is_4dcta, "series '" << s.desc << "': Is4DCTA = " << is4d);
    CHECK(n_images == (int)(s.n_slices * s.n_frames),
          "series '" << s.desc << "': " << n_images << " images");
    CHECK(r["Dimensions"][""] == std::string(s.dimensions),
          "series '" << s.desc << "': dimensions '" << r["Dimensions"][""] << "', expected '"
                     << s.dimensions << "'");
  }
}

// Load each series, starting from a file of another series, as the GUI does
// when a file is dropped and then a series is picked from the list
static void
TestLoad(const std::string &dir)
{
  GuidedNativeImageIO::Pointer parser = GuidedNativeImageIO::New();
  parser->ParseDicomDirectory(dir);
  auto pr = parser->GetLastDicomParseResult();

  for (auto &s : g_Series)
  {
    std::string id = FindSeries(pr, s.desc);
    if (id.empty())
      continue;

    std::string tag = std::string("[") + s.desc + "] ";
    Registry    hints;
    hints["DICOM.SeriesId"] << id;

    GuidedNativeImageIO::Pointer io = GuidedNativeImageIO::New();
    try
    {
      io->ReadNativeImage((dir + "/s2_0001.dcm").c_str(), hints);
    }
    catch (std::exception &exc)
    {
      CHECK(false, tag << "exception while loading: " << exc.what());
      continue;
    }

    auto *native = io->GetNativeImage();
    auto  size = native->GetBufferedRegion().GetSize();
    CHECK(size[0] == 16 && size[1] == 16 && size[2] == s.n_slices && size[3] == s.n_frames,
          tag << "size " << size << ", expected 16 x 16 x " << s.n_slices << " x " << s.n_frames);

    using NativeType = itk::VectorImage<short, 4>;
    auto *nimg = dynamic_cast<NativeType *>(native);
    CHECK(nimg, tag << "native image has unexpected pixel type "
                    << io->GetComponentTypeAsStringInNativeImage());
    if (!nimg || size[2] != s.n_slices || size[3] != s.n_frames)
      continue;

    // The slice number is taken from the position (2mm apart), since the 4D
    // CTA reader stores the slices in the opposite order, with a flipped axis
    int n_bad = 0;
    for (unsigned int t = 0; t < s.n_frames; t++)
      for (unsigned int z = 0; z < s.n_slices; z++)
      {
        NativeType::IndexType idx = { { 8, 8, (long)z, (long)t } };
        NativeType::PointType pt;
        nimg->TransformIndexToPhysicalPoint(idx, pt);
        int expected = s.base + 100 * t + (int)std::lround(pt[2] / 2.0);
        int actual = nimg->GetPixel(idx)[0];
        if (actual != expected)
        {
          if (n_bad++ == 0)
            std::cerr << tag << "slice " << z << " frame " << t << ": " << actual << ", expected "
                      << expected << std::endl;
        }
      }
    CHECK(n_bad == 0, tag << n_bad << " slices have the wrong intensity");
  }
}

int
main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " <testdata_dir>" << std::endl;
    return 1;
  }

  std::string dir = std::string(argv[1]) + "/dicom_ct_exam";

  TestGuessFormat(dir);
  TestParse(dir);
  TestLoad(dir);

  if (g_Failures)
  {
    std::cerr << g_Failures << " check(s) failed" << std::endl;
    return 1;
  }
  std::cout << "All checks passed" << std::endl;
  return 0;
}
