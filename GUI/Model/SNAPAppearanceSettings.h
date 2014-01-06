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
#include "SNAPCommon.h"
#include "Registry.h"
#include <string>
#include "AbstractPropertyContainerModel.h"


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


class OpenGLAppearanceElement : public AbstractPropertyContainerModel
{
public:
  irisITKObjectMacro(OpenGLAppearanceElement, AbstractPropertyContainerModel)

  irisRangedPropertyAccessMacro(NormalColor, Vector3d)
  irisRangedPropertyAccessMacro(ActiveColor, Vector3d)

  irisRangedPropertyAccessMacro(LineThickness, double)
  irisRangedPropertyAccessMacro(DashSpacing, double)
  irisRangedPropertyAccessMacro(FontSize, int)

  irisSimplePropertyAccessMacro(Visible, bool)
  irisSimplePropertyAccessMacro(AlphaBlending, bool)

  /** An enumeration of the fields that an element may possess */
  enum UIElementFeatures
    {
    NORMAL_COLOR = 0, ACTIVE_COLOR, LINE_THICKNESS, DASH_SPACING,
    FONT_SIZE, VISIBLE, ALPHA_BLEND, FEATURE_COUNT
    };

  // Set the validity of all the properties at once using an int array
  // indexed by the enum UIElementFeatures
  void SetValid(const int validity[]);

  // Apply the GL line settings in the element
  void ApplyLineSettings(bool applyThickness = true, bool applyStipple = true) const;

protected:

  SmartPtr<ConcreteRangedDoubleVec3Property> m_NormalColorModel, m_ActiveColorModel;
  SmartPtr<ConcreteRangedDoubleProperty> m_LineThicknessModel, m_DashSpacingModel;
  SmartPtr<ConcreteRangedIntProperty> m_FontSizeModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_VisibleModel, m_AlphaBlendingModel;

  OpenGLAppearanceElement();

};


class GlobalDisplaySettings : public AbstractPropertyContainerModel
{
public:
  irisITKObjectMacro(GlobalDisplaySettings, AbstractPropertyContainerModel)

  /** Enumeration of interpolation modes */
  enum UIGreyInterpolation { NEAREST = 0, LINEAR};

  /** Enumeration of 2D display layouts */
  enum UISliceLayout
  {
    LAYOUT_ASC = 0, LAYOUT_ACS, LAYOUT_SAC, LAYOUT_SCA, LAYOUT_CAS, LAYOUT_CSA, LAYOUT_COUNT
  };

  irisSimplePropertyAccessMacro(FlagDisplayZoomThumbnail, bool)
  irisRangedPropertyAccessMacro(ZoomThumbnailSizeInPercent, double)
  irisRangedPropertyAccessMacro(ZoomThumbnailMaximumSize, int)
  irisSimplePropertyAccessMacro(GreyInterpolationMode, UIGreyInterpolation)
  irisSimplePropertyAccessMacro(FlagLayoutPatientAnteriorShownLeft, bool)
  irisSimplePropertyAccessMacro(FlagLayoutPatientRightShownLeft, bool)
  irisSimplePropertyAccessMacro(SliceLayout, UISliceLayout)

  /**
   * This method uses SliceLayout, FlagLayoutPatientAnteriorShownLeft and
   * FlagLayoutPatientRightShownLeft to generate RAI codes for the three
   * display views.
   * Use in conjunction with IRISApplication::SetDisplayToAnatomyRAI
   */
  void GetAnatomyToDisplayTransforms(
      std::string &rai1, std::string &rai2, std::string &rai3) const;

protected:

  // Global settings
  SmartPtr<ConcreteSimpleBooleanProperty> m_FlagDisplayZoomThumbnailModel;
  SmartPtr<ConcreteRangedDoubleProperty> m_ZoomThumbnailSizeInPercentModel;
  SmartPtr<ConcreteRangedIntProperty> m_ZoomThumbnailMaximumSizeModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_FlagLayoutPatientAnteriorShownLeftModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_FlagLayoutPatientRightShownLeftModel;

  typedef ConcretePropertyModel<UIGreyInterpolation, TrivialDomain> ConcreteInterpolationModel;
  SmartPtr<ConcreteInterpolationModel> m_GreyInterpolationModeModel;

  typedef ConcretePropertyModel<UISliceLayout, TrivialDomain> ConcreteSliceLayoutModel;
  SmartPtr<ConcreteSliceLayoutModel> m_SliceLayoutModel;

  GlobalDisplaySettings();
};



/**
 * \class SNAPAppearanceSettings
 * \brief User interface settings that the user can configure
 */
class SNAPAppearanceSettings : public AbstractModel
{
public:

  irisITKObjectMacro(SNAPAppearanceSettings, AbstractModel)

  FIRES(ChildPropertyChangedEvent)

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

  void LoadFromRegistry(Registry &registry);
  void SaveToRegistry(Registry &registry);

  // Access a user interface element
  OpenGLAppearanceElement *GetUIElement(unsigned int iElement)
    { return m_Elements[iElement]; }

  // Access a user interface element
  const OpenGLAppearanceElement *GetUIElementDefaultSettings(unsigned int iElement)
    { return m_DefaultElementSettings[iElement]; }

  irisSimplePropertyAccessMacro(OverallVisibility, bool)

protected:

  SNAPAppearanceSettings();
  virtual ~SNAPAppearanceSettings() {}

private:
  /** Overall visibility - overrides all other flags */
  SmartPtr<ConcreteSimpleBooleanProperty> m_OverallVisibilityModel;

  /** An array of current user interface elements */
  SmartPtr<OpenGLAppearanceElement> m_Elements[ELEMENT_COUNT];
    
  /** The set of default values for each element */
  SmartPtr<OpenGLAppearanceElement> m_DefaultElementSettings[ELEMENT_COUNT];

  /** A list of flags that indicate for each element, whether each feature is
   * applicable or not. This is used to set up the defaults */
  static const int m_Applicable[ELEMENT_COUNT][OpenGLAppearanceElement::FEATURE_COUNT];

  /** Names of the appearance elements */
  static const char *m_ElementNames[];

  /** Initialize the default settings */
  void InitializeDefaultSettings();
};






#endif // __SNAPAppearanceSettings_h_
