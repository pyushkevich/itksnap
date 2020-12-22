#ifndef SMOOTHLABELMODEL_H
#define SMOOTHLABELMODEL_H

// issue #24: Add Label Smoothing Feature

#include <AbstractModel.h>
#include <PropertyModel.h>

#include "Registry.h"
#include "AbstractPropertyContainerModel.h"
#include "ColorLabelPropertyModel.h"

class GlobalUIModel;

class SmoothLabelsModel : public AbstractPropertyContainerModel
{
public:
  irisITKObjectMacro(SmoothLabelsModel, AbstractModel);

  /** Initialize with the parent model */
  void SetParentModel(GlobalUIModel *parent);

  /** Whether all the labels should be smoothed */
  irisSimplePropertyAccessMacro(SmoothAll, bool)

  /** Model for the label that will be interpolated */
  //irisGenericPropertyAccessMacro(LabelsToSmooth, LabelType, ColorLabelItemSetDomain)

  /** Update the state of the widget whenever it is shown */
  void UpdateOnShow();

  /** Perform the actual interpolation */
  void Smooth();

protected:

  // Constructor
  SmoothLabelsModel();
  virtual ~SmoothLabelsModel() {}

  // Model properties set by user
  SmartPtr<ConcreteSimpleBooleanProperty> m_SmoothAllModel;

  GlobalUIModel *m_Parent;
};

#endif // SMOOTHLABELMODEL_H
