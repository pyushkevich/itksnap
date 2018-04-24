#include "IRISApplication.h"
#include "UIReporterDelegates.h"
#include "itksys/SystemTools.hxx"

class DummySystemInfoDelegate : public SystemInfoDelegate
{
public:

  DummySystemInfoDelegate(const char *argv0) 
    {
    m_ExecutableName = argv0; 
    }

  virtual std::string GetApplicationDirectory()
    {
    return itksys::SystemTools::GetFilenamePath(m_ExecutableName);
    }

  virtual std::string GetApplicationFile()
    {
    return m_ExecutableName;
    }

  virtual std::string GetApplicationPermanentDataLocation()
    {
    return std::string(".itksnap.test");
    }

  virtual std::string GetUserDocumentsLocation()
    {
    return std::string(".itksnap.test");
    }

  virtual std::string EncodeServerURL(const std::string &url)
    {
    return url;
    }


  typedef SystemInfoDelegate::GrayscaleImage GrayscaleImage;
  typedef SystemInfoDelegate::RGBAPixelType RGBAPixelType;
  typedef SystemInfoDelegate::RGBAImageType RGBAImageType;

  virtual void LoadResourceAsImage2D(std::string tag, GrayscaleImage *image) {}
  virtual void LoadResourceAsRegistry(std::string tag, Registry &reg) {}
  virtual void WriteRGBAImage2D(std::string file, RGBAImageType *image) {}

protected:
  std::string m_ExecutableName;
};

int main(int argc, char *argv[])
{
  DummySystemInfoDelegate sidel(argv[0]);
  SystemInterface::SetSystemInfoDelegate(&sidel);

  IRISApplication::Pointer app = IRISApplication::New();
}
