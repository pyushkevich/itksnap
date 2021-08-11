#ifndef SMOOTHLABELMODEL_H
#define SMOOTHLABELMODEL_H

// issue #24: Add Label Smoothing Feature

#include <AbstractModel.h>
#include <PropertyModel.h>
#include <unordered_set>

#include "Registry.h"
#include "AbstractPropertyContainerModel.h"
#include "ColorLabelPropertyModel.h"
#include "GenericImageData.h"

class GlobalUIModel;

class SmoothLabelsModel : public AbstractModel
{
public:
  irisITKObjectMacro(SmoothLabelsModel, AbstractModel);

  enum SigmaUnit {mm, vox};
  const char* SigmaUnitStr[2] = {"mm","vox"};

  /** Initialize with the parent model */
  void SetParentModel(GlobalUIModel *parent);

  /** Update the state of the widget whenever it is shown */
  void UpdateOnShow() {};

  /** Perform the actual smoothing */
  void
  Smooth(std::unordered_set<LabelType> &labelsToSmooth,
         std::vector<double> sigma,
         SigmaUnit unit,
         bool SmoothAllFrames);

  /** Get the model describing the current selected label (and its domain) */
  irisGetMacro(CurrentLabelModel, ConcreteColorLabelPropertyModel *)

  /** Get Parent Model */
  GlobalUIModel* GetParent() const;


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

  // utility method to deep copy image
  template <typename TImage>
  void DeepCopy(typename TImage::Pointer input, typename TImage::Pointer output);

  // utility method to apply c3d smoothing
  void ApplyC3DSmoothing(LabelImageWrapper *liw, std::vector<double> sigma
                         , SigmaUnit sigmaUnit, std::unordered_set<LabelType> labelsToSmooth);
};

#endif // SMOOTHLABELMODEL_H
