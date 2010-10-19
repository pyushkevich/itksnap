/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPAppearanceSettings.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/19 20:28:56 $
  Version:   $Revision: 1.13 $
  Copyright (c) 2007 Paul A. Yushkevich
  
  This file is part of ITK-SNAP 

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "SNAPAppearanceSettings.h"
#include "Registry.h"
#include "FL/gl.h"

using namespace std;

// Columns: NORMAL_COLOR, ACTIVE_COLOR, LINE_THICKNESS, DASH_SPACING, 
//          FONT_SIZE,    VISIBLE,      ALPHA_BLEND,    FEATURE_COUNT
const int 
SNAPAppearanceSettings
::m_Applicable[SNAPAppearanceSettings::ELEMENT_COUNT][SNAPAppearanceSettings::FEATURE_COUNT] = {
    { 1, 0, 1, 1, 0, 1, 1 },    // Crosshairs
    { 1, 0, 0, 0, 1, 1, 1 },    // Markers
    { 1, 1, 1, 1, 0, 0, 1 },    // ROI
    { 1, 0, 0, 0, 0, 0, 0 },    // Slice Background
    { 1, 0, 0, 0, 0, 0, 0 },    // 3D Background
    { 1, 1, 1, 1, 0, 1, 1 },    // Zoom thumbnail
    { 1, 0, 1, 1, 0, 1, 1 },    // 3D Crosshairs
    { 1, 0, 1, 1, 0, 1, 1 },    // Thumbnail Crosshairs
    { 1, 1, 1, 1, 0, 1, 1 },    // 3D Image Box
    { 1, 1, 1, 1, 0, 1, 1 },    // 3D ROI Box
    { 1, 1, 1, 1, 0, 1, 1 },    // Paintbrush outline
    { 1, 0, 1, 0, 1, 1, 1 },    // Rulers
    { 1, 0, 1, 1, 0, 0, 1 },    // POLY_DRAW_MAIN
    { 1, 0, 1, 1, 0, 1, 1 },    // POLY_DRAW_CLOSE
    { 1, 1, 1, 1, 0, 0, 1 }     // POLY_EDIT
    };

SNAPAppearanceSettings::Element 
SNAPAppearanceSettings
::m_DefaultElementSettings[SNAPAppearanceSettings::ELEMENT_COUNT];

void 
SNAPAppearanceSettings
::InitializeDefaultSettings()
{
  // An element pointer for setting properties
  Element *elt;
  
  // Crosshairs
  elt = &m_DefaultElementSettings[CROSSHAIRS];
  elt->NormalColor = Vector3d(0.3, 0.3, 1.0);
  elt->ActiveColor = Vector3d(0.0, 0.0, 0.0);
  elt->LineThickness = 1.0;
  elt->DashSpacing = 1.0;
  elt->FontSize = 0;
  elt->Visible = true;
  elt->AlphaBlending = false;

  // Markers
  elt = &m_DefaultElementSettings[MARKERS];
  elt->NormalColor = Vector3d(1.0, 0.75, 0.0);
  elt->ActiveColor = Vector3d(0.0, 0.0, 0.0);
  elt->LineThickness = 0.0;
  elt->DashSpacing = 0.0;
  elt->FontSize = 16;
  elt->Visible = true;
  elt->AlphaBlending = false;

  // ROI
  elt = &m_DefaultElementSettings[ROI_BOX];
  elt->NormalColor = Vector3d(1.0, 0.0, 0.2);
  elt->ActiveColor = Vector3d(1.0, 1.0, 0.2);
  elt->LineThickness = 1.0;
  elt->DashSpacing = 3.0;
  elt->FontSize = 0;
  elt->Visible = true;
  elt->AlphaBlending = false;

  // Slice background
  elt = &m_DefaultElementSettings[BACKGROUND_3D];
  elt->NormalColor = Vector3d(0.0, 0.0, 0.0);
  elt->ActiveColor = Vector3d(0.0, 0.0, 0.0);
  elt->LineThickness = 0.0;
  elt->DashSpacing = 0.0;
  elt->FontSize = 0;
  elt->Visible = true;
  elt->AlphaBlending = false;

  // 3D Window background
  elt = &m_DefaultElementSettings[BACKGROUND_3D];
  elt->NormalColor = Vector3d(0.0, 0.0, 0.0);
  elt->ActiveColor = Vector3d(0.0, 0.0, 0.0);
  elt->LineThickness = 0.0;
  elt->DashSpacing = 0.0;
  elt->FontSize = 0;
  elt->Visible = true;
  elt->AlphaBlending = false;

  // Zoom thumbail
  elt = &m_DefaultElementSettings[ZOOM_THUMBNAIL];
  elt->NormalColor = Vector3d(1.0, 1.0, 0.0);
  elt->ActiveColor = Vector3d(1.0, 1.0, 1.0);
  elt->LineThickness = 1.0;
  elt->DashSpacing = 0.0;
  elt->FontSize = 0;
  elt->Visible = true;
  elt->AlphaBlending = false;

  // 3D crosshairs
  elt = &m_DefaultElementSettings[CROSSHAIRS_3D];
  elt->NormalColor = Vector3d(0.3, 0.3, 1.0);
  elt->ActiveColor = Vector3d(0.0, 0.0, 0.0);
  elt->LineThickness = 1.0;
  elt->DashSpacing = 1.0;
  elt->FontSize = 0;
  elt->Visible = true;
  elt->AlphaBlending = true;

  // Thumbnail crosshairs
  elt = &m_DefaultElementSettings[CROSSHAIRS_THUMB];
  elt->NormalColor = Vector3d(0.3, 0.3, 1.0);
  elt->ActiveColor = Vector3d(0.0, 0.0, 0.0);
  elt->LineThickness = 1.0;
  elt->DashSpacing = 1.0;
  elt->FontSize = 0;
  elt->Visible = true;
  elt->AlphaBlending = false;

  // Thumbnail crosshairs
  elt = &m_DefaultElementSettings[IMAGE_BOX_3D];
  elt->NormalColor = Vector3d(0.2, 0.2, 0.2);
  elt->ActiveColor = Vector3d(0.4, 0.4, 0.4);
  elt->LineThickness = 1.0;
  elt->DashSpacing = 1.0;
  elt->FontSize = 0;
  elt->Visible = true;
  elt->AlphaBlending = false;

  // Thumbnail crosshairs
  elt = &m_DefaultElementSettings[ROI_BOX_3D];
  elt->NormalColor = Vector3d(1.0, 0.0, 0.2);
  elt->ActiveColor = Vector3d(1.0, 1.0, 0.2);
  elt->LineThickness = 1.0;
  elt->DashSpacing = 3.0;
  elt->FontSize = 0;
  elt->Visible = true;
  elt->AlphaBlending = false;
   
  // Paintbrush outline
  elt = &m_DefaultElementSettings[PAINTBRUSH_OUTLINE];
  elt->NormalColor = Vector3d(1.0, 0.0, 0.2);
  elt->ActiveColor = Vector3d(1.0, 1.0, 0.2);
  elt->LineThickness = 1.0;
  elt->DashSpacing = 1.0;
  elt->FontSize = 0;
  elt->Visible = true;
  elt->AlphaBlending = false;

  // Markers
  elt = &m_DefaultElementSettings[RULER];
  elt->NormalColor = Vector3d(0.3, 1.0, 0.0);
  elt->ActiveColor = Vector3d(0.0, 0.0, 0.0);
  elt->LineThickness = 1.0;
  elt->DashSpacing = 0.0;
  elt->FontSize = 12;
  elt->Visible = true;
  elt->AlphaBlending = true;

  // Polygon outline (drawing)
  elt = &m_DefaultElementSettings[POLY_DRAW_MAIN];
  elt->NormalColor = Vector3d(1.0, 0.0, 0.5);
  elt->LineThickness = 2.0;
  elt->Visible = true;
  elt->AlphaBlending = true;

  // Polygon outline (drawing)
  elt = &m_DefaultElementSettings[POLY_DRAW_CLOSE];
  elt->NormalColor = Vector3d(1.0, 0.0, 0.5);
  elt->LineThickness = 2.0;
  elt->Visible = true;
  elt->DashSpacing = 1.0;
  elt->AlphaBlending = true;

  // Polygon outline (editing)
  elt = &m_DefaultElementSettings[POLY_EDIT];
  elt->NormalColor = Vector3d(1.0, 0.0, 0.0);
  elt->ActiveColor = Vector3d(0.0, 1.0, 0.0);
  elt->LineThickness = 2.0;
  elt->Visible = true;
  elt->AlphaBlending = true;
}

const char *
SNAPAppearanceSettings
::m_ElementNames[SNAPAppearanceSettings::ELEMENT_COUNT] = 
  { "CROSSHAIRS", "MARKERS", "ROI_BOX", "BACKGROUND_2D", "BACKGROUND_3D", 
    "ZOOM_THUMBNAIL", "CROSSHAIRS_3D", "CROSSHAIRS_THUMB", "IMAGE_BOX_3D",
    "ROI_BOX_3D", "RULER", "PAINTBRUSH_OUTLINE", 
    "POLY_DRAW_MAIN", "POLY_DRAW_CLOSE", "POLY_EDIT"};

SNAPAppearanceSettings
::SNAPAppearanceSettings()
{
  // Initialize the default settings
  InitializeDefaultSettings();

  // Set the common flags
  m_FlagLinkedZoomByDefault = true;
  m_FlagMultisessionZoomByDefault = true;
  m_FlagMultisessionPanByDefault = true;
  m_FlagFloatingPointWarningByDefault = true;
  m_FlagEnableHiddenFeaturesByDefault = false;
  m_FlagEnableAutoCheckForUpdateByDefault = -1;
  m_ZoomThumbnailMaximumSize = 160;
  m_ZoomThumbnailSizeInPercent = 30.0;
  m_FlagDisplayZoomThumbnail = true;
  m_GreyInterpolationMode = NEAREST;

  m_SliceLayout = LAYOUT_ASC;
  m_FlagLayoutPatientAnteriorShownLeft = true;
  m_FlagLayoutPatientRightShownLeft = true;
  m_FlagAutoPan = false;

  m_EnumMapInterpolationMode.AddPair(NEAREST,"NearestNeighbor");
  m_EnumMapInterpolationMode.AddPair(LINEAR,"Linear");

  m_EnumMapSliceLayout.AddPair(LAYOUT_ASC,"ASC");
  m_EnumMapSliceLayout.AddPair(LAYOUT_ACS,"ACS");
  m_EnumMapSliceLayout.AddPair(LAYOUT_SAC,"SAC");
  m_EnumMapSliceLayout.AddPair(LAYOUT_SCA,"SCA");
  m_EnumMapSliceLayout.AddPair(LAYOUT_CAS,"CAS");
  m_EnumMapSliceLayout.AddPair(LAYOUT_CSA,"CSA");
  
  // Set the UI elements to their default values  
  for(unsigned int iElement = 0; iElement < ELEMENT_COUNT; iElement++)
    m_Elements[iElement] = m_DefaultElementSettings[iElement];

  // Initial visibility is true
  m_OverallVisibility = true;

}

void
SNAPAppearanceSettings
::LoadFromRegistry(Registry &r)
{
  // Load the flags and settings
  m_FlagDisplayZoomThumbnail =
    r["FlagDisplayZoomThumbnail"][m_FlagDisplayZoomThumbnail];

  m_FlagLinkedZoomByDefault = 
    r["FlagLinkedZoomByDefault"][m_FlagLinkedZoomByDefault];

  m_FlagMultisessionZoomByDefault = 
    r["FlagMultisessionZoomByDefault"][m_FlagMultisessionZoomByDefault];

  m_FlagMultisessionPanByDefault = 
    r["FlagMultisessionPanByDefault"][m_FlagMultisessionPanByDefault];

  m_FlagFloatingPointWarningByDefault = 
    r["FlagFloatingPointWarningByDefault"][m_FlagFloatingPointWarningByDefault];

  m_FlagEnableHiddenFeaturesByDefault = 
    r["FlagEnableHiddenFeaturesByDefault"][m_FlagEnableHiddenFeaturesByDefault];

  m_FlagEnableAutoCheckForUpdateByDefault =
    r["FlagEnableAutoCheckForUpdateByDefault"][m_FlagEnableAutoCheckForUpdateByDefault];

  m_FlagAutoPan =
    r["FlagAutoPan"][m_FlagAutoPan];

  m_ZoomThumbnailSizeInPercent = 
    r["ZoomThumbnailSizeInPercent"][m_ZoomThumbnailSizeInPercent];

  m_ZoomThumbnailMaximumSize = 
    r["ZoomThumbnailMaximumSize"][m_ZoomThumbnailMaximumSize];

  m_GreyInterpolationMode = 
    r["GreyImageInterpolationMode"].GetEnum(m_EnumMapInterpolationMode, NEAREST);

  // Overall visibility is not saved or loaded

  // Read slice layout information
  m_SliceLayout = 
    r["SliceLayout"].GetEnum(m_EnumMapSliceLayout, LAYOUT_ASC);
  m_FlagLayoutPatientAnteriorShownLeft = r["PatientAnteriorShownLeft"][true];
  m_FlagLayoutPatientRightShownLeft = r["PatientRightShownLeft"][true];

  // Load the user interface elements
  for(unsigned int iElement = 0; iElement < ELEMENT_COUNT; iElement++)
    {
    // Create a folder to hold the element
    Registry& f = r.Folder( 
      r.Key("UserInterfaceElement[%s]", m_ElementNames[iElement]) );

    // Get the default element
    const Element &def = m_DefaultElementSettings[iElement];
    Element &elt = m_Elements[iElement];
    
    // Store the element in the folder
    elt.NormalColor = f["NormalColor"][def.NormalColor];
    elt.ActiveColor = f["ActiveColor"][def.ActiveColor];
    elt.LineThickness = f["LineThickness"][def.LineThickness];
    elt.DashSpacing = f["DashSpacing"][def.DashSpacing];
    elt.FontSize = f["FontSize"][def.FontSize];
    elt.AlphaBlending = f["AlphaBlending"][def.AlphaBlending];
    elt.Visible = f["Visible"][def.Visible];
    }
}

void
SNAPAppearanceSettings
::SaveToRegistry(Registry &r)
{
  // Save the flags and settings
  r["FlagDisplayZoomThumbnail"] << m_FlagDisplayZoomThumbnail;
  r["FlagLinkedZoomByDefault"] << m_FlagLinkedZoomByDefault;
  r["FlagMultisessionZoomByDefault"] << m_FlagMultisessionZoomByDefault;
  r["FlagMultisessionPanByDefault"] << m_FlagMultisessionPanByDefault;
  r["FlagFloatingPointWarningByDefault"] << m_FlagFloatingPointWarningByDefault;
  r["FlagEnableHiddenFeaturesByDefault"] << m_FlagEnableHiddenFeaturesByDefault;
  r["FlagEnableAutoCheckForUpdateByDefault"] << m_FlagEnableAutoCheckForUpdateByDefault;
  r["FlagAutoPan"] << m_FlagAutoPan;
  r["ZoomThumbnailSizeInPercent"] << m_ZoomThumbnailSizeInPercent;
  r["ZoomThumbnailMaximumSize"] << m_ZoomThumbnailMaximumSize;
  r["GreyImageInterpolationMode"].PutEnum(m_EnumMapInterpolationMode, m_GreyInterpolationMode);

  // Overall visibility is not saved or loaded

  // Write slice layout information
  r["SliceLayout"].PutEnum(m_EnumMapSliceLayout, m_SliceLayout);
  r["PatientAnteriorShownLeft"] << m_FlagLayoutPatientAnteriorShownLeft;
  r["PatientRightShownLeft"] << m_FlagLayoutPatientRightShownLeft;
  


  // Save each of the screen elements
  for(unsigned int iElement = 0; iElement < ELEMENT_COUNT; iElement++)
    {
    // Create a folder to hold the element
    Registry& f = r.Folder( 
      r.Key("UserInterfaceElement[%s]", m_ElementNames[iElement]) );

    // Get the default element
    Element &elt = m_Elements[iElement];
    
    // Store the element in the folder
    f["NormalColor"] << elt.NormalColor;
    f["ActiveColor"] << elt.ActiveColor;
    f["LineThickness"] << elt.LineThickness;
    f["DashSpacing"] << elt.DashSpacing;
    f["FontSize"] << elt.FontSize;
    f["AlphaBlending"] << elt.AlphaBlending;
    f["Visible"] << elt.Visible;
    }
}

void 
SNAPAppearanceSettings
::ApplyUIElementLineSettings(const Element &elt, bool applyThickness, bool applyStipple)
{
  // Apply the thickness properties
  if(applyThickness)
    {
    // Choose whether to use blending or not
    if( elt.AlphaBlending )
      {
      glEnable(GL_BLEND);
      glEnable(GL_LINE_SMOOTH);
      glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      }
    glLineWidth( elt.LineThickness );
    }
  if(applyStipple && elt.DashSpacing != 0)
    {
    // Set the line thickness and stipple
    glEnable(GL_LINE_STIPPLE);
    glLineStipple( static_cast<GLint>(elt.DashSpacing),
                   0x9999 ); // 0011 0011 0011 0011  // 1001 1001 1001 1001
    }
}

void SNAPAppearanceSettings
::GetAnatomyToDisplayTransforms(string &rai1, string &rai2, string &rai3)
{
  unsigned int order[6][3] = 
    {{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};

  // Start with stock orientations
  string axes[3] = {string("RPS"),string("AIL"),string("RIP")};

  // Switch the configurable directions
  if(!m_FlagLayoutPatientRightShownLeft)
    {
    axes[0][0] = axes[2][0] = 'L';
    }
  if(!m_FlagLayoutPatientAnteriorShownLeft)
    {
    axes[1][0] = 'P';
    }

  // Convert layout index to integer
  size_t i = (size_t) m_SliceLayout;

  // Set the axes
  rai1 = axes[order[i][0]];
  rai2 = axes[order[i][1]];
  rai3 = axes[order[i][2]];
}

