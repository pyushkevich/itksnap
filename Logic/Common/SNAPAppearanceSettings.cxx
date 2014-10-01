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

#include <SNAPOpenGL.h>

using namespace std;

// Columns: NORMAL_COLOR, ACTIVE_COLOR, LINE_THICKNESS, DASH_SPACING, 
//          FONT_SIZE,    VISIBLE,      ALPHA_BLEND,    FEATURE_COUNT
const int 
SNAPAppearanceSettings
::m_Applicable[SNAPAppearanceSettings::ELEMENT_COUNT][OpenGLAppearanceElement::FEATURE_COUNT] = {
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

void 
SNAPAppearanceSettings
::InitializeDefaultSettings()
{
  // Initialize all the elements
  for(int i = 0; i < ELEMENT_COUNT; i++)
    {
    m_DefaultElementSettings[i] = OpenGLAppearanceElement::New();
    m_DefaultElementSettings[i]->SetValid(m_Applicable[i]);
    }

  // An element pointer for setting properties
  OpenGLAppearanceElement *elt;
  
  // Crosshairs
  elt = m_DefaultElementSettings[CROSSHAIRS];
  elt->SetNormalColor(Vector3d(0.3, 0.3, 1.0));
  elt->SetActiveColor(Vector3d(0.0, 0.0, 0.0));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(1.0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetAlphaBlending(false);

  // Markers
  elt = m_DefaultElementSettings[MARKERS];
  elt->SetNormalColor(Vector3d(1.0, 0.75, 0.0));
  elt->SetActiveColor(Vector3d(0.0, 0.0, 0.0));
  elt->SetLineThickness(0.0);
  elt->SetDashSpacing(0.0);
  elt->SetFontSize(16);
  elt->SetVisible(true);
  elt->SetAlphaBlending(false);

  // ROI
  elt = m_DefaultElementSettings[ROI_BOX];
  elt->SetNormalColor(Vector3d(1.0, 0.0, 0.2));
  elt->SetActiveColor(Vector3d(1.0, 1.0, 0.2));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(3.0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetAlphaBlending(false);

  // Slice background
  elt = m_DefaultElementSettings[BACKGROUND_3D];
  elt->SetNormalColor(Vector3d(0.0, 0.0, 0.0));
  elt->SetActiveColor(Vector3d(0.0, 0.0, 0.0));
  elt->SetLineThickness(0.0);
  elt->SetDashSpacing(0.0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetAlphaBlending(false);

  // 3D Window background
  elt = m_DefaultElementSettings[BACKGROUND_3D];
  elt->SetNormalColor(Vector3d(0.0, 0.0, 0.0));
  elt->SetActiveColor(Vector3d(0.0, 0.0, 0.0));
  elt->SetLineThickness(0.0);
  elt->SetDashSpacing(0.0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetAlphaBlending(false);

  // Zoom thumbail
  elt = m_DefaultElementSettings[ZOOM_THUMBNAIL];
  elt->SetNormalColor(Vector3d(1.0, 1.0, 0.0));
  elt->SetActiveColor(Vector3d(1.0, 1.0, 1.0));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(0.0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetAlphaBlending(false);

  // 3D crosshairs
  elt = m_DefaultElementSettings[CROSSHAIRS_3D];
  elt->SetNormalColor(Vector3d(0.3, 0.3, 1.0));
  elt->SetActiveColor(Vector3d(0.0, 0.0, 0.0));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(1.0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetAlphaBlending(true);

  // Thumbnail crosshairs
  elt = m_DefaultElementSettings[CROSSHAIRS_THUMB];
  elt->SetNormalColor(Vector3d(0.3, 0.3, 1.0));
  elt->SetActiveColor(Vector3d(0.0, 0.0, 0.0));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(1.0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetAlphaBlending(false);

  // Thumbnail crosshairs
  elt = m_DefaultElementSettings[IMAGE_BOX_3D];
  elt->SetNormalColor(Vector3d(0.2, 0.2, 0.2));
  elt->SetActiveColor(Vector3d(0.4, 0.4, 0.4));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(1.0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetAlphaBlending(false);

  // Thumbnail crosshairs
  elt = m_DefaultElementSettings[ROI_BOX_3D];
  elt->SetNormalColor(Vector3d(1.0, 0.0, 0.2));
  elt->SetActiveColor(Vector3d(1.0, 1.0, 0.2));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(3.0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetAlphaBlending(false);
   
  // Paintbrush outline
  elt = m_DefaultElementSettings[PAINTBRUSH_OUTLINE];
  elt->SetNormalColor(Vector3d(1.0, 0.0, 0.2));
  elt->SetActiveColor(Vector3d(1.0, 1.0, 0.2));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(1.0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetAlphaBlending(false);

  // Markers
  elt = m_DefaultElementSettings[RULER];
  elt->SetNormalColor(Vector3d(0.3, 1.0, 0.0));
  elt->SetActiveColor(Vector3d(0.0, 0.0, 0.0));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(0.0);
  elt->SetFontSize(12);
  elt->SetVisible(true);
  elt->SetAlphaBlending(true);

  // Polygon outline (drawing)
  elt = m_DefaultElementSettings[POLY_DRAW_MAIN];
  elt->SetNormalColor(Vector3d(1.0, 0.0, 0.5));
  elt->SetActiveColor(Vector3d(1.0, 0.8, 0.9));
  elt->SetLineThickness(2.0);
  elt->SetVisible(true);
  elt->SetAlphaBlending(true);

  // Polygon outline (drawing)
  elt = m_DefaultElementSettings[POLY_DRAW_CLOSE];
  elt->SetNormalColor(Vector3d(1.0, 0.0, 0.5));
  elt->SetActiveColor(Vector3d(1.0, 0.8, 0.9));
  elt->SetLineThickness(2.0);
  elt->SetVisible(false);
  elt->SetDashSpacing(1.0);
  elt->SetAlphaBlending(true);

  // Polygon outline (editing)
  elt = m_DefaultElementSettings[POLY_EDIT];
  elt->SetNormalColor(Vector3d(1.0, 0.0, 0.0));
  elt->SetActiveColor(Vector3d(0.0, 1.0, 0.0));
  elt->SetLineThickness(2.0);
  elt->SetVisible(true);
  elt->SetAlphaBlending(true);
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
  // Initialize the default settings the first time this method is called
  InitializeDefaultSettings();

  // Set the UI elements to their default values  
  for(unsigned int iElement = 0; iElement < ELEMENT_COUNT; iElement++)
    {
    // Create the elements
    m_Elements[iElement] = OpenGLAppearanceElement::New();
    m_Elements[iElement]->DeepCopy(m_DefaultElementSettings[iElement]);

    // Rebroadcast modification events from the elements
    Rebroadcast(m_Elements[iElement], ChildPropertyChangedEvent(), ChildPropertyChangedEvent());
    }

  // Initial visibility is true
  m_OverallVisibilityModel = NewSimpleConcreteProperty(true);
}

void
SNAPAppearanceSettings
::LoadFromRegistry(Registry &r)
{

  // Overall visibility is not saved or loaded (it's a temprary setting)

  // Load the user interface elements
  for(unsigned int iElement = 0; iElement < ELEMENT_COUNT; iElement++)
    {
    // Create a folder to hold the element
    Registry& f = r.Folder( 
      r.Key("UserInterfaceElement[%s]", m_ElementNames[iElement]) );

    m_Elements[iElement]->ReadFromRegistry(f);
    }
}

void
SNAPAppearanceSettings
::SaveToRegistry(Registry &r)
{
  // Save each of the screen elements
  for(unsigned int iElement = 0; iElement < ELEMENT_COUNT; iElement++)
    {
    // Create a folder to hold the element
    Registry& f = r.Folder( 
      r.Key("UserInterfaceElement[%s]", m_ElementNames[iElement]) );

    // Get the default element
    m_Elements[iElement]->WriteToRegistry(f);
    }
}

GlobalDisplaySettings::GlobalDisplaySettings()
{
  // This is needed to read enum of interpolation modes from registry
  RegistryEnumMap<UIGreyInterpolation> emap_interp;
  emap_interp.AddPair(NEAREST,"NearestNeighbor");
  emap_interp.AddPair(LINEAR,"Linear");

  // This is needed to read 2D view layout enums
  RegistryEnumMap<UISliceLayout> emap_layout;
  emap_layout.AddPair(LAYOUT_ASC,"ASC");
  emap_layout.AddPair(LAYOUT_ACS,"ACS");
  emap_layout.AddPair(LAYOUT_SAC,"SAC");
  emap_layout.AddPair(LAYOUT_SCA,"SCA");
  emap_layout.AddPair(LAYOUT_CAS,"CAS");
  emap_layout.AddPair(LAYOUT_CSA,"CSA");

  // Set the common flags
  m_FlagDisplayZoomThumbnailModel =
      NewSimpleProperty("FlagDisplayZoomThumbnail", true);

  m_ZoomThumbnailMaximumSizeModel =
      NewRangedProperty("ZoomThumbnailMaximumSize", 160, 40, 400, 10);

  m_ZoomThumbnailSizeInPercentModel =
      NewRangedProperty("ZoomThumbnailSizeInPercent", 30.0, 5.0, 50.0, 1.0);

  m_GreyInterpolationModeModel =
      NewSimpleEnumProperty("GreyInterpolationMode", NEAREST, emap_interp);

  m_SliceLayoutModel =
      NewSimpleEnumProperty("SliceLayout", LAYOUT_ASC, emap_layout);

  m_FlagLayoutPatientAnteriorShownLeftModel =
      NewSimpleProperty("FlagLayoutPatientAnteriorShownLeft", true);

  m_FlagLayoutPatientRightShownLeftModel =
      NewSimpleProperty("FlagLayoutPatientRightShownLeft", true);

}

void GlobalDisplaySettings
::GetAnatomyToDisplayTransforms(string &rai1, string &rai2, string &rai3) const
{
  unsigned int order[6][3] =
    {{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};

  // Start with stock orientations
  string axes[3] = {string("RPS"),string("AIL"),string("RIP")};

  // Switch the configurable directions
  if(!GetFlagLayoutPatientRightShownLeft())
    {
    axes[0][0] = axes[2][0] = 'L';
    }
  if(!GetFlagLayoutPatientAnteriorShownLeft())
    {
    axes[1][0] = 'P';
    }

  // Convert layout index to integer
  size_t i = (size_t) GetSliceLayout();

  // Set the axes
  rai1 = axes[order[i][0]];
  rai2 = axes[order[i][1]];
  rai3 = axes[order[i][2]];
}


void OpenGLAppearanceElement::SetValid(const int validity[])
{
  m_NormalColorModel->SetIsValid(validity[NORMAL_COLOR]);
  m_ActiveColorModel->SetIsValid(validity[ACTIVE_COLOR]);
  m_LineThicknessModel->SetIsValid(validity[LINE_THICKNESS]);
  m_DashSpacingModel->SetIsValid(validity[DASH_SPACING]);
  m_FontSizeModel->SetIsValid(validity[FONT_SIZE]);
  m_VisibleModel->SetIsValid(validity[VISIBLE]);
  m_AlphaBlendingModel->SetIsValid(validity[ALPHA_BLEND]);
}

void OpenGLAppearanceElement
::ApplyLineSettings(bool applyThickness, bool applyStipple) const
{
  // Apply the thickness properties
  if(applyThickness)
    {
    // Choose whether to use blending or not
    if(GetAlphaBlending())
      {
      glEnable(GL_BLEND);
      glEnable(GL_LINE_SMOOTH);
      glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      }
    glLineWidth(GetLineThickness());
    }
  if(applyStipple && GetDashSpacing() != 0)
    {
    // Set the line thickness and stipple
    glEnable(GL_LINE_STIPPLE);
    glLineStipple( static_cast<GLint>(GetDashSpacing()),
                   0x9999 ); // 0011 0011 0011 0011  // 1001 1001 1001 1001
    }
}

OpenGLAppearanceElement::OpenGLAppearanceElement()
{
  m_NormalColorModel =
      NewRangedProperty("NormalColor",
                        Vector3d(0.0), Vector3d(0.0), Vector3d(1.0), Vector3d(0.01));

  m_ActiveColorModel =
      NewRangedProperty("ActiveColor",
                        Vector3d(0.0), Vector3d(0.0), Vector3d(1.0), Vector3d(0.01));

  m_LineThicknessModel =
      NewRangedProperty("LineThickness", 0.0, 0.0, 5.0, 0.1);

  m_DashSpacingModel =
      NewRangedProperty("DashSpacing", 0.0, 0.0, 5.0, 0.1);

  m_FontSizeModel =
      NewRangedProperty("FontSize", 0, 0, 36, 1);

  m_VisibleModel = NewSimpleProperty("Visible", false);
  m_AlphaBlendingModel = NewSimpleProperty("AlphaBlending", false);
}


