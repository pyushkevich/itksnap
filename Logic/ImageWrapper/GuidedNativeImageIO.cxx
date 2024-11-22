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
#include "itkMINCImageIO.h"
#include "itkNiftiImageIO.h"
#include "itkSiemensVisionImageIO.h"
#include "itkVTKImageIO.h"
#include "itkVoxBoCUBImageIO.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImageSeriesReader.h"
#include "itkImageIOFactory.h"
#include "gdcmFile.h"
#include "gdcmReader.h"
#include "gdcmSerieHelper.h"
#include "gdcmStringFilter.h"
#include "gdcmDataSetHelper.h"
#include "gdcmElement.h"
#include "itkMinimumMaximumImageCalculator.h"
#include "itkShiftScaleImageFilter.h"
#include "itkNumericTraits.h"
#include <itkTimeProbe.h>
#include "itksys/MD5.h"
#include "ExtendedGDCMSerieHelper.h"
#include "itkComposeImageFilter.h"
#include "itkStreamingImageFilter.h"
#include "IncreaseDimensionImageFilter.h"
#include "MultiFrameDicomSeriesSorter.h"
#include "itkStringTools.h"
#include "AllPurposeProgressAccumulator.h"

#include <itk_zlib.h>
#include "itkImportImageFilter.h"
#include <algorithm>
#include "itksys/Base64.h"


using namespace std;

bool GuidedNativeImageIO::m_StaticDataInitialized = false;

RegistryEnumMap<GuidedNativeImageIO::FileFormat> GuidedNativeImageIO::m_EnumFileFormat;
RegistryEnumMap<GuidedNativeImageIO::RawPixelType> GuidedNativeImageIO::m_EnumRawPixelType;

/* name, pattern, can_write, can_store_orientation, can_store_float, can_store_short */
const GuidedNativeImageIO::FileFormatDescriptor 
GuidedNativeImageIO
::m_FileFormatDescrictorArray[] = {
  {"Analyze", "img.gz,hdr,img",         true,  false, true,  true},
  {"DICOM Image Series", "",            false, true,  true,  true},
  {"4D CTA DICOM Series", "",           false, true,  true,  true},
  {"DICOM Single Image", "dcm",         false, true,  true,  true},
  {"Echo Cartesian DICOM", "dcm",       false, true,  true,  true},
  {"GE Version 4", "ge4",               false, false, true,  true},
  {"GE Version 5", "ge5",               false, false, true,  true},
  {"GIPL", "gipl,gipl.gz",              true,  false, true,  true},
  {"MetaImage", "mha,mhd",              true,  true,  true,  true},
  {"MINC", "mnc",                       true,  true,  true,  true},
  {"NiFTI", "nii.gz,nii,nia,nia.gz",    true,  true,  true,  true},
  {"NRRD Volume Sequence", "seq.nrrd",  false, true,  true,  true},
  {"NRRD", "nrrd,nhdr",                 true,  true,  true,  true},
  {"Raw Binary", "raw",                 false, false, true,  true},
  {"Siemens Vision", "ima",             false, false, true,  true},
  {"VoxBo CUB", "cub,cub.gz",           true,  false, true,  true},
  {"VTK Image", "vtk",                  true,  false, true,  true},
  {"Generic ITK Image", "",             true,  true,  true,  true},
  {"INVALID FORMAT", "",                false, false, false, false}};


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

  m_NativeType = itk::IOComponentEnum::UNKNOWNCOMPONENTTYPE;
  m_NativeComponents = 0;
  m_NativeTypeString = m_IOBase->GetComponentTypeAsString(m_NativeType);
  m_NativeFileName = "";
  m_NativeByteOrder = itk::IOByteOrderEnum::OrderNotApplicable;
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

  // Is the DICOM single image an Echo Cartesian DICOM image?
  if (m_FileFormat == FORMAT_DICOM_FILE || m_FileFormat == FORMAT_COUNT)
    m_FileFormat = GuessFormatForFileName(fname, true);

  // Choose the approach based on the file format
  switch(m_FileFormat)
    {
		case FORMAT_MHA:        m_IOBase = itk::MetaImageIO::New();          break;
    case FORMAT_NRRD_SEQ:   m_IOBase = itk::NrrdImageIO::New();          break;
		case FORMAT_NRRD:       m_IOBase = itk::NrrdImageIO::New();          break;
		case FORMAT_ANALYZE:    m_IOBase = itk::NiftiImageIO::New();         break;
		case FORMAT_GIPL:       m_IOBase = itk::GiplImageIO::New();          break;
		case FORMAT_GE4:        m_IOBase = itk::GE4ImageIO::New();           break;
		case FORMAT_GE5:        m_IOBase = itk::GE5ImageIO::New();           break;
		case FORMAT_MINC:       m_IOBase = itk::MINCImageIO::New();          break;
		case FORMAT_NIFTI:      m_IOBase = itk::NiftiImageIO::New();         break;
		case FORMAT_SIEMENS:    m_IOBase = itk::SiemensVisionImageIO::New(); break;
		case FORMAT_VTK:        m_IOBase = itk::VTKImageIO::New();           break;
		case FORMAT_VOXBO_CUB:  m_IOBase = itk::VoxBoCUBImageIO::New();      break;
		case FORMAT_DICOM_DIR:
		case FORMAT_DICOM_DIR_4DCTA:
    case FORMAT_ECHO_CARTESIAN_DICOM:
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
        case PIXELTYPE_CHAR:   CreateRawImageIO<signed char>(fldRaw);    break;
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
::ReadNativeImageHeader(const char *FileName, Registry &folder, itk::Command *progressCmd)
{
	/* Progress Command Usage:
	 * We only add progressCmd as observers to each conditional branch, which
	 * means we don't have shared progress in the header reading method. Assuming
	 * shared logic potion does not have significant impact on overall progress.
	 *
	 * In the future, if we need to distinguish beetween shared progress vs conditional
	 * progress, we need to use an AllPurposeProgressAccumulator to combine the two.
	 */

	// create an empty command to prevent errors
	if (progressCmd == nullptr)
		progressCmd = DoNothingCommandSingleton::GetInstance().GetCommand();


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
	if(m_FileFormat == FORMAT_DICOM_DIR || m_FileFormat == FORMAT_DICOM_DIR_4DCTA)
    {
    // Get the directory where to search for the series
    std::string SeriesDir = FileName;
    if(!itksys::SystemTools::FileIsDirectory(FileName))
      SeriesDir = itksys::SystemTools::GetParentDirectory(FileName);

    // Have we already parsed this directory?
    if(m_LastDicomParseResult.Directory != SeriesDir)
      {
      // Parse the specified directory
			this->ParseDicomDirectory(SeriesDir);
      }

    // Select which series
    std::string SeriesID = m_Hints["DICOM.SeriesId"][""];
    if(SeriesID.length() == 0)
      {
      // There must be at least one series
      if(m_LastDicomParseResult.SeriesMap.size() == 0)
        throw IRISException("Error: DICOM series not found. "
                            "Directory '%s' does not appear to contain a "
                            "series of DICOM images.", FileName);

      // Take the first series ID we have
      SeriesID = m_LastDicomParseResult.SeriesMap.begin()->first;
      }

    // Obtain the filename for this series
    m_DICOMFiles = m_LastDicomParseResult.SeriesMap[SeriesID].FileList;

    // Read the information from the first filename
    if(m_DICOMFiles.size() == 0)
      throw IRISException("Error: DICOM series not found. "
                          "Directory '%s' does not appear to contain a "
                          "series of DICOM images.",FileName);

		if (m_FileFormat == FORMAT_DICOM_DIR)
			{
			SmartPtr<TrivalProgressSource> dcmHdrProgSrc = TrivalProgressSource::New();
      dcmHdrProgSrc->AddObserverToProgressEvents(progressCmd);
      dcmHdrProgSrc->StartProgress();
			dcmHdrProgSrc->AddProgress(0.1);

			// Following this quick parsing of the directory, we need to actually
			// load the image data and sort it in a meaningful order. This is too
			// complicated to replicate here so we revert to gdcm::SerieHelper, but
			// we only have it parse the filenames for the current SeriesId
			ExtendedGDCMSerieHelper helper;
			helper.SetFilesAndOrder(m_DICOMFiles, m_DICOMImagesPerIPP);

			m_IOBase->SetFileName(m_DICOMFiles[0]);

			dcmHdrProgSrc->AddProgress(0.9);

			}
		else if (m_FileFormat == FORMAT_DICOM_DIR_4DCTA)
			{
			MFDS::MultiFrameDicomSeriesSorter::Pointer MFDSSorter
					= MFDS::MultiFrameDicomSeriesSorter::New();

			MFDSSorter->SetFileNameList(m_DICOMFiles);
			MFDSSorter->SetGroupingStrategy(MFDS::MFGroupByIPP2Strategy::New());
			MFDSSorter->SetFrameOrderingStrategy(MFDS::MFOrderByInstanceNumberStrategy::New());
			MFDSSorter->SetSliceOrderingStrategy(MFDS::MFOrderByIPPStrategy::New());
			MFDSSorter->AddObserver(itk::ProgressEvent(), progressCmd);
      MFDSSorter->AddObserver(itk::StartEvent(), progressCmd);
      MFDSSorter->AddObserver(itk::EndEvent(), progressCmd);
			MFDSSorter->Sort();
			m_DicomFilesToFrameMap = MFDSSorter->GetOutput();
			m_IOBase->SetFileName(m_DICOMFiles[0]);
			}

		m_IOBase->ReadImageInformation();
    }
  else if (m_FileFormat == FORMAT_ECHO_CARTESIAN_DICOM)
    {
    SmartPtr<TrivalProgressSource> ecdHeaderProgSrc = TrivalProgressSource::New();
    ecdHeaderProgSrc->AddObserverToProgressEvents(progressCmd);
    ecdHeaderProgSrc->StartProgress();

		gdcm::Reader ecd_reader;
		ecd_reader.SetFileName(FileName);
		std::set<gdcm::Tag> headerTags;

		const gdcm::Tag deltaX(0x18, 0x602c);
		const gdcm::Tag deltaY(0x18, 0x602e);
		const gdcm::Tag deltaZ(0x3001, 0x1003);
		const gdcm::Tag numVolumes(0x28, 0x8);
		const gdcm::Tag height(0x28, 0x10);
		const gdcm::Tag width(0x28, 0x11);
		const gdcm::Tag depth(0x3001, 0x1001);
		const gdcm::Tag frameTime(0x18, 0x1063);

		headerTags.insert(deltaX);
		headerTags.insert(deltaY);
		headerTags.insert(deltaZ);
		headerTags.insert(numVolumes);
		headerTags.insert(height);
		headerTags.insert(width);
		headerTags.insert(depth);
		headerTags.insert(frameTime);

		if (ecd_reader.ReadSelectedTags(headerTags))
			{
			ecdHeaderProgSrc->AddProgress(0.8);
			m_IOBase->SetFileName(FileName);

			gdcm::File &file = ecd_reader.GetFile();
			gdcm::StringFilter sf;
			sf.SetFile(file);

			std::vector<double> ecd_spc;
			try
				{
				ecd_spc.push_back(std::stod(sf.ToString(deltaX)) * 10.0);
				ecd_spc.push_back(std::stod(sf.ToString(deltaY)) * 10.0);
				ecd_spc.push_back(std::stod(sf.ToString(deltaZ)) * 10.0);
				// frame time is the spacing along the time axis
				ecd_spc.push_back(std::stod(sf.ToString(frameTime)));
				}
			catch (const std::exception &e)
				{
				std::cerr << e.what() << std::endl;
				}

			// Set to 4d
			m_IOBase->SetNumberOfDimensions(4);

			// Set spacing in IOBase
			for (unsigned int i = 0; i < 4; ++i)
				m_IOBase->SetSpacing(i, ecd_spc[i]);

			// Set origin in IOBase
			for (unsigned int i = 0; i < 4; ++i)
				m_IOBase->SetOrigin(i, 0.0);

      // Set direction to LAS
      m_IOBase->SetDirection(0, std::vector<double>{-1.0, 0.0, 0.0, 0.0}); // L
      m_IOBase->SetDirection(1, std::vector<double>{0.0, 1.0, 0.0, 0.0});  // A
      m_IOBase->SetDirection(2, std::vector<double>{0.0, 0.0, -1.0, 0.0}); // S
			m_IOBase->SetDirection(3, std::vector<double>{0.0, 0.0, 0.0, 1.0});

			std::vector<itk::ImageIOBase::SizeType> ecd_dim(4);

			try
				{
				ecd_dim[0] = stol(sf.ToString(width));
				ecd_dim[1] = stol(sf.ToString(height));
				ecd_dim[2] = stol(sf.ToString(depth));
				ecd_dim[3] = stol(sf.ToString(numVolumes));
				}
			catch (std::exception &e)
				{
				std::cerr << e.what() << std::endl;
				}

			for (unsigned int i = 0; i < 4; ++i)
				{
				m_IOBase->SetDimensions(i, ecd_dim[i]);
				}

			m_IOBase->SetNumberOfComponents(1);
			m_IOBase->SetComponentType(itk::IOComponentEnum::UCHAR);

			ecdHeaderProgSrc->AddProgress(0.2);
			}
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

		// IOBase not reporting progress. Using a trivial progress source
		SmartPtr<TrivalProgressSource> progSrc = TrivalProgressSource::New();
    progSrc->AddObserverToProgressEvents(progressCmd);
    progSrc->StartProgress();

    m_IOBase->ReadImageInformation();
		progSrc->AddProgress(1.0);
    }

  UpdateMemberVariables();
}

GuidedNativeImageIO::DispatchBase*
GuidedNativeImageIO
::CreateDispatch(itk::IOComponentEnum comp_type)
{
  // Based on the component type, read image in native mode
  switch(comp_type)
    {
    case itk::IOComponentEnum::UCHAR:  return new Dispatch<unsigned char>();   break;
    case itk::IOComponentEnum::CHAR:   return new Dispatch<signed char>();            break;
    case itk::IOComponentEnum::USHORT: return new Dispatch<unsigned short>();  break;
    case itk::IOComponentEnum::SHORT:  return new Dispatch<short>();           break;
    case itk::IOComponentEnum::UINT:   return new Dispatch<unsigned int>();    break;
    case itk::IOComponentEnum::INT:    return new Dispatch<int>();             break;
    case itk::IOComponentEnum::ULONG:  return new Dispatch<unsigned long>();   break;
    case itk::IOComponentEnum::LONG:   return new Dispatch<long>();            break;
    case itk::IOComponentEnum::FLOAT:  return new Dispatch<float>();           break;
    case itk::IOComponentEnum::DOUBLE: return new Dispatch<double>();          break;
    default:
      throw IRISException("Error: Unsupported voxel type."
                          "Unsupported voxel type ('%s') encountered in GuidedNativeImageIO",
                          m_IOBase->GetComponentTypeAsString(
                            m_IOBase->GetComponentType()).c_str());
    }
}

void
GuidedNativeImageIO
::ReadNativeImageData(itk::Command *progressCmd)
{
  // Based on the component type, read image in native mode
  DispatchBase *dispatch = this->CreateDispatch(m_IOBase->GetComponentType());
	dispatch->ReadNative(this, m_NativeFileName.c_str(), m_Hints, progressCmd);
  delete dispatch;

  // Get rid of the IOBase, it may store useless data (in case of NIFTI)
  m_IOBase = NULL;
}

void
GuidedNativeImageIO
::ReadNativeImage(const char *FileName, Registry &folder, itk::Command *progressCmd)
{
	this->ReadNativeImageHeader(FileName, folder, progressCmd);
	this->ReadNativeImageData(progressCmd);
}

template <typename TScalar>
void
GuidedNativeImageIO
::ConvertToVectorImage(
		itk::VectorImage<TScalar, 4> *output, itk::Image<TScalar, 4> *input) const
{
	output->CopyInformation(input);
	output->SetRegions(input->GetBufferedRegion());

	typedef itk::VectorImage<TScalar, 4> NativeImageType;
	typedef typename NativeImageType::PixelContainer PixConType;
	typename PixConType::Pointer pc = PixConType::New();
	pc->SetImportPointer(
				reinterpret_cast<TScalar *>(input->GetBufferPointer()),
				input->GetBufferedRegion().GetNumberOfPixels(), true);
	output->SetPixelContainer(pc);

	// Prevent the container from being deleted
	input->GetPixelContainer()->SetContainerManageMemory(false);
}

template <class TImage>
void
GuidedNativeImageIO
::DeepCopyImage(
		typename TImage::Pointer output, typename TImage::Pointer input) const
{
	output->SetRegions(input->GetLargestPossibleRegion());
	output->SetOrigin(input->GetOrigin());
	output->SetDirection(input->GetDirection());
	output->SetSpacing(input->GetSpacing());
	output->Allocate();

	itk::ImageRegionConstIterator<TImage> inputIterator(input, input->GetLargestPossibleRegion());
	itk::ImageRegionIterator<TImage>      outputIterator(output, output->GetLargestPossibleRegion());

	while (!inputIterator.IsAtEnd())
		{
		outputIterator.Set(inputIterator.Get());
		++inputIterator;
		++outputIterator;
		}
}

void
GuidedNativeImageIO
::UpdateMemberVariables()
{
  auto ncomp = m_IOBase->GetNumberOfComponents();

  // Set the dimensions (if 2D image, we set last dim to 1)
  m_NativeDimensions.fill(1);
  for(size_t i = 0; i < m_IOBase->GetNumberOfDimensions(); i++)
    {
    if(i < 4)
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

template <typename NativeImageType>
void
GuidedNativeImageIO
::UpdateImageHeader(typename NativeImageType::Pointer image)
{
  // Initialize the direction and spacing, etc
  typename NativeImageType::SizeType dim;      dim.Fill(1);
  typename NativeImageType::PointType org;     org.Fill(0.0);
  typename NativeImageType::SpacingType spc;   spc.Fill(1.0);
  typename NativeImageType::DirectionType dir; dir.SetIdentity();

  m_NDimBeforeFolding = m_IOBase->GetNumberOfDimensions();
  size_t nd = (m_NDimBeforeFolding > 4) ? 4 : m_NDimBeforeFolding;

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
  m_NCompBeforeFolding = m_IOBase->GetNumberOfComponents();
  m_NCompAfterFolding = m_NCompBeforeFolding;
  if(m_NDimBeforeFolding > nd)
    {
    for(int i = nd; i < (int) m_NDimBeforeFolding; i++)
      m_NCompAfterFolding *= m_IOBase->GetDimensions(i);
    }

  // Set the regions
  typename NativeImageType::RegionType region;
  typename NativeImageType::IndexType index = {{0, 0, 0, 0}};
  region.SetIndex(index);
  region.SetSize(dim);
  image->SetRegions(region);
  image->SetVectorLength(m_NCompAfterFolding);

  // Set the IO region
  if(m_NDimBeforeFolding <= 4)
    {
    // This is the old code, which we preserve
    itk::ImageIORegion ioRegion(4);
    itk::ImageIORegionAdaptor<4>::Convert(region, ioRegion, index);
    m_IOBase->SetIORegion(ioRegion);
    }
  else
    {
    itk::ImageIORegion ioRegion(m_NDimBeforeFolding);
    itk::ImageIORegion::IndexType ioIndex;
    itk::ImageIORegion::SizeType ioSize;
    for(size_t i = 0; i < m_NDimBeforeFolding; i++)
      {
      ioIndex.push_back(0);
      ioSize.push_back(m_IOBase->GetDimensions(i));
      }
    ioRegion.SetIndex(ioIndex);
    ioRegion.SetSize(ioSize);
    m_IOBase->SetIORegion(ioRegion);
    }
}

template <typename NativeImageType>
typename NativeImageType::Pointer
GuidedNativeImageIO
::Convert4DLoadToMultiComponent(typename NativeImageType::Pointer image4D)
{
  // rearrange the pixel container
  auto nt = m_IOBase->GetDimensions(m_NDimBeforeFolding - 1);

  using ElementType = typename NativeImageType::PixelContainer::Element;
  using ElementIdType = typename NativeImageType::PixelContainer::ElementIdentifier;
  ElementIdType ne = 1; // # of elements in one slice
  typename NativeImageType::SizeType pcSize = image4D->GetLargestPossibleRegion().GetSize();
  for (int i = 0; i < m_IOBase->GetNumberOfDimensions(); ++i)
    ne *= pcSize[i];

  ElementIdType neTP = ne / nt; // # of Elements per Time Point
  ElementType *bufferMC = new ElementType[ne];

  auto *buffer4D = image4D->GetPixelContainer()->GetBufferPointer();

  // reorder voxels from 4d to multi-component
  for (ElementIdType i = 0; i < nt; ++i)
    for (ElementIdType j = 0; j < neTP; ++j)
      bufferMC[j * nt + i] = buffer4D[i * neTP + j];


  // Modify Header
  m_IOBase->SetNumberOfComponents(nt);
  m_IOBase->SetDimensions(m_NDimBeforeFolding - 1, 1);

  // important after modifying header, or UI won't render correctly
  UpdateMemberVariables();

  // create a new multi-component image
  auto imageMC = NativeImageType::New();

  UpdateImageHeader<NativeImageType>(imageMC);
  imageMC->GetPixelContainer()->SetImportPointer(bufferMC, ne, true);

  return imageMC;
}

template <typename NativeImageType>
typename NativeImageType::Pointer
GuidedNativeImageIO
::ConvertMultiComponentLoadTo4D(typename NativeImageType::Pointer imageMC)
{
  // rearrange the pixel container
  auto nc = m_IOBase->GetNumberOfComponents();

  using ElementType = typename NativeImageType::PixelContainer::Element;
  using ElementIdType = typename NativeImageType::PixelContainer::ElementIdentifier;
  ElementIdType neTP = 1, ne; // # of elements in one slice
  typename NativeImageType::SizeType pcSize = imageMC->GetLargestPossibleRegion().GetSize();
  for (int i = 0; i < m_IOBase->GetNumberOfDimensions(); ++i)
    {
    neTP *= pcSize[i];
    }

  ne = neTP * nc; // # of Elements per Time Point
  ElementType *buffer4D = new ElementType[ne];

  auto *bufferMC = imageMC->GetPixelContainer()->GetBufferPointer();

  // reorder voxels from multi-component to 4d
  for (ElementIdType i = 0; i < nc; ++i)
    for (ElementIdType j = 0; j < neTP; ++j)
      buffer4D[i * neTP + j] = bufferMC[j * nc + i];

  // Modify Header
  // -- backup origin, spacing and direction
  std::vector<std::vector<double>> dir {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
  std::vector<double> org {0, 0, 0, 0}, spc {1, 1, 1, 1};
  for (int i = 0; i < m_NDimBeforeFolding; ++i)
    {
    org[i] = m_IOBase->GetOrigin(i);
    spc[i] = m_IOBase->GetSpacing(i);
    for (int j = 0; j < m_NDimBeforeFolding; ++j)
      {
      dir[i][j] = m_IOBase->GetDirection(i)[j];
      }
    }

  m_IOBase->SetNumberOfComponents(1);
  m_NDimBeforeFolding += 1; // increment the dimension
  m_IOBase->SetNumberOfDimensions(m_NDimBeforeFolding);
  m_IOBase->SetDimensions(m_NDimBeforeFolding - 1, nc);

  // -- restore spacing, direction and origin
  for (int i = 0; i < m_NDimBeforeFolding - 1; ++i)
    {
    m_IOBase->SetOrigin(i, org[i]);
    m_IOBase->SetSpacing(i, spc[i]);
    m_IOBase->SetDirection(i, dir[i]);
    }


  // important after modifying header, or UI won't render correctly
  UpdateMemberVariables();

  // create a new multi-component image
  auto image4D = NativeImageType::New();

  UpdateImageHeader<NativeImageType>(image4D);
  image4D->GetPixelContainer()->SetImportPointer(buffer4D, ne, true); // imageMC will take care of this memory
  return image4D;
}


template<class TScalar>
void
GuidedNativeImageIO
::DoReadNative(const char *FileName, Registry &, itk::Command *progressCmd)
{
	if (!progressCmd)
		progressCmd = DoNothingCommandSingleton::GetInstance().GetCommand();


  // Define the image type of interest
  typedef itk::VectorImage<TScalar, 4> NativeImageType;
	typedef itk::Image<TScalar, 3> GreyImageType;
	typedef itk::Image<TScalar, 4> GreyImage4DType;
	typedef itk::ImageSeriesReader<GreyImageType> SeriesReaderType;

  // There is a special handler for the DICOM case!
  if(m_FileFormat == FORMAT_DICOM_DIR && m_DICOMFiles.size() > 1)
    {
    // Filter for increasing dimensionality
    typedef IncreaseDimensionImageFilter<GreyImageType, GreyImage4DType> UpDimFilter;

		SmartPtr<TrivalProgressSource> dcmSeriesProgSrc = TrivalProgressSource::New();
    dcmSeriesProgSrc->AddObserverToProgressEvents(progressCmd);
    dcmSeriesProgSrc->StartProgress();

    if(this->m_DICOMImagesPerIPP == 1)
      {
      // When there is a single volume
			typename SeriesReaderType::Pointer reader = SeriesReaderType::New();
			reader->AddObserver(itk::ProgressEvent(), progressCmd);

      // Set the filenames and read
      reader->SetFileNames(m_DICOMFiles);

      // Set the IO
      // typename GDCMImageIO::Pointer dicomio = GDCMImageIO::New();
      // dicomio->SetMaxSizeLoadEntry(0xffff);
      // m_IOBase = dicomio;
      reader->SetImageIO(m_IOBase);

      // Present this scalar as a 4D image
      typename UpDimFilter::Pointer updim = UpDimFilter::New();
      updim->SetInput(reader->GetOutput());
      updim->Update();
      GreyImage4DType *scalar = updim->GetOutput();

      // Convert the image into VectorImage format. Do this in-place to avoid
      // allocating memory pointlessly
      typename NativeImageType::Pointer vector = NativeImageType::New();
      m_NativeImage = vector;

			ConvertToVectorImage<TScalar>(vector, scalar);

      // Copy the metadata from the first scan in the series
			const typename SeriesReaderType::DictionaryArrayType *darr =
        reader->GetMetaDataDictionaryArray();
      if(darr->size() > 0)
        m_NativeImage->SetMetaDataDictionary(*((*darr)[0]));

			dcmSeriesProgSrc->AddProgress(1.0);
      }
    else
      {
      // Create a filter that will do the composing
      typedef itk::ComposeImageFilter<GreyImage4DType, NativeImageType> ComposeFilter;
      typename ComposeFilter::Pointer composer = ComposeFilter::New();

      // Create a splitter
      typedef itk::StreamingImageFilter<NativeImageType,NativeImageType> StreamingFilter;
      typename StreamingFilter::Pointer streamer = StreamingFilter::New();

      // Create separate volume readers
      int n_slices = m_DICOMFiles.size() / m_DICOMImagesPerIPP;
			std::vector<typename SeriesReaderType::Pointer> readers(m_DICOMImagesPerIPP);
      std::vector<typename UpDimFilter::Pointer> updims(m_DICOMImagesPerIPP);
      for(int i = 0; i < this->m_DICOMImagesPerIPP; i++)
        {
        // Files for the current volume
        std::vector<std::string> myFiles;
        for(int s = 0; s < n_slices; s++)
          myFiles.push_back(m_DICOMFiles[s * m_DICOMImagesPerIPP + i]);

        // Read the current volume
				readers[i] = SeriesReaderType::New();
        readers[i]->SetFileNames(myFiles);
        readers[i]->SetImageIO(m_IOBase);

        updims[i] = UpDimFilter::New();
        updims[i]->SetInput(readers[i]->GetOutput());

        // Input to the composer
        composer->SetInput(i, updims[i]->GetOutput());
        }

      // Do the big update
      composer->Update();

      // Set up the streamer
      streamer->SetNumberOfStreamDivisions(std::min(n_slices, 16));
      streamer->SetInput(composer->GetOutput());
      streamer->Update();

      // The result goes into the native image
      m_NativeImage = streamer->GetOutput();

      // Set the number of components
      m_NativeComponents = m_DICOMImagesPerIPP;

			dcmSeriesProgSrc->AddProgress(1.0);
      }
    } 
	else if (m_FileFormat == FORMAT_DICOM_DIR_4DCTA)
		{
		typename SeriesReaderType::Pointer reader = SeriesReaderType::New();
		reader->SetImageIO(m_IOBase);

		SmartPtr<TrivalProgressSource> progSrc = TrivalProgressSource::New();
    progSrc->AddObserverToProgressEvents(progressCmd);
    progSrc->StartProgress(1.0);

		const float weightReading = 0.7, weightLoading = 0.25, weightMisc = 0.05;
		float readingDelta = weightReading/m_DicomFilesToFrameMap.size();

		std::map<unsigned int, typename GreyImageType::Pointer> frameContainer;

		// read image frame by frame
		for (auto &kv : m_DicomFilesToFrameMap)
			{
			MFDS::FilenamesList fnlist;
			for (auto &df : kv.second)
				fnlist.push_back(df.m_Filename);

			reader->SetFileNames(fnlist);
			reader->Update();

			typename GreyImageType::Pointer crntImg = GreyImageType::New();
			DeepCopyImage<GreyImageType>(crntImg, reader->GetOutput());
			frameContainer[kv.first] = crntImg;

			progSrc->AddProgress(readingDelta);
			}

		// assemble 3d images into the 4d native image
		// -- set first 3 dimensions
		typename GreyImage4DType::PointType origin4d;
		typename GreyImage4DType::DirectionType direction4d;
		typename GreyImage4DType::SpacingType spacing4d;
		typename GreyImage4DType::RegionType region4d;

		auto first3dImg = frameContainer[1];

		for (int i = 0; i < 3; ++i)
			{
			origin4d[i] = first3dImg->GetOrigin()[i];
			for (int j = 0; j < 3; ++j)
				direction4d(i,j) = first3dImg->GetDirection()(i,j);
			spacing4d[i] = first3dImg->GetSpacing()[i];
			region4d.SetIndex(i, first3dImg->GetLargestPossibleRegion().GetIndex()[i]);
			region4d.SetSize(i, first3dImg->GetLargestPossibleRegion().GetSize()[i]);
			}

		origin4d[3] = 0;

		// Flip all image to RAS
		if (first3dImg->GetDirection()(2,2) == 1)
			direction4d(2,2) = -1;

		direction4d(0,3) = 0;
		direction4d(1,3) = 0;
		direction4d(2,3) = 0;
		direction4d(3,3) = 1;

		spacing4d[3] = 0.05; // hardcode 50ms for now, should be extracted from the images

		// region Corner Index: [x, x, x, 0], Size: [x, x, x, nt]
		region4d.SetIndex(3, 0);
		region4d.SetSize(3, frameContainer.size()); // number of time points

		typename GreyImage4DType::Pointer image4D = GreyImage4DType::New();
		image4D->SetOrigin(origin4d);
		image4D->SetDirection(direction4d);
		image4D->SetSpacing(spacing4d);
		image4D->SetRegions(region4d);
		image4D->SetNumberOfComponentsPerPixel(first3dImg->GetNumberOfComponentsPerPixel());
		image4D->Allocate();

		itk::ImageRegionIterator<GreyImage4DType> it4d(image4D, image4D->GetLargestPossibleRegion());

		float loadingDelta = weightLoading/frameContainer.size();

		for (size_t i = 0; i < frameContainer.size(); ++i)
			{
			auto crntTP = i + 1;
			auto crntImg = frameContainer[crntTP];

			itk::ImageRegionConstIterator<GreyImageType> it3d(crntImg, crntImg->GetLargestPossibleRegion());
			while (!it3d.IsAtEnd())
				{
				it4d.Set(it3d.Get());
				++it3d;
				++it4d;
				}
			progSrc->AddProgress(loadingDelta);
			}

		// Convert the image into VectorImage format. Do this in-place to avoid
		// allocating memory pointlessly
		typename NativeImageType::Pointer vector = NativeImageType::New();
		m_NativeImage = vector;

		ConvertToVectorImage<TScalar>(vector, image4D);

		// Copy the metadata from the first scan in the series
		const typename SeriesReaderType::DictionaryArrayType *darr =
			reader->GetMetaDataDictionaryArray();
		if(darr->size() > 0)
			m_NativeImage->SetMetaDataDictionary(*((*darr)[0]));

		progSrc->AddProgress(weightMisc);
		progSrc->EndProgress();
		}
	else if (m_FileFormat == FORMAT_ECHO_CARTESIAN_DICOM)
    {
		SmartPtr<TrivalProgressSource> ecdProgSrc = TrivalProgressSource::New();
    ecdProgSrc->AddObserverToProgressEvents(progressCmd);
    ecdProgSrc->StartProgress();

		// issue #26: 4D Echocardiography Cartesian DICOM (ECD) Image Reading

		gdcm::Reader ecd_data_reader;
		ecd_data_reader.SetFileName(FileName);

		const gdcm::Tag data(0x7fe0, 0x0010);
		typename NativeImageType::SizeType ecd_dim;
		typename NativeImageType::SpacingType ecd_spc;
		typename NativeImageType::DirectionType ecd_dir;
		typename NativeImageType::PointType ecd_org;

		std::set<gdcm::Tag> tSet;
		tSet.insert(data);

		// Only read the data tag
		if (!ecd_data_reader.ReadSelectedTags(tSet))
			std::cerr << "Can not read:" << FileName << std::endl;
		else
			{
			ecdProgSrc->AddProgress(0.3);
			gdcm::DataSet &ds = ecd_data_reader.GetFile().GetDataSet();

			if (!ds.FindDataElement(data))
				std::cerr << "data element not found!" << std::endl;

			const gdcm::DataElement &de = ds.GetDataElement(data);
			if (de.IsEmpty())
				std::cerr << "data element is empty!" << std::endl;

			for (unsigned int i = 0; i < 4; ++i)
				{
				ecd_dim[i] = m_IOBase->GetDimensions(i);
				ecd_spc[i] = m_IOBase->GetSpacing(i);
				for(size_t j = 0; j < 4; j++)
					ecd_dir(j,i) = m_IOBase->GetDirection(i)[j];
				}
			ecd_org.Fill(0);

			unsigned long len = ecd_dim[0] * ecd_dim[1] * ecd_dim[2] * ecd_dim[3];

			const gdcm::ByteValue *bv = de.GetByteValue();

			// Start loading image
			typename NativeImageType::Pointer ecd_image = NativeImageType::New();
			ecd_image->SetOrigin(ecd_org);
			ecd_image->SetSpacing(ecd_spc);
			ecd_image->SetDirection(ecd_dir);

			// Configure target image container
			typename NativeImageType::RegionType ecd_region;
			typename NativeImageType::IndexType ecd_index = {{0, 0, 0, 0}};
			ecd_region.SetIndex(ecd_index);
			ecd_region.SetSize(ecd_dim);
			ecd_image->SetRegions(ecd_region);
			ecd_image->SetVectorLength(1);
			ecd_image->SetNumberOfComponentsPerPixel(1);
			ecd_image->Allocate();

			ecdProgSrc->AddProgress(0.1);

			// -- Extract the buffer from file
			char *ecd_buffer = (char*)malloc(len);
			bv->GetBuffer(ecd_buffer, len);

			// -- Pass buffer into ecd_image
			typedef typename NativeImageType::PixelContainer PixConType;
			typename PixConType::Pointer pPixCon = PixConType::New();
			pPixCon->SetImportPointer(reinterpret_cast<TScalar*>(ecd_buffer), len, true);
			ecd_image->SetPixelContainer(pPixCon);

			ecdProgSrc->AddProgress(0.3);

			// Read and Import Dictionary
			// -- Choose and import basic information into the metadata dictionary

			// -- Following code segment loading metadata dictionary is from itkGDCMImageIO.cxx
			// -- Modified to adapt to 4D Echocardiography Cartesian DICOM (ECD) Image

			gdcm::Reader ecd_meta_reader;
			ecd_meta_reader.SetFileName(FileName);
			typedef itk::ImageIOBase::SizeValueType SizeValueType;
			itk::MetaDataDictionary &dico = m_IOBase->GetMetaDataDictionary();
			gdcm::StringFilter strF;
			strF.SetFile(ecd_meta_reader.GetFile());

			if (!ecd_meta_reader.ReadUpToTag(data))
				std::cerr << "Can not read:" << FileName << std::endl;
			else
				{
				const gdcm::File &file = ecd_meta_reader.GetFile();
				const gdcm::DataSet &ds = file.GetDataSet();

				// Iterate through tags for metadata
				for (auto it = ds.Begin(); it != ds.End(); ++it)
					{

					const gdcm::DataElement &de = *it;
					const gdcm::Tag &tag = de.GetTag();

					// Customized reading of following attributes for the non-standard 4D ECD image
					// -- Depth (z-axis dimension)
					if (tag == gdcm::Tag(0x3001, 0x1001))
						{
						itk::EncapsulateMetaData<std::string>(dico, "Depth", strF.ToString(tag));
						continue;
						}

					// -- Delta Z (physical delta in z direction)
					if (tag == gdcm::Tag(0x3001, 0x1003))
						{
						itk::EncapsulateMetaData<std::string>(dico, "Physical Delta Z", strF.ToString(tag));
						continue;
						}

					// Otherwise read public tags as normal
					gdcm::VR vr = gdcm::DataSetHelper::ComputeVR(file, ds, tag);

					if (vr & (gdcm::VR::OB | gdcm::VR::OF | gdcm::VR::OW | gdcm::VR::SQ | gdcm::VR::UN))
						{
						// itkAssertInDebugAndIgnoreInReleaseMacro( vr & gdcm::VR::VRBINARY );
						/*
						 * Old behavior was to skip SQ, Pixel Data element. I decided that it is not safe to mime64
						 * VR::UN element. There used to be a bug in gdcm 1.2.0 and VR:UN element.
						 */
						if ((tag.IsPublic()) && vr != gdcm::VR::SQ &&
								tag != gdcm::Tag(0x7fe0, 0x0010) /* && vr != gdcm::VR::UN*/)
							{
							const gdcm::ByteValue * bv = de.GetByteValue();
							if (bv)
								{
								// base64 streams have to be a multiple of 4 bytes in length
								int encodedLengthEstimate = 2 * bv->GetLength();
								encodedLengthEstimate = ((encodedLengthEstimate / 4) + 1) * 4;

								auto * bin = new char[encodedLengthEstimate];
								auto   encodedLengthActual =
									static_cast<unsigned int>(itksysBase64_Encode((const unsigned char *)bv->GetPointer(),
																																static_cast<SizeValueType>(bv->GetLength()),
																																(unsigned char *)bin,
																																static_cast<int>(0)));
								std::string encodedValue(bin, encodedLengthActual);
								itk::EncapsulateMetaData<std::string>(dico, tag.PrintAsPipeSeparatedString(), encodedValue);
								delete[] bin;
								}
							}
						}
					else /* if ( vr & gdcm::VR::VRASCII ) */
						{
						// Only copying field from the public DICOM dictionary
						if (tag.IsPublic())
							itk::EncapsulateMetaData<std::string>(dico, tag.PrintAsPipeSeparatedString(), strF.ToString(tag));
						}
					}
				}

			ecd_image->SetMetaDataDictionary(dico);

			m_NativeImage = ecd_image;

			ecdProgSrc->AddProgress(0.2);
			}
    }
  else
    {
    // Non-DICOM DIR: read from single image
    // We no longer use ImageFileReader here because of an issue: the
    // m_IOBase may have an open file handle (from call to ReadImageInfo)
    // so passing it in to the Reader would cause an IO error (this actually
    // happens for GIPL). So we copy some of the code from ImageFileReader

		// Unfortunately the itk reader does not invoke the progress event during
		// the reading. Using a trivial source to have something reported to the UI.
    SmartPtr<TrivalProgressSource> regularImageReadingProgSrc = TrivalProgressSource::New();
    regularImageReadingProgSrc->AddObserverToProgressEvents(progressCmd);
    regularImageReadingProgSrc->StartProgress();

    // Create the native image
    typename NativeImageType::Pointer image = NativeImageType::New();

    UpdateImageHeader<NativeImageType>(image);
    image->Allocate();

    regularImageReadingProgSrc->AddProgress(0.1);

    // Read the image into the buffer
		m_IOBase->Read(image->GetBufferPointer());

    // For seq.nrrd, convert the component dimension to the sequence dimension
    if (m_FileFormat == FORMAT_NRRD_SEQ && m_NCompBeforeFolding > 1 &&
        !m_Load4DAsMultiComponent && !m_LoadMultiComponentAs4D)
      {
      image = this->ConvertMultiComponentLoadTo4D<NativeImageType>(image);
      }

    if (m_Load4DAsMultiComponent && m_NCompBeforeFolding == 1)
      image = this->Convert4DLoadToMultiComponent<NativeImageType>(image);
    else if (m_LoadMultiComponentAs4D && m_NDimBeforeFolding < 4)
      image = this->ConvertMultiComponentLoadTo4D<NativeImageType>(image);

    m_NativeImage = image;

		regularImageReadingProgSrc->AddProgress(0.9);


    // If the image is 4-dimensional or more, we must perform an in-place transpose
    // of the image. The fourth dimension is the one that varies fastest, and in our
    // representation, the image is represented as a VectorImage, where the components
    // of each voxel are the thing that moves fastest. The problem can be represented as
    // a transpose of a N x M array, where N = dimX*dimY*dimZ*dimT and M = dimW

    size_t nd = (m_NDimBeforeFolding > 4) ? 4 : m_NDimBeforeFolding;
    typename NativeImageType::SizeType dim;  dim.Fill(1);

    for(unsigned int i = 0; i < nd; i++)
      dim[i] = m_IOBase->GetDimensions(i);

    if(m_NDimBeforeFolding > 4)
      {
      long N = dim[0] * dim[1] * dim[2] * dim[3];
      long M = m_NCompAfterFolding;
      long move_size = (2 * M) * sizeof(TScalar);
      char *move = new char[move_size];
      TScalar buffer[2];

      // TODO: this is a pretty slow routine. It would be nice to find somehting a little
      // faster than this. But at least we can read 4D data now
      itk::TimeProbe probe;
      probe.Start();
      transpose_toms513(image->GetBufferPointer(), M, N, move, move_size, buffer);
      probe.Stop();

      //std::cout << "Transpose of " << N << " by " << M << " matrix computed in " << probe.GetTotal() << " sec." << std::endl;
      delete[] move;
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
  for (int i = 0; i < 4; ++i)
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

void
GuidedNativeImageIO
::SaveNativeImage(const char *FileName, Registry &folder)
{
  // Cast image from native format to TPixel
  DispatchBase *dispatch = this->CreateDispatch(this->GetComponentTypeInNativeImage());
  dispatch->SaveNative(this, FileName, folder);
  delete dispatch;
}

template<typename TNative>
void
GuidedNativeImageIO
::DoSaveNative(const char *FileName, Registry &folder)
{
  // Get the native image pointer
  ImageBase *native = this->GetNativeImage();

  // Get the native image
  typedef itk::VectorImage<TNative, 4> InputImageType;
  typename InputImageType::Pointer input = 
    reinterpret_cast<InputImageType *>(native);
  assert(input);

  // Use the Save method
  this->SaveImage<InputImageType>(FileName, folder, input);
}

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


std::string
GuidedNativeImageIO
::GetNativeImageMD5Hash()
{
  std::string md5;

  // Cast image from native format to TPixel
  DispatchBase *dispatch = this->CreateDispatch(this->GetComponentTypeInNativeImage());
  md5 = dispatch->GetNativeMD5Hash(this);
  delete dispatch;

  return md5;
}

template<typename TNative>
std::string
GuidedNativeImageIO
::DoGetNativeMD5Hash()
{
  // Get the native image pointer
  ImageBase *native = this->GetNativeImage();

  // Get the native image
  typedef itk::VectorImage<TNative, 4> InputImageType;
  typename InputImageType::Pointer input = 
    reinterpret_cast<InputImageType *>(native);
  assert(input);

  char hex_code[33];
  hex_code[32] = 0;
  itksysMD5 *md5 = itksysMD5_New();
  itksysMD5_Initialize(md5);
  itksysMD5_Append(md5, 
    (unsigned char *) input->GetBufferPointer(), 
    input->GetPixelContainer()->Size() * sizeof(TNative));
  itksysMD5_FinalizeHex(md5, hex_code);
  itksysMD5_Delete(md5);

  return std::string(hex_code);
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
  auto *native = nativeIO->GetNativeImage();

  // Cast image from native format to TPixel
  itk::IOComponentEnum itype = nativeIO->GetComponentTypeInNativeImage();
  switch(itype) 
    {
    case itk::IOComponentEnum::UCHAR:  DoCast<unsigned char>(native);   break;
    case itk::IOComponentEnum::CHAR:   DoCast<signed char>(native);     break;
    case itk::IOComponentEnum::USHORT: DoCast<unsigned short>(native);  break;
    case itk::IOComponentEnum::SHORT:  DoCast<signed short>(native);    break;
    case itk::IOComponentEnum::UINT:   DoCast<unsigned int>(native);    break;
    case itk::IOComponentEnum::INT:    DoCast<signed int>(native);      break;
    case itk::IOComponentEnum::ULONG:  DoCast<unsigned long>(native);   break;
    case itk::IOComponentEnum::LONG:   DoCast<signed long>(native);     break;
    case itk::IOComponentEnum::FLOAT:  DoCast<float>(native);           break;
    case itk::IOComponentEnum::DOUBLE: DoCast<double>(native);          break;
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

  template <typename U = TPixel, typename std::enable_if<std::is_integral<U>::value, int>::type = 0>
  void operator()(TNative *src, TPixel *trg)
  {
    *trg = static_cast<TPixel>(std::round((*src + m_Shift) * m_Scale));
  }

  template <typename U = TPixel, typename std::enable_if<std::is_floating_point<U>::value, int>::type = 0>
  void operator()(TNative *src, TPixel *trg)
  {
    *trg = static_cast<TPixel>((*src + m_Shift) * m_Scale);
  }

protected:

  double m_Shift, m_Scale;

};




template<class TOutputImage>
template<typename TNative>
void
RescaleNativeImageToIntegralType<TOutputImage>
::DoCast(NativeImageType *native)
{
  // Get the native image
  typedef itk::VectorImage<TNative, TOutputImage::ImageDimension> InputImageType;
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
  // and if the output type is not floating point
  if(typeid(OutputComponentType) != typeid(TNative) && itk::NumericTraits<OutputComponentType>::is_integer)
    {
    // We must compute the range of the input data    
    OutputComponentType omax = itk::NumericTraits<OutputComponentType>::max();
    // -- we need to use lowest() instead of min(), because for double/float, min()
    // -- only returns the smallest positive value a type can represent
    // -- here we want to know the range including the negative values
    OutputComponentType omin = itk::NumericTraits<OutputComponentType>::lowest();

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
  itk::ImageBase<4> *native = nativeIO->GetNativeImage();

  // Cast image from native format to TPixel
  itk::IOComponentEnum itype = nativeIO->GetComponentTypeInNativeImage();
  switch(itype) 
    {
    case itk::IOComponentEnum::UCHAR:  DoCast<unsigned char>(native);   break;
    case itk::IOComponentEnum::CHAR:   DoCast<signed char>(native);     break;
    case itk::IOComponentEnum::USHORT: DoCast<unsigned short>(native);  break;
    case itk::IOComponentEnum::SHORT:  DoCast<signed short>(native);    break;
    case itk::IOComponentEnum::UINT:   DoCast<unsigned int>(native);    break;
    case itk::IOComponentEnum::INT:    DoCast<signed int>(native);      break;
    case itk::IOComponentEnum::ULONG:  DoCast<unsigned long>(native);   break;
    case itk::IOComponentEnum::LONG:   DoCast<signed long>(native);     break;
    case itk::IOComponentEnum::FLOAT:  DoCast<float>(native);           break;
    case itk::IOComponentEnum::DOUBLE: DoCast<double>(native);          break;
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
::DoCast(itk::ImageBase<4> *native)
{
  // Get the native image
  typedef itk::VectorImage<TNative, 4> InputImageType;
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
      // issue #26: Check for Echo Cartesian Dicom
      gdcm::Reader reader;
      reader.SetFileName(fname.c_str());

      std::set<gdcm::Tag> tags;
      gdcm::Tag tag_manuf(0x0008, 0x0070); // manufacturer
      tags.insert(tag_manuf);

			gdcm::Tag tag_modality(0x0008,0x0060); // modality
			tags.insert(tag_modality);

      reader.ReadSelectedTags(tags, true);

      gdcm::StringFilter sf;
      sf.SetFile(reader.GetFile());

			// Echo cartesian dicom test
      std::string manuf = sf.ToString(tag_manuf);
      std::transform(manuf.begin(), manuf.end(), manuf.begin(), ::toupper);
			itk::StringTools::Trim(manuf);

      if (!manuf.compare("PMS QLAB CART EXPORT"))
        return FORMAT_ECHO_CARTESIAN_DICOM;

			// 4DCTA test
			std::string modality = sf.ToString(tag_modality);
			std::transform(modality.begin(), modality.end(), modality.begin(), ::toupper);
			itk::StringTools::Trim(modality);
			if (!modality.compare("CT"))
				{
        bool hasSiemens = manuf.find("SIEMENS") != std::string::npos;
        if (hasSiemens ||
						!manuf.compare("GE MEDICAL SYSTEMS"))
					return FORMAT_DICOM_DIR_4DCTA;
				}

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
const gdcm::Tag GuidedNativeImageIO::m_tagSeriesInstanceUID(0x0020,0x000E);
const gdcm::Tag GuidedNativeImageIO::m_tagSeriesNumber(0x0020,0x0011);
const gdcm::Tag GuidedNativeImageIO::m_tagAcquisitionNumber(0x0020,0x0012);
const gdcm::Tag GuidedNativeImageIO::m_tagInstanceNumber(0x0020,0x0013);
const gdcm::Tag GuidedNativeImageIO::m_tagSequenceName(0x0018, 0x0024);
const gdcm::Tag GuidedNativeImageIO::m_tagSliceThickness(0x0018, 0x0050);


#include "gdcmDirectory.h"
#include "gdcmImageReader.h"

void
GuidedNativeImageIO
::ParseDicomDirectory(const std::string &dir, itk::Command *progressCommand)
{
  // We will parse the DICOM directory manually to avoid extra time opening
  // files and also to allow progress reporting

  // Must have a directory
  if(!itksys::SystemTools::FileIsDirectory(dir.c_str()))
    throw IRISException(
        "Error: Not a directory. "
        "Trying to look for DICOM series in '%s', which is not a directory",
        dir.c_str());

  // List of tags used for refined grouping of files - order matters!
  std::vector<gdcm::Tag> tags_refine;
  tags_refine.push_back(m_tagSeriesNumber);
  tags_refine.push_back(m_tagSequenceName);
  tags_refine.push_back(m_tagSliceThickness);
  tags_refine.push_back(m_tagRows);
  tags_refine.push_back(m_tagCols);

  // List of tags that we want to parse - everything else may be ignored
  std::set<gdcm::Tag> tags_all;
  tags_all.insert(tags_refine.begin(), tags_refine.end());
  tags_all.insert(m_tagDesc);
  tags_all.insert(m_tagSeriesInstanceUID);

  //--Dev: Add to read list
  std::set<std::string> sliceLocSet;
  std::map<std::string, std::set<std::string>> sliceMap;

  // Clear the information about the last parse
  m_LastDicomParseResult.Reset();
  m_LastDicomParseResult.Directory = dir;

  // GDCM directory listing
  gdcm::Directory dirList;

  // Load the directory - this should be quick
  dirList.Load(dir, false);
  gdcm::Directory::FilenamesType const &filenames = dirList.GetFilenames();
  for(gdcm::Directory::FilenamesType::const_iterator it = filenames.begin();
    it != filenames.end(); ++it)
    {
    // Process each filename in the directory
    gdcm::Reader reader;
    reader.SetFileName(it->c_str());

    // Try reading this file. Fail quietly.
    bool read = false;
    try { read = reader.ReadSelectedTags(tags_all, true); }
    catch(...) {}

    // If nothing read, keep going
    if(!read)
      continue;

    // Create a string filter to get tags
    gdcm::StringFilter sf;
    sf.SetFile(reader.GetFile());

    // Start with the ID being the UID
    std::string uid = sf.ToString(m_tagSeriesInstanceUID);
    std::string full_id = uid;

    // Iterate over the tags in the refine list
		for(size_t iTag = 0u; iTag < tags_refine.size(); iTag++)
      {
      // Read the tag value
      std::string s = sf.ToString(tags_refine[iTag]);

      // This code is from gdcmSerieHelper
      if( full_id == uid && !s.empty() )
        {
        full_id += "."; // add separator
        }
      full_id += s;
      }

    // Eliminate non-alnum characters, including whitespace...
    //   that may have been introduced by concats.
    for(size_t i=0; i<full_id.size(); i++)
      {
      while(i<full_id.size()
        && !( full_id[i] == '.'
          || (full_id[i] >= 'a' && full_id[i] <= 'z')
          || (full_id[i] >= '0' && full_id[i] <= '9')
          || (full_id[i] >= 'A' && full_id[i] <= 'Z')))
        {
        full_id.erase(i, 1);
        }
      }

    // The info for the current series
    DicomDirectoryParseResult::DicomSeriesInfo &series_info
        = m_LastDicomParseResult.SeriesMap[full_id];

    // The registry for the current series
    Registry &r = series_info.MetaData;

    // Have we found this ID before?
    if(r.IsEmpty())
      {
      r["SeriesId"] << full_id;

      // Read series description
      r["SeriesDescription"] << sf.ToString(m_tagDesc);
      r["SeriesNumber"] << sf.ToString(m_tagSeriesNumber);

      // Read the dimensions
      r["Rows"] << std::atoi(sf.ToString(m_tagRows).c_str());
      r["Columns"] << std::atoi(sf.ToString(m_tagCols).c_str());
      r["NumberOfImages"] << 1;
      }
    else
      {
      // Increement the number of images
      r["NumberOfImages"] << r["NumberOfImages"][0] + 1;
      }

    // Update the dimensions string
    ostringstream oss;
    oss << r["Rows"][0] << " x " << r["Columns"][0] << " x " << r["NumberOfImages"][0];
    r["Dimensions"] << oss.str();

    // Update the filelist
    series_info.FileList.push_back(*it);

    // Indicate some progress
    if(progressCommand)
      progressCommand->Execute(this, itk::ProgressEvent());
    }

  // Complain if no series have been found
  if(m_LastDicomParseResult.SeriesMap.size() == 0)
    throw IRISException(
        "Error: DICOM series not found. "
        "Directory '%s' does not appear to contain a DICOM series.", dir.c_str());
}

void GuidedNativeImageIO::DicomDirectoryParseResult::Reset()
{
  Directory.clear();
  SeriesMap.clear();
}

GuidedNativeImageIO::IOBasePointer
GuidedNativeImageIO
::PeekHeader(std::string filename)
{
  if (m_IOBase)
    return m_IOBase;

  Registry dummyRegistry;
  ReadNativeImageHeader(filename.c_str(), dummyRegistry);
  return m_IOBase;
}

/*
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

      progressCommand->Execute(this, itk::ProgressEvent());
      }
    }
  
  // Complain if no series have been found
  if(reg.size() == 0)
    throw IRISException(
        "Error: DICOM series not found. "
        "Directory '%s' does not appear to contain a DICOM series.", dir.c_str());
}

*/

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
  itk::IOComponentEnum itype = nativeIO->GetComponentTypeInNativeImage();
  switch(itype) 
    {
    case itk::IOComponentEnum::UCHAR:  DoCast<unsigned char>(native);   break;
    case itk::IOComponentEnum::CHAR:   DoCast<signed char>(native);     break;
    case itk::IOComponentEnum::USHORT: DoCast<unsigned short>(native);  break;
    case itk::IOComponentEnum::SHORT:  DoCast<signed short>(native);    break;
    case itk::IOComponentEnum::UINT:   DoCast<unsigned int>(native);    break;
    case itk::IOComponentEnum::INT:    DoCast<signed int>(native);      break;
    case itk::IOComponentEnum::ULONG:  DoCast<unsigned long>(native);   break;
    case itk::IOComponentEnum::LONG:   DoCast<signed long>(native);     break;
    case itk::IOComponentEnum::FLOAT:  DoCast<float>(native);           break;
    case itk::IOComponentEnum::DOUBLE: DoCast<double>(native);          break;
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

#define GuidedNativeImageIOInstantiateMacro(type) \
  template class RescaleNativeImageToIntegralType< itk::Image<type, 4> >; \
  template class RescaleNativeImageToIntegralType< itk::VectorImage<type, 4> >; \
  template class CastNativeImage<itk::Image<type, 4> >; \
  template void GuidedNativeImageIO::SaveImage(const char *, Registry &, itk::Image<type, 3> *);

GuidedNativeImageIOInstantiateMacro(unsigned char)
GuidedNativeImageIOInstantiateMacro(char)
GuidedNativeImageIOInstantiateMacro(unsigned short)
GuidedNativeImageIOInstantiateMacro(short)
GuidedNativeImageIOInstantiateMacro(float)
GuidedNativeImageIOInstantiateMacro(double)
