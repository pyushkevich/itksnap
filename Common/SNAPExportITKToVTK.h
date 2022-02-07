/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPCommon.h,v $
  Language:  C++
  Date:      $Date: 2010/07/01 21:40:24 $
  Version:   $Revision: 1.13 $
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
#ifndef SNAPEXPORTITKTOVTK_H
#define SNAPEXPORTITKTOVTK_H

#include <itkVTKImageExport.h>
#include <vtkImageImport.h>

template <class TImageType>
void ConnectITKExporterToVTKImporter(
    itk::VTKImageExport<TImageType> *exporter, vtkImageImport *importer,
    bool connect_origin = true, bool connect_spacing = true, bool connect_direction = false)
{
  importer->SetUpdateInformationCallback(
    exporter->GetUpdateInformationCallback());
  importer->SetPipelineModifiedCallback(
    exporter->GetPipelineModifiedCallback());
  importer->SetWholeExtentCallback(
    exporter->GetWholeExtentCallback());
  if(connect_spacing)
    importer->SetSpacingCallback(
      exporter->GetSpacingCallback());
  if(connect_origin)
    importer->SetOriginCallback(
      exporter->GetOriginCallback());
  if(connect_direction)
    importer->SetDirectionCallback(
       exporter->GetDirectionCallback());
  importer->SetScalarTypeCallback(
    exporter->GetScalarTypeCallback());
  importer->SetNumberOfComponentsCallback(
    exporter->GetNumberOfComponentsCallback());
  importer->SetPropagateUpdateExtentCallback(
    exporter->GetPropagateUpdateExtentCallback());
  importer->SetUpdateDataCallback(
    exporter->GetUpdateDataCallback());
  importer->SetDataExtentCallback(
    exporter->GetDataExtentCallback());
  importer->SetBufferPointerCallback(
    exporter->GetBufferPointerCallback());
  importer->SetCallbackUserData(
    exporter->GetCallbackUserData());
}

#endif // SNAPEXPORTITKTOVTK_H
