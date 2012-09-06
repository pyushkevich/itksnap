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

  itkEventMacro(ActiveBubbleUpdateEvent, IRISEvent)
  itkEventMacro(BubbleListUpdateEvent, IRISEvent)
  itkEventMacro(BubbleDefaultRadiusUpdateEvent, IRISEvent)
  itkEventMacro(EvolutionIterationEvent, IRISEvent)

  FIRES(ThresholdSettingsUpdateEvent)
  FIRES(ActiveBubbleUpdateEvent)
  FIRES(BubbleListUpdateEvent)
  FIRES(BubbleDefaultRadiusUpdateEvent)
  FIRES(EvolutionIterationEvent)

  void SetParentModel(GlobalUIModel *model);
  irisGetMacro(Parent, GlobalUIModel *)

  virtual void OnUpdate();

  // State machine enums
  enum UIState {
    UIF_THESHOLDING_ENABLED,
    UIF_LOWER_THRESHOLD_ENABLED,
    UIF_UPPER_THRESHOLD_ENABLED,
    UIF_SPEED_AVAILABLE,              // Has speed volume been computed?
    UIF_PREPROCESSING_ACTIVE,         // Is the preprocessing dialog open?
    UIF_BUBBLE_SELECTED,
    UIF_INITIALIZATION_VALID          // Do we have data to start snake evol?
    };

  // Model for the threshold mode
  typedef AbstractPropertyModel<ThresholdSettings::ThresholdMode>
    AbstractThresholdModeModel;

  // Model for the snake mode
  typedef AbstractPropertyModel<SnakeType, GlobalState::SnakeTypeDomain>
    AbstractSnakeTypeModel;

  // Model for the bubble selection
  typedef STLVectorWrapperItemSetDomain<int, Bubble> BubbleDomain;
  typedef AbstractPropertyModel<int, BubbleDomain> AbstractActiveBubbleProperty;

  // Models for the threshold-based preprocessing
  irisGetMacro(ThresholdLowerModel, AbstractRangedDoubleProperty *)
  irisGetMacro(ThresholdUpperModel, AbstractRangedDoubleProperty *)
  irisGetMacro(ThresholdSmoothnessModel, AbstractRangedDoubleProperty *)
  irisGetMacro(ThresholdModeModel, AbstractThresholdModeModel *)
  irisGetMacro(ThresholdPreviewModel, AbstractSimpleBooleanProperty *)

  // Model for bubble selection
  irisGetMacro(ActiveBubbleModel, AbstractActiveBubbleProperty *)

  // Model for bubble radius
  irisGetMacro(BubbleRadiusModel, AbstractRangedDoubleProperty *)

  // Get the model for the snake type
  irisGetMacro(SnakeTypeModel, AbstractSnakeTypeModel *)

  // The models for the evolution page
  irisGetMacro(StepSizeModel, AbstractRangedIntProperty *)
  irisGetMacro(EvolutionIterationModel, AbstractSimpleIntProperty *)

  /** Check the state flags above */
  bool CheckState(UIState state);

  /** Evaluate the threshold function so it can be plotted for the user */
  void EvaluateThresholdFunction(double t, double &x, double &y);

  /** Perform the preprocessing based on thresholds */
  void ApplyThresholdPreprocessing();

  /** Processing that must take place when the thresholding page is shown */
  void OnThresholdingPageEnter();

  /** Processing that must take place when the thresholding page is shown */
  void OnEdgePreprocessingPageEnter();

  /** Do some cleanup when the preprocessing dialog closes */
  void OnPreprocessingDialogClose();

  /** Called when first displaying the snake wizard */
  void OnSnakeModeEnter();

  /** Add bubble at cursor */
  void AddBubbleAtCursor();

  /** Remove bubble at cursor */
  void RemoveBubbleAtCursor();

  /** Called when entering the evolution page */
  void OnEvolutionPageEnter();

  /** Access the model for the step size */


  /** Perform a single step of snake evolution */
  void PerformEvolutionStep();

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

  SmartPtr<AbstractActiveBubbleProperty> m_ActiveBubbleModel;
  bool GetActiveBubbleValueAndRange(int &value, BubbleDomain *range);
  void SetActiveBubbleValue(int value);

  SmartPtr<AbstractRangedDoubleProperty> m_BubbleRadiusModel;
  bool GetBubbleRadiusValueAndRange(double &value, NumericValueRange<double> *range);
  void SetBubbleRadiusValue(double value);

  SmartPtr<ConcreteRangedIntProperty> m_StepSizeModel;

  SmartPtr<AbstractSimpleIntProperty> m_EvolutionIterationModel;
  int GetEvolutionIterationValue();

  // Compute range and def value of radius based on ROI dimensions
  void ComputeBubbleRadiusDefaultAndRange();

  // Range for the bubble radius variable
  NumericValueRange<double> m_BubbleRadiusDomain;

  // Default value for the bubble radius (when there is no selection)
  double m_BubbleRadiusDefaultValue;

  // Parent model
  GlobalUIModel *m_Parent;
  IRISApplication *m_Driver;
  GlobalState *m_GlobalState;


};

#endif // SNAKEWIZARDMODEL_H
