#include "IRISApplication.h"
#include "ImageIODelegates.h"
#include "UIReporterDelegates.h"
#include "TDigestImageFilter.h"
#include <itksys/SystemTools.hxx>
#include <iostream>
#include <cstdlib>

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


int main(int argc, char *argv[])
{
  if (argc < 2)
    {
    std::cerr << "Usage: remote_image_load_test <url-or-path>" << std::endl;
    return 1;
    }
  const char *url = argv[1];

  // --- Setup ---
  SimpleSystemInfoDelegate sidel(argv[0]);
  SystemInterface::SetSystemInfoDelegate(&sidel);

  SimpleColorMapSource cmSource;
  ColorMap::SetColorMapPresetNameSource(&cmSource);

  IRISApplication::Pointer app = IRISApplication::New();

  // --- Load image ---
  std::cout << "Loading: " << url << std::endl;
  IRISWarningList warnings;
  try
    {
    app->OpenImage(url, MAIN_ROLE, warnings);
    }
  catch (std::exception &e)
    {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
    }

  for (auto &w : warnings)
    std::cerr << "Warning: " << w.what() << std::endl;

  // --- Query image ---
  ImageWrapperBase *layer = app->GetMainImage();
  if (!layer)
    {
    std::cerr << "ERROR: main image is null after load" << std::endl;
    return 1;
    }

  auto sz = layer->GetSize();
  std::cout << "Dimensions : " << sz[0] << " x " << sz[1] << " x " << sz[2] << std::endl;

  auto *base = layer->GetImageBase();
  auto sp = base->GetSpacing();
  std::cout << "Spacing    : " << sp[0] << " x " << sp[1] << " x " << sp[2] << " mm" << std::endl;

  auto org = base->GetOrigin();
  std::cout << "Origin     : " << org[0] << ", " << org[1] << ", " << org[2] << std::endl;

  // --- TDigest quantiles ---
  TDigestDataObject *td = layer->GetTDigest();
  if (!td)
    {
    std::cerr << "ERROR: TDigest not available" << std::endl;
    return 1;
    }

  std::cout << "Computing TDigest..." << std::endl;
  td->Update();

  // get_quantile takes normalized rank in [0,1]
  const double ranks[] = {0.0, 0.05, 0.25, 0.50, 0.75, 0.95, 1.0};
  std::cout << "Quantiles  :";
  for (double r : ranks)
    std::cout << "  p" << (int)(r*100) << "=" << td->GetImageQuantile(r);
  std::cout << std::endl;

  return 0;
}
