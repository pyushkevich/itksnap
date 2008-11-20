/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GuidedImageIO.cxx,v $
  Language:  C++
  Date:      $Date: 2008/11/20 04:24:00 $
  Version:   $Revision: 1.8 $
  Copyright (c) 2007 Paul A. Yushkevich
  
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
#include "GuidedImageIO.h"
#include "SNAPCommon.h"
#include "SNAPRegistryIO.h"
#include "ImageCoordinateGeometry.h"
#include "itkImage.h"

#include "itkImageIOBase.h"
#include "itkAnalyzeImageIO.h"
#include "itkGiplImageIO.h"
#include "itkMetaImageIO.h"
#include "itkNrrdImageIO.h"
#include "itkRawImageIO.h"
#include "itkGDCMImageIO.h"
#include "itkGE4ImageIO.h"
#include "itkGE5ImageIO.h"
#include "itkNiftiImageIO.h"
#include "itkSiemensVisionImageIO.h"
#include "itkVTKImageIO.h"
#include "itkVoxBoCUBImageIO.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImageSeriesReader.h"
#include "itkGDCMSeriesFileNames.h"

#include "itkMinimumMaximumImageCalculator.h"
#include "itkShiftScaleImageFilter.h"
#include "itkNumericTraits.h"


using namespace itk;
using namespace std;

const FileFormatDescriptor 
GuidedImageIOBase
::m_FileFormatDescrictorArray[] = {
  {"MetaImage", "mha,mhd",           true,  true,  true,  true},
  {"GIPL", "gipl,gipl.gz",           true,  false, true,  true},
  {"Raw Binary", "raw",              false, false, true,  true},
  {"Analyze", "hdr,img,img.gz",      true,  false, true,  true},
  {"DICOM", "dcm",                   false, true,  true,  true},
  {"GE Version 4", "ge4",            false, false, true,  true},
  {"GE Version 5", "ge5",            false, false, true,  true},
  {"NIFTI", "nii,nia,nii.gz,nia.gz", true,  true,  true,  true},
  {"Siemens Vision", "ima",          false, false, true,  true},
  {"VTK", "vtk",                     true,  false, true,  true},
  {"VoxBo CUB", "cub,cub.gz",        true,  false, true,  true},
  {"NRRD", "nrrd,nhdr",              true,  true,  true,  true},
  {"INVALID FORMAT", "",             false, false, false, false}};

GuidedImageIOBase
::GuidedImageIOBase()
{
  for(int i = 0; i < FORMAT_COUNT; i++)
    m_EnumFileFormat.AddPair(
      (FileFormat)(FORMAT_MHA + i), 
      m_FileFormatDescrictorArray[i].name.c_str());

  m_EnumRawPixelType.AddPair(PIXELTYPE_CHAR, "CHAR");
  m_EnumRawPixelType.AddPair(PIXELTYPE_UCHAR, "UCHAR");
  m_EnumRawPixelType.AddPair(PIXELTYPE_SHORT, "SHORT");
  m_EnumRawPixelType.AddPair(PIXELTYPE_USHORT, "USHORT");
  m_EnumRawPixelType.AddPair(PIXELTYPE_INT, "INT");
  m_EnumRawPixelType.AddPair(PIXELTYPE_UINT, "UINT");
  m_EnumRawPixelType.AddPair(PIXELTYPE_FLOAT, "FLOAT");
  m_EnumRawPixelType.AddPair(PIXELTYPE_DOUBLE, "DOUBLE");
  m_EnumRawPixelType.AddPair(PIXELTYPE_COUNT, "INVALID PIXEL TYPE");
}

GuidedImageIOBase::FileFormat 
GuidedImageIOBase
::GetFileFormat(Registry &folder, FileFormat dflt)
{
  return folder.Entry("Format").GetEnum(m_EnumFileFormat, dflt);  
}

void GuidedImageIOBase
::SetFileFormat(Registry &folder, FileFormat format)
{
  folder.Entry("Format").PutEnum(m_EnumFileFormat, format);
}

GuidedImageIOBase::RawPixelType 
GuidedImageIOBase
::GetPixelType(Registry &folder, RawPixelType dflt)
{
  return folder.Entry("Raw.PixelType").GetEnum(m_EnumRawPixelType, dflt);  
}

void GuidedImageIOBase
::SetPixelType(Registry &folder, RawPixelType type)
{
  folder.Entry("Raw.PixelType").PutEnum(m_EnumRawPixelType, type);
}


template<typename TRaw> 
void
GuidedImageIOBase::RawIOGenerator<TRaw>
::CreateRawImageIO(Registry &folder, typename ImageIOBase::Pointer &ptr)
{
  // Create the Raw IO
  typedef RawImageIO<TRaw,3> IOType;  
  typename IOType::Pointer rawIO = IOType::New();
  
  // Set the header size
  rawIO->SetHeaderSize(folder["HeaderSize"][0]);

  // Read the dimensions and other stuff from the registry
  Vector3i dims = folder["Dimensions"][Vector3i(0)];
  Vector3d spacing = folder["Spacing"][Vector3d(1.0)];
  Vector3d origin = folder["Origin"][Vector3d(0.0)];
  
  // Supply to the IO object
  for(unsigned int i = 0; i < 3; i++)
    {
    rawIO->SetDimensions(i, dims[i]);
    rawIO->SetSpacing(i, spacing[i]);
    rawIO->SetOrigin(i, origin[i]);
    }

  // Set the endianness
  if(folder["BigEndian"][true])
    rawIO->SetByteOrderToBigEndian();
  else
    rawIO->SetByteOrderToLittleEndian();

  // Set the other parameters
  rawIO->SetFileTypeToBinary();

  // Return the pointer
  ptr = rawIO;
}

template<typename TPixel>
void
GuidedImageIO<TPixel>
::CreateImageIO(Registry &folder, FileFormat &format)
{
  // Get the format specified in the folder
  format = GetFileFormat(folder);

  // Choose the approach based on the file format
  if(format == FORMAT_MHA)
    m_IOBase = MetaImageIO::New();
  else if(format == FORMAT_NRRD)
    m_IOBase = NrrdImageIO::New();
  else if(format == FORMAT_ANALYZE)
    m_IOBase = AnalyzeImageIO::New();
  else if(format == FORMAT_GIPL)
    m_IOBase = GiplImageIO::New();
  else if(format == FORMAT_DICOM)
    m_IOBase = GDCMImageIO::New();
  else if(format == FORMAT_GE4)
    m_IOBase = GE4ImageIO::New();
  else if(format == FORMAT_GE5)
    m_IOBase = GE5ImageIO::New();
  else if(format == FORMAT_NIFTI)
    m_IOBase = NiftiImageIO::New();
  else if(format == FORMAT_SIEMENS)
    m_IOBase = SiemensVisionImageIO::New();
  else if(format == FORMAT_VTK)
    m_IOBase = VTKImageIO::New();
  else if(format == FORMAT_VOXBO_CUB)
    m_IOBase = VoxBoCUBImageIO::New();
  else if(format == FORMAT_RAW)
    {
    // Get the Raw header sub-folder
    Registry fldRaw = folder.Folder("Raw");

    // Get the type of pixel from the registry
    RawPixelType type = 
      folder.Entry("Raw.PixelType").GetEnum(m_EnumRawPixelType, PIXELTYPE_COUNT);

    // Use header page values to initialize the RAW io
    if(type == PIXELTYPE_UCHAR) 
      GuidedImageIOBase::RawIOGenerator<unsigned char>::CreateRawImageIO(fldRaw, m_IOBase);
    else if(type == PIXELTYPE_CHAR) 
      GuidedImageIOBase::RawIOGenerator<char>::CreateRawImageIO(fldRaw, m_IOBase);
    else if(type == PIXELTYPE_USHORT) 
      GuidedImageIOBase::RawIOGenerator<unsigned short>::CreateRawImageIO(fldRaw, m_IOBase);
    else if(type == PIXELTYPE_SHORT) 
      GuidedImageIOBase::RawIOGenerator<short>::CreateRawImageIO(fldRaw, m_IOBase);
    else if(type == PIXELTYPE_UINT) 
      GuidedImageIOBase::RawIOGenerator<unsigned int>::CreateRawImageIO(fldRaw, m_IOBase);
    else if(type == PIXELTYPE_INT) 
      GuidedImageIOBase::RawIOGenerator<int>::CreateRawImageIO(fldRaw, m_IOBase);
    else if(type == PIXELTYPE_FLOAT) 
      GuidedImageIOBase::RawIOGenerator<float>::CreateRawImageIO(fldRaw, m_IOBase);
    else if(type == PIXELTYPE_DOUBLE) 
      GuidedImageIOBase::RawIOGenerator<double>::CreateRawImageIO(fldRaw, m_IOBase);
    else
      throw ExceptionObject("Unknown Pixel Type when reading Raw File");
    }
  else
    m_IOBase = NULL;
}

template<typename TPixel>
template<typename TScalar, typename TNative>
void 
GuidedImageIO<TPixel>::ReadAndCastImage()
{
  typedef Image<TNative, 3> InputImageType;
  typedef Image<TScalar, 3> OutputImageType;
  typedef ImageFileReader<InputImageType> ReaderType;

  // Create reader
  typename ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(m_IOBase->GetFileName());
  reader->SetImageIO(m_IOBase);

  // Read image in its native format
  reader->Update();
  typename InputImageType::Pointer input = reader->GetOutput();

  // If the types are identical, we are done
  if(typeid(TScalar) == typeid(TNative))
    {
    m_Image = dynamic_cast<ImageType *>(input.GetPointer());
    return;
    }

  // We must compute a scale and shift factor
  double scale = 1.0, shift = 0.0;

  // Compute the minimum and maximum of the input image
  typedef MinimumMaximumImageCalculator<InputImageType> CalcType;
  typename CalcType::Pointer calc = CalcType::New();
  calc->SetImage(input);
  calc->Compute();  
  TNative imin = calc->GetMinimum();
  TNative imax = calc->GetMaximum();
  TScalar omax = NumericTraits<TScalar>::max();
  TScalar omin = NumericTraits<TScalar>::min();

  // Now we have to be careful, depending on the type of the input voxel
  // For float and double, we map the input range into the output range
  if(!NumericTraits<TNative>::is_integer)
    {
    // Test whether the input image is actually an integer image cast to 
    // floating point. In that case, there is no need for conversion
    bool isint = false;
    if(1.0 * omin <= imin && 1.0 * omax >= imax)
      {
      isint = true;
      typedef itk::ImageRegionConstIterator<InputImageType> IteratorType;
      for(IteratorType it(input, input->GetBufferedRegion()); 
        !it.IsAtEnd(); ++it)
        {
        TNative vin = it.Get();
        TNative vcmp = static_cast<TNative>(static_cast<TScalar>(vin + 0.5));
        if(vin != vcmp)
          { isint = false; break; }
        }
      }

    // If underlying data is really integer, no scale or shift is necessary
    // except that to round (so floating values like 0.9999999 get mapped to 
    // 1 not to 0
    if(isint)
      {
      scale = 1.0; shift = 0.5;
      }

    // If the min and max are the same, we map that value to zero
    else if(imin == imax)
      {
      scale = 1.0; shift = -imax;
      }
    else
      {  
      // Compute the scaling factor to map image into output range
      scale = (1.0 * omax - 1.0 * omin) / (imax - imin);
      shift = omin / scale - imin;
      }
    }

  // For integer types we only need to take action if the range is outside
  // of the supported range. We cast to double to make sure the comparison 
  // is valid
  else if(1.0 * imin < 1.0 * omin || 1.0 * imax > omax)
    {
    // Can we solve the problem by a shift only?
    if(1.0 * imax - 1.0 * imin <= 1.0 * omax - 1.0 * omin)
      {
      scale = 1.0;
      shift = 1.0 * omin - 1.0 * imin;
      }
    }

  // Execute the scale/shift filter
  typedef ShiftScaleImageFilter<InputImageType, OutputImageType> MapType;
  typename MapType::Pointer mapper = MapType::New();
  mapper->SetInput(input);
  mapper->SetScale(scale);
  mapper->SetShift(shift);
  mapper->Update();
  typename OutputImageType::Pointer output = mapper->GetOutput();

  // Cast the output of the filter to the GuidedImageIO's pixel type 
  // (we have to do it like this to avoid templating problems)
  m_Image = dynamic_cast<ImageType *>(output.GetPointer());
  // Store the shift and the scale needed to take the TScalar values 
  // to the TNative values
  m_NativeScale = 1.0 / scale;
  m_NativeShift = - shift;
}


template<typename TPixel>
template<typename TScalar>
void
GuidedImageIO<TPixel>
::ReadFromNative()
{
  // These two types are meant to be the same
  assert(typeid(TScalar) == typeid(TPixel));

  // Read image in its native format and cast to the current format
  ImageIOBase::IOComponentType itype = m_IOBase->GetComponentType();
  switch(itype) 
    {
    case ImageIOBase::UCHAR:  ReadAndCastImage<TScalar,  unsigned char>(); break;
    case ImageIOBase::CHAR:   ReadAndCastImage<TScalar,    signed char>(); break;
    case ImageIOBase::USHORT: ReadAndCastImage<TScalar, unsigned short>(); break;
    case ImageIOBase::SHORT:  ReadAndCastImage<TScalar,   signed short>(); break;
    case ImageIOBase::UINT:   ReadAndCastImage<TScalar,   unsigned int>(); break;
    case ImageIOBase::INT:    ReadAndCastImage<TScalar,     signed int>(); break;
    case ImageIOBase::ULONG:  ReadAndCastImage<TScalar,  unsigned long>(); break;
    case ImageIOBase::LONG:   ReadAndCastImage<TScalar,    signed long>(); break;
    case ImageIOBase::FLOAT:  ReadAndCastImage<TScalar,          float>(); break;
    case ImageIOBase::DOUBLE: ReadAndCastImage<TScalar,         double>(); break;
    default: 
      throw ExceptionObject("Unknown Pixel Type when reading image");
    }

}





                 


template<typename TPixel>
typename GuidedImageIO<TPixel>::ImageType*
GuidedImageIO<TPixel>
::ReadImage(const char *FileName, Registry &folder, bool flagCastFromNative)
{
  // Initialize the scaling and shift to 1 and 0
  m_NativeScale = 1.0; m_NativeShift = 0.0;

  // Create the header corresponding to the current image type
  FileFormat format;
  CreateImageIO(folder, format);

  // There is a special handler for the DICOM case!
  if(format == FORMAT_DICOM)
    {
    // Create an image series reader 
    typedef ImageSeriesReader<ImageType> ReaderType;
    typename ReaderType::Pointer reader = ReaderType::New();

    // Set the IO
    reader->SetImageIO(m_IOBase);

    // Check if the array of filenames has been provided for us
    FilenamesContainer fids = 
      folder.Folder("DICOM.SliceFiles").GetArray(std::string("NULL"));
    
    // If no filenames were specified, read the first series in the directory
    if(fids.size() == 0)
      {
      // Create a names generator. The input must be a directory 
      typedef GDCMSeriesFileNames NamesGeneratorType;
      NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();
      nameGenerator->SetDirectory(FileName);

      // Get the list of series in the directory
      const SerieUIDContainer &sids = nameGenerator->GetSeriesUIDs();

      // There must be at least of series
      if(sids.size() == 0)
        throw ExceptionObject("No DICOM series found in the DICOM directory");
    
      // Read the first DICOM series in the directory
      fids = nameGenerator->GetFileNames(sids.front().c_str());
      }
    
    // Check that there are filenames
    if(fids.size() == 0)
      throw ExceptionObject("No DICOM files found in the DICOM directory");
    
    // Set the filenames and read
    reader->SetFileNames(fids);
    reader->Update();

    // Get the output image
    m_Image = reader->GetOutput();
    } 
  else
    {
    // Try reading the file in our own format 
    typedef ImageFileReader<ImageType> ReaderType;
    typename ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName(FileName);
    if(m_IOBase)
      reader->SetImageIO(m_IOBase);
    reader->Update();

    // If the internal format does not match, try native read
    m_Image = reader->GetOutput();
    m_IOBase = reader->GetImageIO();

    // If we are asked to read in native format
    if(flagCastFromNative && m_IOBase->GetComponentTypeInfo() != typeid(TPixel) )
      {
      // Select the right templated function
      if     (typeid(TPixel) == typeid( unsigned char)) this->ReadFromNative< unsigned char>();
      else if(typeid(TPixel) == typeid(   signed char)) this->ReadFromNative<   signed char>();
      else if(typeid(TPixel) == typeid(unsigned short)) this->ReadFromNative<unsigned short>();
      else if(typeid(TPixel) == typeid(  signed short)) this->ReadFromNative<  signed short>();
      else if(typeid(TPixel) == typeid(  unsigned int)) this->ReadFromNative<  unsigned int>();
      else if(typeid(TPixel) == typeid(    signed int)) this->ReadFromNative<    signed int>();
      else if(typeid(TPixel) == typeid( unsigned long)) this->ReadFromNative< unsigned long>();
      else if(typeid(TPixel) == typeid(   signed long)) this->ReadFromNative<   signed long>();
      else assert(0);
      }
    }

  // Disconnect the image from the readers, allowing them to be deleted
  m_Image->DisconnectPipeline();

  // Return the image pointer
  return m_Image;
}

template<typename TPixel>
std::string
GuidedImageIO<TPixel>
::GetRAICode(ImageType *image, Registry &folder)
{
  // See what's stored in the registry
  std::string rai_registry = folder["Orientation"][""];
  if(ImageCoordinateGeometry::IsRAICodeValid(rai_registry.c_str()))
    return rai_registry;

  // Compute the RAI code from the direction cosines
  return ImageCoordinateGeometry::
    ConvertDirectionMatrixToClosestRAICode(
      m_Image->GetDirection().GetVnlMatrix());
}

template<typename TPixel>
void
GuidedImageIO<TPixel>
::SaveImage(const char *FileName, Registry &folder, ImageType *image)
{
  // Create an Image IO based on the folder
  FileFormat format;
  CreateImageIO(folder, format);

  // Save the image
  typedef ImageFileWriter<ImageType> WriterType;
  typename WriterType::Pointer writer = WriterType::New();
  
  writer->SetFileName(FileName);
  if(m_IOBase)
    writer->SetImageIO(m_IOBase);
  writer->SetInput(image);
  writer->Update();
}

// Instantiate the classes
template class GuidedImageIO<GreyType>;
template class GuidedImageIO<RGBType>;
template class GuidedImageIO<LabelType>;
template class GuidedImageIO<float>;
