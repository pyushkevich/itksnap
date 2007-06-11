/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: GuidedImageIO.cxx,v $
  Language:  C++
  Date:      $Date: 2007/06/11 15:51:15 $
  Version:   $Revision: 1.3 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "GuidedImageIO.h"
#include "SNAPCommon.h"
#include "SNAPRegistryIO.h"
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

using namespace itk;
using namespace std;

GuidedImageIOBase
::GuidedImageIOBase()
{
  m_EnumFileFormat.AddPair(FORMAT_MHA, "MetaImage");
  m_EnumFileFormat.AddPair(FORMAT_NRRD, "NRRD");
  m_EnumFileFormat.AddPair(FORMAT_GIPL, "GIPL");
  m_EnumFileFormat.AddPair(FORMAT_RAW, "Raw Binary");
  m_EnumFileFormat.AddPair(FORMAT_ANALYZE, "Analyze"); 
  m_EnumFileFormat.AddPair(FORMAT_DICOM, "DICOM");
  m_EnumFileFormat.AddPair(FORMAT_GE4, "GE Version 4");
  m_EnumFileFormat.AddPair(FORMAT_GE5, "GE Version 5");
  m_EnumFileFormat.AddPair(FORMAT_NIFTI, "NIFTI");
  m_EnumFileFormat.AddPair(FORMAT_SIEMENS, "Siemens Vision");
  m_EnumFileFormat.AddPair(FORMAT_VTK, "VTK");
  m_EnumFileFormat.AddPair(FORMAT_VOXBO_CUB, "VoxBo CUB");
  m_EnumFileFormat.AddPair(FORMAT_COUNT, "INVALID FORMAT");

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
ImageIOBase *
GuidedImageIOBase::RawIOGenerator<TRaw>
::CreateRawImageIO(Registry &folder)
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
  return rawIO.GetPointer();
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
      m_IOBase = GuidedImageIOBase::RawIOGenerator<unsigned char>::CreateRawImageIO(fldRaw);
    else if(type == PIXELTYPE_CHAR) 
      m_IOBase = GuidedImageIOBase::RawIOGenerator<char>::CreateRawImageIO(fldRaw);
    else if(type == PIXELTYPE_USHORT) 
      m_IOBase = GuidedImageIOBase::RawIOGenerator<unsigned short>::CreateRawImageIO(fldRaw);
    else if(type == PIXELTYPE_SHORT) 
      m_IOBase = GuidedImageIOBase::RawIOGenerator<short>::CreateRawImageIO(fldRaw);
    else if(type == PIXELTYPE_UINT) 
      m_IOBase = GuidedImageIOBase::RawIOGenerator<unsigned int>::CreateRawImageIO(fldRaw);
    else if(type == PIXELTYPE_INT) 
      m_IOBase = GuidedImageIOBase::RawIOGenerator<int>::CreateRawImageIO(fldRaw);
    else if(type == PIXELTYPE_FLOAT) 
      m_IOBase = GuidedImageIOBase::RawIOGenerator<float>::CreateRawImageIO(fldRaw);
    else if(type == PIXELTYPE_DOUBLE) 
      m_IOBase = GuidedImageIOBase::RawIOGenerator<double>::CreateRawImageIO(fldRaw);
    else
      throw itk::ExceptionObject("Unknown Pixel Type when reading Raw File");
    }
  else
    m_IOBase = NULL;
}

template<typename TPixel>
typename GuidedImageIO<TPixel>::ImageType*
GuidedImageIO<TPixel>
::ReadImage(const char *FileName, Registry &folder)
{
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
    // Just read a single image
    typedef ImageFileReader<ImageType> ReaderType;
    typename ReaderType::Pointer reader = ReaderType::New();
  
    // Configure the reader
    reader->SetFileName(FileName);
    if(m_IOBase)
      reader->SetImageIO(m_IOBase);
  
    // Update the reader
    reader->Update();
    m_Image = reader->GetOutput();   
    }

  // Disconnect the image from the readers, allowing them to be deleted
  m_Image->DisconnectPipeline();

  // Return the image pointer
  return m_Image;
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
  typedef itk::ImageFileWriter<ImageType> WriterType;
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
