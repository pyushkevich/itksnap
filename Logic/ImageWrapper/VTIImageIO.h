#ifndef __VTIImageIO_h_
#define __VTIImageIO_h_

#include "itkImageIOBase.h"
#include <vtkSmartPointer.h>

class vtkImageData;

/**
 * \class VTIImageIO
 * \brief ITK ImageIO adapter for VTK's XML Image Data format (.vti).
 *
 * Uses vtkXMLImageDataReader/Writer internally so that .vti files can
 * be read and written through ITK's standard image-file pipeline.
 * Direction cosines are set to identity because vtkImageData does not
 * store them.
 */
class VTIImageIO : public itk::ImageIOBase
{
public:
  itkNewMacro(VTIImageIO);
  itkTypeMacro(VTIImageIO, ImageIOBase);

  bool CanReadFile(const char *filename) override;
  bool CanWriteFile(const char *filename) override;

  void ReadImageInformation() override;
  void WriteImageInformation() override;

  void Read(void *buffer) override;
  void Write(const void *buffer) override;

protected:
  VTIImageIO() = default;
  ~VTIImageIO() override = default;

private:
  // Cached after ReadImageInformation to avoid reading the file twice
  vtkSmartPointer<vtkImageData> m_CachedImage;
};

#endif // __VTIImageIO_h_
