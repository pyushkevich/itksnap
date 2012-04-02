#include "MetaDataAccess.h"
#include <itkImageBase.h>
#include <itkMetaDataDictionary.h>
#include <itkMetaDataObject.h>
#include <itkGDCMImageIO.h>
#include <iostream>
#include <map>

using namespace itk;
using namespace std;

MetaDataAccess::MetaDataAccess(itk::ImageBase<3> *image)
{
  m_Image = image;
}


std::vector<std::string> MetaDataAccess::GetKeysAsArray()
{
  return m_Image->GetMetaDataDictionary().GetKeys();
}

template<class AnyType>
bool
try_get_metadata(itk::MetaDataDictionary &mdd,
                 const string &key, string &output, AnyType deflt)
{
  AnyType v = deflt;
  if(itk::ExposeMetaData<AnyType>(mdd, key, v))
    {
    ostringstream oss;
    oss << v << endl;
    output = oss.str();
    return true;
    }
  else return false;
}

std::string MetaDataAccess::GetValueAsString(const std::string &key)
{
  std::string value;
  itk::MetaDataDictionary &mdd = m_Image->GetMetaDataDictionary();

  // Orientation flag object
  itk::SpatialOrientation::ValidCoordinateOrientationFlags v_oflags =
    itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_INVALID;

  // Is the value a string?
  if(itk::ExposeMetaData<string>(mdd, key, value))
    {
    // For some weird reason, some of the strings returned by this method
    // contain '\0' characters. We will replace them by spaces
    ostringstream sout("");
    for(unsigned int i=0;i<value.length();i++)
      if(value[i] >= ' ') sout << value[i];
    value = sout.str();

    // Make sure the value has more than blanks
    if(value.find_first_not_of(" ") == string::npos)
      value="";
    }
  else if(itk::ExposeMetaData(mdd, key, v_oflags))
    {
    value = GetRAICode(v_oflags);
    }
  else
    {
    bool rc = false;
    if(!rc) rc |= try_get_metadata<double>(mdd, key, value, 0);
    if(!rc) rc |= try_get_metadata<float>(mdd, key, value, 0);
    if(!rc) rc |= try_get_metadata<int>(mdd, key, value, 0);
    if(!rc) rc |= try_get_metadata<unsigned int>(mdd, key, value, 0);
    if(!rc) rc |= try_get_metadata<long>(mdd, key, value, 0);
    if(!rc) rc |= try_get_metadata<unsigned long>(mdd, key, value, 0);
    if(!rc) rc |= try_get_metadata<short>(mdd, key, value, 0);
    if(!rc) rc |= try_get_metadata<unsigned short>(mdd, key, value, 0);
    if(!rc) rc |= try_get_metadata<char>(mdd, key, value, 0);
    if(!rc) rc |= try_get_metadata<unsigned char>(mdd, key, value, 0);

    if(!rc)
      {
      ostringstream oss;
      oss << "Object of type "
        << mdd[key]->GetMetaDataObjectTypeName();
      value = oss.str();
      }
    }

  return value;
}

std::string MetaDataAccess::MapKeyToDICOM(std::string key)
{
  // Try to remap the key to DICOM
  string dcm_label;
  if(itk::GDCMImageIO::GetLabelFromTag(key, dcm_label))
    return dcm_label;
  else
    return key;
}


std::string MetaDataAccess::GetRAICode(
    itk::SpatialOrientation::ValidCoordinateOrientationFlags code)
  {
  std::map<itk::SpatialOrientation::ValidCoordinateOrientationFlags, string> cmap;
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RIP] = "RIP";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LIP] = "LIP";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RSP] = "RSP";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LSP] = "LSP";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RIA] = "RIA";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LIA] = "LIA";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RSA] = "RSA";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LSA] = "LSA";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IRP] = "IRP";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ILP] = "ILP";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SRP] = "SRP";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SLP] = "SLP";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IRA] = "IRA";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ILA] = "ILA";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SRA] = "SRA";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SLA] = "SLA";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RPI] = "RPI";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LPI] = "LPI";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAI] = "RAI";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LAI] = "LAI";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RPS] = "RPS";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LPS] = "LPS";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAS] = "RAS";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LAS] = "LAS";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PRI] = "PRI";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PLI] = "PLI";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ARI] = "ARI";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ALI] = "ALI";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PRS] = "PRS";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PLS] = "PLS";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ARS] = "ARS";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ALS] = "ALS";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IPR] = "IPR";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SPR] = "SPR";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IAR] = "IAR";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SAR] = "SAR";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IPL] = "IPL";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SPL] = "SPL";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IAL] = "IAL";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SAL] = "SAL";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PIR] = "PIR";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PSR] = "PSR";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_AIR] = "AIR";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ASR] = "ASR";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PIL] = "PIL";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PSL] = "PSL";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_AIL] = "AIL";
  cmap[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ASL] = "ASL";
  return cmap[code];
}



