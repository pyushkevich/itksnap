/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: SNAPRegistryIO.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:11 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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
#include "EdgePreprocessingSettings.h"
#include "ThresholdSettings.h"
#include "GuidedImageIO.h"

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
  MeshOptions ReadMeshOptions(
    Registry &registry,const MeshOptions &defaultSet);

  /** Write mesh options to a registry */
  void WriteMeshOptions(
    const MeshOptions &in,Registry &registry);
  
  /** Read edge preprocessing settings from a registry */
  EdgePreprocessingSettings ReadEdgePreprocessingSettings(
    Registry &registry,const EdgePreprocessingSettings &defaultSet);

  /** Write edge preprocessing settings to a registry */
  void WriteEdgePreprocessingSettings(
    const EdgePreprocessingSettings &in,Registry &registry);

  /** Read threshold settings from a registry */
  ThresholdSettings ReadThresholdSettings(
    Registry &registry,const ThresholdSettings &defaultSet);

  /** Write threshold settings to a registry */
  void WriteThresholdSettings(
    const ThresholdSettings &in,Registry &registry);

  /** Read ROI settings from a registry */
  SNAPSegmentationROISettings ReadSegmentationROISettings(
    Registry &folder, const SNAPSegmentationROISettings &defaultSet);
  
  /** Write ROI settings to a registry */
  void WriteSegmentationROISettings(
    const SNAPSegmentationROISettings &in, Registry &folder);



  // Some enumeraticns used by this class
  RegistryEnumMap<CoverageModeType> m_EnumMapCoverage;
  RegistryEnumMap<SnakeParameters::SolverType> m_EnumMapSolver;
  RegistryEnumMap<SnakeParameters::SnakeType> m_EnumMapSnakeType;
  RegistryEnumMap<SNAPSegmentationROISettings::InterpolationMethod> m_EnumMapROI;

};

#endif // __SNAPRegistryIO_h_

