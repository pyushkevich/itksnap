#ifndef METADATAACCESS_H
#define METADATAACCESS_H

#include <SNAPCommon.h>
#include <itkSpatialOrientation.h>
#include <string>
#include <vector>

namespace itk
{
template <unsigned int VDim> class ImageBase;
}

class MetaDataAccess
{
public:
  MetaDataAccess(itk::ImageBase<3> *image);

  std::vector<std::string> GetKeysAsArray();
  std::string GetValueAsString(const std::string &key);

  // Useful routine for mapping orientation strings to text
  static std::string GetRAICode(
      itk::SpatialOrientation::ValidCoordinateOrientationFlags code);

  // Another utility for mapping keys via DICOM dictionary
  static std::string MapKeyToDICOM(std::string key);

private:
  itk::ImageBase<3> *m_Image;


};

#endif // METADATAACCESS_H
