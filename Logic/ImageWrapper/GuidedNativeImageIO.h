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
#include "itkOrientedImage.h"
#include "itkImageIOBase.h"

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
class GuidedNativeImageIO
{
public:

  enum FileFormat {
    FORMAT_MHA=0, FORMAT_GIPL, FORMAT_RAW, FORMAT_ANALYZE,
    FORMAT_DICOM, FORMAT_GE4, FORMAT_GE5, FORMAT_NIFTI, FORMAT_SIEMENS, 
    FORMAT_VTK, FORMAT_VOXBO_CUB, FORMAT_NRRD, FORMAT_COUNT};

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
  };

  // Image type. This is only for 3D images.
  typedef itk::ImageIOBase IOBase;
  typedef itk::SmartPointer<IOBase> IOBasePointer;

  typedef itk::ImageBase<3> ImageBase;
  typedef itk::SmartPointer<ImageBase> ImageBasePointer;

  GuidedNativeImageIO();
  virtual ~GuidedNativeImageIO()
    { DeallocateNativeImage(); }

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

  /**
   * Get the number of components in the native image read by ReadNativeImage.
   */
  size_t GetNumberOfComponentsInNativeImage() const;

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
  template<typename TPixel>
    void SaveImage(const char *FileName, Registry &folder, itk::Image<TPixel,3> *image);

  /** Parse registry to get file format */
  static FileFormat GetFileFormat(Registry &folder, FileFormat dflt = FORMAT_COUNT);

  /** Set the file format in a registry */
  static void SetFileFormat(Registry &folder, FileFormat format);

  /** Get format descriptor for a format */
  static FileFormatDescriptor GetFileFormatDescriptor(FileFormat fmt)
    { return m_FileFormatDescrictorArray[fmt]; }

  /** Parse registry to get pixel type of raw file */
  static RawPixelType GetPixelType(Registry &folder, RawPixelType dflt = PIXELTYPE_COUNT);

  /** Set the file format in a registry */
  static void SetPixelType(Registry &folder, RawPixelType type);

  // Get the output of the last operation
  // irisGetMacro(IOBase, itk::ImageIOBase *);    

private:
  
  /** 
   * Create an ImageIO object using a registry folder. Second parameter is
   * true for reading the file, false for writing the file
   */
  void CreateImageIO(const char *fname, Registry &folder, bool read);

  /** Templated function to create RAW image IO */
  template <typename TRaw> void CreateRawImageIO(Registry &folder);

  /** Templated function that reads a scalar image in its native datatype */
  template <typename TScalar> void DoReadNative(const char *fname, Registry &folder);

  /** 
   * This is a vector image in native format. It stores the data read from the
   * image file. The user must cast it to a desired type to use it.
   */
  ImageBasePointer m_NativeImage;

  // The IO base used to read the files
  IOBasePointer m_IOBase;

  // This information is copied from IOBase in order to delete IOBase at the 
  // earliest possible point, so as to conserve memory
  IOBase::IOComponentType m_NativeType;
  size_t m_NativeComponents;
  unsigned long m_NativeSizeInBytes;
  std::string m_NativeTypeString, m_NativeFileName;
  IOBase::ByteOrder m_NativeByteOrder;

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
};


/**
 * \class RescaleNativeImageToScalar
 * \brief An adapter class that rescales a native-format image from 
 * GuidedNativeImageIO to user-specified scalar type
 */
template<typename TPixel>
class RescaleNativeImageToScalar
{
public:
  RescaleNativeImageToScalar() {}
  virtual ~RescaleNativeImageToScalar() {}

  typedef itk::OrientedImage<TPixel,3> OutputImageType;
  typedef itk::ImageBase<3> NativeImageType;

  // Constructor, takes pointer to native image
  OutputImageType *operator()(GuidedNativeImageIO *nativeIO);

  // Get the scale factor to map from scalar to native
  irisGetMacro(NativeScale, double);

  // Get the shift to map from scalar to native
  irisGetMacro(NativeShift, double);

private:
  typename OutputImageType::Pointer m_Output;
  double m_NativeScale, m_NativeShift;

  // Method that does the casting
  template<typename TNative> void DoCast(itk::ImageBase<3> *native);
};

/**
 * \class CastNativeImageBase
 * \brief An adapter class that casts a native-format image from 
 * GuidedNativeImageIO to an image of type TPixel. The actual casting
 * is done using the functor TCastFunctor. Use derived classes.
 */
template<class TPixel, class TCastFunctor>
class CastNativeImageBase
{
public:
  typedef itk::OrientedImage<TPixel,3> OutputImageType;
  typedef itk::ImageBase<3> NativeImageType;

  // Constructor, takes pointer to native image
  OutputImageType *operator()(GuidedNativeImageIO *nativeIO);

private:
  typename OutputImageType::Pointer m_Output;

  // Method that does the casting
  template<typename TNative> void DoCast(itk::ImageBase<3> *native);
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
template<typename TRGBPixel>
class CastNativeImageToRGB : 
  public CastNativeImageBase<TRGBPixel, CastToArrayFunctor<TRGBPixel, 3> > {};


/**
 * \class CastNativeImageToScalar
 * \brief An adapter class that casts a native-format image from 
 * GuidedNativeImageIO to a scalar image of given type
 */
template<typename TPixel>
class CastNativeImageToScalar : 
  public CastNativeImageBase<TPixel, CastToScalarFunctor<TPixel> > {};


#endif
