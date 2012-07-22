#ifndef LABELEDITORMODEL_H
#define LABELEDITORMODEL_H

#include <AbstractModel.h>
#include <PropertyModel.h>
#include <ColorLabel.h>
#include <ColorLabelTable.h>
#include <GlobalUIModel.h>
#include <ColorLabelPropertyModel.h>

class LabelEditorModel : public AbstractModel
{
public:
  typedef AbstractRangedPropertyModel<double>::Type DoubleValueModel;
  typedef AbstractRangedPropertyModel<int>::Type IntegerValueModel;
  typedef AbstractRangedPropertyModel<bool>::Type BooleanValueModel;
  typedef AbstractPropertyModel<std::string> StringValueModel;
  typedef AbstractPropertyModel<iris_vector_fixed<bool, 2> > BooleanPairValueModel;
  typedef AbstractPropertyModel<Vector3ui> RGBColorValueModel;

  // Standard ITK stuff
  irisITKObjectMacro(LabelEditorModel, AbstractModel)

  /** Initialize with the parent model */
  void SetParentModel(GlobalUIModel *parent);

  /** Get the model describing the current selected label (and its domain) */
  irisGetMacro(CurrentLabelModel, ConcreteColorLabelPropertyModel *)

  /** Get the model for the current label description */
  irisGetMacro(CurrentLabelDescriptionModel, StringValueModel *)

  /** Get the model for the current label id */
  irisGetMacro(CurrentLabelIdModel, IntegerValueModel *)

  /** Get the model for the current label id */
  irisGetMacro(CurrentLabelOpacityModel, IntegerValueModel *)

  /** Get the model for the current label id */
  irisGetMacro(CurrentLabelHiddenStateModel, BooleanPairValueModel *)

  /** Get the model for the current label id */
  irisGetMacro(CurrentLabelColorModel, RGBColorValueModel *)

  /** Change the label id of the selected label */
  bool ReassignLabelId(LabelType newid);

  /**
    States in which the model can be, which allow the activation and
    deactivation of various widgets in the interface
    */
  enum UIState {
    UIF_EDITABLE_LABEL_SELECTED
  };

  /**
    Check the state flags above
    */
  bool CheckState(UIState state);

  /** Create a new label */
  bool MakeNewLabel(bool copyCurrent);

  /** Will deleting the current label affect the segmentation? */
  bool IsLabelDeletionDestructive();

  /** Delete the selected label */
  void DeleteCurrentLabel();

protected:

  // Hidden constructor/destructor
  LabelEditorModel();
  virtual ~LabelEditorModel() {}

  // Helper method to retrieve current color label from the table
  bool GetAndStoreCurrentLabel();

  // Id of the current label
  SmartPtr<IntegerValueModel> m_CurrentLabelIdModel;
  bool GetCurrentLabelIdValueAndRange(
      int &value, NumericValueRange<int> *domain);
  void SetCurrentLabelId(int value);

  // Opacity of the current label
  SmartPtr<IntegerValueModel> m_CurrentLabelOpacityModel;
  bool GetCurrentLabelOpacityValueAndRange(
      int &value, NumericValueRange<int> *domain);
  void SetCurrentLabelOpacity(int value);

  // Description of the current label
  SmartPtr<StringValueModel> m_CurrentLabelDescriptionModel;
  bool GetCurrentLabelDescription(std::string &value);
  void SetCurrentLabelDescription(std::string value);

  // Visibility of the current label
  SmartPtr<BooleanPairValueModel> m_CurrentLabelHiddenStateModel;
  bool GetCurrentLabelHiddenState(iris_vector_fixed<bool, 2> &value);
  void SetCurrentLabelHiddenState(iris_vector_fixed<bool, 2> value);

  // Color of the current label
  SmartPtr<RGBColorValueModel> m_CurrentLabelColorModel;
  bool GetCurrentLabelColor(Vector3ui &value);
  void SetCurrentLabelColor(Vector3ui value);

  // The parent model
  GlobalUIModel *m_Parent;

  // A pointer to the color label table
  ColorLabelTable *m_LabelTable;

  // The label that is currently selected
  SmartPtr<ConcreteColorLabelPropertyModel> m_CurrentLabelModel;

  // The information about the current label (temporarily valid)
  ColorLabel m_SelectedColorLabel;
  LabelType m_SelectedId;
};

#endif // LABELEDITORMODEL_H
