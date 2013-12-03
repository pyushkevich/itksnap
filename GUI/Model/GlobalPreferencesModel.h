#ifndef GLOBALPREFERENCESMODEL_H
#define GLOBALPREFERENCESMODEL_H

#include "PropertyModel.h"
#include "ColorMap.h"
#include "SNAPAppearanceSettings.h"

class MeshOptions;
class GlobalUIModel;
class DefaultBehaviorSettings;

/**
 * This model exposes the different global preferences to the GUI. It is set
 * up for a dialog in which the user must press Apply for the changes to
 * be propagated to the GUI. So this model caches the values the preferences
 * until the apply function is called
 */
class GlobalPreferencesModel : public AbstractModel
{
public:
  irisITKObjectMacro(GlobalPreferencesModel, AbstractModel)

  // Typedefs for interpolation mode
  enum InterpolationMode { NEAREST = 0, LINEAR };
  typedef SimpleItemSetDomain<InterpolationMode, std::string> InterpolationDomain;

  // Typedefs for appearance
  typedef SNAPAppearanceSettings::UIElements UIElement;

  // Default behaviors and permissions
  irisGetMacro(DefaultBehaviorSettings, DefaultBehaviorSettings *)

  // Screen layout

  // Thumbnail properties
  irisSimplePropertyAccessMacro(ThumbnailVisibility, bool)
  irisRangedPropertyAccessMacro(ThumbnailFraction, int)
  irisRangedPropertyAccessMacro(ThumbnailMaxSize, int)

  // Other slice display properties
  irisGenericPropertyAccessMacro(Interpolation, InterpolationMode, InterpolationDomain)

  irisSimplePropertyAccessMacro(DefaultColorMapPreset, std::string)

  // Appearance
  irisSimplePropertyAccessMacro(ActiveUIElement, UIElement)
  irisSimplePropertyAccessMacro(ElementVisibility, bool)
  irisRangedPropertyAccessMacro(ElementNormalColor, Vector3ui)
  irisRangedPropertyAccessMacro(ElementActiveColor, Vector3ui)
  irisRangedPropertyAccessMacro(ElementLineThickness, double)
  irisRangedPropertyAccessMacro(ElementDashSpacing, int)
  irisSimplePropertyAccessMacro(ElementAntiAlias, bool)
  irisSimplePropertyAccessMacro(ElementFontSize, bool)

  // Mesh options
  irisGetMacro(MeshOptions, MeshOptions *)

  /**
   * Set the parent model
   */
  void SetParentModel(GlobalUIModel *parent);

  /**
   * Initialize the internal cached properties based on current system state.
   * This method should be called when opening the properties dialog, or on revert
   */
  void InitializePreferences();

  /**
   * Update the system state with the current cached preferences
   */
  void ApplyPreferences();

protected:

  GlobalPreferencesModel();

  // Default behaviors and permissions (copy of the system's settings)
  SmartPtr<DefaultBehaviorSettings> m_DefaultBehaviorSettings;

  // Thumbnail properties
  SmartPtr<ConcreteSimpleBooleanProperty> m_ThumbnailVisibilityModel;
  SmartPtr<ConcreteRangedIntProperty> m_ThumbnailFractionModel;
  SmartPtr<ConcreteRangedIntProperty> m_ThumbnailMaxSizeModel;

  // Other slice display properties
  typedef ConcretePropertyModel<InterpolationMode, InterpolationDomain> ConcreteInterpolationModel;
  SmartPtr<ConcreteInterpolationModel> m_InterpolationModel;

  SmartPtr<ConcreteSimpleStringProperty> m_DefaultColorMapPresetModel;

  // Appearance
  SmartPtr<ConcretePropertyModel<UIElement, TrivialDomain> > m_ActiveUIElementModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_ElementVisibilityModel;
  SmartPtr<ConcreteRangedUIntVec3Property> m_ElementNormalColorModel;
  SmartPtr<ConcreteRangedUIntVec3Property> m_ElementActiveColorModel;
  SmartPtr<ConcreteRangedDoubleProperty> m_ElementLineThicknessModel;
  SmartPtr<ConcreteRangedIntProperty> m_ElementDashSpacingModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_ElementAntiAliasModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_ElementFontSizeModel;

  // Mesh options (we keep a copy)
  SmartPtr<MeshOptions> m_MeshOptions;

  // Parent model
  GlobalUIModel *m_ParentModel;
};

#endif // GLOBALPREFERENCESMODEL_H
