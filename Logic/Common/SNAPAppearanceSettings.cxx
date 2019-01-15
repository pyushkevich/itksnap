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

// Columns: NORMAL_COLOR, LINE_THICKNESS, DASH_SPACING,
//          FONT_SIZE,    VISIBLE,      ALPHA_BLEND,    FEATURE_COUNT
const int 
SNAPAppearanceSettings
::m_Applicable[SNAPAppearanceSettings::ELEMENT_COUNT][OpenGLAppearanceElement::FEATURE_COUNT] = {
    { 1, 1, 1, 0, 1, 1 },    // Crosshairs
    { 1, 0, 0, 1, 1, 1 },    // Markers
    { 1, 1, 1, 0, 0, 1 },    // ROI
    { 1, 1, 1, 0, 0, 1 },    // ROI_BOX_ACTIVE
    { 1, 0, 0, 0, 0, 0 },    // Slice Background
    { 1, 0, 0, 0, 0, 0 },    // 3D Background
    { 1, 1, 1, 0, 1, 1 },    // Zoom thumbnail
    { 1, 1, 1, 0, 1, 1 },    // Zoom viewport
    { 1, 1, 1, 0, 1, 1 },    // 3D Crosshairs
    { 1, 1, 1, 0, 1, 1 },    // Thumbnail Crosshairs
    { 1, 1, 1, 0, 1, 1 },    // 3D Image Box
    { 1, 1, 1, 0, 1, 1 },    // 3D ROI Box
    { 1, 1, 1, 0, 1, 1 },    // Paintbrush outline
    { 1, 1, 0, 1, 1, 1 },    // Rulers
    { 1, 1, 1, 0, 0, 1 },    // POLY_DRAW_MAIN
    { 1, 1, 1, 0, 1, 1 },    // POLY_DRAW_CLOSE
    { 1, 1, 1, 0, 0, 1 },    // POLY_EDIT
    { 1, 1, 1, 0, 0, 1 },    // POLY_EDIT_SELECT
    { 1, 1, 1, 0, 0, 1 },    // REGISTRATION_WIDGETS
    { 1, 1, 1, 0, 0, 1 },    // REGISTRATION_WIDGETS_ACTIVE
    { 1, 1, 1, 0, 0, 1 },    // REGISTRATION_GRID
    { 1, 1, 0, 0, 0, 1 }     // GRID_LINES
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
  elt->SetColor(Vector3d(0.3, 0.3, 1.0));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(1);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetSmooth(false);

  // Markers
  elt = m_DefaultElementSettings[MARKERS];
  elt->SetColor(Vector3d(1.0, 0.75, 0.0));
  elt->SetLineThickness(0.0);
  elt->SetDashSpacing(0);
  elt->SetFontSize(16);
  elt->SetVisible(true);
  elt->SetSmooth(false);

  // ROI
  elt = m_DefaultElementSettings[ROI_BOX];
  elt->SetColor(Vector3d(1.0, 0.0, 0.2));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(3);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetSmooth(false);

  // ROI (active)
  elt = m_DefaultElementSettings[ROI_BOX_ACTIVE];
  elt->SetColor(Vector3d(1.0, 1.0, 0.2));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(3);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetSmooth(false);

  // Slice background
  elt = m_DefaultElementSettings[BACKGROUND_3D];
  elt->SetColor(Vector3d(0.0, 0.0, 0.0));
  elt->SetLineThickness(0.0);
  elt->SetDashSpacing(0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetSmooth(false);

  // 3D Window background
  elt = m_DefaultElementSettings[BACKGROUND_3D];
  elt->SetColor(Vector3d(0.0, 0.0, 0.0));
  elt->SetLineThickness(0.0);
  elt->SetDashSpacing(0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetSmooth(false);

  // Zoom thumbail
  elt = m_DefaultElementSettings[ZOOM_THUMBNAIL];
  elt->SetColor(Vector3d(1.0, 1.0, 0.0));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetSmooth(false);

  // Zoom viewport marker
  elt = m_DefaultElementSettings[ZOOM_VIEWPORT];
  elt->SetColor(Vector3d(1.0, 1.0, 1.0));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetSmooth(false);

  // 3D crosshairs
  elt = m_DefaultElementSettings[CROSSHAIRS_3D];
  elt->SetColor(Vector3d(0.3, 0.3, 1.0));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(1);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetSmooth(true);

  // Thumbnail crosshairs
  elt = m_DefaultElementSettings[CROSSHAIRS_THUMB];
  elt->SetColor(Vector3d(0.3, 0.3, 1.0));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(1);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetSmooth(false);

  // Thumbnail crosshairs
  elt = m_DefaultElementSettings[IMAGE_BOX_3D];
  elt->SetColor(Vector3d(0.2, 0.2, 0.2));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(1);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetSmooth(false);

  // Thumbnail crosshairs
  elt = m_DefaultElementSettings[ROI_BOX_3D];
  elt->SetColor(Vector3d(1.0, 0.0, 0.2));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(3);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetSmooth(false);
   
  // Paintbrush outline
  elt = m_DefaultElementSettings[PAINTBRUSH_OUTLINE];
  elt->SetColor(Vector3d(1.0, 0.0, 0.2));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(1);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetSmooth(false);

  // Markers
  elt = m_DefaultElementSettings[RULER];
  elt->SetColor(Vector3d(0.3, 1.0, 0.0));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(0);
  elt->SetFontSize(12);
  elt->SetVisible(true);
  elt->SetSmooth(true);

  // Polygon outline (drawing)
  elt = m_DefaultElementSettings[POLY_DRAW_MAIN];
  elt->SetColor(Vector3d(1.0, 0.0, 0.5));
  elt->SetLineThickness(2.0);
  elt->SetVisible(true);
  elt->SetSmooth(true);

  // Polygon outline (completion)
  elt = m_DefaultElementSettings[POLY_DRAW_CLOSE];
  elt->SetColor(Vector3d(1.0, 0.0, 0.5));
  elt->SetLineThickness(2.0);
  elt->SetVisible(false);
  elt->SetDashSpacing(1);
  elt->SetSmooth(true);

  // Polygon outline (editing)
  elt = m_DefaultElementSettings[POLY_EDIT];
  elt->SetColor(Vector3d(1.0, 0.0, 0.0));
  elt->SetLineThickness(2.0);
  elt->SetVisible(true);
  elt->SetSmooth(true);

  // Polygon outline (editing, selected)
  elt = m_DefaultElementSettings[POLY_EDIT_SELECT];
  elt->SetColor(Vector3d(0.0, 1.0, 0.0));
  elt->SetLineThickness(2.0);
  elt->SetVisible(true);
  elt->SetSmooth(true);

  // Registration widget
  elt = m_DefaultElementSettings[REGISTRATION_WIDGETS];
  elt->SetColor(Vector3d(1.0, 1.0, 1.0));
  elt->SetAlpha(0.25);
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetSmooth(true);

  // Registration widget
  elt = m_DefaultElementSettings[REGISTRATION_WIDGETS_ACTIVE];
  elt->SetColor(Vector3d(1.0, 1.0, 0.4));
  elt->SetAlpha(1.0);
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetSmooth(true);

  // Registration widget
  elt = m_DefaultElementSettings[REGISTRATION_GRID];
  elt->SetColor(Vector3d(0.8, 0.8, 0.8));
  elt->SetAlpha(0.25);
  elt->SetLineThickness(0.5);
  elt->SetDashSpacing(0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetSmooth(true);

  // Warp grid lines
  elt = m_DefaultElementSettings[GRID_LINES];
  elt->SetColor(Vector3d(1.0, 1.0, 0.0));
  elt->SetLineThickness(1.0);
  elt->SetDashSpacing(0);
  elt->SetFontSize(0);
  elt->SetVisible(true);
  elt->SetSmooth(true);
}

const char *
SNAPAppearanceSettings
::m_ElementNames[SNAPAppearanceSettings::ELEMENT_COUNT] = 
  { "CROSSHAIRS", "MARKERS", "ROI_BOX", "ROI_BOX_ACTIVE", "BACKGROUND_2D", "BACKGROUND_3D",
    "ZOOM_THUMBNAIL", "ZOOM_VIEWPORT", "CROSSHAIRS_3D", "CROSSHAIRS_THUMB", "IMAGE_BOX_3D",
    "ROI_BOX_3D", "RULER", "PAINTBRUSH_OUTLINE", 
    "POLY_DRAW_MAIN", "POLY_DRAW_CLOSE", "POLY_EDIT", "POLY_EDIT_SELECT",
    "REGISTRATION_WIDGETS", "REGISTRATION_WIDGETS_ACTIVE", "REGISTRATION_GRID", "GRID_LINES"};

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

  // This is needed to read display layout modes
  RegistryEnumMap<LayerLayout> emap_layer_layout;
  emap_layer_layout.AddPair(LAYOUT_STACKED, "Stacked");
  emap_layer_layout.AddPair(LAYOUT_TILED, "Tiled");

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

  m_LayerLayoutModel =
      NewSimpleEnumProperty("LayerLayout", LAYOUT_STACKED, emap_layer_layout);
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
  m_ColorModel->SetIsValid(validity[COLOR]);
  m_AlphaModel->SetIsValid(validity[COLOR]);
  m_LineThicknessModel->SetIsValid(validity[LINE_THICKNESS]);
  m_DashSpacingModel->SetIsValid(validity[DASH_SPACING]);
  m_FontSizeModel->SetIsValid(validity[FONT_SIZE]);
  m_VisibleModel->SetIsValid(validity[VISIBLE]);
  m_SmoothModel->SetIsValid(validity[SMOOTH]);
}

void OpenGLAppearanceElement::ApplyColor() const
{
  glColor4d(this->GetColor()[0], this->GetColor()[1], this->GetColor()[2], this->GetAlpha());
}

void OpenGLAppearanceElement
::ApplyLineSettings(bool applyThickness, bool applyStipple) const
{
  // Apply the transparency settings if smoothing is on or non-1 alpha
  if(GetSmooth() || GetAlpha() < 1.0)
    {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

  // Apply the thickness properties
  if(applyThickness)
    {
    // Choose whether to use blending or not
    if(GetSmooth())
      {
      glEnable(GL_LINE_SMOOTH);
      glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
      }
    glLineWidth((GLfloat) GetLineThickness());
    }
  if(applyStipple && GetDashSpacing() > 0)
    {
    // Set the line thickness and stipple
    glEnable(GL_LINE_STIPPLE);
    glLineStipple( static_cast<GLint>(GetDashSpacing()),
                   0x9999 ); // 0011 0011 0011 0011  // 1001 1001 1001 1001
    }
}

OpenGLAppearanceElement::OpenGLAppearanceElement()
{
  m_ColorModel =
      NewRangedProperty("NormalColor",
                        Vector3d(0.0), Vector3d(0.0), Vector3d(1.0), Vector3d(0.01));

  m_AlphaModel =
      NewRangedProperty("NormalAlpha", 1.0, 0.0, 1.0, 0.01);

  m_LineThicknessModel =
      NewRangedProperty("LineThickness", 0.0, 0.0, 5.0, 0.1);

  m_DashSpacingModel =
      NewRangedProperty("DashSpacing", 0, 0, 9, 1);

  m_FontSizeModel =
      NewRangedProperty("FontSize", 0, 0, 36, 1);

  m_VisibleModel = NewSimpleProperty("Visible", false);
  m_SmoothModel = NewSimpleProperty("Smooth", false);
}


