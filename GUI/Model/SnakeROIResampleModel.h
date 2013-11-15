#ifndef SNAKEROIRESAMPLEMODEL_H
#define SNAKEROIRESAMPLEMODEL_H

#include "PropertyModel.h"
#include "SNAPSegmentationROISettings.h"

class GlobalUIModel;

/**
 * @brief The SnakeROIResampleModel class
 * Handles the resample snake dialog.
 */
class SnakeROIResampleModel : public AbstractModel
{
public:

  irisITKObjectMacro(SnakeROIResampleModel, AbstractModel)

  AbstractRangedDoubleProperty *GetInputSpacingModel(int index)
    { return m_InputSpacingModel[index]; }

  AbstractRangedUIntProperty *GetInputDimensionsModel(int index)
    { return m_InputDimensionsModel[index]; }

  AbstractRangedDoubleProperty *GetOutputSpacingModel(int index)
    { return m_OutputSpacingModel[index]; }

  AbstractRangedUIntProperty *GetOutputDimensionsModel(int index)
    { return m_OutputDimensionsModel[index]; }

  irisSimplePropertyAccessMacro(FixedAspectRatio, bool)

  typedef SNAPSegmentationROISettings::InterpolationMethod InterpolationMode;
  typedef SimpleItemSetDomain<InterpolationMode, std::string> InterpolationModeDomain;
  typedef AbstractPropertyModel<InterpolationMode, InterpolationModeDomain> AbstractInterpolationModeModel;

  irisGetMacro(InterpolationModeModel, AbstractInterpolationModeModel *)

  void SetParentModel(GlobalUIModel *model);

  /** Reset to default (no scaling) */
  void Reset();

  /** Accept the user's changes */
  void Accept();

  /** Availabel quick presets */
  enum ResamplePreset {
    SUPER_2, SUB_2, SUPER_ISO, SUB_ISO
  };

  void ApplyPreset(ResamplePreset preset);

protected:

  SnakeROIResampleModel();
  virtual ~SnakeROIResampleModel() {}

  GlobalUIModel *m_Parent;
  AbstractPropertyModel<SNAPSegmentationROISettings> *m_ROISettingsModel;

  SmartPtr<AbstractRangedDoubleProperty> m_InputSpacingModel[3];
  bool GetInputSpacingValueAndRange(int index, double &value, NumericValueRange<double> *domain);

  SmartPtr<AbstractRangedUIntProperty> m_InputDimensionsModel[3];
  bool GetInputDimensionsValueAndRange(int index, unsigned int &value, NumericValueRange<unsigned int > *domain);

  SmartPtr<AbstractRangedDoubleProperty> m_OutputSpacingModel[3];
  bool GetOutputSpacingValueAndRange(int index, double &value, NumericValueRange<double> *domain);
  void SetOutputSpacingValue(int index, double value);

  SmartPtr<AbstractRangedUIntProperty> m_OutputDimensionsModel[3];
  bool GetOutputDimensionsValueAndRange(int index, unsigned int &value, NumericValueRange<unsigned int> *domain);
  void SetOutputDimensionsValue(int index, unsigned int value);

  SmartPtr<ConcreteSimpleBooleanProperty> m_FixedAspectRatioModel;

  void ComputeCachedDomains();

  void EnforceAspectRatio(int source_idx);

  virtual void OnUpdate();

  // Cached information about the ROI. This is where this model stores its
  // state until the user says 'ok'
  Vector3ui m_ResampleDimensions;

  // Cached domain values - to avoid recomputing all the time
  NumericValueRange<double> m_SpacingDomain[3];
  NumericValueRange<unsigned int> m_DimensionsDomain[3];

  // Map for interpolation modes
  InterpolationModeDomain m_InterpolationModeDomain;

  // Model for the interpolation modes
  typedef ConcretePropertyModel<InterpolationMode, InterpolationModeDomain> ConcreteInterpolationModeModel;
  SmartPtr<ConcreteInterpolationModeModel> m_InterpolationModeModel;
};

#endif // SNAKEROIRESAMPLEMODEL_H
