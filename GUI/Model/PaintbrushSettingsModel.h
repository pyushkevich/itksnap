#ifndef PAINTBRUSHSETTINGSMODEL_H
#define PAINTBRUSHSETTINGSMODEL_H

#include "AbstractModel.h"
#include "PropertyModel.h"
#include "GlobalState.h"

class GlobalUIModel;

class PaintbrushSettingsModel : public AbstractModel
{
public:
  irisITKObjectMacro(PaintbrushSettingsModel, AbstractModel)

  typedef AbstractPropertyModel<PaintbrushShape> AbstractPaintbrushShapeModel;
  typedef AbstractPropertyModel<PaintbrushSmartMode> AbstractPaintbrushSmartModeModel;

  enum UIState {
    UIF_VOLUMETRIC_OK,
    UIF_ADAPTIVE_OK
  };

  irisGetMacro(ParentModel, GlobalUIModel *)
  void SetParentModel(GlobalUIModel *parent);

  virtual void OnUpdate() override;

  bool CheckState(PaintbrushSettingsModel::UIState state);

  irisGenericPropertyAccessMacro(PaintbrushShape, PaintbrushShape, TrivialDomain)
  irisGenericPropertyAccessMacro(PaintbrushSmartMode, PaintbrushSmartMode, TrivialDomain)

  irisRangedPropertyAccessMacro(BrushSize, int)
  irisSimplePropertyAccessMacro(VolumetricBrush, bool)
  irisSimplePropertyAccessMacro(IsotropicBrush, bool)
  irisSimplePropertyAccessMacro(ChaseCursor, bool)

  irisReadOnlySimplePropertyAccessMacro(AdaptiveMode, bool)
  irisReadOnlySimplePropertyAccessMacro(DeepLearningMode, bool)
  irisRangedPropertyAccessMacro(ThresholdLevel, double)
  irisRangedPropertyAccessMacro(SmoothingIterations, int)

protected:

  PaintbrushSettingsModel();
  virtual ~PaintbrushSettingsModel();

  GlobalUIModel *m_ParentModel;

  // Model that sets and retrieves the PaintbrushSettings
  typedef AbstractPropertyModel<PaintbrushSettings> PaintbrushSettingsStructModel;
  SmartPtr<PaintbrushSettingsStructModel> m_PaintbrushSettingsModel;
  PaintbrushSettings GetPaintbrushSettings();
  void SetPaintbrushSettings(PaintbrushSettings ps);

  SmartPtr<AbstractPaintbrushShapeModel> m_PaintbrushShapeModel;  
  SmartPtr<AbstractSimpleBooleanProperty> m_VolumetricBrushModel;
  SmartPtr<AbstractSimpleBooleanProperty> m_IsotropicBrushModel;
  SmartPtr<AbstractSimpleBooleanProperty> m_ChaseCursorModel;

  SmartPtr<AbstractPaintbrushSmartModeModel> m_PaintbrushSmartModeModel;
  bool GetPaintbrushSmartModeValue(PaintbrushSmartMode &value);
  void SetPaintbrushSmartModeValue(PaintbrushSmartMode value);

  SmartPtr<AbstractRangedIntProperty> m_BrushSizeModel;
  bool GetBrushSizeValueAndRange(int &value, NumericValueRange<int> *domain);
  void SetBrushSizeValue(int value);

  SmartPtr<AbstractSimpleBooleanProperty> m_AdaptiveModeModel;
  bool GetAdaptiveModeValue(bool &value);

  SmartPtr<AbstractSimpleBooleanProperty> m_DeepLearningModeModel;
  bool GetDeepLearningModeValue(bool &value);

  SmartPtr<AbstractRangedDoubleProperty> m_ThresholdLevelModel;
  bool GetThresholdLevelValueAndRange(double &value, NumericValueRange<double> *domain);
  void SetThresholdLevelValue(double value);

  SmartPtr<AbstractRangedIntProperty> m_SmoothingIterationsModel;
  bool GetSmoothingIterationValueAndRange(int &value, NumericValueRange<int> *domain);
  void SetSmoothingIterationValue(int value);
};

#endif // PAINTBRUSHSETTINGSMODEL_H
