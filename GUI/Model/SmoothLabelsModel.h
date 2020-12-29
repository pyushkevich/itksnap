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

  // The parent model
  GlobalUIModel *m_Parent;

  // A pointer to the color label table
  ColorLabelTable *m_LabelTable;
};

#endif // SMOOTHLABELMODEL_H
