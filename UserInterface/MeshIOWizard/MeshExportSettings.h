/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: MeshExportSettings.h,v $
  Language:  C++
  Date:      $Date: 2007/05/10 20:19:51 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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

private:
  StringType m_MeshFileName;
  Registry m_MeshFormat;
  bool m_FlagSingleLabel;
  bool m_FlagSingleScene;
  LabelType m_ExportLabel;
};

#endif // __MeshExportSettings_h_
