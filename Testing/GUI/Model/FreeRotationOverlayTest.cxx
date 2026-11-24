// Test: Image Free Rotation rotates the overlays with the main image (#228).
//
// In free rotation mode the main image and the segmentations get the transform
// A. With 'Rotate all layers' on, an overlay whose own (registration)
// transform is R gets R o A (x -> R(A(x))), so it keeps its alignment with
// the main image. This test checks that:
//
//   - switching to free rotation keeps an overlay's registration R
//   - rotating gives the overlay R o A, and rotating again follows the new A
//   - turning 'Rotate all layers' off gives back R, and on again R o A
//   - an overlay added while rotated is rotated too
//   - a workspace saved while rotated stores R, not R o A
//   - leaving free rotation gives back R, and identity for the main image
//     and the segmentation

#include "GlobalUIModel.h"
#include "RegistrationModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "GlobalState.h"
#include "ImageWrapperBase.h"
#include "LabelImageWrapper.h"
#include "AffineTransformHelper.h"
#include "Registry.h"
#include "IRISException.h"
#include "ImageIODelegates.h"
#include "SystemInterface.h"
#include "ColorMap.h"
#include "UIReporterDelegates.h"
#include "itkImage.h"
#include "itkImageFileWriter.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkMatrixOffsetTransformBase.h"
#include "itksys/SystemTools.hxx"
#include <vnl/vnl_math.h>
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

using ITKTransform = AffineTransformHelper::ITKTransformBase;

// Whether two affine transforms are the same
static bool Same(const ITKTransform *a, const ITKTransform *b, double tol = 1e-6)
{
  auto b_inv = AffineTransformHelper::Inverse(b);
  auto diff = AffineTransformHelper::Compose(a, b_inv);
  return AffineTransformHelper::IsIdentity(diff, tol);
}

static bool Same(const ITKTransform *a, const SmartPtr<AffineTransformHelper::ITKTransformMOTB> &b,
                 double tol = 1e-6)
{
  return Same(a, b.GetPointer(), tol);
}

// x -> R(A(x))
static SmartPtr<AffineTransformHelper::ITKTransformMOTB> RA(const ITKTransform *r, const ITKTransform *a)
{
  return AffineTransformHelper::Compose(a, r);
}

template <class TImage>
static void WriteImage(const std::string &fn)
{
  auto img = TImage::New();
  typename TImage::RegionType region;
  for (unsigned int d = 0; d < 3; d++)
    region.SetSize(d, 32);
  img->SetRegions(region);
  double spacing[3] = { 1.0, 1.25, 0.8 };
  img->SetSpacing(spacing);
  img->Allocate();
  for (itk::ImageRegionIteratorWithIndex<TImage> it(img, region); !it.IsAtEnd(); ++it)
    it.Set(static_cast<typename TImage::PixelType>((it.GetIndex()[0] + it.GetIndex()[1]) % 2));

  auto writer = itk::ImageFileWriter<TImage>::New();
  writer->SetInput(img);
  writer->SetFileName(fn);
  writer->Update();
}

// The transform saved in a workspace for the layer with the given file
static SmartPtr<ITKTransform> SavedTransform(const std::string &ws, const std::string &file)
{
  Registry preg;
  preg.ReadFromXMLFile(ws.c_str());
  for (int i = 0; preg.HasFolder(Registry::Key("Layers.Layer[%03d]", i)); i++)
  {
    Registry &folder = preg.Folder(Registry::Key("Layers.Layer[%03d]", i));
    if (itksys::SystemTools::SameFile(folder["AbsolutePath"][""], file))
      return AffineTransformHelper::ReadFromRegistry(&folder);
  }
  throw IRISException("No layer for %s in the workspace", file.c_str());
}

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " <temp_dir>" << std::endl;
    return EXIT_FAILURE;
  }

  std::string tmpdir = std::string(argv[1]) + "/FreeRotationOverlayTest";
  itksys::SystemTools::MakeDirectory(tmpdir);

  DummySystemInfoDelegate sidel(argv[0], tmpdir);
  SystemInterface::SetSystemInfoDelegate(&sidel);
  DummyColorMapPresetNameSource color_map_source;
  ColorMap::SetColorMapPresetNameSource(&color_map_source);

  try
  {
    std::string fn_main = tmpdir + "/main.nii.gz", fn_seg = tmpdir + "/seg.nii.gz";
    std::string fn_ov1 = tmpdir + "/overlay1.nii.gz", fn_ov2 = tmpdir + "/overlay2.nii.gz";
    WriteImage<itk::Image<short, 3>>(fn_main);
    WriteImage<itk::Image<LabelType, 3>>(fn_seg);
    WriteImage<itk::Image<short, 3>>(fn_ov1);
    WriteImage<itk::Image<short, 3>>(fn_ov2);

    SmartPtr<GlobalUIModel> gui = GlobalUIModel::New();
    IRISApplication        *driver = gui->GetDriver();
    IRISWarningList         wl;
    driver->OpenImage(fn_main.c_str(), MAIN_ROLE, wl);
    driver->OpenImage(fn_seg.c_str(), LABEL_ROLE, wl);
    driver->OpenImage(fn_ov1.c_str(), OVERLAY_ROLE, wl);

    GenericImageData  *id = driver->GetCurrentImageData();
    ImageWrapperBase  *main = id->GetMain();
    LabelImageWrapper *seg = driver->GetSelectedSegmentationLayer();
    ImageWrapperBase  *ov1 = id->GetLastOverlay();
    RegistrationModel *reg = gui->GetRegistrationModel();
    driver->SetCursorPosition(Vector3i(16, 16, 16));

    // Register the overlay: a rotation and a translation in Registration mode
    reg->SetFreeRotationMode(false);
    reg->Update();
    check(reg->GetMovingLayerWrapper() == ov1, "the overlay is the moving layer in Registration mode");
    reg->SetCenterOfRotationToCursor();
    reg->ApplyRotation(Vector3d(0.0, 0.0, 1.0), 10.0 * vnl_math::pi / 180.0);
    reg->ApplyTranslation(Vector3d(3.0, -2.0, 1.5));
    auto R = AffineTransformHelper::Compose(ov1->GetITKTransform(), nullptr);   // a copy
    check(!AffineTransformHelper::IsIdentity(R), "the overlay has a registration");

    std::cout << "Free rotation" << std::endl;
    check(driver->GetGlobalState()->GetRotateAllLayers(), "'Rotate all layers' is on by default");
    reg->SetFreeRotationMode(true);
    reg->Update();
    check(Same(ov1->GetITKTransform(), R), "switching to free rotation keeps the overlay's registration");

    // Rotate 30 degrees about an oblique axis
    reg->SetCenterOfRotationToCursor();
    Vector3d axis(1.0, 2.0, 3.0);
    axis.normalize();
    reg->ApplyRotation(axis, 30.0 * vnl_math::pi / 180.0);
    reg->Update();
    const ITKTransform *A = main->GetITKTransform();
    check(!AffineTransformHelper::IsIdentity(A), "the main image is rotated");
    check(Same(seg->GetITKTransform(), A), "the segmentation is rotated with it");
    check(Same(ov1->GetITKTransform(), RA(R, A)), "the overlay is rotated with it (R o A)");

    // Rotate again: the overlay follows the new rotation
    reg->ApplyRotation(Vector3d(0.0, 1.0, 0.0), -15.0 * vnl_math::pi / 180.0);
    reg->ApplyTranslation(Vector3d(-1.0, 0.5, 2.0));
    reg->Update();
    A = main->GetITKTransform();
    check(Same(ov1->GetITKTransform(), RA(R, A)), "the overlay follows a further rotation and translation");

    std::cout << "Rotate all layers" << std::endl;
    driver->GetGlobalState()->SetRotateAllLayers(false);
    reg->Update();
    check(Same(ov1->GetITKTransform(), R), "turning it off gives the overlay back its registration");
    check(Same(seg->GetITKTransform(), A), "the segmentation stays rotated");
    reg->ApplyRotation(Vector3d(1.0, 0.0, 0.0), 5.0 * vnl_math::pi / 180.0);
    reg->Update();
    A = main->GetITKTransform();
    check(Same(ov1->GetITKTransform(), R), "while off, rotating does not move the overlay");
    driver->GetGlobalState()->SetRotateAllLayers(true);
    reg->Update();
    check(Same(ov1->GetITKTransform(), RA(R, A)), "turning it on again rotates the overlay");

    std::cout << "Overlay added while rotated" << std::endl;
    driver->OpenImage(fn_ov2.c_str(), OVERLAY_ROLE, wl);
    reg->Update();
    ImageWrapperBase *ov2 = id->GetLastOverlay();
    check(ov2 != ov1 && Same(ov2->GetITKTransform(), A), "it is rotated with the main image");
    check(Same(ov1->GetITKTransform(), RA(R, A)), "the first overlay is unchanged");

    std::cout << "Workspace" << std::endl;
    std::string fn_ws = tmpdir + "/rotated.itksnap";
    driver->SaveProject(fn_ws);
    auto R_saved = SavedTransform(fn_ws, fn_ov1);
    // Workspaces store numbers with 6 significant digits
    check(Same(R_saved, R, 1e-4), "the saved overlay transform is its registration R");
    check(AffineTransformHelper::IsIdentity(SavedTransform(fn_ws, fn_ov2)),
          "the overlay added while rotated is saved with identity");

    std::cout << "Leaving free rotation" << std::endl;
    reg->SetFreeRotationMode(false);
    reg->Update();
    check(Same(ov1->GetITKTransform(), R), "the overlay has its registration back");
    check(AffineTransformHelper::IsIdentity(ov2->GetITKTransform()), "the added overlay is back to identity");
    check(AffineTransformHelper::IsIdentity(main->GetITKTransform()), "the main image is back to identity");
    check(AffineTransformHelper::IsIdentity(seg->GetITKTransform()), "the segmentation is back to identity");
  }
  catch (std::exception &exc)
  {
    std::cerr << "Exception: " << exc.what() << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << (n_failures ? "FAILED: " + std::to_string(n_failures) + " check(s)" : "PASSED") << std::endl;
  return n_failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
