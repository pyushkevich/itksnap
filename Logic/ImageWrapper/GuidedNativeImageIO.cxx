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
#include "IRISException.h"
#include "SNAPCommon.h"
#include "SNAPRegistryIO.h"
#include "ImageCoordinateGeometry.h"
#include "itkOrientedImage.h"

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
#include "itkImageToVectorImageFilter.h"

#include "itkMinimumMaximumImageCalculator.h"
#include "itkShiftScaleImageFilter.h"
#include "itkNumericTraits.h"


using namespace std;

bool GuidedNativeImageIO::m_StaticDataInitialized = false;

RegistryEnumMap<GuidedNativeImageIO::FileFormat> GuidedNativeImageIO::m_EnumFileFormat;
RegistryEnumMap<GuidedNativeImageIO::RawPixelType> GuidedNativeImageIO::m_EnumRawPixelType;

const GuidedNativeImageIO::FileFormatDescriptor 
GuidedNativeImageIO
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

GuidedNativeImageIO
::GuidedNativeImageIO()
{
  if(!m_StaticDataInitialized)
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
          throw itk::ExceptionObject("Unsupported Pixel Type when reading Raw File");
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
::ReadNativeImage(const char *FileName, Registry &folder)
{
  // Create the header corresponding to the current image type
  CreateImageIO(FileName, folder, true);
  if(!m_IOBase)
    throw itk::ExceptionObject("Unsupported image file type");

  // Read the information about the image
  if(m_FileFormat == FORMAT_DICOM)
    {
    // Check if the array of filenames has been provided for us
    m_DICOMFiles = 
      folder.Folder("DICOM.SliceFiles").GetArray(std::string("NULL"));

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
        throw itk::ExceptionObject("No DICOM series found in the DICOM directory");
    
      // Read the first DICOM series in the directory
      m_DICOMFiles = nameGenerator->GetFileNames(sids.front().c_str());
      }

    // Read the information from the first filename
    if(m_DICOMFiles.size() == 0)
      throw itk::ExceptionObject("No DICOM files found in the DICOM directory");
    m_IOBase->SetFileName(m_DICOMFiles[0]);
    m_IOBase->ReadImageInformation();
    }
  else
    {
    m_IOBase->SetFileName(FileName);
    m_IOBase->ReadImageInformation();
    }

  // Based on the component type, read image in native mode
  switch(m_IOBase->GetComponentType()) 
    {
    case itk::ImageIOBase::UCHAR:  DoReadNative<unsigned char>(FileName, folder);  break;
    case itk::ImageIOBase::CHAR:   DoReadNative<signed char>(FileName, folder);    break;
    case itk::ImageIOBase::USHORT: DoReadNative<unsigned short>(FileName, folder); break;
    case itk::ImageIOBase::SHORT:  DoReadNative<signed short>(FileName, folder);   break;
    case itk::ImageIOBase::UINT:   DoReadNative<unsigned int>(FileName, folder);   break;
    case itk::ImageIOBase::INT:    DoReadNative<signed int>(FileName, folder);     break;
    case itk::ImageIOBase::ULONG:  DoReadNative<unsigned long>(FileName, folder);  break;
    case itk::ImageIOBase::LONG:   DoReadNative<signed long>(FileName, folder);    break;
    case itk::ImageIOBase::FLOAT:  DoReadNative<float>(FileName, folder);          break;
    case itk::ImageIOBase::DOUBLE: DoReadNative<double>(FileName, folder);         break;
    default: 
      throw itk::ExceptionObject("Unknown Pixel Type when reading image");
    }

  // Get rid of the IOBase, it may store useless data (in case of NIFTI)
  m_NativeType = m_IOBase->GetComponentType();
  m_NativeComponents = m_IOBase->GetNumberOfComponents();
  m_NativeTypeString = m_IOBase->GetComponentTypeAsString(m_NativeType);
  m_NativeFileName = m_IOBase->GetFileName();
  m_NativeByteOrder = m_IOBase->GetByteOrder();
  m_NativeSizeInBytes = m_IOBase->GetImageSizeInBytes();
  m_IOBase = NULL;
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
    typedef itk::OrientedImage<TScalar, 3> GreyImageType;

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

    // Convert the image into VectorImage format
    typedef itk::ImageToVectorImageFilter<GreyImageType> FilterType;
    typename FilterType::Pointer flt = FilterType::New();
    flt->SetInput(0, reader->GetOutput());
    flt->Update();
    m_NativeImage = flt->GetOutput();
    m_NativeImage->SetDirection(flt->GetOutput()->GetDirection());

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
      throw itk::ExceptionObject("Unknown Pixel Type when reading image");
    }

  // Return the output image
  return m_Output;
}

template<typename TPixel>
template<typename TNative>
void
RescaleNativeImageToScalar<TPixel>
::DoCast(itk::ImageBase<3> *native)
{
  // Get the native image
  typedef itk::VectorImage<TNative, 3> InputImageType;
  typedef itk::ImageRegionConstIterator<InputImageType> InputIterator;
  typename InputImageType::Pointer input = 
    dynamic_cast<InputImageType *>(native);
  assert(input);

  // Special case: native image is the same as target image
  if(typeid(TPixel) == typeid(TNative))
    {
    typename OutputImageType::PixelContainer *inbuff = 
      dynamic_cast<typename OutputImageType::PixelContainer *>(input->GetPixelContainer());
    assert(inbuff);
    m_Output->SetPixelContainer(inbuff);
    m_NativeScale = 1.0;
    m_NativeShift = 0.0;
    return;
    }

  // Otherwise, allocate the buffer in the output image
  m_Output->Allocate();

  // We must compute the range of the input data
  size_t nvoxels = input->GetBufferedRegion().GetNumberOfPixels();
  size_t ncomp = input->GetNumberOfComponentsPerPixel();
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
      double val = input->GetBufferPointer()[i]; 
      if(val < imin) imin = val;
      if(val > imax) imax = val;
      }
    }

  // We must compute a scale and shift factor
  double scale = 1.0, shift = 0.0;

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
        TNative vin = input->GetBufferPointer()[i];
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

  // Map the values from input vector image to output image. Not using
  // iterators to increase speed and avoid unnecessary constructors
  TNative *bn = input->GetBufferPointer();
  TPixel *bo = m_Output->GetBufferPointer();
  if(ncomp == 1)
    {
    for(size_t i = 0; i < nvoxels; i++, bn++)
      {
      bo[i] = (TPixel) ((*bn + shift)*scale + 0.5);
      }
    }
  else
    {
    for(size_t i = 0; i < nvoxels; i++)
      {
      double val = 0.0;
      for(size_t k = 0; k < ncomp; k++, bn++)
        val += (*bn) * (*bn);
      bo[1] = (TPixel) ((sqrt(val) + shift)*scale + 0.5);
      }
    }

  // Store the shift and the scale needed to take the TPixel values 
  // to the TNative values
  m_NativeScale = 1.0 / scale;
  m_NativeShift = - shift;
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
      throw itk::ExceptionObject("Unknown Pixel Type when reading image");
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

  // Make sure the number of components matches
  TCastFunctor functor;
  
  // If the native image does not have three components, we crash
  if(input->GetNumberOfComponentsPerPixel() != functor.GetNumberOfDimensions())
    throw IRISException(
      "Can not convert image to target format (%s).\n"
      "Image has %d components per pixel, but it should have %d components.",
      typeid(TPixel).name(), 
      input->GetNumberOfComponentsPerPixel(), 
      functor.GetNumberOfDimensions() );

  // Allocate the output image
  m_Output = OutputImageType::New();
  m_Output->CopyInformation(native);
  m_Output->SetRegions(native->GetBufferedRegion());

  // Special case: native image is the same as target image
  if(typeid(TPixel) == typeid(TNative))
    {
    typename OutputImageType::PixelContainer *inbuff = 
      dynamic_cast<typename OutputImageType::PixelContainer *>(input->GetPixelContainer());
    assert(inbuff);
    m_Output->SetPixelContainer(inbuff);
    return;
    }

  // Otherwise, allocate the buffer in the output image
  m_Output->Allocate();

  // We simply cast every component of every pixel from input to output
  size_t nvoxels = native->GetBufferedRegion().GetNumberOfPixels();
  size_t ncomp = functor.GetNumberOfDimensions();

  TNative *bn = input->GetBufferPointer();
  TPixel *bo = m_Output->GetBufferPointer();
  for(size_t i = 0; i < nvoxels; i++, bn+=ncomp, bo++)
    functor(bn, bo);
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

