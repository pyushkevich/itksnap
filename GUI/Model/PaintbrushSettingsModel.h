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

  typedef AbstractPropertyModel<PaintbrushMode> AbstractPaintbrushModeModel;
  typedef ConcretePropertyModel<PaintbrushMode> ConcretePaintbrushModeModel;

  irisGetSetMacro(ParentModel, GlobalUIModel *)

  irisGetMacro(PaintbrushModeModel, AbstractPaintbrushModeModel *)

  irisGetMacro(BrushSizeModel, AbstractRangedIntProperty *)
  irisGetMacro(VolumetricBrushModel, AbstractSimpleBooleanProperty *)
  irisGetMacro(IsotropicBrushModel, AbstractSimpleBooleanProperty *)
  irisGetMacro(ChaseCursorModel, AbstractSimpleBooleanProperty *)

  irisGetMacro(AdaptiveModeModel, AbstractSimpleBooleanProperty *)
  irisGetMacro(ThresholdLevelModel, AbstractRangedDoubleProperty *)
  irisGetMacro(SmoothingIterationsModel, AbstractRangedIntProperty *)

protected:

  PaintbrushSettingsModel();
  virtual ~PaintbrushSettingsModel();

  GlobalUIModel *m_ParentModel;

  // Model that sets and retrieves the PaintbrushSettings
  typedef AbstractPropertyModel<PaintbrushSettings> PaintbrushSettingsStructModel;
  SmartPtr<PaintbrushSettingsStructModel> m_PaintbrushSettingsModel;
  PaintbrushSettings GetPaintbrushSettings();
  void SetPaintbrushSettings(PaintbrushSettings ps);

  SmartPtr<AbstractPaintbrushModeModel> m_PaintbrushModeModel;
  SmartPtr<AbstractSimpleBooleanProperty> m_VolumetricBrushModel;
  SmartPtr<AbstractSimpleBooleanProperty> m_IsotropicBrushModel;
  SmartPtr<AbstractSimpleBooleanProperty> m_ChaseCursorModel;

  SmartPtr<AbstractRangedIntProperty> m_BrushSizeModel;
  bool GetBrushSizeValueAndRange(int &value, NumericValueRange<int> *domain);
  void SetBrushSizeValue(int value);

  SmartPtr<AbstractSimpleBooleanProperty> m_AdaptiveModeModel;
  bool GetAdaptiveModeValue(bool &value);

  SmartPtr<AbstractRangedDoubleProperty> m_ThresholdLevelModel;
  bool GetThresholdLevelValueAndRange(double &value, NumericValueRange<double> *domain);
  void SetThresholdLevelValue(double value);

  SmartPtr<AbstractRangedIntProperty> m_SmoothingIterationsModel;
  bool GetSmoothingIterationValueAndRange(int &value, NumericValueRange<int> *domain);
  void SetSmoothingIterationValue(int value);
};

#endif // PAINTBRUSHSETTINGSMODEL_H
