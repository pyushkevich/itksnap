/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPRegistryIO.cxx,v $
  Language:  C++
  Date:      $Date: 2009/06/09 05:43:00 $
  Version:   $Revision: 1.4 $
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
#include "IRISVectorTypes.h"
#include "SNAPRegistryIO.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "SNAPImageData.h"
#include "HistoryManager.h"
#include <algorithm>
#include <vector>
#include <string>

#include "EdgePreprocessingSettings.h"
#include "ThresholdSettings.h"
#include "IntensityCurveInterface.h"

RegistryEnumMap<CoverageModeType> SNAPRegistryIO::m_EnumMapCoverage;
RegistryEnumMap<SnakeParameters::SolverType> SNAPRegistryIO::m_EnumMapSolver;
RegistryEnumMap<SnakeParameters::SnakeType> SNAPRegistryIO::m_EnumMapSnakeType;
RegistryEnumMap<SNAPSegmentationROISettings::InterpolationMethod> SNAPRegistryIO::m_EnumMapROI;
RegistryEnumMap<LayerRole> SNAPRegistryIO::m_EnumMapLayerRole;
RegistryEnumMap<LayerLayout> SNAPRegistryIO::m_EnumMapLayerLayout;


#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

using namespace std;

SNAPRegistryIO
::SNAPRegistryIO()
{
  BuildEnums();
}

/** Read snake parameters from a registry */
SnakeParameters 
SNAPRegistryIO
::ReadSnakeParameters(
  Registry &registry, const SnakeParameters &defaultSet)
{
  SnakeParameters out;
  
  out.SetAutomaticTimeStep(
    registry["AutomaticTimeStep"][defaultSet.GetAutomaticTimeStep()]);

  out.SetTimeStepFactor(
    registry["TimeStepFactor"][defaultSet.GetTimeStepFactor()]);

  out.SetGround(
    registry["Ground"][defaultSet.GetGround()]);

  out.SetClamp(
    registry["Clamp"][defaultSet.GetClamp()]);

  out.SetPropagationWeight(
    registry["PropagationWeight"][defaultSet.GetPropagationWeight()]);
  
  out.SetPropagationSpeedExponent(
    registry["PropagationSpeedExponent"][defaultSet.GetPropagationSpeedExponent()]);

  out.SetCurvatureWeight(
    registry["CurvatureWeight"][defaultSet.GetCurvatureWeight()]);

  out.SetCurvatureSpeedExponent(
    registry["CurvatureSpeedExponent"][defaultSet.GetCurvatureSpeedExponent()]);

  out.SetLaplacianWeight(
    registry["LaplacianWeight"][defaultSet.GetLaplacianWeight()]);

  out.SetLaplacianSpeedExponent(
    registry["LaplacianSpeedExponent"][defaultSet.GetLaplacianSpeedExponent()]);

  out.SetAdvectionWeight(
    registry["AdvectionWeight"][defaultSet.GetAdvectionWeight()]);

  out.SetAdvectionSpeedExponent(
    registry["AdvectionSpeedExponent"][defaultSet.GetAdvectionSpeedExponent()]);

  out.SetSnakeType(
    registry["SnakeType"].GetEnum(
      m_EnumMapSnakeType,defaultSet.GetSnakeType()));

  out.SetSolver(
    registry["SolverAlgorithm"].GetEnum(
      m_EnumMapSolver,defaultSet.GetSolver()));

  return out;
}

/** Write snake parameters to a registry */
void 
SNAPRegistryIO
::WriteSnakeParameters(const SnakeParameters &in,Registry &registry)
{
  registry["AutomaticTimeStep"] << in.GetAutomaticTimeStep();
  registry["TimeStepFactor"] << in.GetTimeStepFactor();
  registry["Ground"] << in.GetGround();
  registry["Clamp"] << in.GetClamp();
  registry["PropagationWeight"] << in.GetPropagationWeight();
  registry["PropagationSpeedExponent"][in.GetPropagationSpeedExponent()];
  registry["CurvatureWeight"] << in.GetCurvatureWeight();
  registry["CurvatureSpeedExponent"] << in.GetCurvatureSpeedExponent();
  registry["LaplacianWeight"] << in.GetLaplacianWeight();
  registry["LaplacianSpeedExponent"] << in.GetLaplacianSpeedExponent();
  registry["AdvectionWeight"] << in.GetAdvectionWeight();
  registry["AdvectionSpeedExponent"] << in.GetAdvectionSpeedExponent();
  registry["SnakeType"].PutEnum(m_EnumMapSnakeType,in.GetSnakeType());
  registry["SolverAlgorithm"].PutEnum(m_EnumMapSolver,in.GetSolver());
}

/** Read mesh options from a registry */
void
SNAPRegistryIO
::ReadMeshOptions(Registry &registry, MeshOptions *target)
{
  target->ReadFromRegistry(registry);
}

/** Write mesh options to a registry */
void 
SNAPRegistryIO
::WriteMeshOptions(const MeshOptions &in,Registry &registry)
{
  in.WriteToRegistry(registry);
}


/** Write region of interest settings to a registry folder */
void
SNAPRegistryIO
::WriteSegmentationROISettings(
  const SNAPSegmentationROISettings &in, Registry &folder)
{
  folder["ResampleDimensions"] << to_int(in.GetResampleDimensions());
  for(unsigned int d = 0; d < 3; d++)
    {
    Registry &sub = folder.Folder(folder.Key("ROIBox[%d]",d));
    sub["Index"] << in.GetROI().GetIndex(d);
    sub["Size"] << in.GetROI().GetSize(d);
    }
  folder["InterpolationMethod"].PutEnum(m_EnumMapROI,in.GetInterpolationMethod());
}

RegistryEnumMap<CoverageModeType> &SNAPRegistryIO::GetEnumMapCoverage()
{
  BuildEnums();
  return m_EnumMapCoverage;
}

RegistryEnumMap<SnakeParameters::SolverType> &SNAPRegistryIO::GetEnumMapSolver()
{
  BuildEnums();
  return m_EnumMapSolver;
}

RegistryEnumMap<SnakeParameters::SnakeType> &SNAPRegistryIO::GetEnumMapSnakeType()
{
  BuildEnums();
  return m_EnumMapSnakeType;
}

RegistryEnumMap<SNAPSegmentationROISettings::InterpolationMethod> &SNAPRegistryIO::GetEnumMapROI()
{
  BuildEnums();
  return m_EnumMapROI;
}

RegistryEnumMap<LayerRole> &SNAPRegistryIO::GetEnumMapLayerRole()
{
  BuildEnums();
  return m_EnumMapLayerRole;
}

RegistryEnumMap<LayerLayout> &SNAPRegistryIO::GetEnumMapLayerLayout()
{
  BuildEnums();
  return m_EnumMapLayerLayout;
}

SNAPSegmentationROISettings
SNAPRegistryIO
::ReadSegmentationROISettings(
  Registry &folder, const SNAPSegmentationROISettings &dfl)
{
  SNAPSegmentationROISettings out;

  out.SetInterpolationMethod(
    folder["InterpolationMethod"].GetEnum(
      m_EnumMapROI, dfl.GetInterpolationMethod()));

  // Read in the bounding box information
  itk::ImageRegion<3> dflRegion = dfl.GetROI();
  itk::ImageRegion<3> outRegion;
  for(unsigned int d = 0; d < 3; d++)
    {
    Registry &sub = folder.Folder(folder.Key("ROIBox[%d]",d));
    outRegion.SetIndex(d, sub["Index"][(int)dflRegion.GetIndex(d)]);
    outRegion.SetSize(d,  sub["Size"][(int) dflRegion.GetSize(d)]);    
    }
  out.SetROI(outRegion);

  // Read resampling properties. If the folder does not contain the resample
  // dimensions value (added in ITK-SNAP 3.0), we default to the ROI dimensions
  out.SetResampleDimensions(
        to_unsigned_int(
          folder["ResampleDimensions"][to_int(Vector3ui(outRegion.GetSize()))]));

  return out;
}  

void 
SNAPRegistryIO
::WriteImageAssociatedSettings(IRISApplication *app, Registry &registry)
{
  // Get the global state for this object
  GlobalState *gs = app->GetGlobalState();

  // Write snake parameters
  WriteSnakeParameters(
    gs->GetSnakeParameters(),
    registry.Folder("SNAP.SnakeParameters"));

  // Write the preprocessing settings
  app->GetEdgePreprocessingSettings()->WriteToRegistry(registry);

  // Read the threshold settings
  // TODO: how are we going to handle per-component settings?
  // app->GetThresholdSettings()->WriteToRegistry(registry);

  // Write the mesh display options
  WriteMeshOptions(
    *gs->GetMeshOptions(),
    registry.Folder("IRIS.MeshOptions"));

  // Save the intensity mapping curve
  if (app->GetIRISImageData()->IsMainLoaded())
    {
    ImageWrapperBase *main = app->GetCurrentImageData()->GetMain();
    main->GetDisplayMapping()->Save(registry.Folder("IRIS.DisplayMapping"));
    }

  // Write file related information
  registry["Files.Grey.Orientation"] << app->GetImageToAnatomyRAI();
  registry["Files.Grey.Dimensions"] << 
    to_int(app->GetIRISImageData()->GetVolumeExtents());

  // Write information about the current label state
  registry["IRIS.LabelState.DrawingLabel"] << 
    (int) gs->GetDrawingColorLabel();
  registry["IRIS.LabelState.OverwriteLabel"] << 
    (int) gs->GetDrawOverFilter().DrawOverLabel;
  registry["IRIS.LabelState.CoverageMode"].PutEnum(
    m_EnumMapCoverage,gs->GetDrawOverFilter().CoverageMode);
  registry["IRIS.LabelState.PolygonInvert"] << gs->GetPolygonInvert();
  registry["IRIS.LabelState.SegmentationAlpha"] << gs->GetSegmentationAlpha();

  // Write the information about the bounding box and ROI sub-sampling
  WriteSegmentationROISettings(
    gs->GetSegmentationROISettings(), registry.Folder("IRIS.BoundingBox"));

  // Write the color label table
  app->GetColorLabelTable()->SaveToRegistry(registry.Folder("IRIS.LabelTable"));

  // Write the local history
  app->GetSystemInterface()->GetHistoryManager()->SaveLocalHistory(
        registry.Folder("IOHistory"));

  // Write the tiling state
  registry["IRIS.SliceViewLayerLayout"].PutEnum(
        GetEnumMapLayerLayout(), gs->GetSliceViewLayerLayout());
}

bool 
SNAPRegistryIO
::ReadImageAssociatedSettings(
  Registry &registry, IRISApplication *app,
  bool restoreLabels, bool restorePreprocessing,
  bool restoreParameters, bool restoreDisplayOptions)
{
  // Get a pointer to the global state
  GlobalState *gs = app->GetGlobalState();

  // Get the main image if it's loaded
  ImageWrapperBase *main =
      app->IsMainImageLoaded()
      ? app->GetCurrentImageData()->GetMain()
      : NULL;

  // First of all, make sure that the image referred to in the association file
  // matches the image currently loaded
  Vector3i dims = (registry["Files.Grey.Dimensions"])[Vector3i(0)];
  if(dims != to_int(app->GetIRISImageData()->GetVolumeExtents()))
    return false;

  // Read the snake parameters (TODO: expand the parameters to include 
  // different settings for edge and in-out parameters)
  if(restoreParameters)
    {
    gs->SetSnakeParameters(
      SNAPRegistryIO::ReadSnakeParameters(
        registry.Folder("SNAP.SnakeParameters"),
        gs->GetSnakeParameters()));
    }

  // Read the preprocessing settings
  if(restorePreprocessing)
    {
    // Read the edge preprocessing settings
    app->GetEdgePreprocessingSettings()->ReadFromRegistry(
          registry.Folder("SNAP.Preprocessing.Edge"));
    
    // Read the thresholding settings (note that since they depend on an image
    // we have to use re-initialized defaults
    if (main)
      {
      // TODO: do something about reading and writing threshold settings
      // app->GetThresholdSettings()->ReadFromRegistry(
      //      registry, main->GetDefaultScalarRepresentation());
      }
    }

  // Read the display options
  if(restoreDisplayOptions)
    {
    // Read the mesh options
    SNAPRegistryIO::ReadMeshOptions(registry.Folder("IRIS.MeshOptions"), gs->GetMeshOptions());

    // Restore the intensity mapping curve
    if (main)
      {
      main->GetDisplayMapping()->Restore(registry.Folder("IRIS.DisplayMapping"));
      }

    // Tiling state
    gs->SetSliceViewLayerLayout(registry["IRIS.SliceViewLayerLayout"].GetEnum(
          GetEnumMapLayerLayout(), gs->GetSliceViewLayerLayout()));

    }

  // Read the information about the bounding box and ROI sub-sampling
  gs->SetSegmentationROISettings(
    SNAPRegistryIO::ReadSegmentationROISettings(
      registry.Folder("IRIS.BoundingBox"), gs->GetSegmentationROISettings()));

  // Read the label info
  if(restoreLabels) 
    {
    // Restore the table of color labels
    app->GetColorLabelTable()->LoadFromRegistry(registry.Folder("IRIS.LabelTable"));

    // Read the drawing color label
    gs->SetDrawingColorLabel(
      (LabelType)registry["IRIS.LabelState.DrawingLabel"][1]);

    // Read the override color label
    gs->SetDrawOverFilter(
          DrawOverFilter(
            registry["IRIS.LabelState.CoverageMode"].GetEnum(
              m_EnumMapCoverage,gs->GetCoverageMode()),
            (LabelType)registry["IRIS.LabelState.OverwriteLabel"][0]));
    
    // Read the polygon inversion state
    gs->SetPolygonInvert(
      registry["IRIS.LabelState.PolygonInvert"][gs->GetPolygonInvert()]);
    
    // Read the segmentation alpha
    gs->SetSegmentationAlpha(
      registry["IRIS.LabelState.SegmentationAlpha"][gs->GetSegmentationAlpha()]);
    } // If restore labels

  // Read other settings
  // TODO: erase
  /*
  gs->SetLastAssociatedSegmentationFileName(
    registry["Files.Segmentation.FileName"][""]);  
  gs->SetLastAssociatedPreprocessingFileName(
    registry["Files.Preprocessing.FileName"][""]);
    */

  // Read the local history
  app->GetSystemInterface()->GetHistoryManager()->LoadLocalHistory(
        registry.Folder("IOHistory"));

  // Done!
  return true;
}

void SNAPRegistryIO::BuildEnums()
{
  if(m_EnumMapCoverage.Size())
    return;

  // Set up the enum objects
  m_EnumMapCoverage.AddPair(PAINT_OVER_ALL,"OverAll");
  m_EnumMapCoverage.AddPair(PAINT_OVER_VISIBLE,"OverVisible");
  m_EnumMapCoverage.AddPair(PAINT_OVER_ONE,"OverOne");

  m_EnumMapSolver.AddPair(SnakeParameters::DENSE_SOLVER,"Dense");
  m_EnumMapSolver.AddPair(SnakeParameters::LEGACY_SOLVER,"Legacy");
  m_EnumMapSolver.AddPair(SnakeParameters::SPARSE_FIELD_SOLVER,"SparseField");
  m_EnumMapSolver.AddPair(SnakeParameters::NARROW_BAND_SOLVER,"NarrowBand");
  m_EnumMapSolver.AddPair(SnakeParameters::PARALLEL_SPARSE_FIELD_SOLVER,
                              "ParallelSparseField");

  m_EnumMapSnakeType.AddPair(SnakeParameters::EDGE_SNAKE,"EdgeStopping");
  m_EnumMapSnakeType.AddPair(SnakeParameters::REGION_SNAKE,"RegionCompetition");

  m_EnumMapROI.AddPair(SNAPSegmentationROISettings::NEAREST_NEIGHBOR,"Nearest");
  m_EnumMapROI.AddPair(SNAPSegmentationROISettings::TRILINEAR,"TriLinear");
  m_EnumMapROI.AddPair(SNAPSegmentationROISettings::TRICUBIC,"Cubic");
  m_EnumMapROI.AddPair(SNAPSegmentationROISettings::SINC_WINDOW_05,"Sinc05");

  m_EnumMapLayerRole.AddPair(MAIN_ROLE, "MainRole");
  m_EnumMapLayerRole.AddPair(OVERLAY_ROLE, "OverlayRole");
  m_EnumMapLayerRole.AddPair(LABEL_ROLE, "SegmentationRole");
  m_EnumMapLayerRole.AddPair(SNAP_ROLE, "SnakeModeRole");
  m_EnumMapLayerRole.AddPair(NO_ROLE, "InvalidRole");

  m_EnumMapLayerLayout.AddPair(LAYOUT_STACKED, "Stacked");
  m_EnumMapLayerLayout.AddPair(LAYOUT_TILED, "Tiled");
}
