/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GuidedMeshIO.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:43:03 $
  Version:   $Revision: 1.2 $
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

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
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
    FORMAT_VTK=0, FORMAT_STL, FORMAT_BYU, FORMAT_VRML, FORMAT_COUNT };

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
