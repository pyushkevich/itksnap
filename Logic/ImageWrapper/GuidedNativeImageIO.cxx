/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GuidedNativeImageIO.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/14 16:21:04 $
  Version:   $Revision: 1.11 $
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
#include "GuidedNativeImageIO.h"
#include "DICOMSerieUniqueIDsMgr.h"
#include "IRISException.h"
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
#include "itkImageIOFactory.h"
#include "itkGDCMSeriesFileNames.h"
#include "gdcmFile.h"
#include "gdcmScanner.h"
#include "gdcmSmartPointer.h"
#include "gdcmSorter.h"

#include "itkGDCMSeriesFileNames.h"
#include "itkImageToVectorImageFilter.h"
#include "itkMinimumMaximumImageCalculator.h"
#include "itkShiftScaleImageFilter.h"
#include "itkNumericTraits.h"

#include <itk_zlib.h>


using namespace std;

bool GuidedNativeImageIO::m_StaticDataInitialized = false;

RegistryEnumMap<GuidedNativeImageIO::FileFormat> GuidedNativeImageIO::m_EnumFileFormat;
RegistryEnumMap<GuidedNativeImageIO::RawPixelType> GuidedNativeImageIO::m_EnumRawPixelType;

const GuidedNativeImageIO::FileFormatDescriptor 
GuidedNativeImageIO
::m_FileFormatDescrictorArray[] = {
  {"Analyze", "hdr,img,img.gz",      true,  false, true,  true},
  {"DICOM", "dcm",                   false, true,  true,  true},
  {"GE Version 4", "ge4",            false, false, true,  true},
  {"GE Version 5", "ge5",            false, false, true,  true},
  {"GIPL", "gipl,gipl.gz",           true,  false, true,  true},
  {"MetaImage", "mha,mhd",           true,  true,  true,  true},
  {"NiFTI", "nii,nia,nii.gz,nia.gz", true,  true,  true,  true},
  {"NRRD", "nrrd,nhdr",              true,  true,  true,  true},
  {"Raw Binary", "raw",              false, false, true,  true},
  {"Siemens Vision", "ima",          false, false, true,  true},
  {"VoxBo CUB", "cub,cub.gz",        true,  false, true,  true},
  {"VTK Image", "vtk",               true,  false, true,  true},
  {"INVALID FORMAT", "",             false, false, false, false}};



bool GuidedNativeImageIO::FileFormatDescriptor
::TestFilename(std::string fname)
{
  // Check if the filename matches the pattern
  for(size_t i = 0; i < pattern.length(); )
    {
    size_t j = pattern.find(',', i);
    string ext = "." + pattern.substr(i, j-i);
    i+=ext.length();
    if(fname.rfind(ext) == fname.length()-ext.length())
      return true;
    }
  return false;
}

GuidedNativeImageIO
::GuidedNativeImageIO()
{
  if(!m_StaticDataInitialized)
    {
    for(int i = 0; i < FORMAT_COUNT; i++)
      m_EnumFileFormat.AddPair(
        (FileFormat)(i),
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

    m_StaticDataInitialized = true;
    }

  m_NativeType = itk::ImageIOBase::UNKNOWNCOMPONENTTYPE;
  m_NativeComponents = 0;
  m_NativeTypeString = m_IOBase->GetComponentTypeAsString(m_NativeType);
  m_NativeFileName = "";
  m_NativeByteOrder = itk::ImageIOBase::OrderNotApplicable;
  m_NativeSizeInBytes = 0;
}

GuidedNativeImageIO::FileFormat 
GuidedNativeImageIO
::GetFileFormat(Registry &folder, FileFormat dflt)
{
  return folder.Entry("Format").GetEnum(m_EnumFileFormat, dflt);  
}

void GuidedNativeImageIO
::SetFileFormat(Registry &folder, FileFormat format)
{
  folder.Entry("Format").PutEnum(m_EnumFileFormat, format);
}

GuidedNativeImageIO::RawPixelType 
GuidedNativeImageIO
::GetPixelType(Registry &folder, RawPixelType dflt)
{
  return folder.Entry("Raw.PixelType").GetEnum(m_EnumRawPixelType, dflt);  
}

void GuidedNativeImageIO
::SetPixelType(Registry &folder, RawPixelType type)
{
  folder.Entry("Raw.PixelType").PutEnum(m_EnumRawPixelType, type);
}

size_t
GuidedNativeImageIO
::GetNumberOfComponentsInNativeImage() const
{
  return m_NativeComponents;
}

template<typename TRaw> 
void
GuidedNativeImageIO
::CreateRawImageIO(Registry &folder)
{
  // Create the Raw IO
  typedef itk::RawImageIO<TRaw,3> IOType;
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

  // Set the number of components
  rawIO->SetNumberOfComponents(folder["Components"][1]);

  // Set the other parameters
  rawIO->SetFileTypeToBinary();

  // Return the pointer
  m_IOBase = rawIO;
}


void
GuidedNativeImageIO
::CreateImageIO(const char *fname, Registry &folder, bool flag_read)
{
  // Get the format specified in the folder
  m_FileFormat = GetFileFormat(folder);

  // Choose the approach based on the file format
  switch(m_FileFormat)
    {
    case FORMAT_MHA:        m_IOBase = itk::MetaImageIO::New();          break;
    case FORMAT_NRRD:       m_IOBase = itk::NrrdImageIO::New();          break;
    case FORMAT_ANALYZE:    m_IOBase = itk::AnalyzeImageIO::New();       break;
    case FORMAT_GIPL:       m_IOBase = itk::GiplImageIO::New();          break;
    case FORMAT_GE4:        m_IOBase = itk::GE4ImageIO::New();           break;
    case FORMAT_GE5:        m_IOBase = itk::GE5ImageIO::New();           break;
    case FORMAT_NIFTI:      m_IOBase = itk::NiftiImageIO::New();         break;
    case FORMAT_SIEMENS:    m_IOBase = itk::SiemensVisionImageIO::New(); break;
    case FORMAT_VTK:        m_IOBase = itk::VTKImageIO::New();           break;
    case FORMAT_VOXBO_CUB:  m_IOBase = itk::VoxBoCUBImageIO::New();      break;
    case FORMAT_DICOM:      m_IOBase = itk::GDCMImageIO::New();          break;
    case FORMAT_RAW:
      {
      // Get the Raw header sub-folder
      Registry fldRaw = folder.Folder("Raw");

      // Get the type of pixel from the registry
      RawPixelType type = 
        folder.Entry("Raw.PixelType").GetEnum(m_EnumRawPixelType, PIXELTYPE_COUNT);

      // Use header page values to initialize the RAW io
      switch(type)
        {
        case PIXELTYPE_UCHAR:  CreateRawImageIO<unsigned char>(fldRaw);  break;
        case PIXELTYPE_CHAR:   CreateRawImageIO<char>(fldRaw);           break;
        case PIXELTYPE_USHORT: CreateRawImageIO<unsigned short>(fldRaw); break;
        case PIXELTYPE_SHORT:  CreateRawImageIO<short>(fldRaw);          break;
        case PIXELTYPE_UINT:   CreateRawImageIO<unsigned int>(fldRaw);   break;
        case PIXELTYPE_INT:    CreateRawImageIO<int>(fldRaw);            break;
        case PIXELTYPE_FLOAT:  CreateRawImageIO<float>(fldRaw);          break;
        case PIXELTYPE_DOUBLE: CreateRawImageIO<double>(fldRaw);         break;
        default:
          throw IRISException("Error: Unsupported voxel type."
                              "Unsupported voxel type ('%s') when reading raw file.",
                              folder["Raw.PixelType"][""]);
        }
      }
      break;
    default:
      {
      // No IO base was specified in the registry folder. We will use ITK's factory
      // system to find an IO object that can open the file
      m_IOBase = itk::ImageIOFactory::CreateImageIO(fname, 
        flag_read ? itk::ImageIOFactory::ReadMode : itk::ImageIOFactory::WriteMode);
      }
    }
}


void
GuidedNativeImageIO
::ReadNativeImageHeader(const char *FileName, Registry &folder)
{
  // Save the hints
  m_Hints = folder;

  // Create the header corresponding to the current image type
  CreateImageIO(FileName, m_Hints, true);
  if(!m_IOBase)
    throw IRISException("Error: Unsupported or missing image file format. "
                        "ITK-SNAP failed to create an ImageIO object for the "
                        "image '%s' using format '%s'.",
                        FileName, m_Hints["Format"][""]);

  // Read the information about the image
  if(m_FileFormat == FORMAT_DICOM)
    {
    // Check if the array of filenames has been provided for us
    m_DICOMFiles =
      m_Hints.Folder("DICOM.SeriesFiles").GetArray(std::string("NULL"));

    // If no filenames were specified, read the first series in the directory
    if(m_DICOMFiles.size() == 0)
      {
      // Create a names generator. The input must be a directory
      typedef itk::GDCMSeriesFileNames NamesGeneratorType;
      NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();
      nameGenerator->SetDirectory(FileName);

      // Get the list of series in the directory
      const itk::SerieUIDContainer &sids = nameGenerator->GetSeriesUIDs();

      // There must be at least of series
      if(sids.size() == 0)
        throw IRISException("Error: DICOM series not found. "
                            "Directory '%s' does not appear to contain a "
                            "series of DICOM images.",FileName);

      // Read the first DICOM series in the directory
      m_DICOMFiles = nameGenerator->GetFileNames(sids.front().c_str());
      }

    // Read the information from the first filename
    if(m_DICOMFiles.size() == 0)
      throw IRISException("Error: DICOM series not found. "
                          "Directory '%s' does not appear to contain a "
                          "series of DICOM images.",FileName);
    m_IOBase->SetFileName(m_DICOMFiles[0]);
    m_IOBase->ReadImageInformation();
    }
  else
    {
    // Check that the reader actually supports this format. We skip this for
    // the RAW format, because it stupidly refuses to read files named other
    // than with the .raw extension
    if(m_FileFormat != FORMAT_RAW && !m_IOBase->CanReadFile(FileName))
      throw IRISException(
          "Error: Wrong Format. "
          "The IO library for the format '%s' can not read the image file.",
          m_Hints["Format"][""]);
    m_IOBase->SetFileName(FileName);
    m_IOBase->ReadImageInformation();
    }

  // Set the dimensions (if 2D image, we set last dim to 1)
  m_NativeDimensions.fill(1);
  for(size_t i = 0; i < m_IOBase->GetNumberOfDimensions() && i < 3; i++)
    {
    m_NativeDimensions[i] = m_IOBase->GetDimensions(i);
    }

  // Extract properties from IO base
  m_NativeType = m_IOBase->GetComponentType();
  m_NativeComponents = m_IOBase->GetNumberOfComponents();
  m_NativeTypeString = m_IOBase->GetComponentTypeAsString(m_NativeType);
  m_NativeFileName = m_IOBase->GetFileName();
  m_NativeByteOrder = m_IOBase->GetByteOrder();
  m_NativeSizeInBytes = m_IOBase->GetImageSizeInBytes();

  // Also pull out a nickname for this file, if it's in the folder
  m_NativeNickname = m_Hints["Nickname"][""];
}

void
GuidedNativeImageIO
::ReadNativeImageData()
{
  const char *fname = m_NativeFileName.c_str();

  // Based on the component type, read image in native mode
  switch(m_IOBase->GetComponentType())
    {
    case itk::ImageIOBase::UCHAR:  DoReadNative<unsigned char>(fname, m_Hints);  break;
    case itk::ImageIOBase::CHAR:   DoReadNative<signed char>(fname, m_Hints);    break;
    case itk::ImageIOBase::USHORT: DoReadNative<unsigned short>(fname, m_Hints); break;
    case itk::ImageIOBase::SHORT:  DoReadNative<signed short>(fname, m_Hints);   break;
    case itk::ImageIOBase::UINT:   DoReadNative<unsigned int>(fname, m_Hints);   break;
    case itk::ImageIOBase::INT:    DoReadNative<signed int>(fname, m_Hints);     break;
    case itk::ImageIOBase::ULONG:  DoReadNative<unsigned long>(fname, m_Hints);  break;
    case itk::ImageIOBase::LONG:   DoReadNative<signed long>(fname, m_Hints);    break;
    case itk::ImageIOBase::FLOAT:  DoReadNative<float>(fname, m_Hints);          break;
    case itk::ImageIOBase::DOUBLE: DoReadNative<double>(fname, m_Hints);         break;
    default:
      throw IRISException("Error: Unsupported voxel type."
                          "Unsupported voxel type ('%s') when reading raw file.",
                          m_IOBase->GetComponentTypeAsString(
                            m_IOBase->GetComponentType()).c_str());
    }

  // Get rid of the IOBase, it may store useless data (in case of NIFTI)
  m_IOBase = NULL;
}

void
GuidedNativeImageIO
::ReadNativeImage(const char *FileName, Registry &folder)
{
  this->ReadNativeImageHeader(FileName, folder);
  this->ReadNativeImageData();
}

template<class TScalar>
void
GuidedNativeImageIO
::DoReadNative(const char *FileName, Registry &folder)
{
  // Define the image type of interest
  typedef itk::VectorImage<TScalar, 3> NativeImageType;

  // There is a special handler for the DICOM case!
  if(m_FileFormat == FORMAT_DICOM && m_DICOMFiles.size() > 1)
    {
    // It seems that ITK can't yet read DICOM into a VectorImage. 
    typedef itk::Image<TScalar, 3> GreyImageType;

    // Create an image series reader 
    typedef itk::ImageSeriesReader<GreyImageType> ReaderType;
    typename ReaderType::Pointer reader = ReaderType::New();

    // Set the filenames and read
    reader->SetFileNames(m_DICOMFiles);
      
    // Set the IO
    // typename GDCMImageIO::Pointer dicomio = GDCMImageIO::New();
    // dicomio->SetMaxSizeLoadEntry(0xffff);
    // m_IOBase = dicomio;
    reader->SetImageIO(m_IOBase);
    
    // Update
    reader->Update();
    typename GreyImageType::Pointer scalar = reader->GetOutput();

    // Convert the image into VectorImage format. Do this in-place to avoid
    // allocating memory pointlessly
    typename NativeImageType::Pointer vector = NativeImageType::New();
    m_NativeImage = vector;

    vector->CopyInformation(scalar);
    vector->SetRegions(scalar->GetBufferedRegion());

    typedef typename NativeImageType::PixelContainer PixConType;
    typename PixConType::Pointer pc = PixConType::New();
    pc->SetImportPointer(
          reinterpret_cast<TScalar *>(scalar->GetBufferPointer()),
          scalar->GetBufferedRegion().GetNumberOfPixels(), true);
    vector->SetPixelContainer(pc);

    // Prevent the container from being deleted
    scalar->GetPixelContainer()->SetContainerManageMemory(false);

    // Copy the metadata from the first scan in the series
    const typename ReaderType::DictionaryArrayType *darr = 
      reader->GetMetaDataDictionaryArray();
    if(darr->size() > 0)
      m_NativeImage->SetMetaDataDictionary(*((*darr)[0]));
    } 
  else 
    {
    // Non-DICOM: read from single image
    // We no longer use ImageFileReader here because of an issue: the 
    // m_IOBase may have an open file handle (from call to ReadImageInfo)
    // so passing it in to the Reader would cause an IO error (this actually
    // happens for GIPL). So we copy some of the code from ImageFileReader

    // Create the native image
    typename NativeImageType::Pointer image = NativeImageType::New();

    // Initialize the direction and spacing, etc
    typename NativeImageType::SizeType dim;      dim.Fill(1);
    typename NativeImageType::PointType org;     org.Fill(0.0);
    typename NativeImageType::SpacingType spc;   spc.Fill(1.0);
    typename NativeImageType::DirectionType dir; dir.SetIdentity();    
    
    size_t nd = m_IOBase->GetNumberOfDimensions(); 
    if(nd > 3) nd = 3;
    
    for(unsigned int i = 0; i < nd; i++)
      {
      spc[i] = m_IOBase->GetSpacing(i);
      org[i] = m_IOBase->GetOrigin(i);
      for(size_t j = 0; j < nd; j++)
        dir(j,i) = m_IOBase->GetDirection(i)[j];
      dim[i] = m_IOBase->GetDimensions(i);
      }

    image->SetSpacing(spc);
    image->SetOrigin(org);
    image->SetDirection(dir);
    image->SetMetaDataDictionary(m_IOBase->GetMetaDataDictionary());

    // Set the regions and allocate
    typename NativeImageType::RegionType region;
    typename NativeImageType::IndexType index = {{0, 0, 0}};
    region.SetIndex(index);
    region.SetSize(dim);
    image->SetRegions(region);
    image->SetVectorLength(m_IOBase->GetNumberOfComponents());
    image->Allocate();

    // Set the IO region
    itk::ImageIORegion ioRegion(3);
    itk::ImageIORegionAdaptor<3>::Convert(region, ioRegion, index);
    m_IOBase->SetIORegion(ioRegion);

    // Read the image into the buffer
    m_IOBase->Read(image->GetBufferPointer());
    m_NativeImage = image;
    
    /*
    typedef ImageFileReader<NativeImageType> ReaderType;
    typename ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName(FileName);
    reader->SetImageIO(m_IOBase);
    reader->Update();
    m_NativeImage = reader->GetOutput();
    */
    }   

  // Disconnect the image from the readers, allowing them to be deleted
  // m_NativeImage->DisconnectPipeline();

  // Sometimes images have negative voxel spacing, which SNAP does not recognize
  // Check if voxel spacings need to be regularized
  typename NativeImageType::DirectionType direction = m_NativeImage->GetDirection();
  typename NativeImageType::SpacingType spacing = m_NativeImage->GetSpacing();
  typename NativeImageType::DirectionType factor;
  factor.SetIdentity();
  bool needRegularization = false;
  for (int i = 0; i < 3; ++i)
    {
    if (spacing[i] < 0)
      {
      spacing[i] *= -1.0;
      factor[i][i] *= -1.0;
      needRegularization = true;
      }
    }
  if (needRegularization)
    {
    direction *= factor;
    m_NativeImage->SetDirection(direction);
    m_NativeImage->SetSpacing(spacing);
    }
}

/*
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
*/

template<typename TPixel>
void
GuidedNativeImageIO
::SaveImage(const char *FileName, Registry &folder, itk::Image<TPixel,3> *image)
{
  // Create an Image IO based on the folder
  CreateImageIO(FileName, folder, false);

  // Save the image
  typedef itk::ImageFileWriter< itk::Image<TPixel,3> > WriterType;
  typename WriterType::Pointer writer = WriterType::New();
  
  writer->SetFileName(FileName);
  if(m_IOBase)
    writer->SetImageIO(m_IOBase);
  writer->SetInput(image);
  writer->Update();
}







/*****************************************************************************
 * ADAPTER OBJECTS TO CAST NATIVE IMAGE TO GIVEN IMAGE
 ****************************************************************************/

template<typename TPixel>
typename RescaleNativeImageToScalar<TPixel>::OutputImageType *
RescaleNativeImageToScalar<TPixel>::operator()(GuidedNativeImageIO *nativeIO)
{
  // Get the native image pointer
  itk::ImageBase<3> *native = nativeIO->GetNativeImage();

  // Allocate the output image
  m_Output = OutputImageType::New();
  m_Output->CopyInformation(native);
  m_Output->SetMetaDataDictionary(native->GetMetaDataDictionary());
  m_Output->SetRegions(native->GetBufferedRegion());

  // Cast image from native format to TPixel
  itk::ImageIOBase::IOComponentType itype = nativeIO->GetComponentTypeInNativeImage();
  switch(itype) 
    {
    case itk::ImageIOBase::UCHAR:  DoCast<unsigned char>(native);   break;
    case itk::ImageIOBase::CHAR:   DoCast<signed char>(native);     break;
    case itk::ImageIOBase::USHORT: DoCast<unsigned short>(native);  break;
    case itk::ImageIOBase::SHORT:  DoCast<signed short>(native);    break;
    case itk::ImageIOBase::UINT:   DoCast<unsigned int>(native);    break;
    case itk::ImageIOBase::INT:    DoCast<signed int>(native);      break;
    case itk::ImageIOBase::ULONG:  DoCast<unsigned long>(native);   break;
    case itk::ImageIOBase::LONG:   DoCast<signed long>(native);     break;
    case itk::ImageIOBase::FLOAT:  DoCast<float>(native);           break;
    case itk::ImageIOBase::DOUBLE: DoCast<double>(native);          break;
    default: 
      throw IRISException("Unknown pixel type when reading image");
    }

  // Return the output image
  return m_Output;
}

template<typename TPixel, typename TNative>
class RescaleScalarNativeImageToScalarFunctor
{
public:

  RescaleScalarNativeImageToScalarFunctor(double shift, double scale)
    : m_Shift(shift), m_Scale(scale) {}

  RescaleScalarNativeImageToScalarFunctor()
    : m_Shift(0), m_Scale(1) {}

  void operator()(TNative *src, TPixel *trg)
  {
    *trg = (TPixel) ((*src + m_Shift) * m_Scale + 0.5);
  }

  size_t GetNumberOfDimensions() { return 1; }

protected:

  double m_Shift, m_Scale;

};


template<typename TPixel, typename TNative>
class RescaleVectorNativeImageToScalarFunctor
{
public:

  RescaleVectorNativeImageToScalarFunctor(double shift, double scale, size_t ncomp)
    : m_Shift(shift), m_Scale(scale), m_Components(ncomp) {}

  RescaleVectorNativeImageToScalarFunctor()
    : m_Shift(0), m_Scale(1), m_Components(0) {}

  void operator()(TNative *src, TPixel *trg)
  {
    double x = 0;
    for(size_t i = 0; i < m_Components; i++)
      x += src[i] * src[i];
    *trg = (TPixel) ((sqrt(x) + m_Shift) * m_Scale + 0.5);
  }

  size_t GetNumberOfDimensions() { return m_Components; }

protected:

  double m_Shift, m_Scale;
  size_t m_Components;

};

template<typename TPixel>
template<typename TNative>
void
RescaleNativeImageToScalar<TPixel>
::DoCast(itk::ImageBase<3> *native)
{
  // Get the native image
  typedef itk::VectorImage<TNative, 3> InputImageType;
  typedef itk::ImageRegionConstIterator<InputImageType> InputIterator;
  SmartPtr<InputImageType> input = dynamic_cast<InputImageType *>(native);
  assert(input);

  size_t nvoxels = input->GetBufferedRegion().GetNumberOfPixels();
  size_t ncomp = input->GetNumberOfComponentsPerPixel();

  // We must compute a scale and shift factor
  double scale = 1.0, shift = 0.0;

  // Only bother with computing the scale and shift if the types are different
  if(typeid(TPixel) != typeid(TNative))
    {
    TNative *ib = input->GetBufferPointer();

    // We must compute the range of the input data
    double imin = itk::NumericTraits<double>::max();
    double imax = -itk::NumericTraits<double>::max();
    TPixel omax = itk::NumericTraits<TPixel>::max();
    TPixel omin = itk::NumericTraits<TPixel>::min();

    if(ncomp > 1)
      {
      for(InputIterator it(input, input->GetBufferedRegion()); !it.IsAtEnd(); ++it)
        {
        double mag = it.Get().GetSquaredNorm();
        if(mag < imin) imin = mag;
        if(mag > imax) imax = mag;
        }
      imin = sqrt(imin);
      imax = sqrt(imax);
      }
    else
      {
      for(size_t i = 0; i < nvoxels; i++)
        {
        // Have to cast to double here
        double val = ib[i];
        if(val < imin) imin = val;
        if(val > imax) imax = val;
        }
      }


    // Now we have to be careful, depending on the type of the input voxel
    // For float and double, we map the input range into the output range
    if(!itk::NumericTraits<TNative>::is_integer || ncomp > 1)
      {
      // Test whether the input image is actually an integer image cast to
      // floating point. In that case, there is no need for conversion
      bool isint = false;
      if(1.0 * omin <= imin && 1.0 * omax >= imax && ncomp == 1)
        {
        isint = true;
        for(size_t i = 0; i < nvoxels; i++)
          {
          TNative vin = ib[i];
          TNative vcmp = static_cast<TNative>(static_cast<TPixel>(vin + 0.5));
          if(vin != vcmp)
            { isint = false; break; }
          }
        }

      // If underlying data is really integer, no scale or shift is necessary
      // except that to round (so floating values like 0.9999999 get mapped to
      // 1 not to 0
      if(isint)
        {
        scale = 1.0; shift = 0.0;
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
    }


  // Store the shift and the scale needed to take the TPixel values
  // to the TNative values
  m_NativeScale = 1.0 / scale;
  m_NativeShift = - shift;

  // Create a cast functor. Note that if TPixel == TNative, the functor will
  // not be used because the CastNativeImageBase::DoCast will just assign the
  // input pixel container to the output image
  if(ncomp == 1)
    {
    typedef RescaleScalarNativeImageToScalarFunctor<TPixel, TNative> Functor;
    CastNativeImageBase<TPixel, Functor> caster;
    caster.SetFunctor(Functor(shift, scale));
    caster.template DoCast<TNative>(native);
    m_Output = caster.m_Output;
    }
  else
    {
    typedef RescaleVectorNativeImageToScalarFunctor<TPixel, TNative> Functor;
    CastNativeImageBase<TPixel, Functor> caster;
    caster.SetFunctor(Functor(shift, scale, ncomp));
    caster.template DoCast<TNative>(native);
    m_Output = caster.m_Output;
    }
}

template<class TPixel, class TCastFunctor>
typename CastNativeImageBase<TPixel,TCastFunctor>::OutputImageType *
CastNativeImageBase<TPixel,TCastFunctor>
::operator()(GuidedNativeImageIO *nativeIO)
{
  // Get the native image pointer
  itk::ImageBase<3> *native = nativeIO->GetNativeImage();

  // Cast image from native format to TPixel
  itk::ImageIOBase::IOComponentType itype = nativeIO->GetComponentTypeInNativeImage();
  switch(itype) 
    {
    case itk::ImageIOBase::UCHAR:  DoCast<unsigned char>(native);   break;
    case itk::ImageIOBase::CHAR:   DoCast<signed char>(native);     break;
    case itk::ImageIOBase::USHORT: DoCast<unsigned short>(native);  break;
    case itk::ImageIOBase::SHORT:  DoCast<signed short>(native);    break;
    case itk::ImageIOBase::UINT:   DoCast<unsigned int>(native);    break;
    case itk::ImageIOBase::INT:    DoCast<signed int>(native);      break;
    case itk::ImageIOBase::ULONG:  DoCast<unsigned long>(native);   break;
    case itk::ImageIOBase::LONG:   DoCast<signed long>(native);     break;
    case itk::ImageIOBase::FLOAT:  DoCast<float>(native);           break;
    case itk::ImageIOBase::DOUBLE: DoCast<double>(native);          break;
    default: 
      throw IRISException("Error: Unknown pixel type when reading image."
                          "The voxels in the image you are loading have format '%s', "
                          "which is not supported.",
                          nativeIO->GetComponentTypeAsStringInNativeImage().c_str());
    }

  // Return the output image
  return m_Output;
}

template<class TPixel, class TCastFunctor>
template<typename TNative>
void
CastNativeImageBase<TPixel,TCastFunctor>
::DoCast(itk::ImageBase<3> *native)
{
  // Get the native image
  typedef itk::VectorImage<TNative, 3> InputImageType;
  typename InputImageType::Pointer input = 
    reinterpret_cast<InputImageType *>(native);
  assert(input);

  typedef typename InputImageType::PixelContainer InPixCon;
  typedef typename OutputImageType::PixelContainer OutPixCon;

  InPixCon *ipc = input->GetPixelContainer();

  // If the native image does not have three components, we crash
  if(input->GetNumberOfComponentsPerPixel() != m_Functor.GetNumberOfDimensions())
    throw IRISException(
        "Error: Wrong number of components. "
        "Can not convert image to target format ('%s'). "
        "Image has %d components per pixel, but ITK-SNAP expects %d components. ",
        typeid(TPixel).name(),
        input->GetNumberOfComponentsPerPixel(),
        m_Functor.GetNumberOfDimensions() );

  // Allocate the output image
  m_Output = OutputImageType::New();
  m_Output->CopyInformation(native);
  m_Output->SetMetaDataDictionary(native->GetMetaDataDictionary());
  m_Output->SetRegions(native->GetBufferedRegion());

  // Special case: native image is the same as target image
  if(typeid(TPixel) == typeid(TNative))
    {
    typename OutputImageType::PixelContainer *inbuff = 
      dynamic_cast<typename OutputImageType::PixelContainer *>(ipc);
    assert(inbuff);
    m_Output->SetPixelContainer(inbuff);
    return;
    }

  // We are going to map data from native to target format in place in order
  // to save memory. This way, SNAP will never use extra memory when loading
  // an image. Some trickery is needed though.
  size_t nvoxels = input->GetBufferedRegion().GetNumberOfPixels();
  size_t ncomp = input->GetNumberOfComponentsPerPixel();
  size_t szNative = sizeof(TNative) * ncomp;
  size_t szTarget = sizeof(TPixel);

  // Bytes allocated in the current pixel container
  size_t nbNative = input->GetPixelContainer()->Capacity() * szNative;

  // Bytes needed to store the data in target format
  size_t nbTarget = input->GetPixelContainer()->Size() * szTarget;

  // This memory is no longer owned by the input
  ipc->SetContainerManageMemory(false);

  // Pointer to the input buffer
  TNative *ib = ipc->GetImportPointer();

  // If target is larger than native, expand the pixel container
  if(nbNative < nbTarget)
    {
    // We should probably avoid this possibility by forcing at least a short
    // type when loading char data. But if this does happen, all we need to
    // do is increase the capacity of the native data
    ib = reinterpret_cast<TNative *>(realloc(ib, nbTarget));
    }

  // Get a pointer to the output buffer (same as input buffer)
  TPixel *ob = reinterpret_cast<TPixel *>(ib);

  // Finally, we get to the code where we map from input format to the output
  // format. Here again we have to be careful. If the native image is larger or
  // same than the target image, we want to proceed in ascending order, since each
  // input element will be replaced by one or more output elements. But if the
  // native image is smaller, we want to proceed from the end of the memory
  // block in a descending order, so that the native data is not overridden
  if(szTarget > szNative)
    {
    TNative *pn = ib + (nvoxels - 1) * ncomp;
    TPixel *pt = ob + nvoxels - 1;
    for(; pt >= ob; pt--, pn-=ncomp)
      m_Functor(pn, pt);
    }
  else
    {
    TNative *pn = ib;
    TPixel *pt = ob;
    for(; pt < ob + nvoxels; pt++, pn+=ncomp)
      m_Functor(pn, pt);
    }

  // If needed, squeeze the memory
  if(nbTarget < nbNative)
    ob = reinterpret_cast<TPixel *>(realloc(ob, nbTarget));

  // Create a new container wrapped around the same chunk of memory as the
  // native image. The size we pass in here is the number of elements that
  // have been already allocated. We will shrink that memory later
  SmartPtr<OutPixCon> pc = OutPixCon::New();
  pc->SetImportPointer(ob, nvoxels, true);

  // Otherwise, allocate the buffer in the output image
  m_Output->SetPixelContainer(pc);
}

GuidedNativeImageIO::FileFormat
GuidedNativeImageIO::GuessFormatForFileName(
    const std::string &fname, bool checkMagic)
{
  if(checkMagic)
    {
    // Read the first few bytes from the file. We use zlib to automatically
    // handle gz-compressed data.
    const int buf_size=1024;
    char buffer[buf_size];

    gzFile gz = gzopen(fname.c_str(), "rb");
    bool havebuff = (gz!=NULL) && (buf_size==gzread(gz,buffer,buf_size));
    gzclose(gz);

    // Now we will check known magic numbers, especially for formats that
    // are primary formats supported by SNAP

    // Check for DICOM
    if(havebuff && !strncmp(buffer+128,"DICM",4))
      return FORMAT_DICOM;

    // Check for NIFTI. This is important because .hdr files can be either
    // NIFTI or Analyze, so we have to know what we are dealing with.
    if(havebuff && buffer[344]==0x6E && buffer[347]==0x00 &&
       (buffer[345]==0x69 || buffer[345]==0x2B) &&
       buffer[346] > 0x30 && buffer[346] <= 0x39)
      return FORMAT_NIFTI;

    // Potentially, we could check for other formats here too, but this is
    // not as high priority. For one thing, unlike DICOM and NIFTI, the other
    // formats are well defined by their extension.
    }

  // The rest of this method checks by extension
  for(unsigned int i = 0; i < FORMAT_COUNT; i++)
    {
    // Get the format descriptor
    FileFormat fmt = static_cast<FileFormat>(i);
    FileFormatDescriptor fd = GetFileFormatDescriptor(fmt);

    // Check if there is a filename match
    if(fd.TestFilename(fname))
      return fmt;
    }

  // Nothing matched
  return FORMAT_COUNT;
}

#include "gdcmReader.h"
#include "gdcmPixmap.h"
#include "gdcmPixmapReader.h"
#include "gdcmImageReader.h"
#include "gdcmImageHelper.h"

bool stringCompare( const string &left, const string &right )
{
  for( string::const_iterator lit = left.begin(), rit = right.begin(); lit != left.end() && rit != right.end(); ++lit, ++rit )
  {
    if( tolower( *lit ) < tolower( *rit ) )
      return true;
    else if( tolower( *lit ) > tolower( *rit ) )
      return false;
    if( left.size() < right.size() )
        return true;
  }
  return false;
}

const char * tagToValueString(const gdcm::Scanner::TagToValue & attv, const gdcm::Tag & aTag)
{
    gdcm::Scanner::TagToValue::const_iterator it =  attv.find( aTag );
    const char * value = it->second;
    return(value);
}

int tagToValueInt(const gdcm::Scanner::TagToValue & attv, const gdcm::Tag & aTag)
{
    const char * value = tagToValueString(attv, aTag);
    int nRes = atoi(value);
    return(nRes);
}

gdcm::Tag tagRows(0x0028, 0x0010);
gdcm::Tag tagCols(0x0028, 0x0011);
gdcm::Tag tagDesc(0x0008, 0x103e);
gdcm::Tag tagTextDesc(0x0028, 0x0010);

bool getDims(int aarrnSzXY[2], const gdcm::SmartPointer < gdcm::Scanner > apScanner, const string & astrFileName) {
    
  if( apScanner->IsKey( astrFileName.c_str() ) )
    {
    std::cerr << "INFO:" << astrFileName
              << " is a proper Key for the Scanner (this is a DICOM file)" << std::endl;
    }
  else
    {
    return(false);
    }
    
  const gdcm::Scanner::TagToValue &ttv = apScanner->GetMapping(astrFileName.c_str());
    
  aarrnSzXY[0] = tagToValueInt(ttv, tagRows);
  aarrnSzXY[1] = tagToValueInt(ttv, tagCols);
  return(true);

}

const char * getTag(gdcm::SmartPointer < gdcm::Scanner > apScanner,
                    const gdcm::Tag & aTag,
                    const string & astrFileName) {
    
  const char * pchRes = 0;
  if( apScanner->IsKey( astrFileName.c_str() ) )
    {
        std::cerr << "INFO:" << astrFileName
        << " is a proper Key for the Scanner (this is a DICOM file)" << std::endl;
        return(pchRes);
    }
    
  const gdcm::Scanner::TagToValue &ttv = apScanner->GetMapping(astrFileName.c_str());
  pchRes = tagToValueString(ttv, aTag);

  return(pchRes);

}

bool scanTags(gdcm::SmartPointer < gdcm::Scanner > apScanner,
          const gdcm::Directory::FilenamesType & aFileNames)
{
  if( !apScanner->Scan( aFileNames ) )
    {
        return(false);
    }
    
  apScanner->Print( std::cout );
  return(true);
}



struct DICOMFileInfo
{
  string m_strFileName;
  int m_arrnDims[2];
  
  bool operator<(const DICOMFileInfo & aDICOMFileInfo) const
    {
    return(m_strFileName < aDICOMFileInfo.m_strFileName);
    }
  DICOMFileInfo & operator=(const DICOMFileInfo & aDICOMFileInfo)
    {
    m_strFileName = aDICOMFileInfo.m_strFileName;
    int nI;
    for(nI = 0; nI < 2; nI++)
      {
      m_arrnDims[nI] = aDICOMFileInfo.m_arrnDims[nI];
      }
    return(*this);
    }
};

gdcm::SmartPointer < gdcm::Scanner > Scan(
  const std::string &dir,
  const GuidedNativeImageIO::DicomRequest &req)
{
  
  gdcm::Directory dirList;
  //unsigned int nfiles =
    dirList.Load(dir, false);
    
  const gdcm::Directory::FilenamesType &filenames = dirList.GetFilenames();
    
  //int nTotFilesNr = filenames.size();
  //aarrDFI.resize(nTotFilesNr);

  gdcm::SmartPointer < gdcm::Scanner > pScanner =  gdcm::Scanner::New();
  pScanner->AddTag( tagRows );
  pScanner->AddTag( tagCols );
  pScanner->AddTag( tagDesc );
  // Get textual description
  pScanner->AddTag( tagTextDesc );
    
  for(GuidedNativeImageIO::DicomRequest::const_iterator it = req.begin(); it != req.end(); ++it)
    {
    pScanner->AddTag( gdcm::Tag(it->group, it->elem) );
    }

  if( !pScanner->Scan( filenames ) )
    {
    throw IRISException("Error: Could not scan directory %s" ,dir.c_str());
    }
  return(pScanner);

  //If necessary for debugging:
  //pScanner->Print( std::cout );

  //int nI = 0;
  //for(gdcm::Directory::FilenamesType::const_iterator it = filenames.begin(); it != filenames.end(); ++it, nI++)
  //  {
  //  DICOMFileInfo & dfi = aarrDFI[nI];
  //  dif.m_strFileName = *it;
  //  if(getDims(dif.m_arrnDims, pScanner, dif.m_strFileName) == false)
  //    {
  //    return(false);
  //    }
  //  }
  //return(true);
    
}

bool SortHeaders(map < string, vector< DICOMFileInfo > > & amap_UID_FileNames,
                 gdcm::SmartPointer < gdcm::Scanner > apScanner,
                 const std::string &dir)
{
  amap_UID_FileNames.clear();
  
  SNAP::DICOMSerieUniqueIDsMgr uniqueIDsMgr;
  uniqueIDsMgr.Clear();
  //uniqueIDsMgr.SetUseSeriesDetails(m_UseSeriesDetails ?true?);
  //uniqueIDsMgr.SetLoadMode( ( m_LoadSequences ? 0 : gdcm::LD_NOSEQ )
  //                           | ( m_LoadPrivateTags ? 0 : gdcm::LD_NOSHADOW ) );
  uniqueIDsMgr.SetDirectory(dir, false);
  
  
  gdcm::Directory dirList;
  //unsigned int nfiles =
    dirList.Load(dir, false);
  const gdcm::Directory::FilenamesType &filenames = dirList.GetFilenames();
  gdcm::Directory::FilenamesType::const_iterator itFN = filenames.begin();
  
  
  vector < string > arrstrSeriesUIDs;
  SNAP::FileList *flist = uniqueIDsMgr.GetFirstSingleSerieUIDFileSet();
  while ( flist )
    {
    int nFilesNr = flist->size();
    if(nFilesNr > 0)
      {
      vector< DICOMFileInfo > arrDFIs(nFilesNr);

      gdcm::File *file = ( *flist )[0]; //for example take the first one
      std::string id = uniqueIDsMgr.CreateUniqueSeriesIdentifier(file).c_str();
      arrstrSeriesUIDs.push_back( id.c_str() );
      int nI;
      for(nI =0; nI < nFilesNr; nI++)
        {
        DICOMFileInfo dfi;
          dfi.m_strFileName = string( itFN->c_str()
                                   );
        if(getDims(dfi.m_arrnDims, apScanner, dfi.m_strFileName) == false)
          {
          return(false);
          }
        arrDFIs[nI] = dfi;
        }
      amap_UID_FileNames[id] = arrDFIs;
      itFN++;
      }
    flist = uniqueIDsMgr.GetNextSingleSerieUIDFileSet();
    }
  if ( !arrstrSeriesUIDs.size() )
  {
    cerr << "Warning: No Series were found" << endl;
    return(false);
  }
  
  map < string, vector< DICOMFileInfo > >::iterator it_UID_DFI;
  for(it_UID_DFI = amap_UID_FileNames.begin(); it_UID_DFI != amap_UID_FileNames.end(); it_UID_DFI++)
    {
    vector< DICOMFileInfo > & arrDFIs = it_UID_DFI->second;
    sort(arrDFIs.begin(), arrDFIs.end());
    }
  return true;
  
}

void GuidedNativeImageIO::ParseDicomDirectory(
    const std::string &dir,
    GuidedNativeImageIO::RegistryArray &reg,
    const GuidedNativeImageIO::DicomRequest &req)
{
  // Must have a directory
  if(!itksys::SystemTools::FileIsDirectory(dir.c_str()))
    throw IRISException(
      "Error: Not a directory. "
      "Trying to look for DICOM series in '%s', which is not a directory",
      dir.c_str());
    
  // Clear output
  reg.clear();

  gdcm::SmartPointer < gdcm::Scanner > pScanner = Scan(dir, req);

  map < string, vector< DICOMFileInfo > > map_UID_FileNames;
  if(SortHeaders(map_UID_FileNames, pScanner, dir) == false)
    {
    throw IRISException(
      "Error: DICOM series read. "
      "Directory '%s' does not appear to contain a valid DICOM series.", dir.c_str());
    }
  
  int nSeriesNr = map_UID_FileNames.size();

  
  map < string, vector< DICOMFileInfo > >::iterator it_UID_DFI;
  for(it_UID_DFI = map_UID_FileNames.begin(); it_UID_DFI != map_UID_FileNames.end(); it_UID_DFI++)
    {
    vector< DICOMFileInfo > & arrDFIs = it_UID_DFI->second;
    int nFilesNr = arrDFIs.size();
      
    if(nFilesNr > 0)
      {
      
      int nSzZ = nFilesNr;
        
      DICOMFileInfo & dfi0 = arrDFIs[0];
        
      string strFileName0 = dfi0.m_strFileName;
      int nSzX0 = dfi0.m_arrnDims[0], nSzY0 = dfi0.m_arrnDims[1];
      bool same_dims = true;
    
      int nIndxFiles;
      for(nIndxFiles = 0; nIndxFiles < nFilesNr; nIndxFiles++)
        {
        DICOMFileInfo & dfi = arrDFIs[nIndxFiles];
        string strFileName = dfi.m_strFileName;
        int nSzX = dfi.m_arrnDims[0], nSzY = dfi.m_arrnDims[1];
        if((nSzX0 != nSzX) || (nSzY0 != nSzY))
          {
          same_dims = false;
          }
        }
    
      // Create a registry entryx
      Registry r;
          
      // Create its unique series ID
      r["SeriesId"] << it_UID_DFI->first;
        
      // Also store the files
      //r.Folder("SeriesFiles").PutArray(sortedFilenamesType);
            
      // Get textual description
      const char * pchValue = getTag(pScanner, gdcm::Tag(0x0008, 0x103e), strFileName0);
      if(pchValue)
        {
        r["SeriesDescription"] << pchValue;;
        }

      // Get dimensions
      std::ostringstream oss;
      if(same_dims)
        {
        oss << nSzX0 << " x " << nSzY0;
        if(nSzZ != 1)
          oss << " x " << nSzZ;
        }
      else
        {
        oss << "Variable";
        }
      r["Dimensions"] << oss.str();

      // Get the number of images
      r["NumberOfImages"] << nSzZ;
            
      // Get all other fields that the user wants
      for(DicomRequest::const_iterator it = req.begin(); it != req.end(); ++it)
        {
        pchValue = getTag(pScanner, gdcm::Tag(it->group, it->elem), strFileName0);
        if(pchValue)
          {
            r[it->code] << pchValue;
          }
        }
            
        reg.push_back( r );
      }
    }
  

  
  // Complain if no series have been found
  if(reg.size() == 0)
    throw IRISException(
        "Error: DICOM series not found. "
        "Directory '%s' does not appear to contain a DICOM series.", dir.c_str());
}

/*

template<typename TPixel>
typename CastNativeImageToScalar<TPixel>::OutputImageType *
CastNativeImageToScalar<TPixel>
::operator()(GuidedNativeImageIO *nativeIO)
{
  // Get the native image pointer
  itk::ImageBase<3> *native = nativeIO->GetNativeImage();

  // Allocate the output image
  m_Output = OutputImageType::New();
  m_Output->CopyInformation(native);
  m_Output->SetRegions(native->GetBufferedRegion());
  m_Output->Allocate();

  // Cast image from native format to TPixel
  itk::ImageIOBase::IOComponentType itype = nativeIO->GetComponentTypeInNativeImage();
  switch(itype) 
    {
    case itk::ImageIOBase::UCHAR:  DoCast<unsigned char>(native);   break;
    case itk::ImageIOBase::CHAR:   DoCast<signed char>(native);     break;
    case itk::ImageIOBase::USHORT: DoCast<unsigned short>(native);  break;
    case itk::ImageIOBase::SHORT:  DoCast<signed short>(native);    break;
    case itk::ImageIOBase::UINT:   DoCast<unsigned int>(native);    break;
    case itk::ImageIOBase::INT:    DoCast<signed int>(native);      break;
    case itk::ImageIOBase::ULONG:  DoCast<unsigned long>(native);   break;
    case itk::ImageIOBase::LONG:   DoCast<signed long>(native);     break;
    case itk::ImageIOBase::FLOAT:  DoCast<float>(native);           break;
    case itk::ImageIOBase::DOUBLE: DoCast<double>(native);          break;
    default: 
      throw itk::ExceptionObject("Unknown Pixel Type when reading image");
    }

  // Return the output image
  return m_Output;
}

template<typename TPixel>
template<typename TNative>
void
CastNativeImageToScalar<TPixel>
::DoCast(itk::ImageBase<3> *native)
{
  // Get the native image
  typedef itk::VectorImage<TNative, 3> InputImageType;
  typename InputImageType::Pointer input = 
    reinterpret_cast<InputImageType *>(native);
  assert(input);

  // If the native image does not have one component, we crash
  if(input->GetNumberOfComponentsPerPixel() != 1)
    throw itk::ExceptionObject(
      "Specified image can not be read as a scalar image.\n"
      "It does not have 1 component per pixel.");


  // We simply cast every component of every pixel from input to output
  typedef itk::ImageRegionConstIterator<InputImageType> InputIterator;
  typedef itk::ImageRegionIterator<OutputImageType> OutputIterator;
  InputIterator it(input, input->GetBufferedRegion());
  OutputIterator oit(m_Output, m_Output->GetBufferedRegion());
  for(; !it.IsAtEnd(); ++it, ++oit)
    {
    oit.Set((TPixel) it.Get()[0]);
    }
}

*/

template class RescaleNativeImageToScalar<GreyType>;
template class CastNativeImageBase<RGBType, CastToArrayFunctor<RGBType, 3> >;
template class CastNativeImageBase<LabelType, CastToScalarFunctor<LabelType> >;
template class CastNativeImageBase<float, CastToScalarFunctor<float> >;

/*
template class CastNativeImageToRGB<RGBType>; 
template class CastNativeImageToScalar<LabelType>; 
template class CastNativeImageToScalar<float>; 
*/

template void GuidedNativeImageIO::SaveImage(const char *, Registry &, itk::Image<GreyType,3> *);
template void GuidedNativeImageIO::SaveImage(const char *, Registry &, itk::Image<LabelType,3> *);
template void GuidedNativeImageIO::SaveImage(const char *, Registry &, itk::Image<float,3> *);
template void GuidedNativeImageIO::SaveImage(const char *, Registry &, itk::Image<RGBType,3> *);



