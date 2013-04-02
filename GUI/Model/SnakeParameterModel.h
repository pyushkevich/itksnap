#ifndef SNAKEPARAMETERMODEL_H
#define SNAKEPARAMETERMODEL_H

#include "SNAPCommon.h"
#include "AbstractModel.h"
#include "PropertyModel.h"
#include "SnakeParameters.h"

class GlobalUIModel;
class SnakeParametersPreviewPipeline;

namespace itk
{
template <class TPixel, unsigned int VDim> class Image;
}

/**
 * @brief The SnakeParameterModel class
 * This model handles interaction with snake parameters
 */
class SnakeParameterModel : public AbstractModel
{
public:

  irisITKObjectMacro(SnakeParameterModel, AbstractModel)

  // This model fires a custom DemoLoopEvent when the demo loop is run
  itkEventMacro(DemoLoopEvent, IRISEvent)
  FIRES(DemoLoopEvent)

  irisGetMacro(ParentModel, GlobalUIModel *)
  void SetParentModel(GlobalUIModel *model);

  virtual void OnUpdate();

  // State machine enums
  enum UIState {
    UIF_CASELLES_MODE,
    UIF_ZHU_MODE,
    UIF_EXPONENTS_SHOWN
  };

  // To make life easier, we access the parameters using an index, instead
  // of having an endless list of models.
  enum ParamIndex { ALHPA = 0, BETA, GAMMA };

  // Access one of the models for the weights alpha, beta, gamma
  AbstractRangedDoubleProperty *GetWeightModel(int index)
    { return m_WeightModel[index]; }

  // Access one of the models for the exponents of corresp. terms
  AbstractRangedIntProperty *GetExponentModel(int index)
    { return m_ExponentModel[index]; }

  // Speedup factor
  irisRangedPropertyAccessMacro(SpeedupFactor, double)

  // The model for whether the advanced mode (exponents) is on
  irisSimplePropertyAccessMacro(AdvancedEquationMode, bool)
  irisSimplePropertyAccessMacro(CasellesOrAdvancedMode, bool)

  // Whether the demo is being animated
  irisSimplePropertyAccessMacro(AnimateDemo, bool)

  // Whether or not we are in Zhu (region competition) mode
  bool IsRegionSnake();

  // Preview pipeline
  irisGetMacro(PreviewPipeline, SnakeParametersPreviewPipeline *)

  // Run a loop of the demo animation
  void PerformAnimationStep();

  // Run a loop of the demo animation
  void ResetAnimation();

  // Parameter IO
  void LoadParameters(const std::string &file);
  void SaveParameters(const std::string &file);
  void RestoreDefaults();


protected:

  SnakeParameterModel();
  virtual ~SnakeParameterModel();

  SmartPtr<AbstractRangedDoubleProperty> m_WeightModel[3];
  bool GetWeightValueAndRange(
      int index, double &value, NumericValueRange<double> *domain);
  void SetWeightValue(int index, double value);

  SmartPtr<AbstractRangedIntProperty> m_ExponentModel[3];
  bool GetExponentValueAndRange(
      int index, int &value, NumericValueRange<int> *domain);
  void SetExponentValue(int index, int value);

  SmartPtr<AbstractRangedDoubleProperty> m_SpeedupFactorModel;
  bool GetSpeedupFactorValueAndRange(
      double &value, NumericValueRange<double> *domain);
  void SetSpeedupFactorValue(double value);

  SmartPtr<ConcreteSimpleBooleanProperty> m_AdvancedEquationModeModel;

  SmartPtr<AbstractSimpleBooleanProperty> m_CasellesOrAdvancedModeModel;
  bool GetCasellesOrAdvancedModeValue();

  SmartPtr<ConcreteSimpleBooleanProperty> m_AnimateDemoModel;

  void SetupPreviewPipeline();

  // Preview pipeline
  SnakeParametersPreviewPipeline *m_PreviewPipeline;

  // Example images for the preview pipeline
  typedef itk::Image<short, 2> ExampleImageType;
  SmartPtr<ExampleImageType> m_ExampleImage[2];

  // Parent model
  GlobalUIModel *m_ParentModel;

  // Model governing the parameters
  AbstractPropertyModel<SnakeParameters, TrivialDomain> *m_ParametersModel;
};

#endif // SNAKEPARAMETERMODEL_H
