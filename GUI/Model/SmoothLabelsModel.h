#ifndef SMOOTHLABELMODEL_H
#define SMOOTHLABELMODEL_H

// issue #24: Add Label Smoothing Feature

#include <AbstractModel.h>
#include <PropertyModel.h>
#include <unordered_set>

#include "Registry.h"
#include "AbstractPropertyContainerModel.h"
#include "ColorLabelPropertyModel.h"

class GlobalUIModel;

class SmoothLabelsModel : public AbstractModel
{
public:
  irisITKObjectMacro(SmoothLabelsModel, AbstractModel);

  enum SigmaUnit {mm, vox};

  /** Initialize with the parent model */
  void SetParentModel(GlobalUIModel *parent);

  /** Model for the label that will be smoothed */
  //irisGenericPropertyAccessMacro(LabelsToSmooth, LabelType, ColorLabelItemSetDomain)

  /** Update the state of the widget whenever it is shown */
  void UpdateOnShow();

  /** Perform the actual smoothing */
  void Smooth(std::unordered_set<LabelType> &labelsToSmooth, std::vector<double> &sigma, SigmaUnit unit);

  /** Get the model describing the current selected label (and its domain) */
  irisGetMacro(CurrentLabelModel, ConcreteColorLabelPropertyModel *)


protected:

  // Constructor
  SmoothLabelsModel();
  virtual ~SmoothLabelsModel() {}

  // The parent model
  GlobalUIModel *m_Parent;

  // A pointer to the color label table
  ColorLabelTable *m_LabelTable;

  // The label that is currently selected
  SmartPtr<ConcreteColorLabelPropertyModel> m_CurrentLabelModel;
};

#endif // SMOOTHLABELMODEL_H
