#ifndef POLYGONSETTINGSMODEL_H
#define POLYGONSETTINGSMODEL_H

#include "PropertyModel.h"

class GlobalUIModel;

class PolygonSettingsModel : public AbstractModel
{
public:
  irisITKObjectMacro(PolygonSettingsModel, AbstractModel)

  irisGetMacro(ParentModel, GlobalUIModel *)

  void SetParentModel(GlobalUIModel *model);

  irisSimplePropertyAccessMacro(FreehandIsPiecewise, bool)
  irisRangedPropertyAccessMacro(FreehandSegmentLength, int)

protected:

  PolygonSettingsModel();
  virtual ~PolygonSettingsModel() {}

  GlobalUIModel *m_ParentModel;

  SmartPtr<AbstractSimpleBooleanProperty> m_FreehandIsPiecewiseModel;
  bool GetFreehandIsPiecewiseValue(bool &value);
  void SetFreehandIsPiecewiseValue(bool value);

  SmartPtr<AbstractRangedIntProperty> m_FreehandSegmentLengthModel;
  bool GetFreehandSegmentLengthValueAndRange(int &value, NumericValueRange<int> *range);
  void SetFreehandSegmentLengthValue(int value);

private:

  int m_LastFreehandRate;

};

#endif // POLYGONSETTINGSMODEL_H
