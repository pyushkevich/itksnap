#include "IRISApplication.h"
#include "ImageIODelegates.h"
#include "UIReporterDelegates.h"
#include "LayerIterator.h"
#include "TDigestImageFilter.h"
#include <itksys/SystemTools.hxx>
#include <iostream>
#include <cstdlib>
#include <string>

// Minimal delegate — returns system temp dir, stubs everything else
class SimpleSystemInfoDelegate : public SystemInfoDelegate
{
public:
  SimpleSystemInfoDelegate(const char *argv0) { m_Exe = argv0; }

  std::string GetApplicationDirectory() override
    { return itksys::SystemTools::GetFilenamePath(m_Exe); }
  std::string GetApplicationFile() override { return m_Exe; }
  std::string GetApplicationPermanentDataLocation() override { return ".itksnap_test"; }
  std::string GetUserDocumentsLocation() override { return ".itksnap_test"; }
  std::string GetTempDirectory() override
    {
    const char *t = getenv("TMPDIR");
    return t ? std::string(t) : std::string("/tmp");
    }
  std::string EncodeServerURL(const std::string &url) override { return url; }

  typedef SystemInfoDelegate::GrayscaleImage GrayscaleImage;
  typedef SystemInfoDelegate::RGBAPixelType  RGBAPixelType;
  typedef SystemInfoDelegate::RGBAImageType  RGBAImageType;
  void LoadResourceAsImage2D(std::string, GrayscaleImage *) override {}
  void LoadResourceAsRegistry(std::string, Registry &) override {}
  void WriteRGBAImage2D(std::string, RGBAImageType *) override {}

private:
  std::string m_Exe;
};

class SimpleColorMapSource : public AbstractColorMapPresetNameSource
{
public:
  std::string GetPresetName(ColorMap::SystemPreset p, bool) override
    { return "Preset " + std::to_string(static_cast<int>(p)); }
};


// Print geometry + intensity quantiles for one image layer
void PrintLayerInfo(ImageWrapperBase *layer)
{
  auto sz = layer->GetSize();
  std::cout << "  Dimensions : " << sz[0] << " x " << sz[1] << " x " << sz[2] << std::endl;

  auto *base = layer->GetImageBase();
  auto sp = base->GetSpacing();
  std::cout << "  Spacing    : " << sp[0] << " x " << sp[1] << " x " << sp[2] << " mm" << std::endl;

  auto org = base->GetOrigin();
  std::cout << "  Origin     : " << org[0] << ", " << org[1] << ", " << org[2] << std::endl;

  std::cout << "  Components : " << layer->GetNumberOfComponents() << std::endl;

  TDigestDataObject *td = layer->GetTDigest();
  if (td)
    {
    td->Update();
    const double ranks[] = {0.0, 0.05, 0.25, 0.50, 0.75, 0.95, 1.0};
    std::cout << "  Quantiles  :";
    for (double r : ranks)
      std::cout << "  p" << static_cast<int>(r * 100) << "=" << td->GetImageQuantile(r);
    std::cout << std::endl;
    }
  else
    {
    std::cout << "  Quantiles  : (not available)" << std::endl;
    }
}


// Human-readable role names
static const char *RoleName(LayerRole role)
{
  switch (role)
    {
    case MAIN_ROLE:    return "MainRole";
    case OVERLAY_ROLE: return "OverlayRole";
    case LABEL_ROLE:   return "SegmentationRole";
    default:           return "UnknownRole";
    }
}


static void PrintUsage(const char *prog)
{
  std::cerr << "Usage:" << std::endl;
  std::cerr << "  " << prog << " -g <url-or-path>   load a single image" << std::endl;
  std::cerr << "  " << prog << " -w <url-or-path>   load a workspace" << std::endl;
}


int main(int argc, char *argv[])
{
  if (argc < 3)
    {
    PrintUsage(argv[0]);
    return 1;
    }

  std::string flag = argv[1];
  std::string path = argv[2];

  if (flag != "-g" && flag != "-w")
    {
    PrintUsage(argv[0]);
    return 1;
    }

  // --- Setup ---
  SimpleSystemInfoDelegate sidel(argv[0]);
  SystemInterface::SetSystemInfoDelegate(&sidel);

  SimpleColorMapSource cmSource;
  ColorMap::SetColorMapPresetNameSource(&cmSource);

  IRISApplication::Pointer app = IRISApplication::New();
  IRISWarningList warnings;

  // ── Single image (-g) ────────────────────────────────────────────────
  if (flag == "-g")
    {
    std::cout << "Loading image: " << path << std::endl;
    try
      {
      app->OpenImage(path.c_str(), MAIN_ROLE, warnings);
      }
    catch (std::exception &e)
      {
      std::cerr << "ERROR: " << e.what() << std::endl;
      return 1;
      }

    for (auto &w : warnings)
      std::cerr << "Warning: " << w.what() << std::endl;

    ImageWrapperBase *layer = app->GetMainImage();
    if (!layer)
      {
      std::cerr << "ERROR: main image is null after load" << std::endl;
      return 1;
      }

    std::cout << "Nickname   : " << layer->GetNickname() << std::endl;
    const std::string &remoteURL = layer->GetRemoteURL();
    if (!remoteURL.empty())
      std::cout << "Remote URL : " << remoteURL << std::endl;

    PrintLayerInfo(layer);
    return 0;
    }

  // ── Workspace (-w) ───────────────────────────────────────────────────
  std::cout << "Loading workspace: " << path << std::endl;
  try
    {
    app->OpenProject(path, warnings);
    }
  catch (std::exception &e)
    {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
    }

  for (auto &w : warnings)
    std::cerr << "Warning: " << w.what() << std::endl;

  // Iterate over all image layers in load order
  GenericImageData *imgdata = app->GetCurrentImageData();
  int layerIndex = 0;
  for (LayerIterator it(imgdata); !it.IsAtEnd(); ++it)
    {
    ImageWrapperBase *layer = it.GetLayer();
    LayerRole         role  = it.GetRole();

    std::cout << std::endl;
    std::cout << "Layer " << layerIndex++ << std::endl;
    std::cout << "  Role       : " << RoleName(role) << std::endl;
    std::cout << "  Nickname   : " << layer->GetNickname() << std::endl;

    const std::string &custom = layer->GetCustomNickname();
    if (!custom.empty())
      std::cout << "  Custom nick: " << custom << std::endl;

    std::cout << "  File       : " << layer->GetFileName() << std::endl;

    const std::string &remoteURL = layer->GetRemoteURL();
    if (!remoteURL.empty())
      std::cout << "  Remote URL : " << remoteURL << std::endl;

    PrintLayerInfo(layer);
    }

  if (layerIndex == 0)
    {
    std::cerr << "ERROR: no layers found after workspace load" << std::endl;
    return 1;
    }

  return 0;
}
