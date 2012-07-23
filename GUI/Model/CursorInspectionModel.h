#ifndef CURSORINSPECTIONMODEL_H
#define CURSORINSPECTIONMODEL_H

#include <AbstractModel.h>
#include <PropertyModel.h>

class GlobalUIModel;

/**
  This little model handles the logic for the cursor inspection page
  */
class CursorInspectionModel : public AbstractModel
{
public:

  irisITKObjectMacro(CursorInspectionModel, AbstractModel)

  void SetParentModel(GlobalUIModel *parent);

  irisGetMacro(Parent, GlobalUIModel*)

  /** Get the model for the label under the cursor */
  irisGetMacro(LabelUnderTheCursorIdModel, AbstractSimpleLabelTypeProperty*)

  /** Get the model for the label description under the cursor */
  irisGetMacro(LabelUnderTheCursorTitleModel, AbstractSimpleStringProperty*)

  /** Get the model for the cursor location */
  AbstractRangedUIntVec3Property *GetCursorPositionModel() const;

protected:

  CursorInspectionModel();


private:

  // Label under the cursor
  SmartPtr<AbstractSimpleLabelTypeProperty> m_LabelUnderTheCursorIdModel;
  bool GetLabelUnderTheCursorIdValue(LabelType &value);

  // Title of the label under the cursor
  SmartPtr<AbstractSimpleStringProperty> m_LabelUnderTheCursorTitleModel;
  bool GetLabelUnderTheCursorTitleValue(std::string &value);

  // A pointer to the parent's cusror model
  AbstractRangedUIntVec3Property *m_CursorPositionModel;

  // Parent
  GlobalUIModel *m_Parent;

};

#endif // CURSORINSPECTIONMODEL_H
