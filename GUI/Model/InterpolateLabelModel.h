#ifndef INTERPOLATELABELMODEL_H
#define INTERPOLATELABELMODEL_H

#include <AbstractModel.h>
#include <PropertyModel.h>

#include "ColorLabelPropertyModel.h"

class GlobalUIModel;

class InterpolateLabelModel : public AbstractModel
{    
public:

  // Standard ITK stuff
  irisITKObjectMacro(InterpolateLabelModel, AbstractModel)

  /** Initialize with the parent model */
  void SetParentModel(GlobalUIModel *parent);

  /** Whether all the labels should be interpolated */
  irisSimplePropertyAccessMacro(InterpolateAll, bool)

  /** Model for the label that will be interpolated */
  irisGenericPropertyAccessMacro(InterpolateLabel, LabelType, ColorLabelItemSetDomain)

  /** Model for the label that will be drawn with */
  irisGenericPropertyAccessMacro(DrawingLabel, LabelType, ColorLabelItemSetDomain)

  /** Model for the label that will be drawn over */
  irisGenericPropertyAccessMacro(DrawOverFilter, DrawOverFilter, DrawOverLabelItemSetDomain)

  /** Smoothing factor */
  irisRangedPropertyAccessMacro(Smoothing, double)

  /** Whether all the labels should be interpolated */
  irisSimplePropertyAccessMacro(RetainScaffold, bool)

  /** Whether all the labels should be interpolated */
  irisSimplePropertyAccessMacro(UseLevelSet, bool)

  /** Whether all the labels should be interpolated */
  irisRangedPropertyAccessMacro(LevelSetCurvature, double)

  /** Perform the actual interpolation */
  void Interpolate();

protected:

  // Constructor
  InterpolateLabelModel();
  virtual ~InterpolateLabelModel() {}

  // Models for the main things settable by the user
  SmartPtr<ConcreteSimpleBooleanProperty> m_InterpolateAllModel;
  SmartPtr<ConcreteColorLabelPropertyModel> m_InterpolateLabelModel;
  SmartPtr<ConcreteColorLabelPropertyModel> m_DrawingLabelModel;
  SmartPtr<ConcreteDrawOverFilterPropertyModel> m_DrawOverFilterModel;

  SmartPtr<ConcreteSimpleBooleanProperty> m_RetainScaffoldModel;

  SmartPtr<ConcreteRangedDoubleProperty> m_SmoothingModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_UseLevelSetModel;
  SmartPtr<ConcreteRangedDoubleProperty> m_LevelSetCurvatureModel;


  // The parent model
  GlobalUIModel *m_Parent;
};

#endif // INTERPOLATELABELMODEL_H
