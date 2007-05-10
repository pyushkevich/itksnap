/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: GuidedMeshIO.h,v $
  Language:  C++
  Date:      $Date: 2007/05/10 20:19:50 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __GuidedMeshIO_h_
#define __GuidedMeshIO_h_

#include "Registry.h"

class vtkPolyData;

/**
 \class GuidedMeshIO
 \brief Class that handles mesh file IO based on 'guidance' from registry 
 files.
 */
class GuidedMeshIO
{
public:
  virtual ~GuidedMeshIO() { /*To avoid compiler warning.*/ }
  
  enum FileFormat {
    FORMAT_VTK=0, FORMAT_STL, FORMAT_BYU, FORMAT_COUNT };

  /** Default constructor */
  GuidedMeshIO();

  /** Parse registry to get file format */
  FileFormat GetFileFormat(Registry &folder, FileFormat dflt = FORMAT_COUNT);

  /** Set the file format in a registry */
  void SetFileFormat(Registry &folder, FileFormat format);

  /** Registry mappings for these enums */
  RegistryEnumMap<FileFormat> m_EnumFileFormat;

  /** Save an image using the Registry folder to specify parameters */
  void SaveMesh(const char *FileName, Registry &folder, vtkPolyData *mesh);

};

#endif
