/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GuidedNativeImageIO.h,v $
  Language:  C++
  Date:      $Date: 2010/10/14 16:21:04 $
  Version:   $Revision: 1.6 $
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
#ifndef __GuidedNativeImageIO_h_
#define __GuidedNativeImageIO_h_

#include "Registry.h"
#include "itkSmartPointer.h"
#include "itkImage.h"
#include "itkImageIOBase.h"
#include "itkVectorImage.h"
#include "itkGDCMSeriesFileNames.h"
#include "gdcmTag.h"

  
namespace itk
{
  template<class TPixel, unsigned int VDim> class Image;
  class ImageIOBase;
}


/**
 * \class GuidedNativeImageIO
 * \brief This class performs image IO based on user-supplied parameters such
 * as explicit IO type, and for some types such as raw, the necessary additional
 * information.
 */
class GuidedNativeImageIO : public itk::Object
{
public:

  irisITKObjectMacro(GuidedNativeImageIO, itk::Object)

  enum FileFormat {
    FORMAT_ANALYZE=0,
    FORMAT_DICOM_DIR,       // A directory containing multiple DICOM files
    FORMAT_DICOM_FILE,      // A single DICOM file
    FORMAT_GE4, FORMAT_GE5, FORMAT_GIPL,
    FORMAT_MHA, FORMAT_NIFTI, FORMAT_NRRD, FORMAT_RAW, FORMAT_SIEMENS,
    FORMAT_VOXBO_CUB, FORMAT_VTK, FORMAT_COUNT};

  enum RawPixelType {
    PIXELTYPE_UCHAR=0, PIXELTYPE_CHAR, PIXELTYPE_USHORT, PIXELTYPE_SHORT, 
    PIXELTYPE_UINT, PIXELTYPE_INT, PIXELTYPE_FLOAT, PIXELTYPE_DOUBLE,
    PIXELTYPE_COUNT};

  /**
   * A descriptor of file formats supported in ITK. Describes whether
   * the format can support different needs in SNAP 
   */
  struct FileFormatDescriptor 
  {
    std::string name;
    std::string pattern;
    bool can_write;
    bool can_store_orientation;
    bool can_store_float;
    bool can_store_short;

    bool TestFilename(std::string fname);
  };


  // Image type. This is only for 3D images.
  typedef itk::ImageIOBase IOBase;
  typedef itk::SmartPointer<IOBase> IOBasePointer;

  typedef itk::ImageBase<3> ImageBase;
  typedef itk::SmartPointer<ImageBase> ImageBasePointer;

  /**
   * This method loads an image from the given filename. A registry can be 
   * supplied to assist the loading. The registry specifies the desired IO
   * type, and for special types, such as Raw it can provide the parameters
   * such as header size and image dimensions. The image is read in native
   * format and stored inside of this object. In order to cast the image to 
   * the format of interest, the user must cast the image to one of the 
   * desired formats.
   */
  void ReadNativeImage(const char *FileName, Registry &folder);

  void ReadNativeImageHeader(const char *FileName, Registry &folder);

  void ReadNativeImageData();

  /**
   * Get the number of components in the native image read by ReadNativeImage.
   */
  size_t GetNumberOfComponentsInNativeImage() const;

  /**
    Access the IO header stored in the IO object. This is only temporarily
    available between calls to ReadNativeImageHeader() and ReadNativeImageData().
    The purpose is to allow users to check the validity of the header before
    actually loading the data completely.
    */
  itk::ImageIOBase *GetIOBase()
    { return m_IOBase; }

  /**
   * Get the component type in the native image
   */
  itk::ImageIOBase::IOComponentType GetComponentTypeInNativeImage() const
    { return m_NativeType; }

  itk::ImageIOBase::ByteOrder GetByteOrderInNativeImage() const
    { return m_NativeByteOrder; }

  std::string GetComponentTypeAsStringInNativeImage() const
    { return m_NativeTypeString; }

  std::string GetFileNameOfNativeImage() const
    { return m_NativeFileName; }

  unsigned long GetFileSizeOfNativeImage() const
    { return m_NativeSizeInBytes; }

  std::string GetNicknameOfNativeImage() const
    { return m_NativeNickname; }

  Vector3ui GetDimensionsOfNativeImage() const
    { return m_NativeDimensions; }

  /**
   * This method returns the image internally stored in this object. This is
   * a pointer to an itk::VectorImage of some native format. Use one of the
   * Cast objects to cast it to an image of the type you want
   */
  itk::ImageBase<3> *GetNativeImage() const
    { return m_NativeImage; }

  /**
   * Has a native image been loaded?
   */
  bool IsNativeImageLoaded() const
    { return m_NativeImage.IsNotNull(); }

  /**
   * Discard the native image. Use this once you've cast the native image to 
   * the format of interest.
   */
  void DeallocateNativeImage()
    { m_IOBase = NULL; m_NativeImage = NULL; }

  /** 
   * Get RAI code for an image. If there is nothing in the registry, this will
   * try getting the code from the image header. If there is no way to get the
   * image code, this will return an empty string
   */
  // std::string GetRAICode(ImageType *image, Registry &folder);

  /** 
   * Save an image using the Registry folder to specify parameters. This method
   * is templated over the image type.
   */
  template<class TImageType>
    void SaveImage(const char *FileName, Registry &folder, TImageType *image);

  /** Parse registry to get file format */
  static FileFormat GetFileFormat(Registry &folder, FileFormat dflt = FORMAT_COUNT);

  /** Set the file format in a registry */
  static void SetFileFormat(Registry &folder, FileFormat format);

  /** Get format descriptor for a format */
  static FileFormatDescriptor GetFileFormatDescriptor(FileFormat fmt)
    { return m_FileFormatDescrictorArray[fmt]; }

  /**
    Determine file format for a filename. This method can optionally try to
    open the file and scan the magic number, for the formats that are known
    */
  static FileFormat GuessFormatForFileName(
      const std::string &string, bool checkMagic);

  /** Parse registry to get pixel type of raw file */
  static RawPixelType GetPixelType(Registry &folder, RawPixelType dflt = PIXELTYPE_COUNT);

  /** Set the file format in a registry */
  static void SetPixelType(Registry &folder, RawPixelType type);

  /** A field used to request DICOM fields */
  struct DicomRequestField
  {
    short group, elem;
    std::string code;
    DicomRequestField(short g, short e, std::string c)
      : group(g), elem(e), code(c) {}
    DicomRequestField()
      : group(0), elem(0) {}
  };

  /** A parameter for ParseDicomDirectory */
  typedef std::vector<DicomRequestField> DicomRequest;

  /** Output for ParseDicomDirectory */
  typedef std::vector<Registry> RegistryArray;

  /**
   * Get series information from a DICOM directory. This will list all the
   * files in the DICOM directory and generate a registry for each series in
   * the directory. By default, the following registry entries are generated
   * for each series:
   *   - SeriesDescription
   *   - Dimensions
   *   - NumberOfImages
   *   - SeriesId
   *   - SeriesFiles (an array with filenames)
   * You can also ask for additional DICOM entries to be extracted by giving
   * a list of DICOM keys in the third optional parameter.
   */
  void ParseDicomDirectory(
      const std::string &dir,
      RegistryArray &reg,
      const DicomRequest &req = DicomRequest());


  /**
   * Create an ImageIO object using a registry folder. Second parameter is
   * true for reading the file, false for writing the file
   */
  void CreateImageIO(const char *fname, Registry &folder, bool read);

  // Get the output of the last operation
  // irisGetMacro(IOBase, itk::ImageIOBase *);    

protected:

  GuidedNativeImageIO();
  virtual ~GuidedNativeImageIO()
    { DeallocateNativeImage(); }

  /** Templated function to create RAW image IO */
  template <typename TRaw> void CreateRawImageIO(Registry &folder);

  /** Templated function that reads a scalar image in its native datatype */
  template <typename TScalar> void DoReadNative(const char *fname, Registry &folder);

  /** 
   This is a vector image in native format. It stores the data read from the
   image file. The user must cast it to a desired type to use it. Once it has
   been cast, the native image becomes unusable because casting is done inplace
   so the buffer initially allocated for the native image may be resized and
   overridden.
   */
  ImageBasePointer m_NativeImage;

  // The IO base used to read the files
  IOBasePointer m_IOBase;

  // GDCM series reader/parser (for DICOM)
  SmartPtr<itk::GDCMSeriesFileNames> m_GDCMSeries;
  std::string m_GDCMSeriesDirectory;

  // This information is copied from IOBase in order to delete IOBase at the 
  // earliest possible point, so as to conserve memory
  IOBase::IOComponentType m_NativeType;
  size_t m_NativeComponents;
  unsigned long m_NativeSizeInBytes;
  std::string m_NativeTypeString, m_NativeFileName;
  std::string m_NativeNickname;
  IOBase::ByteOrder m_NativeByteOrder;
  Vector3ui m_NativeDimensions;

  // Copy of the registry passed in when reading header
  Registry m_Hints;

  // The file format
  FileFormat m_FileFormat;

  // List of filenames for DICOM
  std::vector<std::string> m_DICOMFiles;

  /** Registry mappings for these enums */
  static bool m_StaticDataInitialized;
  static RegistryEnumMap<FileFormat> m_EnumFileFormat;
  static RegistryEnumMap<RawPixelType> m_EnumRawPixelType;

  // File format descriptors
  static const FileFormatDescriptor m_FileFormatDescrictorArray[];

  static const gdcm::Tag m_tagRows;
  static const gdcm::Tag m_tagCols;
  static const gdcm::Tag m_tagDesc;
  static const gdcm::Tag m_tagTextDesc;
  static const gdcm::Tag m_tagSeriesInstanceUID;
  static const gdcm::Tag m_tagSeriesNumber;
  static const gdcm::Tag m_tagAcquisitionNumber;
  static const gdcm::Tag m_tagInstanceNumber;
  
};


/**
 * \class RescaleNativeImageToIntegralType
 * \brief An adapter class that rescales a native-format image from 
 * GuidedNativeImageIO to user-specified scalar type
 */
template<class TOutputImage>
class RescaleNativeImageToIntegralType
{
public:
  RescaleNativeImageToIntegralType() {}
  virtual ~RescaleNativeImageToIntegralType() {}

  typedef TOutputImage                                         OutputImageType;
  typedef typename TOutputImage::PixelType                     OutputPixelType;
  typedef itk::ImageBase<3>                                    NativeImageType;

  // Constructor, takes pointer to native image
  OutputImageType *operator()(GuidedNativeImageIO *nativeIO);

  // Get the scale factor to map from scalar to native
  irisGetMacro(NativeScale, double)

  // Get the shift to map from scalar to native
  irisGetMacro(NativeShift, double)

private:
  typename OutputImageType::Pointer m_Output;
  double m_NativeScale, m_NativeShift;

  // Method that does the casting
  template<typename TNative> void DoCast(itk::ImageBase<3> *native);
};

template<class TPixel> class TrivialCastFunctor
{
public:
  typedef TPixel PixelType;
  template<class TNative> void operator()(TNative *src, TPixel *trg)
    { *trg = static_cast<TPixel>(*src); }
};


/**
 * \class CastNativeImageBase
 * \brief An adapter class that casts a native-format image from 
 * GuidedNativeImageIO to an output image type. The actual casting
 * is done using the functor TCastFunctor. Use derived classes.
 */
template<class TOutputImage,
         class TCastFunctor =
           TrivialCastFunctor<typename TOutputImage::InternalPixelType> >
class CastNativeImage
{
public:
  typedef TOutputImage                                         OutputImageType;
  typedef RescaleNativeImageToIntegralType<OutputImageType>       RescalerType;
  typedef typename RescalerType::NativeImageType               NativeImageType;
  typedef typename OutputImageType::PixelType                  OutputPixelType;
  typedef typename OutputImageType::InternalPixelType      OutputComponentType;

  // Constructor, takes pointer to native image
  OutputImageType *operator()(GuidedNativeImageIO *nativeIO);

  // Set the functor
  void SetFunctor(TCastFunctor functor) 
    { m_Functor = functor; }

private:
  typename OutputImageType::Pointer m_Output;
  TCastFunctor m_Functor;

  // Method that does the casting
  template<typename TNative> void DoCast(itk::ImageBase<3> *native);

  friend class RescaleNativeImageToIntegralType<OutputImageType>;
};

// Functor used for scalar to scalar casting
template<class TPixel> class CastToScalarFunctor
{
public:
  template<class TNative> void operator()(TNative *src, TPixel *trg)
    { *trg = (TPixel) *src; }
  size_t GetNumberOfDimensions() { return 1; }
};

// Functor used for scalar to array casting
template<class TPixel, unsigned int VDim> class CastToArrayFunctor
{
public:
  template<class TNative> void operator()(TNative *src, TPixel *trg)
    { for(unsigned int i = 0; i < VDim; i++) (*trg)[i] = (typename TPixel::ComponentType) src[i]; }
  size_t GetNumberOfDimensions() { return VDim; }
};

/**
 * \class CastNativeImageToRGB
 * \brief An adapter class that casts a native-format image from 
 * GuidedNativeImageIO to an RGB image
 */
/*
template<typename TRGBPixel>
class CastNativeImageToRGB : 
  public CastNativeImageBase<TRGBPixel, CastToArrayFunctor<TRGBPixel, 3> > {};
*/

/**
 * \class CastNativeImageToScalar
 * \brief An adapter class that casts a native-format image from 
 * GuidedNativeImageIO to a scalar image of given type
 */
/*
template<typename TPixel>
class CastNativeImageToScalar : 
  public CastNativeImageBase<TPixel, CastToScalarFunctor<TPixel> > {};
*/

#endif
