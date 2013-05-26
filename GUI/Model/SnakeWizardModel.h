#ifndef SNAKEWIZARDMODEL_H
#define SNAKEWIZARDMODEL_H

#include "SNAPCommon.h"
#include "AbstractModel.h"
#include "PropertyModel.h"
#include "ThresholdSettings.h"
#include "GlobalState.h"

class GlobalUIModel;
class IRISApplication;
class ImageWrapperBase;


class SnakeWizardModel : public AbstractModel
{
public:

  irisITKObjectMacro(SnakeWizardModel, AbstractModel)


  // This is a complex model, so there is some granularization over
  // the model update events that it fires.
  itkEventMacro(ThresholdSettingsUpdateEvent, IRISEvent)
  itkEventMacro(EdgePreprocessingSettingsUpdateEvent, IRISEvent)

  itkEventMacro(ActiveBubbleUpdateEvent, IRISEvent)
  itkEventMacro(BubbleListUpdateEvent, IRISEvent)
  itkEventMacro(BubbleDefaultRadiusUpdateEvent, IRISEvent)
  itkEventMacro(EvolutionIterationEvent, IRISEvent)
  itkEventMacro(GMMModifiedEvent, IRISEvent)

  FIRES(ThresholdSettingsUpdateEvent)
  FIRES(EdgePreprocessingSettingsUpdateEvent)
  FIRES(ActiveBubbleUpdateEvent)
  FIRES(BubbleListUpdateEvent)
  FIRES(BubbleDefaultRadiusUpdateEvent)
  FIRES(EvolutionIterationEvent)
  FIRES(GMMModifiedEvent)

  void SetParentModel(GlobalUIModel *model);
  irisGetMacro(Parent, GlobalUIModel *)

  virtual void OnUpdate();

  // State machine enums
  enum UIState {
    UIF_THESHOLDING_ENABLED,
    UIF_LOWER_THRESHOLD_ENABLED,
    UIF_UPPER_THRESHOLD_ENABLED,
    UIF_EDGEPROCESSING_ENABLED,
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
  // typedef STLVectorWrapperItemSetDomain<int, Bubble> BubbleDomain;
  // typedef AbstractPropertyModel<int, BubbleDomain> AbstractActiveBubbleProperty;

  // Model for whether the current pre-processing model is in preview mode or not
  irisGetMacro(PreviewModel, AbstractSimpleBooleanProperty *)

  // Models for the threshold-based preprocessing
  irisGetMacro(ThresholdLowerModel, AbstractRangedDoubleProperty *)
  irisGetMacro(ThresholdUpperModel, AbstractRangedDoubleProperty *)
  irisGetMacro(ThresholdSmoothnessModel, AbstractRangedDoubleProperty *)
  irisGetMacro(ThresholdModeModel, AbstractThresholdModeModel *)

  // Models for the edge-based preprocessing
  irisGetMacro(EdgePreprocessingSigmaModel, AbstractRangedDoubleProperty *)
  irisGetMacro(EdgePreprocessingKappaModel, AbstractRangedDoubleProperty *)
  irisGetMacro(EdgePreprocessingExponentModel, AbstractRangedDoubleProperty *)

  // Model for bubble selection
  irisGetMacro(ActiveBubbleModel, AbstractSimpleIntProperty *)

  // Model for bubble radius
  irisGetMacro(BubbleRadiusModel, AbstractRangedDoubleProperty *)

  // Get the model for the snake type
  irisGetMacro(SnakeTypeModel, AbstractSnakeTypeModel *)

  // The models for the evolution page
  irisGetMacro(StepSizeModel, AbstractRangedIntProperty *)
  irisGetMacro(EvolutionIterationModel, AbstractSimpleIntProperty *)

  /** Check the state flags above */
  bool CheckState(UIState state);

  /** A component index in an anatomic image wrapper */
  typedef std::pair<ImageWrapperBase *, int> ComponentInfo;

  /**
   * This method allows a quick lookup between components involved in
   * multi-variate segmentation algorithms and corresponding layers. For
   * example, snake mode may be launched with a six-component main image
   * and a single-component overlay. Then there are seven components used
   * in total, and calling this method with 0-5 will return the main image,
   * and calling it with 6 will return the overlay
   */
  ComponentInfo GetLayerAndIndexForNthComponent(int n);

  /**
   * Returns the total number of components available for multi-variate
   * segmentation methods. \see GetLayerAndIndexForNthComponent().
   */
  int GetNumberOfComponentsForSegmentation();

  /** Evaluate the threshold function so it can be plotted for the user */
  void EvaluateThresholdFunction(unsigned int n, float *x, float *y);

  /** Evaluate the threshold function so it can be plotted for the user */
  void EvaluateEdgePreprocessingFunction(unsigned int n, float *x, float *y);

  /** Perform the preprocessing based on thresholds */
  void ApplyThresholdPreprocessing();

  /** Processing that must take place when the thresholding page is shown */
  void OnThresholdingPageEnter();

  /** Processing that must take place when the thresholding page is shown */
  void OnEdgePreprocessingPageEnter();

  /** Processing that must take place when the clustering page is shown */
  void OnClusteringPageEnter();

  /** Do some cleanup when the preprocessing dialog closes */
  void OnPreprocessingDialogClose();

  /** Called when first displaying the snake wizard */
  void OnSnakeModeEnter();

  /** Add bubble at cursor */
  void AddBubbleAtCursor();

  /** Remove bubble at cursor */
  void RemoveBubbleAtCursor();

  /** Update a bubble */
  bool UpdateBubble(int index, Bubble bubble);

  /** Called when entering the evolution page */
  void OnEvolutionPageEnter();

  /** Called when entering the evolution page */
  void OnEvolutionPageBack();

  /** Called when entering the evolution page */
  void OnEvolutionPageFinish();

  /** Perform a single step of snake evolution */
  void PerformEvolutionStep();

  /** Rewind the evolution */
  void RewindEvolution();

  /* ===================================================================
   * CLUSTERING SUPPORT (GMM)
   * =================================================================== */
  irisRangedPropertyAccessMacro(NumberOfClusters, int)
  irisRangedPropertyAccessMacro(NumberOfGMMSamples, int)

  void ReinitializeClustering();

  void PerformClusteringIteration();

  // TODO: get rid of this?
  bool SetClusterForegroundState(int cluster, bool state);

  // TODO: get rid of this?
  bool SetClusterWeight(int cluster, double weight);


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

  SmartPtr<AbstractSimpleBooleanProperty> m_PreviewModel;
  bool GetPreviewValue(bool &value);
  void SetPreviewValue(bool value);

  SmartPtr<AbstractRangedDoubleProperty> m_EdgePreprocessingSigmaModel;
  bool GetEdgePreprocessingSigmaValueAndRange(double &x, NumericValueRange<double> *range);
  void SetEdgePreprocessingSigmaValue(double x);

  SmartPtr<AbstractRangedDoubleProperty> m_EdgePreprocessingExponentModel;
  bool GetEdgePreprocessingExponentValueAndRange(double &x, NumericValueRange<double> *range);
  void SetEdgePreprocessingExponentValue(double x);

  SmartPtr<AbstractRangedDoubleProperty> m_EdgePreprocessingKappaModel;
  bool GetEdgePreprocessingKappaValueAndRange(double &x, NumericValueRange<double> *range);
  void SetEdgePreprocessingKappaValue(double x);

  SmartPtr<AbstractSnakeTypeModel> m_SnakeTypeModel;
  bool GetSnakeTypeValueAndRange(SnakeType &value, GlobalState::SnakeTypeDomain *range);
  void SetSnakeTypeValue(SnakeType value);

  SmartPtr<AbstractSimpleIntProperty> m_ActiveBubbleModel;
  bool GetActiveBubbleValue(int &value);
  void SetActiveBubbleValue(int value);

  SmartPtr<AbstractRangedDoubleProperty> m_BubbleRadiusModel;
  bool GetBubbleRadiusValueAndRange(double &value, NumericValueRange<double> *range);
  void SetBubbleRadiusValue(double value);

  SmartPtr<ConcreteRangedIntProperty> m_StepSizeModel;

  SmartPtr<AbstractSimpleIntProperty> m_EvolutionIterationModel;
  int GetEvolutionIterationValue();

  // Helper function to check if particular set of models is active
  bool AreThresholdModelsActive();
  bool AreEdgePreprocessingModelsActive();

  // For models that work on only a single scalar image layer, report which
  // layer is currently selected as the target layer
  ScalarImageWrapperBase *GetSelectedScalarLayer();

  // Compute range and def value of radius based on ROI dimensions
  void ComputeBubbleRadiusDefaultAndRange();

  // Range for the bubble radius variable
  NumericValueRange<double> m_BubbleRadiusDomain;

  // Default value for the bubble radius (when there is no selection)
  double m_BubbleRadiusDefaultValue;

  /* ===================================================================
   * CLUSTERING SUPPORT (GMM)
   * =================================================================== */

  // Model for the number of clusters
  SmartPtr<AbstractRangedIntProperty> m_NumberOfClustersModel;
  bool GetNumberOfClustersValueAndRange(int &value, NumericValueRange<int> *range);
  void SetNumberOfClustersValue(int value);

  // Model for the number of clusters
  SmartPtr<AbstractRangedIntProperty> m_NumberOfGMMSamplesModel;
  bool GetNumberOfGMMSamplesValueAndRange(int &value, NumericValueRange<int> *range);
  void SetNumberOfGMMSamplesValue(int value);

  // TODO: this should be handled through the ITK modified mechanism
  void TagGMMPreprocessingFilterModified();

  // A list of all components available to clustering and other multi-variate
  // segmentation code. This list is updated in OnUpdate() in response to any
  // events that change the number of available layers.
  std::vector<ComponentInfo> m_ComponentInfo;

  // Parent model
  GlobalUIModel *m_Parent;
  IRISApplication *m_Driver;
  GlobalState *m_GlobalState;


};

#endif // SNAKEWIZARDMODEL_H
