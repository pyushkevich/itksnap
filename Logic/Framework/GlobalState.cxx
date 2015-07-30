/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GlobalState.cxx,v $
  Language:  C++
  Date:      $Date: 2009/06/16 04:55:45 $
  Version:   $Revision: 1.8 $
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
#include "GlobalState.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "MeshOptions.h"
#include "DefaultBehaviorSettings.h"

GlobalState
::GlobalState()
{
  m_GreyFileExtension = NULL;
  m_CrosshairsPosition[0] = 0;
  m_CrosshairsPosition[1] = 0;
  m_CrosshairsPosition[2] = 0;
  m_UpdateSliceFlag = 1;
  m_InterpolateGrey = false;
  m_InterpolateSegmentation = false;
  m_LockHeld = 0;
  m_LockOwner = 0;

  // Segmentation alpha model - range [0 1]
  m_SegmentationAlphaModel = NewRangedConcreteProperty<double>(0.5, 0.0, 1.0, 0.01);

  // SNAP is off initially
  m_SNAPActive = false;

  // Snake stuff:
  m_SpeedValid = false;
  m_ShowROI = false;
  m_DraggingROI = false;

  // Snake type model initialization
  m_SnakeTypeModel = ConcretePropertyModel<SnakeType, SnakeTypeDomain>::New();
  m_SnakeTypeModel->SetValue(IN_OUT_SNAKE);
  SnakeTypeDomain snakedomain;
  snakedomain[IN_OUT_SNAKE] = "Region Competition";
  snakedomain[EDGE_SNAKE] = "Edge Attraction";
  m_SnakeTypeModel->SetDomain(snakedomain);

  // Last used preprocessing mode
  m_LastUsedPreprocessingModeModel = NewSimpleConcreteProperty(PREPROCESS_THRESHOLD);

  // Initialize the property model for the ROI settings
  m_SegmentationROISettingsModel
      = NewSimpleConcreteProperty(SNAPSegmentationROISettings());

  m_SpeedViewZero = false;

  m_SnakeParametersModel = ConcreteSnakeParametersModel::New();
  m_SnakeParametersModel->SetValue(SnakeParameters::GetDefaultInOutParameters());

  // Bubbles
  m_ActiveBubble = -1;

  // Set paintbrush defaults
  m_PaintbrushSettings.radius = 4;
  m_PaintbrushSettings.mode = PAINTBRUSH_RECTANGULAR;
  m_PaintbrushSettings.volumetric = false;
  m_PaintbrushSettings.isotropic = false;
  m_PaintbrushSettings.chase = false;
  m_PaintbrushSettings.watershed.level = 0.2;
  m_PaintbrushSettings.watershed.smooth_iterations = 15;


  m_PolygonDrawingContextMenuModel = NewSimpleConcreteProperty(false);

  // Create the drawing label model
  m_DrawingColorLabelModel = ConcreteColorLabelPropertyModel::New();

  // Create the draw-over label model
  m_DrawOverFilterModel = ConcreteDrawOverFilterPropertyModel::New();

  // Polygon inversion - create and initialize
  m_PolygonInvertModel = NewSimpleConcreteProperty(false);

  m_SnakeInitializedWithManualSegmentationModel = NewSimpleConcreteProperty(false);

  // Mesh options
  m_MeshOptions = MeshOptions::New();

  // Default behaviors
  m_DefaultBehaviorSettings = DefaultBehaviorSettings::New();

  // Project stuff
  m_ProjectFilenameModel = NewSimpleConcreteProperty(std::string());

  // Initialize the properties
  m_ToolbarModeModel = NewSimpleConcreteProperty(CROSSHAIRS_MODE);
  m_ToolbarMode3DModel = NewSimpleConcreteProperty(TRACKBALL_MODE);

  m_SliceViewLayerLayoutModel = NewSimpleConcreteProperty(LAYOUT_STACKED);

  m_SelectedLayerIdModel = NewSimpleConcreteProperty(0ul);

  // Set annotation defaults
  m_AnnotationSettings.shownOnAllSlices = false;
  m_AnnotationModeModel = NewSimpleConcreteProperty(ANNOTATION_RULER);

  m_AnnotationColorModel = NewSimpleConcreteProperty(Vector3d(1, 0, 0));
}

void GlobalState::SetDriver(IRISApplication *parent)
{
  m_Driver = parent;

  m_DrawingColorLabelModel->Initialize(parent->GetColorLabelTable());
  m_DrawingColorLabelModel->SetValue(
        parent->GetColorLabelTable()->FindNextValidLabel(0,false));

  // Create the draw-over label model
  m_DrawOverFilterModel->Initialize(parent->GetColorLabelTable());
  m_DrawOverFilterModel->SetValue(DrawOverFilter(PAINT_OVER_ALL, 0));
}


GlobalState
::~GlobalState() 
{
  if(m_GreyFileExtension != NULL)
    delete [] m_GreyFileExtension;
}

#ifdef DRAWING_LOCK

// TODO: What's this for?
// mutual exclusion on the drawing lock - only one window can draw at a time
int 
GlobalState
::GetDrawingLock( short windowID ) 
{
  if ((!m_LockHeld) || (m_LockOwner == windowID)) 
    {
    m_LockHeld = 1;
    m_LockOwner = windowID;
    return 1;
    }
  return 0;
}

int 
GlobalState
::ReleaseDrawingLock(short windowID) 
{
  if (m_LockHeld && (m_LockOwner != windowID)) 
    {
    cerr << "hmph: Cannot ReleaseDrawingLock(): not lock owner" << endl;
    return 0;
    }
  m_LockHeld = 0;
  m_LockOwner = 0;
  return 1;
}

#endif /* DRAWING_LOCK */


void GlobalState::SetCoverageMode(CoverageModeType coverage)
{
  DrawOverFilter f = this->GetDrawOverFilter();
  f.CoverageMode = coverage;
  this->SetDrawOverFilter(f);
}

CoverageModeType GlobalState::GetCoverageMode() const
{
  return this->GetDrawOverFilter().CoverageMode;
}

bool GlobalState::isSegmentationROIValid()
{
  return this->GetSegmentationROI().GetNumberOfPixels() > 0;
}

void GlobalState::SetSegmentationROI(const GlobalState::RegionType &roi)
{
  SNAPSegmentationROISettings s = this->GetSegmentationROISettings();
  s.SetROI(roi);
  this->SetSegmentationROISettings(s);
}

GlobalState::RegionType GlobalState::GetSegmentationROI()
{
  return this->GetSegmentationROISettings().GetROI();
}


