/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GuidedImageIO.h,v $
  Language:  C++
  Date:      $Date: 2008/11/15 12:20:38 $
  Version:   $Revision: 1.5 $
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
#ifndef __GuidedImageIO_h_
#define __GuidedImageIO_h_

#include "Registry.h"
#include "itkSmartPointer.h"
#include "itkImage.h"
#include "itkImageIOBase.h"

namespace itk
{
  template<class TPixel, unsigned int VDim> class Image;
  class ImageIOBase;
}

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


class GuidedImageIOBase
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

  /** Default constructor */
  GuidedImageIOBase();

  /** Destructor */  
  virtual ~GuidedImageIOBase() { }
  
  /** Parse registry to get file format */
  FileFormat GetFileFormat(Registry &folder, FileFormat dflt = FORMAT_COUNT);

  /** Set the file format in a registry */
  void SetFileFormat(Registry &folder, FileFormat format);

  /** Get format descriptor for a format */
  static const FileFormatDescriptor GetFileFormatDescriptor(FileFormat fmt)
    { return m_FileFormatDescrictorArray[fmt]; }

  /** Parse registry to get pixel type of raw file */
  RawPixelType GetPixelType(Registry &folder, RawPixelType dflt = PIXELTYPE_COUNT);

  /** Set the file format in a registry */
  void SetPixelType(Registry &folder, RawPixelType type);

  // Get the output of the last operation
  irisGetMacro(IOBase, itk::ImageIOBase *);    
    
  /** Registry mappings for these enums */
  RegistryEnumMap<FileFormat> m_EnumFileFormat;
  RegistryEnumMap<RawPixelType> m_EnumRawPixelType;

protected:

  // Class that create a Raw image IO object
  template<typename TRaw> class RawIOGenerator 
    {
    public:
      static void CreateRawImageIO(Registry &folder, typename itk::ImageIOBase::Pointer &ptr);
    };
  
  // The IO Base Object
  itk::SmartPointer<itk::ImageIOBase> m_IOBase;

  // File format descriptors
  static const FileFormatDescriptor m_FileFormatDescrictorArray[];

};

/**
 * \class GuidedImageIO
 * \brief This class performs image IO based on user-supplied parameters such
 * as explicit IO type, and for some types such as raw, the necessary additional
 * information.
 */
template<typename TPixel>
class GuidedImageIO : public GuidedImageIOBase
{
public:

  // An enum of different file types used in SNAP
  typedef GuidedImageIOBase::FileFormat FileFormat;
  typedef GuidedImageIOBase::RawPixelType RawPixelType;

  // Image type. This is only for 3D images.
  typedef itk::Image<TPixel, 3> ImageType;
  typedef itk::SmartPointer<ImageType> ImagePointer;
  typedef itk::SmartPointer<itk::ImageIOBase> IOBasePointer;

  /**
   * This method loads an image from the given filename. A registry can be 
   * supplied to assist the loading. The registry specifies the desired IO
   * type, and for special types, such as Raw it can provide the parameters
   * such as header size and image dimensions.
   */
  ImageType* ReadImage(
    const char *FileName, Registry &folder, bool flagCastFromNative);

  /** 
   * Get RAI code for an image. If there is nothing in the registry, this will
   * try getting the code from the image header. If there is no way to get the
   * image code, this will return an empty string
   */
  std::string GetRAICode(ImageType *image, Registry &folder);

  /** Save an image using the Registry folder to specify parameters */
  void SaveImage(const char *FileName, Registry &folder, ImageType *image);

private:
  
  /** Create an ImageIO object using a registry */
  void CreateImageIO(Registry &folder, FileFormat &format);

  /** Templated function that reads a scalar image in its native datatype */
  template <typename TScalar> void ReadFromNative();
  template <typename TScalar, typename TNative> void ReadAndCastImage();

  // The image resulting from the load operations
  ImagePointer m_Image;

  // Mapping from stored voxel intensities to native (input) intensities
  double m_NativeScale, m_NativeShift;
};

#endif
