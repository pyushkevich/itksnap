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
#include <set>

class vtkPolyData;
class MeshWrapperBase;

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
    FORMAT_VTK=0, FORMAT_STL, FORMAT_BYU, FORMAT_VRML, FORMAT_VTP, FORMAT_COUNT };

  struct MeshFormatDescriptor
  {
    std::string name;
    std::set<std::string> extensions;
    bool can_read;
    bool can_write;
  };

  typedef const std::map<FileFormat, const MeshFormatDescriptor> MeshFormatDescriptorMap;

  /** Default constructor */
  GuidedMeshIO();

  /** Parse registry to get file format */
  FileFormat GetFileFormat(Registry &folder, FileFormat dflt = FORMAT_COUNT);

  /** Set the file format in a registry */
  static void SetFileFormat(Registry &folder, FileFormat format);

  /** Get enum description */
  std::string GetFormatDescription(FileFormat formatEnum);

  /** Get format from extension */
  static FileFormat GetFormatByExtension(std::string extension);

  /** Save an image using the Registry folder to specify parameters */
  void SaveMesh(const char *FileName, Registry &folder, vtkPolyData *mesh);

  /** Load a mesh */
  void LoadMesh(const char *FileName, FileFormat format,
                SmartPtr<MeshWrapperBase> wrapper, unsigned int tp, LabelType id);

  /** Get the error message if the IO is not successful */
  std::string GetErrorMessage() const;

  /** Check if a format is readable by itk-snap */
  static bool can_read (FileFormat fmt);

  /** Check if a format is writtable by itk-snap */
  static bool can_write (FileFormat fmt);

  /** Map stores supported mesh format descriptors */
  static const MeshFormatDescriptorMap m_MeshFormatDescriptorMap;

  /** Get Enum File Format Registry Map */
  static RegistryEnumMap<FileFormat> &GetEnumFileFormat()
  { return m_EnumFileFormat; }

  static bool IsFilePolyData(const char *filename);

  static FileFormat GetFormatByFilename(const char *filename);

protected:
  /** Registry mappings for these enums */
  static RegistryEnumMap<FileFormat> m_EnumFileFormat;

  // Error message for unsucessful IO
  std::string m_ErrorMessage;

  // Export format string to help importing into 3D slicer
  // -- Hardcode to RAS since snap force exporting in RAS system
  // -- Reference: https://www.slicer.org/wiki/Documentation/Nightly/Developers/Tutorials/MigrationGuide/Slicer#Slicer_5.0:_Models_are_saved_in_LPS_coordinate_system_by_default
  const std::string slicer_coord_sys_string = "SPACE=RAS";

  std::string GetSlicerCoordSysComment() const;
};

#endif
