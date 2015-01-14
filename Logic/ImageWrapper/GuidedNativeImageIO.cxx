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

#include "itkImage.h"
#include "itkImageIOBase.h"
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
#include "gdcmReader.h"
#include "gdcmStringFilter.h"
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
  {"Analyze", "img.gz,hdr,img",      true,  false, true,  true},
  {"DICOM Image Series", "",         false, true,  true,  true},
  {"DICOM Single Image", "dcm",      false, true,  true,  true},
  {"GE Version 4", "ge4",            false, false, true,  true},
  {"GE Version 5", "ge5",            false, false, true,  true},
  {"GIPL", "gipl,gipl.gz",           true,  false, true,  true},
  {"MetaImage", "mha,mhd",           true,  true,  true,  true},
  {"NiFTI", "nii.gz,nii,nia,nia.gz", true,  true,  true,  true},
  {"NRRD", "nrrd,nhdr",              true,  true,  true,  true},
  {"Raw Binary", "raw",              false, false, true,  true},
  {"Siemens Vision", "ima",          false, false, true,  true},
  {"VoxBo CUB", "cub,cub.gz",        true,  false, true,  true},
  {"VTK Image", "vtk",               true,  false, true,  true},
  {"INVALID FORMAT", "",             false, false, false, false}};


/*************************************************************************/
/* THE FOLLOWING CODE IS TAKEN FROM FFTW */

/* In-place transpose routine from TOMS, which follows the cycles of
   the permutation so that it writes to each location only once.
   Because of cache-line and other issues, however, this routine is
   typically much slower than transpose-gcd or transpose-cut, even
   though the latter do some extra writes.  On the other hand, if the
   vector length is large then the TOMS routine is best.

   The TOMS routine also has the advantage of requiring less buffer
   space for the case of gcd(nx,ny) small.  However, in this case it
   has been superseded by the combination of the generalized
   transpose-cut method with the transpose-gcd method, which can
   always transpose with buffers a small fraction of the array size
   regardless of gcd(nx,ny). */

/*
 * TOMS Transpose.  Algorithm 513 (Revised version of algorithm 380).
 *
 * These routines do in-place transposes of arrays.
 *
 * [ Cate, E.G. and Twigg, D.W., ACM Transactions on Mathematical Software,
 *   vol. 3, no. 1, 104-110 (1977) ]
 *
 * C version by Steven G. Johnson (February 1997).
 */

/*
 * "a" is a 1D array of length ny*nx*N which constains the nx x ny
 * matrix of N-tuples to be transposed.  "a" is stored in row-major
 * order (last index varies fastest).  move is a 1D array of length
 * move_size used to store information to speed up the process.  The
 * value move_size=(ny+nx)/2 is recommended.  buf should be an array
 * of length 2*N.
 *
 */

template <typename INT>
INT gcd(INT a, INT b)
{
  INT r;
  do {
    r = a % b;
    a = b;
    b = r;
    } while (r != 0);

  return a;
}

template <typename R, typename INT>
void transpose_toms513(R *a, INT nx, INT ny, char *move, INT move_size, R *buf)
{
  INT i, im, mn;
  R *b, *c, *d;
  INT ncount;
  INT k;

  /* check arguments and initialize: */
  assert(ny > 0 && nx > 0 && move_size > 0);

  b = buf;

  /* Cate & Twigg have a special case for nx == ny, but we don't
  bother, since we already have special code for this case elsewhere. */

  c = buf + 1;
  ncount = 2;		/* always at least 2 fixed points */
  k = (mn = ny * nx) - 1;

  for (i = 0; i < move_size; ++i)
    move[i] = 0;

  if (ny >= 3 && nx >= 3)
    ncount += gcd(ny - 1, nx - 1) - 1;	/* # fixed points */

  i = 1;
  im = ny;

  while (1) {
    INT i1, i2, i1c, i2c;
    INT kmi;

    /** Rearrange the elements of a loop
        and its companion loop: **/

    i1 = i;
    kmi = k - i;
    i1c = kmi;
    b[0] = a[i1];
    c[0] = a[i1c];

    while (1) {
      i2 = ny * i1 - k * (i1 / nx);
      i2c = k - i2;
      if (i1 < move_size)
        move[i1] = 1;
      if (i1c < move_size)
        move[i1c] = 1;
      ncount += 2;
      if (i2 == i)
        break;
      if (i2 == kmi) {
        d = b;
        b = c;
        c = d;
        break;
        }
      a[i1] = a[i2];
      a[i1c] = a[i2c];
      i1 = i2;
      i1c = i2c;
      }

    a[i1] = b[0];
    a[i1c] = c[0];

    if (ncount >= mn)
      break;	/* we've moved all elements */

    /** Search for loops to rearrange: **/

    while (1) {
      INT max = k - i;
      ++i;
//      assert(i <= max);
      im += ny;
      if (im > k)
        im -= k;
      i2 = im;
      if (i == i2)
        continue;
      if (i >= move_size) {
        while (i2 > i && i2 < max) {
          i1 = i2;
          i2 = ny * i1 - k * (i1 / nx);
          }
        if (i2 == i)
          break;
        } else if (!move[i])
        break;
      }
    }
}


bool GuidedNativeImageIO::FileFormatDescriptor
::TestFilename(std::string fname)
{
  if(fname.length() == 0)
    return false;

  // Check if the filename matches the pattern
  for(size_t i = 0; i < pattern.length(); )
    {
    size_t j = pattern.find(',', i);
    string ext = "." + pattern.substr(i, j-i);
    i+=ext.length();
    size_t pos = fname.rfind(ext);
    if(pos == std::string::npos)
      continue;

    if(pos == fname.length() - ext.length())
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
    case FORMAT_ANALYZE:    m_IOBase = itk::NiftiImageIO::New();       break;
    case FORMAT_GIPL:       m_IOBase = itk::GiplImageIO::New();          break;
    case FORMAT_GE4:        m_IOBase = itk::GE4ImageIO::New();           break;
    case FORMAT_GE5:        m_IOBase = itk::GE5ImageIO::New();           break;
    case FORMAT_NIFTI:      m_IOBase = itk::NiftiImageIO::New();         break;
    case FORMAT_SIEMENS:    m_IOBase = itk::SiemensVisionImageIO::New(); break;
    case FORMAT_VTK:        m_IOBase = itk::VTKImageIO::New();           break;
    case FORMAT_VOXBO_CUB:  m_IOBase = itk::VoxBoCUBImageIO::New();      break;
    case FORMAT_DICOM_DIR:
    case FORMAT_DICOM_FILE: m_IOBase = itk::GDCMImageIO::New();          break;
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
  if(m_FileFormat == FORMAT_DICOM_DIR)
    {
    // Get the directory where to search for the series
    std::string SeriesDir = FileName;
    if(!itksys::SystemTools::FileIsDirectory(FileName))
      SeriesDir = itksys::SystemTools::GetParentDirectory(FileName);

    // NOTE: for the time being, we are relying on GDCMSeriesFileNames for
    // proper sorting of the DICOM data. This is marked as deprecated in GDCM 2
    if(m_GDCMSeries.IsNull() || m_GDCMSeriesDirectory != SeriesDir)
      {
      m_GDCMSeries = itk::GDCMSeriesFileNames::New();
      m_GDCMSeries->SetUseSeriesDetails(true);
      m_GDCMSeries->SetDirectory(SeriesDir);
      m_GDCMSeriesDirectory = SeriesDir;
      }

    // Select which series
    std::string SeriesID = m_Hints["DICOM.SeriesId"][""];
    if(SeriesID.length() == 0)
      {
      // Get the list of series in the directory
      const itk::SerieUIDContainer &sids = m_GDCMSeries->GetSeriesUIDs();

      // There must be at least of series
      if(sids.size() == 0)
        throw IRISException("Error: DICOM series not found. "
                            "Directory '%s' does not appear to contain a "
                            "series of DICOM images.",FileName);

      // Read the first DICOM series in the directory
      SeriesID = sids.front();
      }

    // Use the series provided by the user
    m_DICOMFiles = m_GDCMSeries->GetFileNames(SeriesID.c_str());

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

  // Get the data dimensions
  int ncomp = m_IOBase->GetNumberOfComponents();

  // Set the dimensions (if 2D image, we set last dim to 1)
  m_NativeDimensions.fill(1);
  for(size_t i = 0; i < m_IOBase->GetNumberOfDimensions(); i++)
    {
    if(i < 3)
      m_NativeDimensions[i] = m_IOBase->GetDimensions(i);
    else
      ncomp *= m_IOBase->GetDimensions(i);
    }

  // Extract properties from IO base
  m_NativeType = m_IOBase->GetComponentType();
  m_NativeComponents = ncomp;
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

#include <itkTimeProbe.h>

template<class TScalar>
void
GuidedNativeImageIO
::DoReadNative(const char *FileName, Registry &folder)
{
  // Define the image type of interest
  typedef itk::VectorImage<TScalar, 3> NativeImageType;

  // There is a special handler for the DICOM case!
  if(m_FileFormat == FORMAT_DICOM_DIR && m_DICOMFiles.size() > 1)
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
    
    size_t nd_actual = m_IOBase->GetNumberOfDimensions();
    size_t nd = (nd_actual > 3) ? 3 : nd_actual;
    
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

    // Fold in any higher number of dimensions as additional components.
    int ncomp = m_IOBase->GetNumberOfComponents();
    if(nd_actual > nd)
      {
      for(int i = nd; i < nd_actual; i++)
        ncomp *= m_IOBase->GetDimensions(i);
      }

    // Set the regions and allocate
    typename NativeImageType::RegionType region;
    typename NativeImageType::IndexType index = {{0, 0, 0}};
    region.SetIndex(index);
    region.SetSize(dim);
    image->SetRegions(region);
    image->SetVectorLength(ncomp);
    image->Allocate();

    // Set the IO region
    if(nd_actual <= 3)
      {
      // This is the old code, which we preserve
      itk::ImageIORegion ioRegion(3);
      itk::ImageIORegionAdaptor<3>::Convert(region, ioRegion, index);
      m_IOBase->SetIORegion(ioRegion);
      }
    else
      {
      itk::ImageIORegion ioRegion(nd_actual);
      itk::ImageIORegion::IndexType ioIndex;
      itk::ImageIORegion::SizeType ioSize;
      for(int i = 0; i < nd_actual; i++)
        {
        ioIndex.push_back(0);
        ioSize.push_back(m_IOBase->GetDimensions(i));
        }
      ioRegion.SetIndex(ioIndex);
      ioRegion.SetSize(ioSize);
      m_IOBase->SetIORegion(ioRegion);
      }

    // Read the image into the buffer
    m_IOBase->Read(image->GetBufferPointer());
    m_NativeImage = image;

    // If the image is 4-dimensional or more, we must perform an in-place transpose
    // of the image. The fourth dimension is the one that varies fastest, and in our
    // representation, the image is represented as a VectorImage, where the components
    // of each voxel are the thing that moves fastest. The problem can be represented as
    // a transpose of a N x M array, where N = dimX*dimY*dimZ and M = dimW
    if(nd_actual > 3)
      {
      long N = dim[0] * dim[1] * dim[2];
      long M = ncomp;
      long move_size = (2 * M) * sizeof(TScalar);
      char *move = new char[move_size];
      TScalar buffer[2];

      // TODO: this is a pretty slow routine. It would be nice to find somehting a little
      // faster than this. But at least we can read 4D data now
      itk::TimeProbe probe;
      probe.Start();
      transpose_toms513(image->GetBufferPointer(), M, N, move, move_size, buffer);
      probe.Stop();

      std::cout << "Transpose of " << N << " by " << M << " matrix computed in " << probe.GetTotal() << " sec." << std::endl;
      delete move;
      }

    
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

template<typename TImageType>
void
GuidedNativeImageIO
::SaveImage(const char *FileName, Registry &folder, TImageType *image)
{
  // Create an Image IO based on the folder
  CreateImageIO(FileName, folder, false);

  // Save the image
  typedef itk::ImageFileWriter<TImageType> WriterType;
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

template<class TOutputImage>
typename RescaleNativeImageToIntegralType<TOutputImage>::OutputImageType *
RescaleNativeImageToIntegralType<TOutputImage>::operator()(
    GuidedNativeImageIO *nativeIO)
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

template<typename TPixel, typename TNative>
class RescaleVectorNativeImageToVectorFunctor
{
public:

  typedef TPixel PixelType;

  RescaleVectorNativeImageToVectorFunctor(double shift, double scale)
    : m_Shift(shift), m_Scale(scale) {}

  RescaleVectorNativeImageToVectorFunctor()
    : m_Shift(0), m_Scale(1) {}

  void operator()(TNative *src, TPixel *trg)
  {
    *trg = (TPixel) ((*src + m_Shift) * m_Scale + 0.5);
  }

protected:

  double m_Shift, m_Scale;

};




template<class TOutputImage>
template<typename TNative>
void
RescaleNativeImageToIntegralType<TOutputImage>
::DoCast(itk::ImageBase<3> *native)
{
  // Get the native image
  typedef itk::VectorImage<TNative, 3> InputImageType;
  typedef itk::ImageRegionConstIterator<InputImageType> InputIterator;
  SmartPtr<InputImageType> input = dynamic_cast<InputImageType *>(native);

  assert(input);
  assert(input->GetPixelContainer()->Size() > 0);

  // Get the number of components in the native image
  size_t ncomp = input->GetNumberOfComponentsPerPixel();

  // We must compute a scale and shift factor
  double scale = 1.0, shift = 0.0;

  // The type of the component in the output image. Now, the output image here
  // may be either a VectorImage or an Image.
  typedef typename OutputImageType::InternalPixelType OutputComponentType;

  // Only bother with computing the scale and shift if the types are different
  if(typeid(OutputComponentType) != typeid(TNative))
    {
    // We must compute the range of the input data    
    OutputComponentType omax = itk::NumericTraits<OutputComponentType>::max();
    OutputComponentType omin = itk::NumericTraits<OutputComponentType>::min();

    // Scan over all the image components. Avoid using iterators here because of
    // unnecessary overhead for vector images.
    TNative *ib_begin = input->GetBufferPointer();
    TNative *ib_end = ib_begin + input->GetPixelContainer()->Size();

    TNative imin_nat = *ib_begin, imax_nat = *ib_begin;

    // Iterate over all the components in the input image
    for(TNative *buffer = ib_begin + 1; buffer < ib_end; ++buffer)
      {
      TNative val = *buffer;
      if(val < imin_nat) imin_nat = val;
      if(val > imax_nat) imax_nat = val;
      }

    // Cast the values to double
    double imin = static_cast<double>(imin_nat), imax = static_cast<double>(imax_nat);

    // Now we have to be careful, depending on the type of the input voxel
    // For float and double, we map the input range into the output range
    if(!itk::NumericTraits<TNative>::is_integer)
      {
      // Test whether the input image is actually an integer image cast to
      // floating point. In that case, there is no need for conversion
      bool isint = false;
      if(1.0 * omin <= imin && 1.0 * omax >= imax && ncomp == 1)
        {
        isint = true;

        // Another pass through the image? Why is this necessary?
        for(TNative *buffer = ib_begin; buffer < ib_end; ++buffer)
          {
          TNative vin = *buffer;
          TNative vcmp = static_cast<TNative>(static_cast<OutputComponentType>(vin + 0.5));
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

        // Does the input range include zero?
        if(imin <= 0 && imax >= 0)
          {
          // If so, there will be no shift, allowing zero to map to zero
          scale = std::min((double) omax, -1.0 * (double) omin) * 1.0 / std::max(imax, -imin);
          shift = 0.0;
          }

        else
          {
          // Zero is not in the range, so just use the complete range
          scale = (1.0 * omax - 1.0 * omin) / (imax - imin);
          shift = omin / scale - imin;
          }
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
  typedef RescaleVectorNativeImageToVectorFunctor<OutputComponentType, TNative> Functor;
  CastNativeImage<OutputImageType, Functor> caster;
  caster.SetFunctor(Functor(shift, scale));
  caster.template DoCast<TNative>(native);
  m_Output = caster.m_Output;
}

template<class TOutputImage, class TCastFunctor>
typename CastNativeImage<TOutputImage,TCastFunctor>::OutputImageType *
CastNativeImage<TOutputImage,TCastFunctor>
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

#include "itkMeasurementVectorTraits.h"

template<class TOutputImage, class TCastFunctor>
template<typename TNative>
void
CastNativeImage<TOutputImage,TCastFunctor>
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

  // Allocate the output image
  m_Output = OutputImageType::New();
  m_Output->CopyInformation(native);
  m_Output->SetMetaDataDictionary(native->GetMetaDataDictionary());
  m_Output->SetRegions(native->GetBufferedRegion());

  // CAREFUL! At this point, it may be that the number of components in the
  // output is still 1 (because the output is an itk::Image) and the number
  // of components in the input is not 1. In that case, we can either throw
  // an exception or use the first component of the input image to fill out
  // the output image. For the time being, we will throw an exception.
  int ncomp = input->GetNumberOfComponentsPerPixel();
  int ncomp_out = m_Output->GetNumberOfComponentsPerPixel();
  if(ncomp != ncomp_out)
    {
    throw IRISException("Unable to cast an input image with %d components to "
                        "an output image with %d components", ncomp, ncomp_out);
    }

  // Special case: native image is the same as target image
  if(typeid(OutputComponentType) == typeid(TNative))
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
  size_t szNative = sizeof(TNative);
  size_t szTarget = sizeof(OutputComponentType);

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
  OutputComponentType *ob = reinterpret_cast<OutputComponentType *>(ib);

  // Finally, we get to the code where we map from input format to the output
  // format. Here again we have to be careful. If the native image is larger or
  // same than the target image, we want to proceed in ascending order, since each
  // input element will be replaced by one or more output elements. But if the
  // native image is smaller, we want to proceed from the end of the memory
  // block in a descending order, so that the native data is not overridden
  unsigned long nval =  nvoxels * ncomp;
  if(szTarget > szNative)
    {
    TNative *pn = ib + nval - 1;
    OutputComponentType *pt = ob + nval - 1;
    for(; pt >= ob; pt--, pn--)
      m_Functor(pn, pt);
    }
  else
    {
    TNative *pn = ib;
    OutputComponentType *pt = ob;
    for(; pt < ob + nval; pt++, pn++)
      m_Functor(pn, pt);
    }

  // If needed, squeeze the memory
  if(nbTarget < nbNative)
    ob = reinterpret_cast<OutputComponentType *>(realloc(ob, nbTarget));

  // Create a new container wrapped around the same chunk of memory as the
  // native image. The size we pass in here is the number of elements that
  // have been already allocated. We will shrink that memory later
  SmartPtr<OutPixCon> pc = OutPixCon::New();
  pc->SetImportPointer(ob, nval, true);

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
      {
      // PY: Now that I cleaned up the DICOM reader, we should never default
      // to single DICOM file anymore. That's just too annoying
      return FORMAT_DICOM_DIR;
      }

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
      {
      return fmt;
      }
    }

  // Nothing matched
  return FORMAT_COUNT;
}


const gdcm::Tag GuidedNativeImageIO::m_tagRows(0x0028, 0x0010);
const gdcm::Tag GuidedNativeImageIO::m_tagCols(0x0028, 0x0011);
const gdcm::Tag GuidedNativeImageIO::m_tagDesc(0x0008, 0x103e);
const gdcm::Tag GuidedNativeImageIO::m_tagTextDesc(0x0028, 0x0010);
const gdcm::Tag GuidedNativeImageIO::m_tagSeriesInstanceUID(0x0020,0x000E);
const gdcm::Tag GuidedNativeImageIO::m_tagSeriesNumber(0x0020,0x0011);
const gdcm::Tag GuidedNativeImageIO::m_tagAcquisitionNumber(0x0020,0x0012);
const gdcm::Tag GuidedNativeImageIO::m_tagInstanceNumber(0x0020,0x0013);



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

  // Use the ITK stuff for parsing
  if(m_GDCMSeries.IsNull() || m_GDCMSeriesDirectory != dir)
    {
    m_GDCMSeries = itk::GDCMSeriesFileNames::New();
    m_GDCMSeries->SetUseSeriesDetails(true);
    m_GDCMSeries->SetDirectory(dir);
    m_GDCMSeriesDirectory = dir;
    }

  // List all the unique series ids
  const itk::SerieUIDContainer uids = m_GDCMSeries->GetSeriesUIDs();
  for(int i = 0; i < uids.size(); i++)
    {
    // Get the filenames for this serie
    const itk::FilenamesContainer &fc = m_GDCMSeries->GetFileNames(uids[i]);
    if(!fc.size())
      continue;

    // Initialize the registry for this parsing
    Registry r;
    r["SeriesId"] << uids[i];

    // Get tags for this file
    std::set<gdcm::Tag> tagset;
    tagset.insert(m_tagRows);
    tagset.insert(m_tagCols);
    tagset.insert(m_tagSeriesNumber);
    tagset.insert(m_tagDesc);

    // Read the tags
    gdcm::Reader reader;
    reader.SetFileName(fc.front().c_str());
    bool read = false;

    try { read = reader.ReadSelectedTags(tagset); }
    catch(...) { read = false; }

    if(read)
      {
      gdcm::StringFilter sf;
      sf.SetFile(reader.GetFile());

      // Read series description
      r["SeriesDescription"] << sf.ToString(m_tagDesc);
      r["SeriesNumber"] << sf.ToString(m_tagSeriesNumber);

      // Read the dimensions
      ostringstream oss;
      oss << sf.ToString(m_tagRows) << " x "
          << sf.ToString(m_tagCols) << " x "
          << fc.size();

      r["Dimensions"] << oss.str();
      r["NumberOfImages"] << fc.size();

      // Add the registry to the list
      reg.push_back(r);
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

template class RescaleNativeImageToIntegralType<itk::Image<GreyType, 3> >;
template class RescaleNativeImageToIntegralType<itk::VectorImage<GreyType, 3> >;

template class CastNativeImage<itk::Image<unsigned short, 3> >;

// template class CastNativeImageBase<RGBType, CastToArrayFunctor<RGBType, 3> >;
// template class CastNativeImageBase<LabelType, CastToScalarFunctor<LabelType> >;
// template class CastNativeImageBase<float, CastToScalarFunctor<float> >;

/*
template class CastNativeImageToRGB<RGBType>; 
template class CastNativeImageToScalar<LabelType>; 
template class CastNativeImageToScalar<float>; 
*/

template void GuidedNativeImageIO::SaveImage(const char *, Registry &, itk::Image<GreyType,3> *);
template void GuidedNativeImageIO::SaveImage(const char *, Registry &, itk::Image<LabelType,3> *);
template void GuidedNativeImageIO::SaveImage(const char *, Registry &, itk::Image<float,3> *);
template void GuidedNativeImageIO::SaveImage(const char *, Registry &, itk::Image<RGBType,3> *);



