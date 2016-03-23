#ifndef INTERPOLATELABELMODEL_H
#define INTERPOLATELABELMODEL_H

#include <AbstractModel.h>
#include <PropertyModel.h>

#include "ColorLabelPropertyModel.h"

class GlobalUIModel;

class InterpolateLabelModel
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
  irisRangedPropertyAccessMacro(Smoothing, bool)

  /** Perform the actual interpolation */
  void Interpolate();

protected:

  // Constructor
  InterpolateLabelModel();
  virtual ~InterpolateLabelModel() {}

  // Models for the main things settable by the user
  SmartPtr<ConcreteSimpleBooleanProperty> m_InterpolateAll;
  SmartPtr<ConcreteColorLabelPropertyModel> m_InterpolateLabel;
  SmartPtr<ConcreteColorLabelPropertyModel> m_DrawingLabel;
  SmartPtr<ConcreteDrawOverFilterPropertyModel> m_DrawOverFilter;
  SmartPtr<ConcreteRangedBooleanProperty> m_Smoothing;

  // The parent model
  GlobalUIModel *m_Parent;
};

#endif // INTERPOLATELABELMODEL_H
