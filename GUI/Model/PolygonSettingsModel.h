#ifndef POLYGONSETTINGSMODEL_H
#define POLYGONSETTINGSMODEL_H

#include "PropertyModel.h"

class GlobalUIModel;
class Registry;

class PolygonSettingsModel : public AbstractModel
{
public:
  irisITKObjectMacro(PolygonSettingsModel, AbstractModel)

  irisGetMacro(ParentModel, GlobalUIModel *)

  void SetParentModel(GlobalUIModel *model);

  void LoadFromRegistry(Registry &folder);
  void SaveToRegistry(Registry &folder);

  irisSimplePropertyAccessMacro(DeepLearningMode, bool)
  irisSimplePropertyAccessMacro(FreehandIsPiecewise, bool)
  irisRangedPropertyAccessMacro(FreehandSegmentLength, int)

  /** The deep learning pipeline used for lasso interactions */
  using DeepLearningPipelineDomain = OrderedItemSetDomain<std::string, std::string>;
  irisGenericPropertyAccessMacro(DeepLearningPipeline, std::string, DeepLearningPipelineDomain)

  /**
   * The pipeline to use for lasso interactions: the selected one if the server
   * provides it and it supports lasso interactions, otherwise the first one
   * that does. Empty if there is none.
   */
  std::string GetEffectiveDeepLearningPipeline();

  protected : PolygonSettingsModel();
  virtual ~PolygonSettingsModel() {}

  GlobalUIModel *m_ParentModel;

  SmartPtr<AbstractSimpleBooleanProperty> m_FreehandIsPiecewiseModel;
  bool                                    GetFreehandIsPiecewiseValue(bool &value);
  void                                    SetFreehandIsPiecewiseValue(bool value);

  SmartPtr<AbstractRangedIntProperty> m_FreehandSegmentLengthModel;
  bool GetFreehandSegmentLengthValueAndRange(int &value, NumericValueRange<int> *range);
  void SetFreehandSegmentLengthValue(int value);

  // Deep learning mode
  SmartPtr<ConcreteSimpleBooleanProperty> m_DeepLearningModeModel;
  void OnDeepLearningModeChange();

  // Deep learning pipeline for lasso interactions
  SmartPtr<AbstractPropertyModel<std::string, DeepLearningPipelineDomain>> m_DeepLearningPipelineModel;
  bool GetDeepLearningPipelineValueAndRange(std::string &value, DeepLearningPipelineDomain *range);
  void SetDeepLearningPipelineValue(std::string value);
  std::string m_DeepLearningPipelineId;


private:
  int m_LastFreehandRate;
};

#endif // POLYGONSETTINGSMODEL_H
