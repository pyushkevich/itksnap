#ifndef METADATAACCESS_H
#define METADATAACCESS_H

#include <SNAPCommon.h>
#include <itkSpatialOrientation.h>
#include <string>

namespace itk
{
template <unsigned int VDim> class ImageBase;
}

class MetaDataAccess
{
public:
  MetaDataAccess(itk::ImageBase<3> *image);

  size_t GetNumberOfKeys() const;

  std::string GetKey(size_t i);
  std::string GetValueAsString(const std::string &key);
  std::string GetValueAsString(size_t i);

  // Useful routine for mapping orientation strings to text
  static std::string GetRAICode(
      itk::SpatialOrientation::ValidCoordinateOrientationFlags code);

  // Another utility for mapping keys via DICOM dictionary
  static std::string MapKeyToDICOM(std::string key);

private:
  itk::ImageBase<3> *m_Image;


};

#endif // METADATAACCESS_H
