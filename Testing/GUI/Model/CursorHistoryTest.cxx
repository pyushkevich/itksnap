// Test for the cursor history model used by the back, forward and reset
// buttons. The model is given a fake clock, so that we can control the time
// between cursor changes without waiting.
//
// We check that a drag and a quick scroll each make a single entry, that a
// pause or a new click starts a new entry, that back and forward work like in
// a browser, that the time point counts as a coordinate but 4D replay does
// not fill the history, and that the history starts over with a new image.

#include "GlobalUIModel.h"
#include "CursorHistoryModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "GlobalState.h"
#include "ImageWrapperBase.h"
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

  std::string tmpdir = std::string(argv[2]) + "/CursorHistoryTest";
  itksys::SystemTools::RemoveADirectory(tmpdir);
  itksys::SystemTools::MakeDirectory(tmpdir);
  DummySystemInfoDelegate sidel(argv[0], tmpdir);
  SystemInterface::SetSystemInfoDelegate(&sidel);
  DummyColorMapPresetNameSource color_map_source;
  ColorMap::SetColorMapPresetNameSource(&color_map_source);

  try
  {
    SmartPtr<GlobalUIModel> gui = GlobalUIModel::New();
    IRISApplication        *driver = gui->GetDriver();
    IRISWarningList         wl;

    SmartPtr<CursorHistoryModel> hist = CursorHistoryModel::New();
    hist->SetParentModel(gui);
    double now = 1000.0;
    hist->SetClock([&now]() { return now; });

    // A 4D image (38 x 35 x 38 x 11)
    std::string fn_img = std::string(argv[1]) + "/img4d_11f.nii.gz";
    driver->OpenImage(fn_img.c_str(), MAIN_ROLE, wl);
    Vector3i center = driver->GetCursorPosition();
    using CHM = CursorHistoryModel;

    std::cout << "Opening" << std::endl;
    check(hist->GetNumberOfEntries() == 1, "the history starts with the place the image opened at");
    check(!hist->CheckState(CHM::UIF_CAN_GO_BACK) && !hist->CheckState(CHM::UIF_CAN_GO_FORWARD),
          "there is nowhere to go back or forward to");

    // The drag kinds, as passed by the crosshairs interaction mode
    const auto START = IRISApplication::CURSOR_DRAG_START;
    const auto DRAG = IRISApplication::CURSOR_DRAG;
    const auto END = IRISApplication::CURSOR_DRAG_END;

    std::cout << "A drag" << std::endl;
    now += 5;
    driver->SetCursorPosition(Vector3i(5, 10, 10), false, START);
    for (int i = 1; i <= 10; i++)
    {
      now += 0.02;
      driver->SetCursorPosition(Vector3i(5 + i, 10, 10), false, DRAG);
    }
    now += 3;   // holding the button still, then moving on, is the same drag
    driver->SetCursorPosition(Vector3i(16, 10, 10), false, DRAG);
    driver->SetCursorPosition(Vector3i(16, 10, 10), false, END);
    check(hist->GetNumberOfEntries() == 2, "a drag makes one place (" +
                                             std::to_string(hist->GetNumberOfEntries()) + " in all)");

    std::cout << "Clicks and scrolling" << std::endl;
    now += 0.2;   // a click right after the drag is a new place
    driver->SetCursorPosition(Vector3i(20, 20, 20), false, START);
    driver->SetCursorPosition(Vector3i(20, 20, 20), false, END);
    check(hist->GetNumberOfEntries() == 3, "a new click is a new place, even right away");

    now += 2;
    for (int z = 21; z <= 30; z++)
    {
      now += 0.1;
      driver->SetCursorPosition(Vector3i(20, 20, z));
    }
    check(hist->GetNumberOfEntries() == 4, "a fast scroll through 10 slices makes one place");
    now += 1.5;
    driver->SetCursorPosition(Vector3i(20, 20, 31));
    check(hist->GetNumberOfEntries() == 5, "scrolling on after a pause of 1.5 s is a new place");

    std::cout << "Time point" << std::endl;
    now += 2;
    driver->SetCursorTimePoint(3);
    check(hist->GetNumberOfEntries() == 6, "a time point change after a pause is a place");
    now += 2;
    driver->GetGlobalState()->Set4DReplay(true);
    for (int t = 4; t < 11; t++)
    {
      now += 1.5;   // slower than the pause: would make a place per frame
      driver->SetCursorTimePoint(t);
    }
    driver->GetGlobalState()->Set4DReplay(false);
    check(hist->GetNumberOfEntries() == 7, "4D replay is one place, where it stopped (" +
                                             std::to_string(hist->GetNumberOfEntries()) + " in all)");

    std::cout << "Back and Forward" << std::endl;
    hist->GoBack();
    check(driver->GetCursorTimePoint() == 3 && driver->GetCursorPosition() == Vector3i(20, 20, 31),
          "Back goes to the place before the replay (" + str(driver->GetCursorPosition()) + ", t=" +
            std::to_string(driver->GetCursorTimePoint()) + ")");
    hist->GoBack();
    check(driver->GetCursorTimePoint() == 0 && driver->GetCursorPosition() == Vector3i(20, 20, 31),
          "Back again goes to the place before the time point change");
    hist->GoBack();
    hist->GoBack();
    check(driver->GetCursorPosition() == Vector3i(20, 20, 20), "Back twice more goes to the click");
    check(hist->CheckState(CHM::UIF_CAN_GO_FORWARD), "Forward is available");
    hist->GoForward();
    check(driver->GetCursorPosition() == Vector3i(20, 20, 30), "Forward goes to the end of the scroll");
    check(hist->GetNumberOfEntries() == 7, "going Back and Forward adds no places");

    now += 2;
    driver->SetCursorPosition(Vector3i(1, 2, 3));
    check(hist->GetNumberOfEntries() == 5 && !hist->CheckState(CHM::UIF_CAN_GO_FORWARD),
          "moving after Back drops the places ahead (" + std::to_string(hist->GetNumberOfEntries()) +
            " places)");

    std::cout << "Reset" << std::endl;
    now += 0.1;   // right after a move, Reset is still a new place
    hist->Reset();
    check(driver->GetCursorPosition() == center, "Reset goes to the center (" +
                                                    str(driver->GetCursorPosition()) + ")");
    check(hist->GetNumberOfEntries() == 6, "Reset is a new place");
    hist->GoBack();
    check(driver->GetCursorPosition() == Vector3i(1, 2, 3), "Back returns from Reset");

    std::cout << "Clicks, scrolling and jumps" << std::endl;
    now += 2;
    driver->SetCursorPosition(Vector3i(25, 25, 25), false, START);
    driver->SetCursorPosition(Vector3i(25, 25, 25), false, END);
    unsigned int n = hist->GetNumberOfEntries();
    now += 0.1;   // scrolling right after a click is not part of the click
    driver->SetCursorPosition(Vector3i(25, 25, 26));
    now += 0.1;
    driver->SetCursorPosition(Vector3i(25, 25, 27));
    check(hist->GetNumberOfEntries() == n + 1, "scrolling right after a click is one new place");
    now += 0.1;   // a jump, like a click in the 3D view, is a new place right away
    driver->SetCursorPosition(Vector3i(3, 3, 3), false, IRISApplication::CURSOR_JUMP);
    check(hist->GetNumberOfEntries() == n + 2, "a jump is a new place, even right after a move");
    now += 0.1;   // and a move right after a jump is a new place too
    driver->SetCursorPosition(Vector3i(3, 3, 4));
    check(hist->GetNumberOfEntries() == n + 3, "a move right after a jump is a new place");
    hist->GoBack();
    hist->GoBack();
    check(driver->GetCursorPosition() == Vector3i(25, 25, 27), "Back twice returns to the end of the scroll");

    std::cout << "Size limit" << std::endl;
    for (int i = 0; i < 80; i++)
    {
      now += 2;
      driver->SetCursorPosition(Vector3i(i % 30, (i / 30) + 1, 5));
    }
    check(hist->GetNumberOfEntries() == CHM::MAX_ENTRIES,
          "the history keeps the last " + std::to_string(CHM::MAX_ENTRIES) + " places");

    std::cout << "New main image" << std::endl;
    driver->UnloadMainImage();
    check(hist->GetNumberOfEntries() == 0, "closing the image clears the history");
    driver->OpenImage(fn_img.c_str(), MAIN_ROLE, wl);
    check(hist->GetNumberOfEntries() == 1 && !hist->CheckState(CHM::UIF_CAN_GO_BACK),
          "opening an image starts the history over");
  }
  catch (std::exception &exc)
  {
    std::cerr << "Exception: " << exc.what() << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << (n_failures ? "FAILED: " + std::to_string(n_failures) + " check(s)" : "PASSED") << std::endl;
  return n_failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
