#ifndef INTERPOLATELABELMODEL_H
#define INTERPOLATELABELMODEL_H

#include <AbstractModel.h>
#include <PropertyModel.h>

#include "Registry.h"
#include "AbstractPropertyContainerModel.h"
#include "ColorLabelPropertyModel.h"


class GlobalUIModel;

class InterpolateLabelModel : public AbstractPropertyContainerModel
{    
public:

  enum InterpolationType
  {
      DEFAULT = 0, LEVEL_SET, MORPHOLOGY
  };

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
  irisRangedPropertyAccessMacro(DefaultSmoothing, double)

  irisRangedPropertyAccessMacro(LevelSetSmoothing, double)

  /** Whether all the labels should be interpolated */
  irisSimplePropertyAccessMacro(RetainScaffold, bool)

  /** Whether all the labels should be interpolated */

  /** Whether all the labels should be interpolated */
  irisRangedPropertyAccessMacro(LevelSetCurvature, double)

  /** Whether to use signed distance transform for morphological interpolation */
  irisSimplePropertyAccessMacro(MorphologyUseDistance, bool)

  /** Whether to use optimal slice alignment for morphological interpolation */
  irisSimplePropertyAccessMacro(MorphologyUseOptimalAlignment, bool)

  /** Which interpolation method to use */
  irisSimplePropertyAccessMacro(InterpolationMethod, InterpolationType)

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

  SmartPtr<ConcreteRangedDoubleProperty> m_DefaultSmoothingModel;

  SmartPtr<ConcreteRangedDoubleProperty> m_LevelSetSmoothingModel;
  SmartPtr<ConcreteRangedDoubleProperty> m_LevelSetCurvatureModel;

  SmartPtr<ConcreteSimpleBooleanProperty> m_MorphologyUseDistanceModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_MorphologyUseOptimalAlignmentModel;

  typedef ConcretePropertyModel<InterpolationType, TrivialDomain> ConcreteInterpolationType;
  SmartPtr<ConcreteInterpolationType> m_InterpolationMethodModel;


  // The parent model
  GlobalUIModel *m_Parent;
};

#endif // INTERPOLATELABELMODEL_H
