// Test: an image or workspace reopens where the user left it (#157).
//
// The cursor position and the time point of the main image are saved with
// its settings when it is closed, and in a workspace when it is saved. This
// test checks, with a 4D image, that:
//
//   - reopening the image restores the cursor and the time point
//   - with the RestoreCursor preference off, the image opens at its center and
//     at the first time point
//   - a workspace restores them too, and its 4D segmentation is shown at the
//     same time point as the main image
//   - resetting the cursor puts it at the center of the main image, also when
//     the active segmentation (the reference space) has a different grid
//   - a workspace whose active segmentation has a different grid restores
//     the cursor too

#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "GlobalState.h"
#include "DefaultBehaviorSettings.h"
#include "ImageWrapperBase.h"
#include "LabelImageWrapper.h"
#include "LayerIterator.h"
#include "CursorHistoryModel.h"
#include "ImageIODelegates.h"
#include "IRISException.h"
#include "SystemInterface.h"
#include "ColorMap.h"
#include "UIReporterDelegates.h"
#include "itksys/SystemTools.hxx"
#include <iostream>

class DummySystemInfoDelegate : public SystemInfoDelegate
{
public:
  DummySystemInfoDelegate(const char *argv0, std::string dataDir)
    : m_ExecutableName(argv0), m_DataDir(dataDir) {}

  std::string GetApplicationDirectory() override
  { return itksys::SystemTools::GetFilenamePath(m_ExecutableName); }

  std::string GetApplicationFile() override { return m_ExecutableName; }
  std::string GetApplicationPermanentDataLocation() override { return m_DataDir; }
  std::string GetUserDocumentsLocation() override { return m_DataDir; }
  std::string GetTempDirectory() override { return m_DataDir; }
  std::string EncodeServerURL(const std::string &url) override { return url; }

  void LoadResourceAsImage2D(std::string, GrayscaleImage *) override {}
  void LoadResourceAsRegistry(std::string, Registry &) override {}
  void WriteRGBAImage2D(std::string, RGBAImageType *) override {}

protected:
  std::string m_ExecutableName, m_DataDir;
};

class DummyColorMapPresetNameSource : public AbstractColorMapPresetNameSource
{
public:
  std::string GetPresetName(ColorMap::SystemPreset preset, bool) override
  { return "Color Map " + std::to_string(static_cast<int>(preset)); }
};

static int n_failures = 0;

static void check(bool condition, const std::string &what)
{
  std::cout << (condition ? "  ok    " : "  FAIL  ") << what << std::endl;
  if (!condition)
    n_failures++;
}

static std::string str(const Vector3i &v)
{
  return std::to_string(v[0]) + "," + std::to_string(v[1]) + "," + std::to_string(v[2]);
}

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    std::cerr << "Usage: " << argv[0] << " <testdata_dir> <temp_dir>" << std::endl;
    return EXIT_FAILURE;
  }

  std::string datadir = argv[1];
  std::string tmpdir = std::string(argv[2]) + "/CursorRestoreTest";
  itksys::SystemTools::RemoveADirectory(tmpdir);
  itksys::SystemTools::MakeDirectory(tmpdir);

  // The application data (where the settings of each image are kept) go to
  // the temporary directory
  DummySystemInfoDelegate sidel(argv[0], tmpdir);
  SystemInterface::SetSystemInfoDelegate(&sidel);
  DummyColorMapPresetNameSource color_map_source;
  ColorMap::SetColorMapPresetNameSource(&color_map_source);

  try
  {
    std::string fn_img = datadir + "/img4d_11f.nii.gz";
    std::string fn_seg = datadir + "/seg4d_11f.nii.gz";

    SmartPtr<GlobalUIModel> gui = GlobalUIModel::New();
    IRISApplication        *driver = gui->GetDriver();
    DefaultBehaviorSettings *dbs = driver->GetGlobalState()->GetDefaultBehaviorSettings();
    IRISWarningList         wl;

    driver->OpenImage(fn_img.c_str(), MAIN_ROLE, wl);
    Vector3i size = to_int(driver->GetCurrentImageData()->GetMain()->GetSize());
    Vector3i center = driver->GetCursorPosition();
    unsigned int nt = driver->GetNumberOfTimePoints();
    std::cout << "Image " << str(size) << " x " << nt << ", opened at " << str(center)
              << " time point " << driver->GetCursorTimePoint() << std::endl;
    check(nt > 7, "the test image has more than 7 time points");
    check(center == size / 2 && driver->GetCursorTimePoint() == 0,
          "an image without saved settings opens at its center and the first time point");

    std::cout << "Reopening the image" << std::endl;
    Vector3i pos(size[0] / 4, size[1] / 3, size[2] / 4 + 1);
    driver->SetCursorPosition(pos);
    driver->SetCursorTimePoint(7);
    driver->UnloadMainImage();
    driver->OpenImage(fn_img.c_str(), MAIN_ROLE, wl);
    check(driver->GetCursorPosition() == pos,
          "the cursor is restored (" + str(driver->GetCursorPosition()) + ", expected " + str(pos) + ")");
    check(driver->GetCursorTimePoint() == 7,
          "the time point is restored (" + std::to_string(driver->GetCursorTimePoint()) + ")");

    std::cout << "With RestoreCursor off" << std::endl;
    dbs->SetRestoreCursor(false);
    driver->UnloadMainImage();
    driver->OpenImage(fn_img.c_str(), MAIN_ROLE, wl);
    check(driver->GetCursorPosition() == center && driver->GetCursorTimePoint() == 0,
          "the image opens at its center and the first time point");
    dbs->SetRestoreCursor(true);

    std::cout << "Workspace" << std::endl;
    driver->OpenImage(fn_seg.c_str(), LABEL_ROLE, wl);
    Vector3i ws_pos(size[0] / 2 + 2, size[1] / 4, size[2] / 3);
    driver->SetCursorPosition(ws_pos);
    driver->SetCursorTimePoint(5);
    std::string fn_ws = tmpdir + "/restore.itksnap";
    driver->SaveProject(fn_ws);

    // Close it at a different place, so the image's own settings differ from
    // the workspace's
    driver->SetCursorPosition(center);
    driver->SetCursorTimePoint(1);
    driver->UnloadMainImage();

    driver->OpenWorkspace(fn_ws, wl);
    check(driver->GetCursorPosition() == ws_pos,
          "the workspace's cursor is restored (" + str(driver->GetCursorPosition()) + ", expected " + str(ws_pos) + ")");
    check(driver->GetCursorTimePoint() == 5,
          "the workspace's time point is restored (" + std::to_string(driver->GetCursorTimePoint()) + ")");
    LabelImageWrapper *seg = driver->GetSelectedSegmentationLayer();
    check(seg && seg->GetNumberOfTimePoints() == nt && seg->GetTimePointIndex() == 5,
          "its 4D segmentation is shown at the same time point (" +
            std::to_string(seg ? seg->GetTimePointIndex() : -1) + ")");

    std::cout << "Resetting the cursor" << std::endl;
    driver->UnloadMainImage();
    driver->OpenImage((datadir + "/MRIcrop-orig.gipl.gz").c_str(), MAIN_ROLE, wl);
    driver->OpenImage((datadir + "/MRIcrop-seg.gipl.gz").c_str(), LABEL_ROLE, wl);
    driver->OpenImage((datadir + "/MRIcrop-seg-hippoR-04mm.nii.gz").c_str(), LABEL_ROLE, wl,
                      nullptr, nullptr, true);
    ImageWrapperBase *main = driver->GetCurrentImageData()->GetMain();
    auto ref_region = driver->GetCurrentImageData()->GetReferenceSpace()->GetBufferedRegion();
    Vector3i ref_size(ref_region.GetSize()[0], ref_region.GetSize()[1], ref_region.GetSize()[2]);
    check(ref_size != to_int(main->GetSize()),
          "the active segmentation has a grid other than the main image's (" + str(ref_size) + ")");
    driver->SetCursorPosition(Vector3i(1, 2, 3));
    driver->ResetCursorPosition();

    // Map the cursor back to the main image's voxels: it should be at its center
    itk::ContinuousIndex<double, 3> c_ref, c_main;
    for (unsigned int d = 0; d < 3; d++)
      c_ref[d] = driver->GetCursorPosition()[d];
    main->TransformReferenceCIndexToWrappedImageCIndex(c_ref, c_main);
    Vector3i main_center = to_int(main->GetSize()) / 2;
    double dist = 0;
    for (unsigned int d = 0; d < 3; d++)
      dist = std::max(dist, std::abs(c_main[d] - main_center[d]));
    check(dist < 1.0, "the cursor is at the center of the main image (" + std::to_string(dist) +
                        " voxels from it)");

    // The settings of the main image used to be saved with the size of the
    // reference space, so they were not read back when the active segmentation
    // had a different grid
    std::cout << "Workspace with a segmentation on a different grid" << std::endl;
    Vector3i grid_pos = ref_size / 3;
    driver->SetCursorPosition(grid_pos);
    Vector3d grid_ras = driver->GetCurrentImageData()->GetCursorPositionRAS();
    std::string fn_ws2 = tmpdir + "/grid.itksnap";
    driver->SaveProject(fn_ws2);
    driver->ResetCursorPosition();
    driver->UnloadMainImage();

    driver->OpenWorkspace(fn_ws2, wl);
    Vector3d ras = driver->GetCurrentImageData()->GetCursorPositionRAS();
    check(driver->GetCursorPosition() == grid_pos && (ras - grid_ras).inf_norm() < 1e-3,
          "the workspace's cursor is restored (" + str(driver->GetCursorPosition()) +
            ", expected " + str(grid_pos) + ")");
    CursorHistoryModel *chm = gui->GetCursorHistoryModel();
    check(chm->GetNumberOfEntries() == 1 && !chm->CheckState(CursorHistoryModel::UIF_CAN_GO_BACK),
          "the cursor history starts with the restored position (" +
            std::to_string(chm->GetNumberOfEntries()) + " entries)");

    // Selecting a segmentation with a different grid keeps the cursor where it
    // is, so it should not be recorded as a new place either
    unsigned long other_seg = 0;
    for (LayerIterator it = driver->GetCurrentImageData()->GetLayers(LABEL_ROLE); !it.IsAtEnd(); ++it)
      if (it.GetLayer() != driver->GetSelectedSegmentationLayer())
        other_seg = it.GetLayer()->GetUniqueId();
    check(other_seg != 0, "the workspace has another segmentation");
    driver->GetGlobalState()->SetSelectedSegmentationLayerId(other_seg);
    check(chm->GetNumberOfEntries() == 1,
          "selecting it does not add to the cursor history (" +
            std::to_string(chm->GetNumberOfEntries()) + " entries)");
  }
  catch (std::exception &exc)
  {
    std::cerr << "Exception: " << exc.what() << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << (n_failures ? "FAILED: " + std::to_string(n_failures) + " check(s)" : "PASSED") << std::endl;
  return n_failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
