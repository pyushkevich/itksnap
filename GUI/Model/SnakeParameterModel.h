#ifndef SNAKEPARAMETERMODEL_H
#define SNAKEPARAMETERMODEL_H

#include "SNAPCommon.h"
#include "AbstractModel.h"
#include "PropertyModel.h"

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

  // The model for whether the advanced mode (exponents) is on
  irisGetMacro(AdvancedEquationModeModel, AbstractSimpleBooleanProperty *)
  irisGetMacro(CasellesOrAdvancedModeModel, AbstractSimpleBooleanProperty *)

  // Whether or not we are in Zhu (region competition) mode
  bool IsRegionSnake();

  // Preview pipeline
  irisGetMacro(PreviewPipeline, SnakeParametersPreviewPipeline *);


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

  SmartPtr<ConcreteSimpleBooleanProperty> m_AdvancedEquationModeModel;

  SmartPtr<AbstractSimpleBooleanProperty> m_CasellesOrAdvancedModeModel;
  bool GetCasellesOrAdvancedModeValue();

  void SetupPreviewPipeline();

  // Preview pipeline
  SnakeParametersPreviewPipeline *m_PreviewPipeline;

  // Example images for the preview pipeline
  typedef itk::Image<short, 2> ExampleImageType;
  SmartPtr<ExampleImageType> m_ExampleImage[2];

  // Parent model
  GlobalUIModel *m_Parent;
};

#endif // SNAKEPARAMETERMODEL_H
