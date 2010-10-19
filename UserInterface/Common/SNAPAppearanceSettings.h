/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPAppearanceSettings.h,v $
  Language:  C++
  Date:      $Date: 2010/10/19 20:28:56 $
  Version:   $Revision: 1.12 $
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
#ifndef __SNAPAppearanceSettings_h_
#define __SNAPAppearanceSettings_h_

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

// Include the common items from the logic part of SNAP
#include "SNAPCommonUI.h"
#include "Registry.h"
#include <string>

/**
 * \class SNAPAppearanceSettings
 * \brief User interface settings that the user can configure
 */
class SNAPAppearanceSettings
{
public:
  /**
   * A structure that describes the appearance of a screen element
   */
  struct Element
  {
    Vector3d NormalColor;
    Vector3d ActiveColor;
    double LineThickness;
    double DashSpacing;
    int FontSize;
    bool Visible, AlphaBlending;
  };

  /** An enumeration of available screen elements */
  enum UIElements
    {
    CROSSHAIRS = 0, MARKERS, ROI_BOX,
    BACKGROUND_2D, BACKGROUND_3D,
    ZOOM_THUMBNAIL, CROSSHAIRS_3D, CROSSHAIRS_THUMB,
    IMAGE_BOX_3D, ROI_BOX_3D, PAINTBRUSH_OUTLINE, RULER, 
    POLY_DRAW_MAIN, POLY_DRAW_CLOSE, POLY_EDIT,
    ELEMENT_COUNT
    };

  /** An enumeration of the fields that an element may possess */
  enum UIElementFeatures
    {
    NORMAL_COLOR = 0, ACTIVE_COLOR, LINE_THICKNESS, DASH_SPACING,
    FONT_SIZE, VISIBLE, ALPHA_BLEND, FEATURE_COUNT
    };

  /** Enumeration of interpolation modes */
  enum UIGreyInterpolation
    {
    NEAREST = 0, LINEAR
    };

  /** Enumeration of 2D display layouts */
  enum UISliceLayout
    {
    LAYOUT_ASC = 0, LAYOUT_ACS, LAYOUT_SAC, LAYOUT_SCA, LAYOUT_CAS, LAYOUT_CSA, LAYOUT_COUNT
    };

  SNAPAppearanceSettings();
  virtual ~SNAPAppearanceSettings() {}

  void LoadFromRegistry(Registry &registry);
  void SaveToRegistry(Registry &registry);

  // Access a user interface element
  Element &GetUIElement(unsigned int iElement)
    { return m_Elements[iElement]; }

  // Set a user interface element
  void SetUIElement(unsigned int iElement, const Element &value)
    { m_Elements[iElement] = value; }

  // Check whether the feature is applicable to an element
  static bool IsFeatureApplicable(unsigned int iElement, unsigned int iFeature)
    { return m_Applicable[iElement][iFeature] != 0; }

  // Apply the GL settings associated with an appearance element
  static void ApplyUIElementLineSettings(const Element &elt,
    bool applyThickness = true, bool applyStipple = true);

  irisGetMacro(FlagDisplayZoomThumbnail, bool);
  irisSetMacro(FlagDisplayZoomThumbnail, bool);

  irisGetMacro(FlagLinkedZoomByDefault, bool);
  irisSetMacro(FlagLinkedZoomByDefault, bool);

  irisGetMacro(FlagMultisessionZoomByDefault, bool);
  irisSetMacro(FlagMultisessionZoomByDefault, bool);

  irisGetMacro(FlagMultisessionPanByDefault, bool);
  irisSetMacro(FlagMultisessionPanByDefault, bool);

  irisGetMacro(FlagFloatingPointWarningByDefault, bool);
  irisSetMacro(FlagFloatingPointWarningByDefault, bool);

  irisGetMacro(FlagEnableAutoCheckForUpdateByDefault, int);
  irisSetMacro(FlagEnableAutoCheckForUpdateByDefault, int);

  irisGetMacro(FlagEnableHiddenFeaturesByDefault, bool);
  irisSetMacro(FlagEnableHiddenFeaturesByDefault, bool);

  irisGetMacro(ZoomThumbnailSizeInPercent, double);
  irisSetMacro(ZoomThumbnailSizeInPercent, double);

  irisGetMacro(ZoomThumbnailMaximumSize, int);
  irisSetMacro(ZoomThumbnailMaximumSize, int);

  irisGetMacro(GreyInterpolationMode, UIGreyInterpolation);
  irisSetMacro(GreyInterpolationMode, UIGreyInterpolation);

  irisGetMacro(FlagLayoutPatientAnteriorShownLeft, bool);
  irisSetMacro(FlagLayoutPatientAnteriorShownLeft, bool);

  irisGetMacro(FlagLayoutPatientRightShownLeft, bool);
  irisSetMacro(FlagLayoutPatientRightShownLeft, bool);

  irisGetMacro(FlagAutoPan, bool);
  irisSetMacro(FlagAutoPan, bool);

  irisGetMacro(SliceLayout, UISliceLayout);
  irisSetMacro(SliceLayout, UISliceLayout);

  irisGetMacro(OverallVisibility, bool);
  irisSetMacro(OverallVisibility, bool);

  /** 
   * This method uses SliceLayout, FlagLayoutPatientAnteriorShownLeft and
   * FlagLayoutPatientRightShownLeft to generate RAI codes for the three
   * display views. 
   * Use in conjunction with IRISApplication::SetDisplayToAnatomyRAI
   */
  void GetAnatomyToDisplayTransforms(std::string &rai1, std::string &rai2, std::string &rai3);

private:
  // Global settings
  bool m_FlagDisplayZoomThumbnail;
  bool m_FlagLinkedZoomByDefault;
  bool m_FlagMultisessionZoomByDefault;
  bool m_FlagMultisessionPanByDefault;
  bool m_FlagFloatingPointWarningByDefault;
  bool m_FlagEnableHiddenFeaturesByDefault;
  bool m_FlagAutoPan;
  int m_FlagEnableAutoCheckForUpdateByDefault;
  double m_ZoomThumbnailSizeInPercent;
  int m_ZoomThumbnailMaximumSize;
  bool m_OverallVisibility;

  // Interpolation used for rendering slices (for now, linear or n-nbr)
  UIGreyInterpolation m_GreyInterpolationMode;

  /** This is needed to read enum of interpolation modes from registry */
  RegistryEnumMap<UIGreyInterpolation> m_EnumMapInterpolationMode;

  // Current 2D view layout
  UISliceLayout m_SliceLayout;

  // View layout additional flags
  bool m_FlagLayoutPatientAnteriorShownLeft;
  bool m_FlagLayoutPatientRightShownLeft;

  // This is needed to read 2D view layout enums
  RegistryEnumMap<UISliceLayout> m_EnumMapSliceLayout;

  /** An array of user interface elements */
  Element m_Elements[ELEMENT_COUNT];
    
  /** A list of flags that indicate for each element, whether each feature is 
   * applicable or not */
  static const int m_Applicable[ELEMENT_COUNT][FEATURE_COUNT];

  /** Names of the appearance elements */
  static const char *m_ElementNames[];

  /** The set of default values for each element */
  static Element m_DefaultElementSettings[ELEMENT_COUNT];

  /** Initialize the default settings */
  static void InitializeDefaultSettings();
};


#endif // __SNAPAppearanceSettings_h_
