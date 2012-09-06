/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: itkVoxBoCUBImageIO.cxx,v $
  Language:  C++
  Date:      $Date: 2008/11/20 04:23:28 $
  Version:   $Revision: 1.4 $  

  Copyright (c) Insight Software Consortium. All rights reserved.
  
  This file is part of ITK-SNAP 

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 


=========================================================================*/
#include "itkVoxBoCUBImageIO.h"
#include "itkIOCommon.h"
#include "itkExceptionObject.h"
#include "itkMetaDataObject.h"
#include "itkByteSwapper.h"
#include <iostream>
#include <list>
#include <string>
#include <math.h>

// Commented out because zlib is not available through Insight Applications
#ifdef SNAP_GZIP_SUPPORT
#include <zlib.h>
#endif
#include "itkSpatialOrientationAdapter.h"

namespace itk {


/**
 * A generic reader and writer object for VoxBo files. Basically it
 * provides uniform access to gzip and normal files
 */
class GenericCUBFileAdaptor
{
public:
    virtual ~GenericCUBFileAdaptor() {}
  virtual unsigned char ReadByte() = 0;
  virtual void ReadData(void *data, unsigned long bytes) = 0;
  virtual void WriteData(const void *data, unsigned long bytes) = 0;

  std::string ReadHeader()
    {
    // Read everything up to the \f symbol
    std::ostringstream oss;
    unsigned char byte = ReadByte();
    while(byte != '\f')
      {
      oss << byte;
      byte = ReadByte();
      }

    // Read the next byte
    unsigned char term = ReadByte();
    if(term == '\r')
      term = ReadByte();

    // Throw exception if term is not there
    if(term != '\n')
      {
      ExceptionObject exception;
      exception.SetDescription("Header is not terminated by newline.");
      throw exception;
      }

    // Return the header string
    return oss.str();
    }
};

/**
 * A reader for gzip files
 */
#ifdef SNAP_GZIP_SUPPORT

class CompressedCUBFileAdaptor : public GenericCUBFileAdaptor
{
public:
  CompressedCUBFileAdaptor(const char *file, const char *mode)
    {
    m_GzFile = ::gzopen(file, mode);
    if(m_GzFile == NULL)
      {
      ExceptionObject exception;
      exception.SetDescription("File cannot be accessed");
      throw exception;
      }
    }
  
  ~CompressedCUBFileAdaptor()
    {
    if(m_GzFile)
      ::gzclose(m_GzFile);
    }

  unsigned char ReadByte()
    {
    int byte = ::gzgetc(m_GzFile);
    if(byte < 0)
      {
      std::ostringstream oss;
      oss << "Error reading byte from file at position: " << ::gztell(m_GzFile);
      ExceptionObject exception;
      exception.SetDescription(oss.str().c_str());
      throw exception;
      }
    return static_cast<unsigned char>(byte);
    }
  
  void ReadData(void *data, unsigned long bytes)
    {
    if(m_GzFile == NULL)
      {
      ExceptionObject exception;
      exception.SetDescription("File cannot be read");
      throw exception;
      }

    int bread = ::gzread(m_GzFile, data, bytes);
    if(bread != bytes)
      {
      std::ostringstream oss;
      oss << "File size does not match header: " 
        << bytes << " bytes requested but only "
        << bread << " bytes available!" << std::endl
        << "At file position " << ::gztell(m_GzFile);
      ExceptionObject exception;
      exception.SetDescription(oss.str().c_str());
      throw exception;
      }
    }
  
  void WriteData(const void *data, unsigned long bytes)
    {
    if(m_GzFile == NULL)
      {
      ExceptionObject exception;
      exception.SetDescription("File cannot be written");
      throw exception;
      }

    int bwritten = ::gzwrite(m_GzFile, (void *) data, bytes);
    if(bwritten != bytes)
      {
      ExceptionObject exception;
      exception.SetDescription("Could not write all bytes to file");
      throw exception;
      }
    }

private:
  ::gzFile m_GzFile;
};

#endif // SNAP_GZIP_SUPPORT

/**
 * A reader for non-gzip files
 */
class DirectCUBFileAdaptor : public GenericCUBFileAdaptor
{
public:
  DirectCUBFileAdaptor(const char *file, const char *mode)
    {
    m_File = fopen(file, mode);
    if(m_File == NULL)
      {
      ExceptionObject exception;
      exception.SetDescription("File cannot be read");
      throw exception;
      }
    }

  virtual ~DirectCUBFileAdaptor()
    {
    if(m_File)
      fclose(m_File);
    }

  unsigned char ReadByte()
    {
    int byte = fgetc(m_File);
    if(byte == EOF)
      {
      std::ostringstream oss;
      oss << "Error reading byte from file at position: " << ::ftell(m_File);
      ExceptionObject exception;
      exception.SetDescription(oss.str().c_str());
      throw exception;
      }
    return static_cast<unsigned char>(byte);
    }
  
  void ReadData(void *data, unsigned long bytes)
    {
    if(m_File == NULL)
      {
      ExceptionObject exception;
      exception.SetDescription("File cannot be read");
      throw exception;
      }

    size_t bread = fread(data, 1, bytes, m_File);
    if(static_cast<unsigned long>(bread) != bytes)
      {
      std::ostringstream oss;
      oss << "File size does not match header: " 
        << bytes << " bytes requested but only "
        << bread << " bytes available!" << std::endl
        << "At file position " << ftell(m_File);
      ExceptionObject exception;
      exception.SetDescription(oss.str().c_str());
      throw exception;
      }
    }

  void WriteData(const void *data, unsigned long bytes)
    {
    if(m_File == NULL)
      {
      ExceptionObject exception;
      exception.SetDescription("File cannot be written");
      throw exception;
      }

    size_t bwritten = fwrite(data, 1, bytes, m_File);
    if(static_cast<unsigned long>(bwritten) != bytes)
      {
      ExceptionObject exception;
      exception.SetDescription("Could not write all bytes to file");
      throw exception;
      }
    }
private:
  FILE *m_File;
};


/**
 * A swap helper class, used to perform swapping for any input
 * data type.
 */
template<typename TPixel> class VoxBoCUBImageIOSwapHelper
{
public:
  typedef ImageIOBase::ByteOrder ByteOrder;
  static void SwapIfNecessary(
    void *buffer, unsigned long numberOfBytes, ByteOrder order)
    {
    if ( order == ImageIOBase::LittleEndian )
      {
      ByteSwapper<TPixel>::SwapRangeFromSystemToLittleEndian(
        (TPixel*)buffer, numberOfBytes / sizeof(TPixel) );
      }
    else if ( order == ImageIOBase::BigEndian )
      {
      ByteSwapper<TPixel>::SwapRangeFromSystemToBigEndian(
        (TPixel *)buffer, numberOfBytes / sizeof(TPixel) );
      }
    }
};


// Strings
const char *VoxBoCUBImageIO::VB_IDENTIFIER_SYSTEM = "VB98";
const char *VoxBoCUBImageIO::VB_IDENTIFIER_FILETYPE = "CUB1";
const char *VoxBoCUBImageIO::VB_DIMENSIONS = "VoxDims(XYZ)";
const char *VoxBoCUBImageIO::VB_SPACING = "VoxSizes(XYZ)";
const char *VoxBoCUBImageIO::VB_ORIGIN = "Origin(XYZ)";
const char *VoxBoCUBImageIO::VB_DATATYPE = "DataType";
const char *VoxBoCUBImageIO::VB_BYTEORDER = "Byteorder";
const char *VoxBoCUBImageIO::VB_ORIENTATION = "Orientation";
const char *VoxBoCUBImageIO::VB_BYTEORDER_MSB = "msbfirst";
const char *VoxBoCUBImageIO::VB_BYTEORDER_LSB = "lsbfirst";
const char *VoxBoCUBImageIO::VB_DATATYPE_BYTE = "Byte";
const char *VoxBoCUBImageIO::VB_DATATYPE_INT = "Integer";
const char *VoxBoCUBImageIO::VB_DATATYPE_FLOAT = "Float";
const char *VoxBoCUBImageIO::VB_DATATYPE_DOUBLE = "Double";

/** Constructor */
VoxBoCUBImageIO::VoxBoCUBImageIO()
{
  InitializeOrientationMap();
  m_ByteOrder = BigEndian;
  m_Reader = NULL;
  m_Writer = NULL;
}


/** Destructor */
VoxBoCUBImageIO::~VoxBoCUBImageIO()
{
  if(m_Reader)
    delete m_Reader;
  if(m_Writer)
    delete m_Writer;
}

GenericCUBFileAdaptor *
VoxBoCUBImageIO::CreateReader(const char *filename)
{
  try
    {
    bool compressed;
    if(CheckExtension(filename, compressed))
      if(compressed)
#ifdef SNAP_GZIP_SUPPORT
          return new CompressedCUBFileAdaptor(filename, "rb");
#else
          return NULL;
#endif
      else
        return new DirectCUBFileAdaptor(filename, "rb");
    else
      return NULL;
    }
  catch(...)
    {
    return NULL;
    }
}

GenericCUBFileAdaptor *
VoxBoCUBImageIO::CreateWriter(const char *filename)
{
  try
    {
    bool compressed;
    if(CheckExtension(filename, compressed))
      if(compressed)
#ifdef SNAP_GZIP_SUPPORT
          return new CompressedCUBFileAdaptor(filename, "rb");
#else
          return NULL;
#endif        
      else
        return new DirectCUBFileAdaptor(filename, "wb");
    else
      return NULL;
    }
  catch(...)
    {
    return NULL;
    }
}

bool VoxBoCUBImageIO::CanReadFile( const char* filename ) 
{ 
  // First check if the file can be read
  GenericCUBFileAdaptor *reader = CreateReader(filename);
  if(reader == NULL)
    {
    itkDebugMacro(<<"The file is not a valid CUB file");
    return false;
    }
    
  // Now check the content
  bool iscub = true;
  try 
    {
    // Get the header
    std::istringstream iss(reader->ReadHeader());

    // Read the first two words
    std::string word;

    // Read the first line from the file
    iss >> word;
    if(word != VB_IDENTIFIER_SYSTEM)
      iscub = false;

    // Read the second line
    iss >> word;
    if(word != VB_IDENTIFIER_FILETYPE)
      iscub = false;
    }
  catch(...)
    { 
    iscub = false; 
    }

  delete reader;
  return iscub;
}

bool VoxBoCUBImageIO::CanWriteFile( const char * name )
{
  bool compressed;
  return CheckExtension(name, compressed);
}

void VoxBoCUBImageIO::Read(void* buffer)
{
  if(m_Reader == NULL)
    {
    ExceptionObject exception(__FILE__, __LINE__);
    exception.SetDescription("File cannot be read");
    throw exception;
    }

  m_Reader->ReadData(buffer, GetImageSizeInBytes());
  this->SwapBytesIfNecessary(buffer, GetImageSizeInBytes());
}

/** 
 *  Read Information about the VoxBoCUB file
 *  and put the cursor of the stream just before the first data pixel
 */
void VoxBoCUBImageIO::ReadImageInformation()
{
  // Make sure there is no other reader
  if(m_Reader)
    delete m_Reader;

  // Create a reader
  m_Reader = CreateReader(m_FileName.c_str());
  if(m_Reader == NULL)
    {
    ExceptionObject exception(__FILE__, __LINE__);
    exception.SetDescription("File cannot be read");
    throw exception;
    }

  // Set the number of dimensions to three
  SetNumberOfDimensions(3);

  // Read the file header
  std::istringstream issHeader(m_Reader->ReadHeader());

  // Read every string in the header. Parse the strings that are special
  while(issHeader.good())
    {
    // Read a line from the stream
    char linebuffer[512];
    issHeader.getline(linebuffer, 512);

    // Get the key string
    std::istringstream iss(linebuffer);
    std::string key;

    // Read the key and strip the colon from it
    iss >> key;
    if(key.size() > 0 && key[key.size() - 1] == ':')
      {
      // Strip the colon off the key
      key = key.substr(0, key.size() - 1);

      // Check if this is a relevant key
      if(key == VB_DIMENSIONS)
        {
        iss >> m_Dimensions[0];
        iss >> m_Dimensions[1];
        iss >> m_Dimensions[2];
        }

      else if(key == VB_SPACING)
        {
        iss >> m_Spacing[0];
        iss >> m_Spacing[1];
        iss >> m_Spacing[2];
        }

      else if(key == VB_ORIGIN)
        {
        double ox, oy, oz;
        iss >> ox; iss >> oy; iss >> oz;
        m_Origin[0] = ox * m_Spacing[0];
        m_Origin[1] = oy * m_Spacing[1];
        m_Origin[2] = oz * m_Spacing[2];
        }

      else if(key == VB_DATATYPE)
        {
        std::string type;
        iss >> type;
        m_PixelType = SCALAR;
        if(type == VB_DATATYPE_BYTE)
          m_ComponentType = UCHAR;
        else if(type == VB_DATATYPE_INT)
          m_ComponentType = USHORT;
        else if(type == VB_DATATYPE_FLOAT)
          m_ComponentType = FLOAT;
        else if(type == VB_DATATYPE_DOUBLE)
          m_ComponentType = DOUBLE;
        }

      else if(key == VB_BYTEORDER)
        {
        std::string type;
        iss >> type;
        if(type == VB_BYTEORDER_MSB)
          SetByteOrderToBigEndian();
        else if(type == VB_BYTEORDER_LSB)
          SetByteOrderToLittleEndian();
        else
          {
          ExceptionObject exception(__FILE__, __LINE__);
          exception.SetDescription("Unknown byte order constant");
          throw exception;
          }
        }

      else if(key == VB_ORIENTATION)
        {
        std::string code;
        iss >> code;

        // Set the orientation code in the data dictionary
        OrientationMap::const_iterator it = m_OrientationMap.find(code);
        if(it != m_OrientationMap.end())
          {
              //Octavian original begin
          //itk::MetaDataDictionary &dic =this->GetMetaDataDictionary();
          //EncapsulateMetaData<OrientationFlags>(
            //dic, ITK_CoordinateOrientation, it->second);
              //Octavian original end
          //NOTE:  The itk::ImageIOBase direction is a std::vector<std::vector > >, and threeDDirection is a 3x3 matrix
          itk::SpatialOrientationAdapter soAdaptor;
          itk::SpatialOrientationAdapter::DirectionType threeDDirection=soAdaptor.ToDirectionCosines(it->second);
          this->m_Direction[0][0]=threeDDirection[0][0];
          this->m_Direction[0][1]=threeDDirection[0][1];
          this->m_Direction[0][2]=threeDDirection[0][2];
          this->m_Direction[1][0]=threeDDirection[1][0];
          this->m_Direction[1][1]=threeDDirection[1][1];
          this->m_Direction[1][2]=threeDDirection[1][2];
          this->m_Direction[2][0]=threeDDirection[2][0];
          this->m_Direction[2][1]=threeDDirection[2][1];
          this->m_Direction[2][2]=threeDDirection[2][2];
          }
        }

      else
        {
        // Encode the right hand side of the string in the meta-data dic
        std::string word;
        std::ostringstream oss;
        while(iss >> word)
          {
          if(oss.str().size())
            oss << " ";
          oss << word;
          }
        itk::MetaDataDictionary &dic =this->GetMetaDataDictionary();
        EncapsulateMetaData<std::string>(dic, key, oss.str());
        }
      }
    }
}

void 
VoxBoCUBImageIO
::WriteImageInformation(void)
{
  // See if we have a writer already
  if(m_Writer != NULL)
    delete m_Writer;

  // First check if the file can be written to
  m_Writer = CreateWriter(m_FileName.c_str());
  if(m_Writer == NULL)
    {
    ExceptionObject exception(__FILE__, __LINE__);
    exception.SetDescription("File cannot be read");
    throw exception;
    }

  // Check that the number of dimensions is correct
  if(GetNumberOfDimensions() != 3)
    {
    ExceptionObject exception(__FILE__, __LINE__);
    exception.SetDescription("Unsupported number of dimensions");
    throw exception;
    }

  // Put together a header
  std::ostringstream header;

  // Write the identifiers
  header << VB_IDENTIFIER_SYSTEM << std::endl;
  header << VB_IDENTIFIER_FILETYPE << std::endl;

  // Write the image dimensions
  header << VB_DIMENSIONS << ": " 
    << m_Dimensions[0] << " "
    << m_Dimensions[1] << " "
    << m_Dimensions[2] << std::endl;

  // Write the spacing
  header << VB_SPACING << ": "
    << m_Spacing[0] << " "
    << m_Spacing[1] << " "
    << m_Spacing[2] << std::endl;

  // Write the origin (have to convert to bytes)
  header << VB_ORIGIN << ": "
    << static_cast< int >( m_Origin[0] / m_Spacing[0] + 0.5 ) << " "
    << static_cast< int >( m_Origin[1] / m_Spacing[1] + 0.5 ) << " "
    << static_cast< int >( m_Origin[2] / m_Spacing[2] + 0.5 ) << std::endl;

  // Write the byte order
  header << VB_BYTEORDER << ": "
    << (ByteSwapper<char>::SystemIsBigEndian() 
      ? VB_BYTEORDER_MSB : VB_BYTEORDER_LSB) << std::endl;

  // Write the data type 
  switch(m_ComponentType) 
    {
    case CHAR: 
    case UCHAR:
      header << VB_DATATYPE << ": " << VB_DATATYPE_BYTE << std::endl;
      break;
    case SHORT:
    case USHORT:
      header << VB_DATATYPE << ": " << VB_DATATYPE_INT << std::endl;
      break;
    case FLOAT:
      header << VB_DATATYPE << ": " << VB_DATATYPE_FLOAT << std::endl;
      break;
    case DOUBLE:
      header << VB_DATATYPE << ": " << VB_DATATYPE_DOUBLE << std::endl;
      break;
    default:
      ExceptionObject exception(__FILE__, __LINE__);
      exception.SetDescription("Unsupported pixel component type");
      throw exception;
    }

  // Write the orientation code
    //Octavian original begin
  //MetaDataDictionary &dic = GetMetaDataDictionary();
  //OrientationFlags oflag = SpatialOrientation::ITK_COORDINATE_ORIENTATION_INVALID;
  //if(ExposeMetaData<OrientationFlags>(dic, ITK_CoordinateOrientation, oflag))
      //Octavian original end
    //NOTE:  The itk::ImageIOBase direction is a std::vector<std::vector > >, and threeDDirection is a 3x3 matrix
    itk::SpatialOrientationAdapter soAdaptor;
    itk::SpatialOrientationAdapter::DirectionType threeDDirection;
    threeDDirection[0][0]=this->m_Direction[0][0];
    threeDDirection[0][1]=this->m_Direction[0][1];
    threeDDirection[0][2]=this->m_Direction[0][2];
    threeDDirection[1][0]=this->m_Direction[1][0];
    threeDDirection[1][1]=this->m_Direction[1][1];
    threeDDirection[1][2]=this->m_Direction[1][2];
    threeDDirection[2][0]=this->m_Direction[2][0];
    threeDDirection[2][1]=this->m_Direction[2][1];
    threeDDirection[2][2]=this->m_Direction[2][2];
    OrientationFlags     oflag = soAdaptor.FromDirectionCosines(threeDDirection);
    {
    InverseOrientationMap::const_iterator it = 
      m_InverseOrientationMap.find(oflag);
    if(it != m_InverseOrientationMap.end())
      {
      header << VB_ORIENTATION << ": " << it->second << std::endl;
      }
    }

  // Write the terminating characters
  header << "\f\n";

  // Write the header to the file as data
  m_Writer->WriteData(header.str().c_str(), header.str().size());
}

/** The write function is not implemented */
void 
VoxBoCUBImageIO
::Write( const void* buffer) 
{
  WriteImageInformation();
  m_Writer->WriteData(buffer, GetImageSizeInBytes());
}

/** Print Self Method */
void VoxBoCUBImageIO::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
  os << indent << "PixelType " << m_PixelType << "\n";
}


bool VoxBoCUBImageIO::CheckExtension(const char* filename, bool &isCompressed)
{
  std::string fname = filename;
  if ( fname == "" )
  {
    itkDebugMacro(<< "No filename specified.");
    return false;
  }

  bool extensionFound = false;
  isCompressed = false;

  std::string::size_type giplPos = fname.rfind(".cub");
  if ((giplPos != std::string::npos)
      && (giplPos == fname.length() - 4))
    {
      extensionFound = true;
    }

  giplPos = fname.rfind(".cub.gz");
  if ((giplPos != std::string::npos)
      && (giplPos == fname.length() - 7))
    {
    extensionFound = true;
    isCompressed = true;
    }

  return extensionFound;
}

void 
VoxBoCUBImageIO
::InitializeOrientationMap()
{
  m_OrientationMap["RIP"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_RIP;
  m_OrientationMap["LIP"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_LIP;
  m_OrientationMap["RSP"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_RSP;
  m_OrientationMap["LSP"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_LSP;
  m_OrientationMap["RIA"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_RIA;
  m_OrientationMap["LIA"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_LIA;
  m_OrientationMap["RSA"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_RSA;
  m_OrientationMap["LSA"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_LSA;
  m_OrientationMap["IRP"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_IRP;
  m_OrientationMap["ILP"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_ILP;
  m_OrientationMap["SRP"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_SRP;
  m_OrientationMap["SLP"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_SLP;
  m_OrientationMap["IRA"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_IRA;
  m_OrientationMap["ILA"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_ILA;
  m_OrientationMap["SRA"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_SRA;
  m_OrientationMap["SLA"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_SLA;
  m_OrientationMap["RPI"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_RPI;
  m_OrientationMap["LPI"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_LPI;
  m_OrientationMap["RAI"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAI;
  m_OrientationMap["LAI"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_LAI;
  m_OrientationMap["RPS"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_RPS;
  m_OrientationMap["LPS"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_LPS;
  m_OrientationMap["RAS"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAS;
  m_OrientationMap["LAS"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_LAS;
  m_OrientationMap["PRI"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_PRI;
  m_OrientationMap["PLI"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_PLI;
  m_OrientationMap["ARI"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_ARI;
  m_OrientationMap["ALI"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_ALI;
  m_OrientationMap["PRS"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_PRS;
  m_OrientationMap["PLS"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_PLS;
  m_OrientationMap["ARS"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_ARS;
  m_OrientationMap["ALS"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_ALS;
  m_OrientationMap["IPR"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_IPR;
  m_OrientationMap["SPR"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_SPR;
  m_OrientationMap["IAR"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_IAR;
  m_OrientationMap["SAR"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_SAR;
  m_OrientationMap["IPL"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_IPL;
  m_OrientationMap["SPL"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_SPL;
  m_OrientationMap["IAL"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_IAL;
  m_OrientationMap["SAL"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_SAL;
  m_OrientationMap["PIR"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_PIR;
  m_OrientationMap["PSR"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_PSR;
  m_OrientationMap["AIR"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_AIR;
  m_OrientationMap["ASR"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_ASR;
  m_OrientationMap["PIL"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_PIL;
  m_OrientationMap["PSL"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_PSL;
  m_OrientationMap["AIL"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_AIL;
  m_OrientationMap["ASL"] = SpatialOrientation::ITK_COORDINATE_ORIENTATION_ASL;

  OrientationMap::const_iterator it;
  for(it = m_OrientationMap.begin(); it != m_OrientationMap.end(); ++it)
    m_InverseOrientationMap[it->second] = it->first;

}

void 
VoxBoCUBImageIO
::SwapBytesIfNecessary(void *buffer, unsigned long numberOfBytes)
{
  if(m_ComponentType == CHAR)
    VoxBoCUBImageIOSwapHelper<char>::SwapIfNecessary(
      buffer, numberOfBytes, m_ByteOrder);
  else if(m_ComponentType == UCHAR)
    VoxBoCUBImageIOSwapHelper<unsigned char>::SwapIfNecessary(
      buffer, numberOfBytes, m_ByteOrder);
  else if(m_ComponentType == SHORT)
    VoxBoCUBImageIOSwapHelper<short>::SwapIfNecessary(
      buffer, numberOfBytes, m_ByteOrder);
  else if(m_ComponentType == USHORT)
    VoxBoCUBImageIOSwapHelper<unsigned short>::SwapIfNecessary(
      buffer, numberOfBytes, m_ByteOrder);
  else if(m_ComponentType == INT)
    VoxBoCUBImageIOSwapHelper<int>::SwapIfNecessary(
      buffer, numberOfBytes, m_ByteOrder);
  else if(m_ComponentType == UINT)
    VoxBoCUBImageIOSwapHelper<unsigned int>::SwapIfNecessary(
      buffer, numberOfBytes, m_ByteOrder);
  else if(m_ComponentType == LONG)
    VoxBoCUBImageIOSwapHelper<long>::SwapIfNecessary(
      buffer, numberOfBytes, m_ByteOrder);
  else if(m_ComponentType == ULONG)
    VoxBoCUBImageIOSwapHelper<unsigned long>::SwapIfNecessary(
      buffer, numberOfBytes, m_ByteOrder);
  else if(m_ComponentType == FLOAT)
    VoxBoCUBImageIOSwapHelper<float>::SwapIfNecessary(
      buffer, numberOfBytes, m_ByteOrder);
  else if(m_ComponentType == DOUBLE)
    VoxBoCUBImageIOSwapHelper<double>::SwapIfNecessary(
      buffer, numberOfBytes, m_ByteOrder);
  else 
    {
    ExceptionObject exception(__FILE__, __LINE__);
    exception.SetDescription("Pixel Type Unknown");
    throw exception;
    }
}

} // end namespace itk
