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
#include <algorithm>
#include <vector>
#include <string>

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

using namespace std;

SNAPRegistryIO
::SNAPRegistryIO()
{
  // Set up the enum objects
  m_EnumMapCoverage.AddPair(PAINT_OVER_ALL,"OverAll");
  m_EnumMapCoverage.AddPair(PAINT_OVER_COLORS,"OverVisible");
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
MeshOptions 
SNAPRegistryIO
::ReadMeshOptions(
  Registry &registry, const MeshOptions &defaultSet)
{
  MeshOptions out;
  
  out.SetUseGaussianSmoothing(
    registry["UseGaussianSmoothing"][defaultSet.GetUseGaussianSmoothing()]);
    
  out.SetUseDecimation(
    registry["UseDecimation"][defaultSet.GetUseDecimation()]);

  out.SetUseMeshSmoothing(
    registry["UseMeshSmoothing"][defaultSet.GetUseMeshSmoothing()]);

  out.SetGaussianStandardDeviation(
    registry["GaussianStandardDeviation"][defaultSet.GetGaussianStandardDeviation()]);

  out.SetGaussianError(
    registry["GaussianError"][defaultSet.GetGaussianError()]);

  out.SetDecimateTargetReduction(
    registry["DecimateTargetReduction"][defaultSet.GetDecimateTargetReduction()]);

  out.SetDecimateInitialError(
    registry["DecimateInitialError"][defaultSet.GetDecimateInitialError()]);

  out.SetDecimateAspectRatio(
    registry["DecimateAspectRatio"][defaultSet.GetDecimateAspectRatio()]);

  out.SetDecimateFeatureAngle(
    registry["DecimateFeatureAngle"][defaultSet.GetDecimateFeatureAngle()]);

  out.SetDecimateErrorIncrement(
    registry["DecimateErrorIncrement"][defaultSet.GetDecimateErrorIncrement()]);

  out.SetDecimateMaximumIterations(
    registry["DecimateMaximumIterations"][defaultSet.GetDecimateMaximumIterations()]);

  out.SetDecimatePreserveTopology(
    registry["DecimatePreserveTopology"][defaultSet.GetDecimatePreserveTopology()]);

  out.SetMeshSmoothingRelaxationFactor(
    registry["MeshSmoothingRelaxationFactor"][defaultSet.GetMeshSmoothingRelaxationFactor()]);

  out.SetMeshSmoothingIterations(
    registry["MeshSmoothingIterations"][defaultSet.GetMeshSmoothingIterations()]);

  out.SetMeshSmoothingConvergence(
    registry["MeshSmoothingConvergence"][defaultSet.GetMeshSmoothingConvergence()]);

  out.SetMeshSmoothingFeatureAngle(
    registry["MeshSmoothingFeatureAngle"][defaultSet.GetMeshSmoothingFeatureAngle()]);

  out.SetMeshSmoothingFeatureEdgeSmoothing(
    registry["MeshSmoothingFeatureEdgeSmoothing"][defaultSet.GetMeshSmoothingFeatureEdgeSmoothing()]);

  out.SetMeshSmoothingBoundarySmoothing(
    registry["MeshSmoothingBoundarySmoothing"][defaultSet.GetMeshSmoothingBoundarySmoothing()]);

  return out;
}

/** Write mesh options to a registry */
void 
SNAPRegistryIO
::WriteMeshOptions(const MeshOptions &in,Registry &registry)
{
  registry["UseGaussianSmoothing"] << in.GetUseGaussianSmoothing();
  registry["UseDecimation"] << in.GetUseDecimation();
  registry["UseMeshSmoothing"] << in.GetUseMeshSmoothing();
  registry["GaussianStandardDeviation"] << in.GetGaussianStandardDeviation();
  registry["GaussianError"] << in.GetGaussianError();
  registry["DecimateTargetReduction"] << in.GetDecimateTargetReduction();
  registry["DecimateInitialError"] << in.GetDecimateInitialError();
  registry["DecimateAspectRatio"] << in.GetDecimateAspectRatio();
  registry["DecimateFeatureAngle"] << in.GetDecimateFeatureAngle();
  registry["DecimateErrorIncrement"] << in.GetDecimateErrorIncrement();
  registry["DecimateMaximumIterations"] << in.GetDecimateMaximumIterations();
  registry["DecimatePreserveTopology"] << in.GetDecimatePreserveTopology();
  registry["MeshSmoothingRelaxationFactor"] << in.GetMeshSmoothingRelaxationFactor();
  registry["MeshSmoothingIterations"] << in.GetMeshSmoothingIterations();
  registry["MeshSmoothingConvergence"] << in.GetMeshSmoothingConvergence();
  registry["MeshSmoothingFeatureAngle"] << in.GetMeshSmoothingFeatureAngle();
  registry["MeshSmoothingFeatureEdgeSmoothing"] << in.GetMeshSmoothingFeatureEdgeSmoothing();
  registry["MeshSmoothingBoundarySmoothing"] << in.GetMeshSmoothingBoundarySmoothing();
}

/** Read edge preprocessing settings from a registry */
EdgePreprocessingSettings 
SNAPRegistryIO
::ReadEdgePreprocessingSettings(
  Registry &registry, const EdgePreprocessingSettings &defaultSet)
{
  EdgePreprocessingSettings out;

  out.SetGaussianBlurScale(
    registry["GaussianBlurScale"][defaultSet.GetGaussianBlurScale()]);

  out.SetRemappingSteepness(
    registry["RemappingSteepness"][defaultSet.GetRemappingSteepness()]);

  out.SetRemappingExponent(
    registry["RemappingExponent"][defaultSet.GetRemappingExponent()]);
  
  return out;  
}

/** Write edge preprocessing settings to a registry */
void 
SNAPRegistryIO
::WriteEdgePreprocessingSettings(const EdgePreprocessingSettings &in,
                                 Registry &registry)
{
  registry["GaussianBlurScale"] << in.GetGaussianBlurScale();
  registry["RemappingSteepness"] << in.GetRemappingSteepness();
  registry["RemappingExponent"] << in.GetRemappingExponent();
}

/** Read threshold settings from a registry */
ThresholdSettings 
SNAPRegistryIO
::ReadThresholdSettings(
  Registry &registry, const ThresholdSettings &defaultSet)
{
  ThresholdSettings out;
  
  out.SetLowerThreshold(
    registry["LowerThreshold"][defaultSet.GetLowerThreshold()]);

  out.SetUpperThreshold(
    registry["UpperThreshold"][defaultSet.GetUpperThreshold()]);

  out.SetSmoothness(
    registry["Smoothness"][defaultSet.GetSmoothness()]);

  out.SetLowerThresholdEnabled(
    registry["LowerThresholdEnabled"][defaultSet.IsLowerThresholdEnabled()]);

  out.SetUpperThresholdEnabled(
    registry["UpperThresholdEnabled"][defaultSet.IsUpperThresholdEnabled()]);

  // Check that what we've read is valid
  if(!out.IsValid())
    out = defaultSet;

  return out;
}

/** Write threshold settings to a registry */
void 
SNAPRegistryIO
::WriteThresholdSettings(const ThresholdSettings &in,Registry &registry)
{
  registry["LowerThreshold"] << in.GetLowerThreshold();
  registry["UpperThreshold"] << in.GetUpperThreshold();
  registry["Smoothness"] << in.GetSmoothness();
  registry["LowerThresholdEnabled"] << in.IsLowerThresholdEnabled();
  registry["UpperThresholdEnabled"] << in.IsUpperThresholdEnabled();
}

/** Write region of interest settings to a registry folder */
void
SNAPRegistryIO
::WriteSegmentationROISettings(
  const SNAPSegmentationROISettings &in, Registry &folder)
{
  folder["ResampleFlag"] << in.GetResampleFlag();
  folder["VoxelScale"] << in.GetVoxelScale();  
  for(unsigned int d = 0; d < 3; d++)
    {
    Registry &sub = folder.Folder(folder.Key("ROIBox[%d]",d));
    sub["Index"] << in.GetROI().GetIndex(d);
    sub["Size"] << in.GetROI().GetSize(d);
    }
  folder["InterpolationMethod"].PutEnum(m_EnumMapROI,in.GetInterpolationMethod());
}

SNAPSegmentationROISettings 
SNAPRegistryIO
::ReadSegmentationROISettings(
  Registry &folder, const SNAPSegmentationROISettings &dfl)
{
  SNAPSegmentationROISettings out;

  // Read resampling properties
  out.SetResampleFlag(folder["ResampleFlag"][dfl.GetResampleFlag()]);
  out.SetVoxelScale(folder["VoxelScale"][dfl.GetVoxelScale()]);
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
  WriteEdgePreprocessingSettings(
    gs->GetEdgePreprocessingSettings(),
    registry.Folder("SNAP.Preprocessing.Edge"));
  WriteThresholdSettings(
    gs->GetThresholdSettings(),
    registry.Folder("SNAP.Preprocessing.Region"));

  // Write the mesh display options
  WriteMeshOptions(
    gs->GetMeshOptions(),
    registry.Folder("IRIS.MeshOptions"));

  // Save the intensity mapping curve
  if (app->GetIRISImageData()->IsGreyLoaded())
    {
    app->GetCurrentImageData()->GetGrey()->GetIntensityMapFunction()->
      SaveToRegistry(registry.Folder("IRIS.IntensityCurve"));
    }

  // Write file related information
  registry["Files.Segmentation.FileName"] << gs->GetLastAssociatedSegmentationFileName();
  registry["Files.Preprocessing.FileName"] << gs->GetLastAssociatedPreprocessingFileName();
  registry["Files.Grey.Orientation"] << app->GetImageToAnatomyRAI();
  registry["Files.Grey.Dimensions"] << 
    to_int(app->GetIRISImageData()->GetVolumeExtents());

  // Write information about the current label state
  registry["IRIS.LabelState.DrawingLabel"] << 
    (int) gs->GetDrawingColorLabel();
  registry["IRIS.LabelState.OverwriteLabel"] << 
    (int) gs->GetOverWriteColorLabel();
  registry["IRIS.LabelState.CoverageMode"].PutEnum(
    m_EnumMapCoverage,gs->GetCoverageMode());
  registry["IRIS.LabelState.PolygonInvert"] << gs->GetPolygonInvert();
  registry["IRIS.LabelState.OverallAlpha"] << 
    (int) gs->GetSegmentationAlpha();

  // Write the information about the bounding box and ROI sub-sampling
  WriteSegmentationROISettings(
    gs->GetSegmentationROISettings(), registry.Folder("IRIS.BoundingBox"));

  // Write the color label table
  app->GetColorLabelTable()->SaveToRegistry(registry.Folder("IRIS.LabelTable"));
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

  // First of all, make sure that the image referred to in the association file
  // matches the image currently loaded
  Vector3i xxx=iris_vector_fixed<int,3>(7);
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
    gs->SetEdgePreprocessingSettings(
      SNAPRegistryIO::ReadEdgePreprocessingSettings(
        registry.Folder("SNAP.Preprocessing.Edge"),
        gs->GetEdgePreprocessingSettings()));
    
    // Read the thresholding settings (note that since they depend on an image
    // we have to use re-initialized defaults
    if (app->GetIRISImageData()->IsGreyLoaded())
      {
    gs->SetThresholdSettings(
      SNAPRegistryIO::ReadThresholdSettings(
        registry.Folder("SNAP.Preprocessing.Region"),
        ThresholdSettings::MakeDefaultSettings(
          app->GetIRISImageData()->GetGrey())));
	 }
    }

  // Read the display options
  if(restoreDisplayOptions)
    {
    // Read the mesh options
    gs->SetMeshOptions(
      SNAPRegistryIO::ReadMeshOptions(
        registry.Folder("IRIS.MeshOptions"),
        gs->GetMeshOptions()));

    // Restore the intensity mapping curve
    if (app->GetIRISImageData()->IsGreyLoaded())
      {
      app->GetCurrentImageData()->GetGrey()->GetIntensityMapFunction()->
        LoadFromRegistry(registry.Folder("IRIS.IntensityCurve"));
      app->GetCurrentImageData()->GetGrey()->UpdateIntensityMapFunction();
	 }
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
    gs->SetOverWriteColorLabel(
      (LabelType)registry["IRIS.LabelState.OverwriteLabel"][0]);
    
    // Read the coverage mode
    gs->SetCoverageMode(
      registry["IRIS.LabelState.CoverageMode"].GetEnum(
        m_EnumMapCoverage,gs->GetCoverageMode()));      

    // Read the polygon inversion state
    gs->SetPolygonInvert(
      registry["IRIS.LabelState.PolygonInvert"][gs->GetPolygonInvert()]);
    
    // Read the segmentation alpha
    gs->SetSegmentationAlpha(
      registry["IRIS.LabelState.OverallAlpha"][gs->GetSegmentationAlpha()]);
    } // If restore labels

  // Read other settings
  gs->SetLastAssociatedSegmentationFileName(
    registry["Files.Segmentation.FileName"][""]);  
  gs->SetLastAssociatedPreprocessingFileName(
    registry["Files.Preprocessing.FileName"][""]);

  // Done!
  return true;
}
