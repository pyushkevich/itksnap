#ifndef GLOBALPREFERENCESMODEL_H
#define GLOBALPREFERENCESMODEL_H

#include "PropertyModel.h"
#include "ColorMap.h"
#include "SNAPAppearanceSettings.h"

/**
 * This model exposes the different global preferences to the GUI. It is set
 * up for a dialog in which the user must press Apply for the changes to
 * be propagated to the GUI. So this model caches the values the preferences
 * until the apply function is called
 */
class GlobalPreferencesModel : public AbstractModel
{
public:
  GlobalPreferencesModel();

  // Default behaviors
  irisSimplePropertyAccessMacro(LinkedZoom, bool)
  irisSimplePropertyAccessMacro(ContinuousMeshUpdate, bool)
  irisSimplePropertyAccessMacro(Synchronization, bool)
  irisSimplePropertyAccessMacro(SyncCursor, bool)
  irisSimplePropertyAccessMacro(SyncZoom, bool)
  irisSimplePropertyAccessMacro(SyncPan, bool)

  // Permissions
  irisSimplePropertyAccessMacro(CheckForUpdates, bool)

  // Screen layout

  // Thumbnail properties
  irisSimplePropertyAccessMacro(ThumbnailVisibility, bool)
  irisRangedPropertyAccessMacro(ThumbnailFraction, int)
  irisRangedPropertyAccessMacro(ThumbnailMaxSize, int)

  // Other slice display properties
  enum InterpolationMode { NEAREST = 0, LINEAR };
  typedef SimpleItemSetDomain<InterpolationMode, std::string> InterpolationDomain;
  irisGenericPropertyAccessMacro(Interpolation, InterpolationMode, InterpolationDomain)

  irisSimplePropertyAccessMacro(DefaultColorMapPreset, std::string)

  // Appearance
  typedef SNAPAppearanceSettings::UIElements UIElement;
  irisSimplePropertyAccessMacro(ActiveUIElement, UIElement)
  irisSimplePropertyAccessMacro(ElementVisibility, bool)
  irisRangedPropertyAccessMacro(ElementNormalColor, Vector3ui)
  irisRangedPropertyAccessMacro(ElementActiveColor, Vector3ui)
  irisRangedPropertyAccessMacro(ElementLineThickness, double)
  irisRangedPropertyAccessMacro(ElementDashSpacing, int)
  irisSimplePropertyAccessMacro(ElementAntiAlias, bool)
  irisSimplePropertyAccessMacro(ElementFontSize, bool)



protected:

  // Default behaviors
  SmartPtr<ConcreteSimpleBooleanProperty> m_LinkedZoomModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_ContinuousMeshUpdateModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SynchronizationModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncCursorModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncZoomModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_SyncPanModel;

  // Permissions
  SmartPtr<ConcreteSimpleBooleanProperty> m_CheckForUpdatesModel;

  // Screen layout

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
};

#endif // GLOBALPREFERENCESMODEL_H
