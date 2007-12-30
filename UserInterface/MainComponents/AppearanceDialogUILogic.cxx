/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: AppearanceDialogUILogic.cxx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:17 $
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

=========================================================================*/
#include "AppearanceDialogUILogic.h"
#include "SNAPAppearanceSettings.h"
#include "UserInterfaceBase.h"
#include "GlobalState.h"
#include "IRISApplication.h"
#include "SystemInterface.h"
#include <string>

using namespace std;

const int
AppearanceDialogUILogic
::m_MapMenuToElementIndex[] = 
  { 
    SNAPAppearanceSettings::MARKERS,
    SNAPAppearanceSettings::CROSSHAIRS,
    SNAPAppearanceSettings::ROI_BOX,
    SNAPAppearanceSettings::PAINTBRUSH_OUTLINE,
    SNAPAppearanceSettings::BACKGROUND_2D,
    SNAPAppearanceSettings::BACKGROUND_3D,
    SNAPAppearanceSettings::CROSSHAIRS_3D,
    SNAPAppearanceSettings::IMAGE_BOX_3D,
    SNAPAppearanceSettings::ROI_BOX_3D,
    SNAPAppearanceSettings::ZOOM_THUMBNAIL,
    SNAPAppearanceSettings::CROSSHAIRS_THUMB
  };

AppearanceDialogUILogic
::AppearanceDialogUILogic()
{
  m_DefaultAppearance = new SNAPAppearanceSettings();
}

AppearanceDialogUILogic
::~AppearanceDialogUILogic()
{
  delete m_DefaultAppearance;
}

void
AppearanceDialogUILogic
::Register(UserInterfaceBase *parent) 
{
  m_Parent = parent;
  m_Appearance = parent->GetAppearanceSettings();
  m_GlobalState = parent->GetDriver()->GetGlobalState();
} 

// Close callbacks
void  
AppearanceDialogUILogic
::OnCloseAction()
{
  m_WinDisplayOptions->hide();
}

// General slice options
void  
AppearanceDialogUILogic
::OnSliceDisplayApplyAction()
{
  // Store the appearance options
  m_Appearance->SetFlagDisplayZoomThumbnail(
    m_ChkOptionsSliceThumbnailOn->value() != 0);
  
  m_Appearance->SetFlagLinkedZoomByDefault(
    m_ChkOptionsSliceLinkedZoom->value() != 0);
  
  m_Appearance->SetZoomThumbnailSizeInPercent(
    m_InOptionsSliceThumbnailPercent->value());
  
  m_Appearance->SetZoomThumbnailMaximumSize(
    (int) m_InOptionsSliceThumbnailMaxSize->value());

  // Place the options in the registry
  m_Appearance->SaveToRegistry(
    m_Parent->GetSystemInterface()->Folder("UserInterface.AppearanceSettings"));  

  // Redraw the UI windows
  m_Parent->RedrawWindows();
}

void  
AppearanceDialogUILogic
::OnSliceDisplayResetAction()
{
  // Restore the appearance options
  m_Appearance->SetFlagDisplayZoomThumbnail(
    m_DefaultAppearance->GetFlagDisplayZoomThumbnail());
  
  m_Appearance->SetFlagLinkedZoomByDefault(
    m_DefaultAppearance->GetFlagLinkedZoomByDefault());
  
  m_Appearance->SetZoomThumbnailSizeInPercent(
    m_DefaultAppearance->GetZoomThumbnailSizeInPercent());
  
  m_Appearance->SetZoomThumbnailMaximumSize(
    m_DefaultAppearance->GetZoomThumbnailMaximumSize());

  // Place the options in the registry
  m_Appearance->SaveToRegistry(
    m_Parent->GetSystemInterface()->Folder("UserInterface.AppearanceSettings"));  

  // Redraw the UI windows
  m_Parent->RedrawWindows();

  // Refill the options
  FillAppearanceSettings();
}

// View arrangement callbacks
void  
AppearanceDialogUILogic
::OnScreenLayoutApplyAction()
{
  unsigned int order[6][3] = 
    {{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};

  // Search for the selected orientation toggle
  unsigned int i;
  for(i=0;i<6;i++)
    if(m_BtnOptionsViews2D[i]->value())
      break;

  // Make sure something is selected
  if(i == 6) return;

  // Start with stock orientations
  string axes[3] = {string("RPS"),string("AIL"),string("RIP")};

  // Switch the configurable directions
  if(m_ChkOptionsViews2DRightIsLeft->value() == 0)
    {
    axes[0][0] = axes[2][0] = 'L';
    }
  if(m_ChkOptionsViews2DNoseLeft->value() == 0)
    {
    axes[1][0] = 'P';
    }

  // Check if the configuration is different
  if(axes[order[i][0]] != string(m_Parent->GetDriver()->GetDisplayToAnatomyRAI(0)) ||
     axes[order[i][1]] != string(m_Parent->GetDriver()->GetDisplayToAnatomyRAI(1)) ||
     axes[order[i][2]] != string(m_Parent->GetDriver()->GetDisplayToAnatomyRAI(2)))
    {
    // Assign the configuration
    m_Parent->GetDriver()->SetDisplayToAnatomyRAI(
      axes[order[i][0]].c_str(), 
      axes[order[i][1]].c_str(), 
      axes[order[i][2]].c_str());
    
    // Reassign slices to windows
    m_Parent->OnImageGeometryUpdate();

    // Redraw the windows
    m_Parent->RedrawWindows();
    }  
}

void  
AppearanceDialogUILogic
::OnScreenLayoutResetAction()
{
  
}

void  
AppearanceDialogUILogic
::OnSliceAnatomyOptionsChange(unsigned int order)
{
  
}

// 3D Rendering callbacks
void  
AppearanceDialogUILogic
::On3DRenderingApplyAction()
{
  // Get the current mesh options
  MeshOptions mops;

  // Set the Gaussian properties
  mops.SetGaussianStandardDeviation(
    m_InRenderOptionsGaussianStandardDeviation->value());
  mops.SetUseGaussianSmoothing(
    m_InUseGaussianSmoothing->value());
  mops.SetGaussianError(
    m_InRenderOptionsGaussianError->value());

  // Triangle Decimation
  mops.SetUseDecimation(
    m_InUseDecimate->value());
  mops.SetDecimateAspectRatio(
    m_InRenderOptionsDecimateAspectRatio->value());
  mops.SetDecimateErrorIncrement(
    m_InRenderOptionsDecimateErrorIncrement->value());
  mops.SetDecimateFeatureAngle(
    m_InRenderOptionsDecimateFeatureAngle->value());
  mops.SetDecimateInitialError(
    m_InRenderOptionsDecimateInitialError->value());
  mops.SetDecimateMaximumIterations(
    (unsigned int)m_InRenderOptionsDecimateIterations->value());
  mops.SetDecimatePreserveTopology(
    m_InRenderOptionsDecimateTopology->value());
  mops.SetDecimateTargetReduction(
    m_InRenderOptionsDecimateReductions->value());

  // Mesh Smoothing
  mops.SetUseMeshSmoothing(
    m_InUseMeshSmoothing->value());  
  mops.SetMeshSmoothingBoundarySmoothing(
    m_InRenderOptionsMeshSmoothBoundarySmoothing->value());
  mops.SetMeshSmoothingConvergence(
    m_InRenderOptionsMeshSmoothConvergence->value());
  mops.SetMeshSmoothingFeatureAngle(
    m_InRenderOptionsMeshSmoothFeatureAngle->value());
  mops.SetMeshSmoothingFeatureEdgeSmoothing(
    m_InRenderOptionsMeshSmoothFeatureEdge->value());
  mops.SetMeshSmoothingIterations(
    (unsigned int)m_InRenderOptionsMeshSmoothIterations->value());
  mops.SetMeshSmoothingRelaxationFactor(
    m_InRenderOptionsMeshSmoothRelaxation->value());

  // Save the mesh options
  m_GlobalState->SetMeshOptions(mops); 
  
  // Enable the update button on the 3D window
  m_Parent->OnIRISMeshDisplaySettingsUpdate();
}

void  
AppearanceDialogUILogic
::On3DRenderingResetAction()
{
  // Reset the options in the system to defaults
  MeshOptions mops;
  m_GlobalState->SetMeshOptions(mops);

  // Refill the options
  FillRenderingOptions();
}

// Element appearance callbacks
void  
AppearanceDialogUILogic
::OnUIElementUpdate()
{
  // Nothing for now
}

void  
AppearanceDialogUILogic
::OnUIElementSelection(int value)
{
  // Must have a value!
  if(value < 0) return;

  // Find out the which element was selected
  int iElement = m_MapMenuToElementIndex[value];

  // Make sure a legit element was found
  if(iElement == SNAPAppearanceSettings::ELEMENT_COUNT)
    return;

  // Set the user interface properties 
  SNAPAppearanceSettings::Element &e = m_Appearance->GetUIElement(iElement);
  m_InColorNormal->rgb( e.NormalColor[0], e.NormalColor[1], e.NormalColor[2] );
  m_InColorActive->rgb( e.ActiveColor[0], e.ActiveColor[1], e.ActiveColor[2] );
  m_InLineThickness->value( e.LineThickness ); 
  m_InDashSpacing->value( e.DashSpacing ); 
  m_InFontSize->value( e.FontSize ); 
  m_InVisible->value( e.Visible );
  m_InAlphaBlending->value( e.AlphaBlending );

  // Create an array of widgets for cleaner code
  Fl_Widget *w[] = 
    { m_InColorNormal, m_InColorActive, m_InLineThickness,
      m_InDashSpacing, m_InFontSize, m_InVisible, m_InAlphaBlending };

  // Set the active/inactive status
  for(unsigned int iWidget = 0; iWidget < SNAPAppearanceSettings::FEATURE_COUNT; iWidget++)
    {
    if(m_Appearance->IsFeatureApplicable(iElement, iWidget))
      {
      w[iWidget]->activate();
      if(iWidget < 2) w[iWidget]->show();
      }
    else
      {
      w[iWidget]->deactivate();
      if(iWidget < 2) w[iWidget]->hide();
      }
    }  
}

void  
AppearanceDialogUILogic
::OnElementAppearanceResetAllAction()
{
  // Reset each element
  for(unsigned int i = 0; i < SNAPAppearanceSettings::ELEMENT_COUNT; i++)
    m_Appearance->SetUIElement(i,m_DefaultAppearance->GetUIElement(i));

  // Redraw the current control
  OnUIElementSelection(m_InUIElement->value());

  // Redraw the windows
  m_Parent->RedrawWindows();
  
  // Place the options in the registry
  m_Appearance->SaveToRegistry(
    m_Parent->GetSystemInterface()->Folder("UserInterface.AppearanceSettings"));  
}

void  
AppearanceDialogUILogic
::OnElementAppearanceResetAction()
{
  // Must have a value!
  if(m_InUIElement->value() < 0) return;

  // Find out the which element was selected
  int iElement = m_MapMenuToElementIndex[m_InUIElement->value()];

  // Make sure a legit element was found
  if(iElement == SNAPAppearanceSettings::ELEMENT_COUNT)
    return;

  // Reset the element
  m_Appearance->SetUIElement(
    iElement,m_DefaultAppearance->GetUIElement(iElement));

  // Redraw the controls
  OnUIElementSelection(iElement);

  // Redraw the windows
  m_Parent->RedrawWindows();
  
  // Place the options in the registry
  m_Appearance->SaveToRegistry(
    m_Parent->GetSystemInterface()->Folder("UserInterface.AppearanceSettings"));  
}

void  
AppearanceDialogUILogic
::OnElementAppearanceApplyAction()
{
  // Must have a value!
  if(m_InUIElement->value() < 0) return;

  // Find out the which element was selected
  int iElement = m_MapMenuToElementIndex[m_InUIElement->value()];

  // Make sure a legit element was found
  if(iElement == SNAPAppearanceSettings::ELEMENT_COUNT)
    return;

  // Set the element's properties
  SNAPAppearanceSettings::Element &e = m_Appearance->GetUIElement(iElement);
  e.NormalColor[0] = (float) m_InColorNormal->r();
  e.NormalColor[1] = (float) m_InColorNormal->g();
  e.NormalColor[2] = (float) m_InColorNormal->b();
  e.ActiveColor[0] = (float) m_InColorActive->r();
  e.ActiveColor[1] = (float) m_InColorActive->g();
  e.ActiveColor[2] = (float) m_InColorActive->b();
  e.LineThickness = (float) m_InLineThickness->value();
  e.DashSpacing = (float) m_InDashSpacing->value();
  e.FontSize = (int) m_InFontSize->value();
  e.AlphaBlending = m_InAlphaBlending->value() != 0;
  e.Visible = m_InVisible->value() != 0;
  
  // Redraw the windows
  m_Parent->RedrawWindows();
  
  // Place the options in the registry
  m_Appearance->SaveToRegistry(
    m_Parent->GetSystemInterface()->Folder("UserInterface.AppearanceSettings"));  
}

void 
AppearanceDialogUILogic
::ShowDialog()
{
  // Fill out the panels of the dialog
  FillAppearanceSettings();
  FillSliceLayoutOptions();
  FillRenderingOptions();

  // Describe the currently selected UI element
  OnUIElementSelection( m_InUIElement->value() );

  // Show the dialog
  m_WinDisplayOptions->show();
}

void 
AppearanceDialogUILogic
::FillRenderingOptions() 
{
  // Get the current mesh options
  MeshOptions mops = m_GlobalState->GetMeshOptions();

  // Set the Gaussian properties
  m_InRenderOptionsGaussianStandardDeviation->value(
    mops.GetGaussianStandardDeviation());
  m_InUseGaussianSmoothing->value(
    mops.GetUseGaussianSmoothing());
  m_InRenderOptionsGaussianError->value(
    mops.GetGaussianError());

  // Triangle Decimation
  m_InUseDecimate->value(
    mops.GetUseDecimation());
  m_InRenderOptionsDecimateAspectRatio->value(
    mops.GetDecimateAspectRatio());
  m_InRenderOptionsDecimateErrorIncrement->value(
    mops.GetDecimateErrorIncrement());
  m_InRenderOptionsDecimateFeatureAngle->value(
    mops.GetDecimateFeatureAngle());
  m_InRenderOptionsDecimateInitialError->value(
    mops.GetDecimateInitialError());
  m_InRenderOptionsDecimateIterations->value(
    (double)mops.GetDecimateMaximumIterations());
  m_InRenderOptionsDecimateTopology->value(
    mops.GetDecimatePreserveTopology());
  m_InRenderOptionsDecimateReductions->value(
    mops.GetDecimateTargetReduction());

  // Mesh Smoothing
  m_InUseMeshSmoothing->value(
    mops.GetUseMeshSmoothing());  
  m_InRenderOptionsMeshSmoothBoundarySmoothing->value(
    mops.GetMeshSmoothingBoundarySmoothing());
  m_InRenderOptionsMeshSmoothConvergence->value(
    mops.GetMeshSmoothingConvergence());
  m_InRenderOptionsMeshSmoothFeatureAngle->value(
    mops.GetMeshSmoothingFeatureAngle());
  m_InRenderOptionsMeshSmoothFeatureEdge->value(
    mops.GetMeshSmoothingFeatureEdgeSmoothing());
  m_InRenderOptionsMeshSmoothIterations->value(
    (double)mops.GetMeshSmoothingIterations());
  m_InRenderOptionsMeshSmoothRelaxation->value(
    mops.GetMeshSmoothingRelaxationFactor());
}

void 
AppearanceDialogUILogic
::FillSliceLayoutOptions() 
{
  // Hack
  if(!m_OutDisplayOptionsPanel[0]->value() ||
     strlen(m_OutDisplayOptionsPanel[0]->value()) == 0)
    {
    OnSliceAnatomyOptionsChange(0);
    }
}

void 
AppearanceDialogUILogic
::FillAppearanceSettings()
{
  // Propagate the settings to the controls 
  m_ChkOptionsSliceThumbnailOn->value(
    m_Appearance->GetFlagDisplayZoomThumbnail() ? 1 : 0);
  m_ChkOptionsSliceLinkedZoom->value(
    m_Appearance->GetFlagLinkedZoomByDefault() ? 1 : 0);
  m_InOptionsSliceThumbnailPercent->value(
    m_Appearance->GetZoomThumbnailSizeInPercent());
  m_InOptionsSliceThumbnailMaxSize->value(
    (double) m_Appearance->GetZoomThumbnailMaximumSize());
}
