/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPRegistryIO.h,v $
  Language:  C++
  Date:      $Date: 2009/07/22 21:06:23 $
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
#ifndef __SNAPRegistryIO_h_
#define __SNAPRegistryIO_h_

#include "GlobalState.h"
#include "Registry.h"
#include "SnakeParameters.h"
#include "MeshOptions.h"

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

class IRISApplication;

class SNAPRegistryIO 
{
public:
  SNAPRegistryIO();
  virtual ~SNAPRegistryIO() {}  
  
  /** Write settings associated with a greyscale image to a registry */
  void WriteImageAssociatedSettings(
    IRISApplication *app, Registry &registry);

  /** Read settings associated with a greyscale image and apply them to 
   * the current application */
  bool ReadImageAssociatedSettings(
    Registry &registry, IRISApplication *app,
    bool restoreLabels, bool restorePreprocessing,
    bool restoreParameters, bool restoreDisplayOptions);

  /** Read snake parameters from a registry */
  SnakeParameters ReadSnakeParameters(
    Registry &registry,const SnakeParameters &defaultSet);

  /** Write snake parameters to a registry */
  void WriteSnakeParameters(
    const SnakeParameters &in,Registry &registry);
  
  /** Read mesh options from a registry */
  void ReadMeshOptions(
    Registry &registry, MeshOptions *target);

  /** Write mesh options to a registry */
  void WriteMeshOptions(
    const MeshOptions &in,Registry &registry);
  
  /** Read ROI settings from a registry */
  SNAPSegmentationROISettings ReadSegmentationROISettings(
    Registry &folder, const SNAPSegmentationROISettings &defaultSet);
  
  /** Write ROI settings to a registry */
  void WriteSegmentationROISettings(
    const SNAPSegmentationROISettings &in, Registry &folder);

  static RegistryEnumMap<CoverageModeType> &GetEnumMapCoverage();
  static RegistryEnumMap<SnakeParameters::SolverType> &GetEnumMapSolver();
  static RegistryEnumMap<SnakeParameters::SnakeType> &GetEnumMapSnakeType();
  static RegistryEnumMap<SNAPSegmentationROISettings::InterpolationMethod> &GetEnumMapROI();
  static RegistryEnumMap<LayerRole> &GetEnumMapLayerRole();
  static RegistryEnumMap<LayerLayout> &GetEnumMapLayerLayout();

protected:

  // Method to build all the enums
  static void BuildEnums();

  // Some enumeraticns used by this class and others
  static RegistryEnumMap<CoverageModeType> m_EnumMapCoverage;
  static RegistryEnumMap<SnakeParameters::SolverType> m_EnumMapSolver;
  static RegistryEnumMap<SnakeParameters::SnakeType> m_EnumMapSnakeType;
  static RegistryEnumMap<SNAPSegmentationROISettings::InterpolationMethod> m_EnumMapROI;
  static RegistryEnumMap<LayerRole> m_EnumMapLayerRole;
  static RegistryEnumMap<LayerLayout> m_EnumMapLayerLayout;

};

#endif // __SNAPRegistryIO_h_

