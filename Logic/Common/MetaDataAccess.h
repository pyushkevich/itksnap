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

template <unsigned int VDim>
class MetaDataAccess
{
public:
  MetaDataAccess(itk::ImageBase<VDim> *image);

  std::vector<std::string> GetKeysAsArray();
  std::string GetValueAsString(const std::string &key);

  bool HasKey(const std::string &key) const;

  // Useful routine for mapping orientation strings to text
  static std::string GetRAICode(
      itk::SpatialOrientation::ValidCoordinateOrientationFlags code);

  // Another utility for mapping keys via DICOM dictionary
  static std::string MapKeyToDICOM(std::string key);

private:
  itk::ImageBase<VDim> *m_Image;


};

#endif // METADATAACCESS_H
