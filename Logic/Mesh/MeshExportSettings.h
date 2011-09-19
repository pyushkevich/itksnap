/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: MeshExportSettings.h,v $
  Language:  C++
  Date:      $Date: 2008/11/15 12:20:38 $
  Version:   $Revision: 1.3 $
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
#ifndef __MeshExportSettings_h_
#define __MeshExportSettings_h_

#include "SNAPCommon.h"
#include "GuidedMeshIO.h"
#include <string>

/**
 * \class MeshExportSettings
 * The set of values that are collected during interaction with the
 * mesh export wizard. These settings can be passed on to other parts
 * of the program.
 */
class MeshExportSettings {
public:
  typedef std::string StringType;
  typedef GuidedMeshIO::FileFormat FileFormat;

  irisGetMacro(MeshFileName, StringType);
  irisSetMacro(MeshFileName, StringType);

  irisGetMacro(MeshFormat, Registry);
  irisSetMacro(MeshFormat, Registry);

  irisGetMacro(FlagSingleLabel, bool);
  irisSetMacro(FlagSingleLabel, bool);

  irisGetMacro(FlagSingleScene, bool);
  irisSetMacro(FlagSingleScene, bool);

  irisGetMacro(ExportLabel, LabelType);
  irisSetMacro(ExportLabel, LabelType);

  virtual ~MeshExportSettings() {}

private:
  StringType m_MeshFileName;
  Registry m_MeshFormat;
  bool m_FlagSingleLabel;
  bool m_FlagSingleScene;
  LabelType m_ExportLabel;
};

#endif // __MeshExportSettings_h_
