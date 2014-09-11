#ifndef SNAKEWIZARDMODEL_H
#define SNAKEWIZARDMODEL_H

#include "SNAPCommon.h"
#include "AbstractModel.h"
#include "PropertyModel.h"
#include "ThresholdSettings.h"
#include "GlobalState.h"
#include "ImageWrapperBase.h"

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
  itkEventMacro(RFClassifierModifiedEvent, IRISEvent)

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

  // Interaction modes (correspond to wizard pages)
  enum InteractionMode {
    MODE_PREPROCESSING,
    MODE_BUBBLES,
    MODE_EVOLUTION,
    MODE_NONE
  };

  // State machine enums
  enum UIState {
    UIF_THESHOLDING_ENABLED,
    UIF_LOWER_THRESHOLD_ENABLED,
    UIF_UPPER_THRESHOLD_ENABLED,
    UIF_EDGEPROCESSING_ENABLED,
    UIF_CAN_GENERATE_SPEED,
    UIF_SPEED_AVAILABLE,              // Has speed volume been computed?
    UIF_PREPROCESSING_ACTIVE,         // Is the preprocessing dialog open?
    UIF_BUBBLE_SELECTED,
    UIF_INITIALIZATION_VALID,          // Do we have data to start snake evol?
    UIF_BUBBLE_MODE
    };

  // Model for the threshold mode
  typedef AbstractPropertyModel<ThresholdSettings::ThresholdMode>
    AbstractThresholdModeModel;

  // Model for the snake mode
  typedef AbstractPropertyModel<SnakeType, GlobalState::SnakeTypeDomain>
    AbstractSnakeTypeModel;

  // Model describing the current preprocessing mode
  typedef SimpleItemSetDomain<PreprocessingMode, std::string> PreprocessingModeDomain;
  typedef AbstractPropertyModel<PreprocessingMode, PreprocessingModeDomain> AbstractPreprocessingModeModel;

  // Model for layer selection
  typedef SimpleItemSetDomain<unsigned long, std::string> LayerSelectionDomain;
  typedef AbstractPropertyModel<unsigned long, LayerSelectionDomain> AbstractLayerSelectionModel;

  // Model for scalar representation selection within a layer
  typedef std::pair<ScalarRepresentation, int> LayerScalarRepIndex;
  typedef SimpleItemSetDomain<LayerScalarRepIndex, std::string> ScalarRepSelectionDomain;
  typedef AbstractPropertyModel<LayerScalarRepIndex, ScalarRepSelectionDomain> AbstractScalarRepSelectionModel;

  // Model for the bubble selection
  // typedef STLVectorWrapperItemSetDomain<int, Bubble> BubbleDomain;
  // typedef AbstractPropertyModel<int, BubbleDomain> AbstractActiveBubbleProperty;

  // Model for whether the current pre-processing model is in preview mode or not
  irisGetMacro(PreviewModel, AbstractSimpleBooleanProperty *)

  // Get the active layer for single-layer processing modes
  ScalarImageWrapperBase *GetActiveScalarLayer(PreprocessingMode mode);

  // Models for the current speed rendering style. These are true if the speed
  // is displayed using the selected style, false otherwise
  irisGetMacro(BlueWhiteSpeedModeModel, AbstractSimpleBooleanProperty *)
  irisGetMacro(RedTransparentSpeedModeModel, AbstractSimpleBooleanProperty *)

  // Models for the threshold-based preprocessing
  irisGetMacro(ThresholdLowerModel, AbstractRangedDoubleProperty *)
  irisGetMacro(ThresholdUpperModel, AbstractRangedDoubleProperty *)
  irisGetMacro(ThresholdSmoothnessModel, AbstractRangedDoubleProperty *)
  irisGetMacro(ThresholdModeModel, AbstractThresholdModeModel *)

  // Model for selecting which image layer is used for thresholding
  irisGetMacro(ThresholdActiveLayerModel, AbstractLayerSelectionModel *)
  irisGetMacro(ThresholdActiveScalarRepModel, AbstractScalarRepSelectionModel *)

  // Models for the edge-based preprocessing
  irisGetMacro(EdgePreprocessingSigmaModel, AbstractRangedDoubleProperty *)
  irisGetMacro(EdgePreprocessingKappaModel, AbstractRangedDoubleProperty *)
  irisGetMacro(EdgePreprocessingExponentModel, AbstractRangedDoubleProperty *)


  // Called when entering proprocessing mode (i.e., back from button page)
  void OnBubbleModeBack();

  // What happens when we enter bubble mode
  void OnBubbleModeEnter();

  // Model for bubble selection
  irisGetMacro(ActiveBubbleModel, AbstractSimpleIntProperty *)

  // Model for bubble radius
  irisGetMacro(BubbleRadiusModel, AbstractRangedDoubleProperty *)

  // Get the model for the snake type
  irisGetMacro(SnakeTypeModel, AbstractSnakeTypeModel *)

  // Get the model for the current preprocessing mode
  irisGetMacro(PreprocessingModeModel, AbstractPreprocessingModeModel *)

  // The models for the evolution page
  irisGetMacro(StepSizeModel, AbstractRangedIntProperty *)
  irisGetMacro(EvolutionIterationModel, AbstractSimpleIntProperty *)

  /** Check the state flags above */
  bool CheckState(UIState state);

  /** A reference to a component used in automatic segmentation */
  struct ComponentInfo
  {
    ImageWrapperBase *ImageWrapper;
    ScalarImageWrapperBase *ComponentWrapper;
    int ComponentIndex;
  };

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
  void ApplyPreprocessing();

  /** Do some cleanup when the preprocessing dialog closes */
  void CompletePreprocessing();

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

  /**
   * Perform a single step of snake evolution. Returns true if the evolution
   * has converged
   */
  bool PerformEvolutionStep();

  /** Rewind the evolution */
  void RewindEvolution();

  /** Cancel segmentation and return to IRIS */
  void OnCancelSegmentation();

  /* ===================================================================
   * CLUSTERING SUPPORT (GMM)
   * =================================================================== */

  /** Model controlling the number of clusters in the GMM */
  irisRangedPropertyAccessMacro(NumberOfClusters, int)

  /** Model controlling the number of sampled for GMM optimization */
  irisRangedPropertyAccessMacro(NumberOfGMMSamples, int)

  /** Model controlling the cluster used for the foreground probability */
  irisRangedPropertyAccessMacro(ForegroundCluster, int)

  typedef SimpleItemSetDomain<int, std::string> ComponentDomain;
  typedef ConcretePropertyModel<int, ComponentDomain> ComponentIndexModel;

  irisGetMacro(ClusterPlottedComponentModel, ComponentIndexModel *)

  void ReinitializeClustering();

  void PerformClusteringIteration();

  // TODO: get rid of this?
  bool SetClusterForegroundState(int cluster, bool state);

  // TODO: get rid of this?
  double GetClusterWeight(int cluster);
  bool SetClusterWeight(int cluster, double weight);

  double GetClusterNativeMean(int cluster, int component);
  bool SetClusterNativeMean(int cluster, int component, double x);

  double GetClusterNativeCovariance(int cluster, int comp1, int comp2);
  double GetClusterNativeTotalVariance(int cluster);

  double EvaluateClusterMarginalUnivariatePDF(int cluster, int component, double x);

  /* ===================================================================
   * SUPERVISED CLASSIFICATION SUPPORT (RANDOM FORESTS)
   * =================================================================== */

  /** Model controlling the class/label used for the foreground probability */
  typedef STLMapWrapperItemSetDomain<LabelType, ColorLabel> ForegroundClassDomain;
  irisGenericPropertyAccessMacro(ForegroundClassColorLabel, LabelType, ForegroundClassDomain)

  /** Model for the forest size */
  irisRangedPropertyAccessMacro(ForestSize, int)

  /** Train the random forest classifier when the user hits the 'train' button */
  void TrainClassifier();

  /** Clear the classification examples (i.e., clear the classification) */
  void ClearSegmentation();

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

  SmartPtr<AbstractLayerSelectionModel> m_ThresholdActiveLayerModel;
  bool GetThresholdActiveLayerValueAndRange(unsigned long &value, LayerSelectionDomain *range);
  void SetThresholdActiveLayerValue(unsigned long value);

  SmartPtr<AbstractScalarRepSelectionModel> m_ThresholdActiveScalarRepModel;
  bool GetThresholdActiveScalarRepValueAndRange(LayerScalarRepIndex &value, ScalarRepSelectionDomain *range);
  void SetThresholdActiveScalarRepValue(LayerScalarRepIndex value);

  SmartPtr<AbstractSimpleBooleanProperty> m_PreviewModel;
  bool GetPreviewValue(bool &value);
  void SetPreviewValue(bool value);

  SmartPtr<AbstractSimpleBooleanProperty> m_BlueWhiteSpeedModeModel;
  bool GetBlueWhiteSpeedModeValue(bool &value);
  void SetBlueWhiteSpeedModeValue(bool value);

  SmartPtr<AbstractSimpleBooleanProperty> m_RedTransparentSpeedModeModel;
  bool GetRedTransparentSpeedModeValue(bool &value);
  void SetRedTransparentSpeedModeValue(bool value);

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

  SmartPtr<AbstractPreprocessingModeModel> m_PreprocessingModeModel;
  bool GetPreprocessingModeValueAndRange(PreprocessingMode &value, PreprocessingModeDomain *range);
  void SetPreprocessingModeValue(PreprocessingMode value);

  SmartPtr<AbstractSimpleIntProperty> m_ActiveBubbleModel;
  bool GetActiveBubbleValue(int &value);
  void SetActiveBubbleValue(int value);

  SmartPtr<AbstractRangedDoubleProperty> m_BubbleRadiusModel;
  bool GetBubbleRadiusValueAndRange(double &value, NumericValueRange<double> *range);
  void SetBubbleRadiusValue(double value);

  SmartPtr<ConcreteRangedIntProperty> m_StepSizeModel;

  SmartPtr<AbstractSimpleIntProperty> m_EvolutionIterationModel;
  int GetEvolutionIterationValue();

  // Get the threshold settings for the active layer
  ThresholdSettings *GetThresholdSettings();

  // Helper function to check if particular set of models is active
  bool AreThresholdModelsActive();
  bool AreEdgePreprocessingModelsActive();

  // Helper function to check if we can generate a speed image given the
  // current pre-segmentation settings.
  bool CanGenerateSpeedVolume();

  // For models that work on only a single scalar image layer, report which
  // layer is currently selected as the target layer
  ScalarImageWrapperBase *GetSelectedScalarLayer();

  // Compute range and def value of radius based on ROI dimensions
  void ComputeBubbleRadiusDefaultAndRange();

  // Range for the bubble radius variable
  NumericValueRange<double> m_BubbleRadiusDomain;

  // Default value for the bubble radius (when there is no selection)
  double m_BubbleRadiusDefaultValue;

  // Are we in bubble mode
  InteractionMode m_InteractionMode;

  void SetInteractionMode(InteractionMode mode);

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

  // Model for the active cluster
  SmartPtr<AbstractRangedIntProperty> m_ForegroundClusterModel;
  bool GetForegroundClusterValueAndRange(int &value, NumericValueRange<int> *range);
  void SetForegroundClusterValue(int value);

  // Model for the selected component
  SmartPtr<ComponentIndexModel> m_ClusterPlottedComponentModel;

  // TODO: this should be handled through the ITK modified mechanism
  void TagGMMPreprocessingFilterModified();

  // A list of all components available to clustering and other multi-variate
  // segmentation code. This list is updated in OnUpdate() in response to any
  // events that change the number of available layers.
  std::vector<ComponentInfo> m_ComponentInfo;

  // Update the model for component selection
  void UpdateClusterPlottedComponentModel();


  /* ===================================================================
   * SUPERVISED CLASSIFICATION SUPPORT (Random Forest)
   * =================================================================== */

  // A mapping of currently defined classes to color label descriptors
  std::map<LabelType, ColorLabel> m_ActiveClasses;

  // Model for the foreground color label
  typedef AbstractPropertyModel<LabelType, ForegroundClassDomain>
    AbstractForegroundClassPropertyModel;

  // A list of suggested labels, from which the user can choose one to draw
  // These labels are the N most recently used labels
  std::map<LabelType, ColorLabel> m_ClassifyQuickLabels;

  // The size of the suggested label list (static)
  static unsigned int m_ClassifyQuickLabelsCount;

  // Model for the suggested labels from which the user can choose one to draw


  SmartPtr<AbstractForegroundClassPropertyModel> m_ForegroundClassColorLabelModel;
  bool GetForegroundClassColorLabelValueAndRange(LabelType &value, ForegroundClassDomain *range);
  void SetForegroundClassColorLabelValue(LabelType value);

  SmartPtr<AbstractRangedIntProperty> m_ForestSizeModel;
  bool GetForestSizeValueAndRange(int &value, NumericValueRange<int> *range);
  void SetForestSizeValue(int value);

  // TODO: this should be handled through the ITK modified mechanism
  void TagRFPreprocessingFilterModified();

  // Parent model
  GlobalUIModel *m_Parent;
  IRISApplication *m_Driver;
  GlobalState *m_GlobalState;


};

#endif // SNAKEWIZARDMODEL_H
