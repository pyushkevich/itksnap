#ifndef SNAKEROIRESAMPLEMODEL_H
#define SNAKEROIRESAMPLEMODEL_H

#include "PropertyModel.h"

class GlobalUIModel;

/**
 * @brief The SnakeROIResampleModel class
 * Handles the resample snake dialog.
 */
class SnakeROIResampleModel : public AbstractModel
{
public:

  irisITKObjectMacro(SnakeROIResampleModel, AbstractModel)

  irisRangedPropertyAccessMacro(InputSpacing, Vector3d)
  irisRangedPropertyAccessMacro(OutputSpacing, Vector3d)

  typedef SimpleItemSetDomain<double, std::string> ScaleFactorDomain;
  typedef AbstractPropertyModel<double, ScaleFactorDomain> AbstractScaleFactorModel;
  AbstractScaleFactorModel *GetNthScaleFactorModel(int index) const
    { return m_ScaleFactorModel[index]; }

  void SetParentModel(GlobalUIModel *model);

protected:

  SnakeROIResampleModel();
  virtual ~SnakeROIResampleModel() {}

  GlobalUIModel *m_Parent;

  SmartPtr<AbstractRangedDoubleVec3Property> m_InputSpacingModel;
  bool GetInputSpacingValueAndRange(Vector3d &value, NumericValueRange<Vector3d> *domain);

  SmartPtr<AbstractRangedDoubleVec3Property> m_OutputSpacingModel;
  bool GetOutputSpacingValueAndRange(Vector3d &value, NumericValueRange<Vector3d> *domain);
  void SetOutputSpacingValue(Vector3d value);

  SmartPtr<AbstractScaleFactorModel> m_ScaleFactorModel[3];
  bool GetIndexedScaleFactorValueAndRange(int index, double &value, ScaleFactorDomain *domain);
  void SetIndexedScaleFactorValue(int index, double value);

  void ComputeSpacingDomain(NumericValueRange<Vector3d> *domain);

  ScaleFactorDomain m_ScaleFactorDomain;
};

#endif // SNAKEROIRESAMPLEMODEL_H
