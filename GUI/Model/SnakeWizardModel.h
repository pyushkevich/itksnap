#ifndef SNAKEWIZARDMODEL_H
#define SNAKEWIZARDMODEL_H

#include "SNAPCommon.h"
#include "AbstractModel.h"
#include "PropertyModel.h"
#include "ThresholdSettings.h"
#include "GlobalState.h"

class GlobalUIModel;
class IRISApplication;

class SnakeWizardModel : public AbstractModel
{
public:

  irisITKObjectMacro(SnakeWizardModel, AbstractModel)

  // This is a complex model, so there is some granularization over
  // the model update events that it fires.
  itkEventMacro(ThresholdSettingsUpdateEvent, IRISEvent)

  FIRES(ThresholdSettingsUpdateEvent)

  void SetParentModel(GlobalUIModel *model);
  irisGetMacro(Parent, GlobalUIModel *)

  virtual void OnUpdate();

  // State machine enums
  enum UIState {
    UIF_THESHOLDING_ENABLED,
    UIF_LOWER_THRESHOLD_ENABLED,
    UIF_UPPER_THRESHOLD_ENABLED
    };

  // Model for the threshold mode
  typedef AbstractPropertyModel<ThresholdSettings::ThresholdMode>
    AbstractThresholdModeModel;

  // Model for the snake mode
  typedef AbstractPropertyModel<SnakeType, GlobalState::SnakeTypeDomain> AbstractSnakeTypeModel;

  // Models for the threshold-based preprocessing
  irisGetMacro(ThresholdLowerModel, AbstractRangedDoubleProperty *)
  irisGetMacro(ThresholdUpperModel, AbstractRangedDoubleProperty *)
  irisGetMacro(ThresholdSmoothnessModel, AbstractRangedDoubleProperty *)
  irisGetMacro(ThresholdModeModel, AbstractThresholdModeModel *)
  irisGetMacro(ThresholdPreviewModel, AbstractSimpleBooleanProperty *)

  // Get the model for the snake type
  irisGetMacro(SnakeTypeModel, AbstractSnakeTypeModel *)

  /** Check the state flags above */
  bool CheckState(UIState state);

  /** Evaluate the threshold function so it can be plotted for the user */
  void EvaluateThresholdFunction(double t, double &x, double &y);

  /** Perform the preprocessing based on thresholds */
  void ApplyThresholdPreprocessing();


protected:
  SnakeWizardModel();
  virtual ~SnakeWizardModel() {}

  SmartPtr<AbstractRangedDoubleProperty> m_ThresholdUpperModel;
  bool GetThresholdUpperValueAndRange(double &x, NumericValueRange<double> *range);
  void SetThresholdUpperValue(double x);

  SmartPtr<AbstractRangedDoubleProperty> m_ThresholdLowerModel;
  bool GetThresholdLowerValueAndRange(double &x, NumericValueRange<double> *range);
  void SetThresholdLowerValue(double x);

  SmartPtr<AbstractRangedDoubleProperty> m_ThresholdSmoothnessModel;
  bool GetThresholdSmoothnessValueAndRange(double &x, NumericValueRange<double> *range);
  void SetThresholdSmoothnessValue(double x);

  SmartPtr<AbstractThresholdModeModel> m_ThresholdModeModel;
  bool GetThresholdModeValue(ThresholdSettings::ThresholdMode &x);
  void SetThresholdModeValue(ThresholdSettings::ThresholdMode x);

  SmartPtr<AbstractSimpleBooleanProperty> m_ThresholdPreviewModel;
  bool GetThresholdPreviewValue(bool &value);
  void SetThresholdPreviewValue(bool value);

  SmartPtr<AbstractSnakeTypeModel> m_SnakeTypeModel;
  bool GetSnakeTypeValueAndRange(SnakeType &value, GlobalState::SnakeTypeDomain *range);
  void SetSnakeTypeValue(SnakeType value);

  GlobalUIModel *m_Parent;
  IRISApplication *m_Driver;
  GlobalState *m_GlobalState;
};

#endif // SNAKEWIZARDMODEL_H
